# Drawmethod chroma keys and Substitute

Author: Damon V. Caskey

Date: 2026-11-14

## Scope

Each drawmethod has five chroma keys and five Substitute pairs, named A through
E. Every slot starts disabled. The slots are generic: creators choose their
colors, with no reserved black, white, red, green, or blue entries.

Chroma makes matching source pixels transparent for that draw. Substitute
replaces an exact final RGB result, including a color produced by blending.
These operations work with indexed sprites, indexed or RGB32 source screens,
and dots, lines, and boxes, using an RGB32 destination. Existing transforms,
clipping, repeat order, palette lookup, and blend behavior remain in effect.

The native renderer executes supported color rules in its fragment shader.
The software renderer implements the same contract when native drawing is
unavailable. These are reusable drawmethod controls; public programmable
materials and RGB24 asset loading remain separate work. Games must opt into
these rules explicitly.

## Processing order

1. Resolve source coverage and the sampled texel. Existing sprite gaps and
   requested zero-key transparency still apply.
2. Resolve indexed color through the current source palette or drawmethod
   palette override.
3. Compare the resolved RGB against enabled chroma keys. Matching samples leave
   the destination untouched. Dots, lines, and boxes compare their supplied
   primitive color.
4. Apply the existing fill, tint, opacity, and blend operations that apply to
   the draw. Chroma therefore tests the source before fill or tint changes it.
5. Clamp and quantize the completed output to RGB bytes, then apply at most one
   matching Substitute pair before writing the pixel.

Matching is exact, with no tolerance or nearest-color search. Rendering ignores
the unused high byte of packed RGB32 colors. Script setters accept only integer
values from `0` through `0xFFFFFF`; use `rgbcolor(r, g, b)` to obtain the engine's
packed color convention. Numeric zero is valid black, not a disabled sentinel.

Substitute never fills source gaps, revives a chroma-discarded sample, or scans
untouched destination pixels. Zero-opacity masks and all-zero channel opacity,
when that channel-opacity mode applies, do not trigger substitution. Existing
opacity arithmetic, including its `/256` behavior, remains unchanged.

Sprite transparent index zero remains a hole regardless of its palette RGB.
For screens, existing requested zero-key behavior also takes precedence:
indexed zero or RGB black discarded by that setting cannot be restored by
Substitute. Opaque black from a nonzero palette entry remains eligible unless
an enabled chroma key matches it.

## Constant-based API

`set_draw_method(target, property, value[, replacement])` accepts either a real
drawmethod handle or `NULL()` for common script drawmethod state. The matching
getter is `get_draw_method(target, property[, component])`. Entity pointers
belong to the legacy entity API described below.

Resolve these property names with `openborconstant()`:

| Property | Setter arguments after the property | Meaning |
| --- | --- | --- |
| `DM_CHROMA_KEY_A` through `DM_CHROMA_KEY_E` | One packed RGB color | Store and enable a source chroma key. |
| `DM_SUBSTITUTE_A` through `DM_SUBSTITUTE_E` | Packed RGB match, packed RGB replacement | Validate, store, and enable one complete pair. |
| Either individual slot property | `NULL()` | Disable that slot while retaining its stored values. |
| `DM_CHROMA_KEY_ENABLED` | Integer mask from 0 to 31 | Replace the enabled chroma-slot mask. |
| `DM_SUBSTITUTE_ENABLED` | Integer mask from 0 to 31 | Validate and replace the enabled Substitute-slot mask. |

Bits `1`, `2`, `4`, `8`, and `16` select slots A, B, C, D, and E respectively.
Mask `0` disables every slot in that list; mask `31` enables all five stored
slots. `DRAWMETHOD_COLOR_SLOTS` and `DRAWMETHOD_COLOR_MASK` are also available
through `openborconstant()` and currently return `5` and `31`.

Setting a slot enables that slot, but does not override the drawmethod's
overall enabled flag. Disabled drawmethods retain their settings without
applying them. Resetting to the default drawmethod clears both lists; copying
a drawmethod copies the complete lists. Queued draws capture their own copies,
so changing or disabling a rule after queueing does not change earlier entries.

### Read stored values and enable state

| Component constant | Value | Individual chroma slot | Individual Substitute slot |
| --- | ---: | --- | --- |
| `DM_COLOR_MATCH` | 0, the default | Stored key RGB | Stored match RGB |
| `DM_COLOR_SUBSTITUTE` | 1 | Invalid | Stored replacement RGB |
| `DM_COLOR_ENABLED` | 2 | 0 or 1 | 0 or 1 |

Reading `DM_CHROMA_KEY_ENABLED` or `DM_SUBSTITUTE_ENABLED` returns the complete
mask and accepts only the default component, `DM_COLOR_MATCH`. Stored color
getters remain readable while their slots are disabled.

```c
int match = get_draw_method(NULL(), openborconstant("DM_SUBSTITUTE_A"));
int replacement = get_draw_method(NULL(), openborconstant("DM_SUBSTITUTE_A"),
                                 openborconstant("DM_COLOR_SUBSTITUTE"));
int enabled = get_draw_method(NULL(), openborconstant("DM_SUBSTITUTE_A"),
                             openborconstant("DM_COLOR_ENABLED"));
int enabled_mask = get_draw_method(NULL(), openborconstant("DM_SUBSTITUTE_ENABLED"));
```

### Validation and atomic updates

Every enabled Substitute match must be unique. No enabled replacement may
equal any enabled Substitute match, including its own. This prevents ambiguous
matches and forbids a replacement that the active list still intercepts.
Substitution never cascades even when values were configured outside scripts.

Pair updates and mask changes validate the complete resulting active list
before publishing it. Invalid types, arity, colors, masks, components, duplicate
matches, or replacement collisions produce a script error and leave the rule
state unchanged. Disabled stored pairs do not conflict with live rules, but
re-enabling them validates them again. Enabling untouched default Substitute
slots fails because their stored black-to-black pairs are invalid.

For a multi-slot reconfiguration, disable the affected slots first, then write
the new complete pairs. Each pair setter enables its slot; configure the new
list before queueing draws that depend on it. Disabling one slot uses one
`NULL()` argument, without a second replacement argument.

## Practical examples

### Keep a written result from becoming transparent black

Given an existing `source_screen` and RGB32 output, this example starts with
color rules disabled, queues a draw protected against final black, then
disables that rule for later draws:

```c
set_draw_method(NULL(), openborconstant("DM_SUBSTITUTE_A"),
                rgbcolor(0, 0, 0), rgbcolor(1, 1, 1));

drawscreen(source_screen, 0, 0, 0);

set_draw_method(NULL(), openborconstant("DM_SUBSTITUTE_A"), NULL());
```

Final black produced by this draw's fill, tint, or blending becomes near-black.
This is useful when a completed RGB32 screen will later be sampled with black
as a transparency key. The queued draw retains its rule even after the last
line disables the common slot.

Use the four-argument `drawscreen()` form when inheriting common drawmethod
state. Its existing optional fifth blend argument constructs a fresh default
drawmethod and does not inherit these common rules.

### Let a palette change alter visible coverage

Suppose an indexed image uses nonzero palette indices for optional edge pixels.
When its selected palette maps those indices to magenta, this chroma key hides
them:

```c
set_draw_method(NULL(), openborconstant("DM_CHROMA_KEY_A"),
                rgbcolor(255, 0, 255));

drawscreen(indexed_screen, 0, 0, 0);

set_draw_method(NULL(), openborconstant("DM_CHROMA_KEY_A"), NULL());
```

Choosing a palette that maps those same indices to another RGB color reveals
them on subsequent draws. The chroma rule changes coverage without editing
the source indices. Every visible index resolving to magenta is keyed; the
rule does not distinguish two indices with the same RGB value. This affects
rendered pixels, not collision boxes or gameplay geometry.

### Use an allocated drawmethod or the existing property API

```c
void method = allocate_drawmethod();

set_draw_method(method, openborconstant("DM_SUBSTITUTE_A"),
                rgbcolor(0, 0, 0), rgbcolor(1, 1, 1));

set_drawmethod_property(method, openborconstant("DRAWMETHOD_PROPERTY_CHROMA_KEY_A"),
                        rgbcolor(255, 0, 255));

set_drawmethod_property(method, openborconstant("DRAWMETHOD_PROPERTY_SUBSTITUTE_B"),
                        rgbcolor(255, 255, 255), rgbcolor(254, 254, 254));
```

Every `DM_*` property in the first table has a corresponding
`DRAWMETHOD_PROPERTY_*` name. `set_drawmethod_property()` and
`get_drawmethod_property()` expose the same color arguments and getter
components, but require a real drawmethod handle. They do not interpret
`NULL()` as common state. Newly allocated methods follow the existing drawmethod
ownership and drawing APIs; allocation alone does not select one for a draw.

### Existing entity and named-property API

`changedrawmethod(entity_or_NULL, name, value[, replacement])` and
`getdrawmethod(entity_or_NULL, name[, component])` expose the same rules:

| Names | Meaning |
| --- | --- |
| `"chromakeya"` through `"chromakeye"` | Individual chroma slots A through E |
| `"substitutea"` through `"substitutee"` | Individual Substitute slots A through E |
| `"chromakeyenabled"` | Chroma enable mask |
| `"substituteenabled"` | Substitute enable mask |

```c
changedrawmethod(entity_handle, "enabled", 1);
changedrawmethod(entity_handle, "chromakeya", rgbcolor(255, 0, 255));
changedrawmethod(entity_handle, "substitutea", rgbcolor(0, 0, 0), rgbcolor(1, 1, 1));

int replacement = getdrawmethod(entity_handle, "substitutea",
                               openborconstant("DM_COLOR_SUBSTITUTE"));

changedrawmethod(entity_handle, "chromakeya", NULL());
changedrawmethod(entity_handle, "substituteenabled", 0);
```

Here `entity_handle` is a valid entity pointer. Passing `NULL()` selects common
script state instead. Getter components and validation match the constant-based
API.

## Native execution, fallback, and logging

When native scene drawing initializes, startup and video reinitialization add
a separate **Color Rules** section in `Logs/OpenBorLog.txt`. It states whether
chroma and Substitute are native, their five-slot capacities, RGB32 destination
scope, and matching order. Read these rows alongside the general scene/affine
result; the existing `NATIVE` result line alone does not establish native
color-rule availability. If no native backend initializes, the general
`SOFTWARE` result applies to all scene drawing, including these rules.

The renderer attempts supported combined and individual shader variants.
If the combined program cannot initialize, it prefers retaining existing
native affine support, then tries color-only and legacy variants as applicable.
Missing native color support therefore preserves usable native operations,
while draws with active color rules use software. Startup explicitly warns
that significant slowdown is possible. Warnings are not repeated for each draw.

Rules that need an unsupported destination format are a different condition:
active color rules require RGB32 in both renderers. Normal runtime logs an
error and shuts down instead of silently ignoring the rules or quantizing a
near-black replacement back into black. Indexed chroma also requires a usable
palette, even when fill will later replace the visible color. Substitute-only
indexed fills can use the supplied fill color without a palette.

With profiling enabled, `native_chroma_draws` and `native_substitute_draws`
confirm successful native submissions with those rules active.
`color_software_draws` counts high-level software draw dispatches with active
color rules. Declining native color support uses fallback reason `color_rules`;
other limitations retain their own reasons. These counts do not measure matched
pixels or prove hardware acceleration. See
[color-rule accounting](engine-profiling.md#drawmethod-color-rule-accounting).

Enabling chroma or Substitute alone adds no destination snapshot or readback.
Existing blending, self-drawing, and CPU fallback can still need those transfers.
Uniform state is cached, including changes back to disabled rules.

## Limits and validation

Protection applies only to pixels eligible for this draw. Later draws, clears,
CPU writes, palette changes, or output filters can produce the same color again.
Protect each relevant writing operation when a screen must maintain a color
constraint. Substitute may deliberately change a visually meaningful exact
color, so choose keys and replacements for the intended artwork and blend modes.

RGB32's high byte remains unused; chroma does not create a persistent alpha
channel. Existing mask/opacity behavior still controls partial transparency.
Exact RGB rules provide no color range, alpha gradient, palette-index identity,
or programmable shading language.

Run `bash tests/run_gpu_tests.sh` for validation. Current coverage includes
13 complete software images and 13 complete native images, all five slots,
palette/fill order, blend-produced matches, source coverage, zero opacity,
primitive colors, repeat/affine geometry, queue copies, self-draws, and state
reuse without unnecessary transfers. Capability fixtures also exercise optional
shader compilation/linking failures and reduced resource limits.

The runner also links the production script APIs to verify constant lookup,
common/object/legacy call forms, atomic validation, getters, reset behavior,
and read-only defaults. Run `bash tests/run_drawmethod_color_script_tests.sh`
for that focused check without creating a graphics context.

Mesa llvmpipe execution verifies pixel correctness and native-path accounting.
It runs on the CPU and establishes no physical-GPU FPS improvement. Real-device
performance still requires matched gameplay measurements.

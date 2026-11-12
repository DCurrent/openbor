# Experimental affine rendering

Branch: `experiment/curved-transform` - standard affine foundation. Curved road
and cylindrical wall warps are **not yet implemented**.

## Script interface

Call `changedrawmethod(entity, "affine", m00, m01, m10, m11, tx, ty)` to
set all six coefficients in one validated, atomic operation. This also enables
the new affine path. Pass `NULL()` instead of an entity to use the current
script draw method (for `drawsprite` or `drawscreen`).

For example, an identity transform:

```c
changedrawmethod(NULL(), "affine", 1, 0, 0, 1, 0, 0);
```

The coefficients map source coordinates relative to the draw pivot into the
destination screen:

```text
dest_x = draw_x + tx + m00 * local_x + m01 * local_y
dest_y = draw_y + ty + m10 * local_x + m11 * local_y
```

Here `local_x = source_x - (sprite_centerx + drawmethod_centerx)` and likewise
for Y. A source screen uses `(0, 0)` as its built-in pivot. The `flipx` and
`flipy` flags, when enabled, negate the corresponding **local** source axes
before applying the matrix. The matrix operates on continuous coordinates;
the renderer samples the source at destination pixel centers with nearest
neighbor filtering.

Some examples:

```c
// Stretch horizontally by 150% and vertically by 75%.
changedrawmethod(NULL(), "affine", 1.5, 0, 0, 0.75, 0, 0);

// Shear horizontally by 0.4 units for every unit down the image.
changedrawmethod(NULL(), "affine", 1, 0.4, 0, 1, 0, 0);

// Rotate 90 degrees clockwise in screen coordinates (Y grows downward).
changedrawmethod(NULL(), "affine", 0, -1, 1, 0, 0, 0);

// Move the transformed image 12 pixels right and 8 up.
changedrawmethod(NULL(), "affine", 1, 0, 0, 1, 12, -8);

// Return to normal rendering without discarding stored coefficients.
changedrawmethod(NULL(), "affineenabled", 0);
```

For animation, individual coefficients are accessible with
`changedrawmethod(entity, property, value)` and
`getdrawmethod(entity, property)`. Supported property names are `affine00`,
`affine01`, `affine10`, `affine11`, `affinex`, and `affiney`. Setting any
coefficient enables affine rendering. `affineenabled` controls that flag;
`getdrawmethod(entity, "affine")` is another way to read its value.

## Interaction with the existing renderer

- Affine rendering is off by default. The ordinary screen and sprite fast
  paths are unchanged unless it is explicitly enabled.
- When enabled, affine takes precedence over legacy `rotate`, `scalex`,
  `scaley`, `shiftx`, and water/perspective transforms. Their stored values
  remain intact and take effect again when affine is disabled.
- Draw position, source pivot, extra center offsets, flip flags, repeats,
  clipping, transparency, palette remapping, fill, tint and blending still
  participate in drawing. Affine only replaces **geometry**.
- A singular matrix (determinant zero) draws nothing. Invalid non-finite
  numbers are rejected by the script setter, and ignored by the native
  rasterizer if passed directly.
- This branch adds no frame delay or input timing changes.

## Design for future curve options

Affine is a constant 2x3 mapping, so it cannot bend a straight line. Two
planned nonlinear options are independent of this initial milestone:

1. **Road/path curve** - progressively shift or rotate horizontal strips
   around a supplied path, suitable for a racing-game floor or roadway.
2. **Cylindrical bend** - curve a vertical image about an axis for columns,
   rounded walls and similar surfaces.

Both will require their own warp parameters and sampling logic. Keeping the
new affine path separate from the legacy water and shear code leaves a clean
place to add those mappings without changing the default rendering behavior.

## Validation

Run the standalone raster tests from the repository root:

```sh
gcc -std=gnu99 -O2 -ffunction-sections -fdata-sections \
    -Iengine -Iengine/source -Iengine/source/gamelib \
    tests/test_affine_transform.c engine/source/gamelib/transform.c \
    -Wl,--gc-sections -lm -o test_affine_transform
./test_affine_transform
```

The tests cover identity, source pivot and translation, flips, right-angle
rotation, non-uniform scale, shear with clipping, a collapsed matrix, and
offscreen transforms. Benchmark performance against both the legacy and new
paths on representative sprite and subscreen sizes before merging.

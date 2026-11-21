# Video controls

Author: Damon V. Caskey

Date: 2026-11-20

## Display size, sampling and final effects

Video Options separates the size of the displayed game from the way it is
sampled and the visual effect applied to the completed scene. These controls
operate after the module has drawn its game-resolution screen. They do not
change the module's resolution, entity coordinates or scripted screen sizes.

| Control | Purpose | Choices |
| --- | --- | --- |
| Display Mode | Select desktop windowed or fullscreen presentation | Window or Full |
| Window Scale | Set the desktop window's size relative to the module resolution | 0.25x through 8.00x in 0.25x steps |
| Display Scale | Set Android's centered scene size on the device | Automatic, then 0.25x through 8.00x in 0.25x steps |
| Scaling Filter | Sample the completed image at its display size | Bilinear, Nearest or High Quality |
| Final Filter | Give the completed game an emulator-style appearance | Disabled, Bilinear, Advance Mame2x, Scanlines, Scanlines TV, TV 2x or Dot Matrix |
| Fullscreen Type | Fit the scene with its aspect ratio or stretch it to the display | Preserve Aspect Ratio or Stretch to Screen |
| Display Fit | Choose Android's Automatic sizing behavior | Preserve Aspect Ratio or Stretch to Screen |

Brightness, Gamma, Window Offset and FPS Limit retain their existing roles.
Fullscreen display remains automatic on desktop; the stored Window Scale
becomes effective again when returning to a window. Android uses its existing
fullscreen device surface rather than a desktop-style movable window.

The retired Video Backend selector is removed. Every supported display uses
native GPU rendering; the display controls select its presentation behavior.

## Window and display scale

Window Scale is a convenience for playing a low-resolution module on a large
desktop. Each left or right input changes its multiplier by 0.25, with endpoints
at 0.25x and 8x. Scale does not wrap when an endpoint is reached. For a module
whose scene is 320 by 240 pixels, some useful window sizes are:

| Window Scale | Requested scene display size |
| --- | --- |
| 1.00x | 320 by 240 |
| 1.25x | 400 by 300 |
| 2.00x | 640 by 480 |
| 4.00x | 1280 by 960 |
| 8.00x | 2560 by 1920 |

Android's Automatic setting preserves the existing aspect/stretch behavior.
Positive Display Scale values instead create a centered viewport from the
original scene resolution multiplied by the selected scale. Smaller views
leave black space around the game; views larger than the device are clipped
at its edges. Touch controls keep their device-space positions. Direct
fullscreen movies retain their own presentation geometry. Display Fit is
available only with Automatic scale; its stored choice is restored when
returning to Automatic.

The scale is always relative to the original scene. Enabling a 2x Final Filter
does not double the window or Android viewport again. Its intermediate texture
is sampled into the same requested display size.

## Scaling Filter

Scaling Filter controls sampling when the completed image is fitted to its
display area:

| Selection | Behavior |
| --- | --- |
| Nearest | Choose the nearest source texel; preserve hard pixel boundaries |
| Bilinear | Interpolate adjacent texels for smooth enlargement or reduction |
| High Quality | Use the existing OpenGL sharp-bilinear coordinate mapping to keep pixel interiors sharp while interpolating display transitions |

The selection remains available at 1x even where two or more modes produce the
same pixels. Changing scale does not change the selected sampling mode.
Desktop defaults to High Quality; Android defaults to Nearest.

## Final Filter

Final Filter processes the completed game scene before display scaling and
gamma/brightness correction. All six existing effects remain available with
their existing saved IDs and RGB8 formulas. Enabled effects create a private
2x GPU image; Disabled bypasses that pass.

The historical activation rule remains: an enabled Final Filter runs in
fullscreen, or in a desktop window whose scale is at least 2x. Selecting an
effect below 2x still stores the choice. The menu identifies that choice as
inactive until the window is enlarged or fullscreen is selected. Returning
below 2x does not erase the chosen effect.

The Final Filter named Bilinear is the established 2x visual effect with its
own neighbor phase, border rules and integer arithmetic. Scaling Filter's
Bilinear selection is ordinary display sampling. Choosing either does not
change the other. See [GPU output filters](gpu-output-filters.md) for each
effect's exact equations and activation behavior.

## In-game resampling

Drawmethod resampling, including `DM_RESAMPLE`, belongs to the module's drawing
operations. It reconstructs individual sprites, screens and other sources
while the game scene is composed. Named cubic and Lanczos choices remain
available through the existing drawing and movie interfaces.

The presentation controls apply afterward. Final Filter sees the finished
scene; Scaling Filter maps that result to the display. Neither menu selection
overrides an entity's drawmethod or a movie's resampling selection. Movies
composed into a scene receive that scene's Final Filter. Direct fullscreen
movie output remains independent of the scene effect.

## Existing settings

The packed configuration layout and compatibility version remain unchanged.
The old backend-preference field is retained as a settings-version marker;
the menu no longer treats it as a renderer switch. Loading an older file
migrates only its display controls:

| Previous configuration | Restored Scaling Filter |
| --- | --- |
| Desktop OpenGL | High Quality, matching the old presenter |
| Desktop SDL with Bilinear | Bilinear |
| Desktop SDL with Simple | Nearest |
| Android with Bilinear or Simple | Bilinear or Nearest, respectively |

Migrated files preserve explicit choices among all three sampling modes on
later loads. Final Filter IDs, controls, audio preferences and other packed
fields retain their existing positions and values. Module-specific saves and
the default configuration both use the same migration.

Incomplete or incompatible configuration files leave the engine defaults in
place. Loading never applies a partial settings record before migration.

Finite scale values are bounded to the supported range. Android retains zero
as Automatic. Nonfinite values fall back to 1x on desktop and Automatic on
Android before any window-size conversion. Fractional values in range remain
valid; loading a file does not round them to menu increments.

## Verification

Run the control and persistence tests without a graphics driver:

```sh
bash tests/run_video_settings_tests.sh
```

The runner builds desktop and Android variants using the real normalization
module plus the production settings save/load/default and menu-action
functions. It checks the old packed layout, legacy migration, default values,
saved selection combinations, quarter-step limits through 8x, inactive final
effect selection and independence between controls.

The production desktop presenter suite verifies all three sampling modes,
fractional and 8x presentation, final-filter activity and mode changes.
`tests/run_android_scene_tests.sh` verifies Android scene sizing, filtering,
touch composition and context handling through the actual GLES presenter.
`tests/run_gpu_tests.sh` includes these regressions with the broader renderer
suite. Host tests do not replace physical-device display and input validation.

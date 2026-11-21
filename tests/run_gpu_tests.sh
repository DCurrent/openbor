#!/usr/bin/env bash
# Damon V. Caskey
# 2026-11-12
# Build focused renderer regressions without the game's audio/media dependencies.
# Linux/SDL2 is the reference headless correctness-test environment.
set -euo pipefail

test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-gpu-tests.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=c99 -O2 -Wall -Werror -D_DEFAULT_SOURCE -DPP_TEST
            -ffunction-sections -fdata-sections -Iengine -Iengine/source
            -Iengine/source/gamelib -Iengine/source/gfxlib)
test_link_flags=(-Wl,--gc-sections -lm)
test_profile_sources=(engine/source/gamelib/engine_profile.c)

# Damon V. Caskey, 2026-11-20: check the real menu actions and legacy settings
# layout before driver-dependent tests, including every scale through 8x.
OPENBOR_TEST_BUILD_DIR="$test_build_dir" bash tests/run_video_settings_tests.sh
# Append optional sanitizer or diagnostic flags after the defaults so callers
# can select the same instrumentation for both CPU and native test binaries.
if [[ -n ${OPENBOR_TEST_CFLAGS:-} ]]; then
    read -r -a test_extra_flags <<< "$OPENBOR_TEST_CFLAGS"
    test_flags+=("${test_extra_flags[@]}")
fi

# Damon V. Caskey
# 2026-11-12
# Distinguish an unavailable graphics context from a test assertion failure.
# CI can require native GL coverage with OPENBOR_REQUIRE_GPU_TESTS=1.
run_test()
{
    local test_status
    # Preserve the executable's failure code. Only its explicit unavailable-
    # context result may become a skip, and CI can require that coverage.
    if "$1"; then
        return 0
    else
        test_status=$?
        if [[ $test_status == 77 && ${OPENBOR_REQUIRE_GPU_TESTS:-0} != 1 ]]; then
            printf 'SKIP: %s requires an SDL/OpenGL context.\n' "$1"
            return 0
        fi
        return "$test_status"
    fi
}

# Verify profiler configuration in separate processes because environment
# settings are intentionally sampled once at engine startup. Explicit fixtures
# then supply deterministic clocks and query completion without SDL.
"$test_compiler" "${test_flags[@]}" tests/test_engine_profile.c \
    "${test_profile_sources[@]}" "${test_link_flags[@]}" \
    -o "$test_build_dir/test_engine_profile"
env -u OPENBOR_PROFILE -u OPENBOR_PROFILE_FRAMES -u OPENBOR_PROFILE_WARMUP \
    "$test_build_dir/test_engine_profile" --environment-defaults
OPENBOR_PROFILE=0 "$test_build_dir/test_engine_profile" --environment-disabled
OPENBOR_PROFILE=1 OPENBOR_PROFILE_FRAMES=7 OPENBOR_PROFILE_WARMUP=2 \
    "$test_build_dir/test_engine_profile" --environment-custom
OPENBOR_PROFILE=1 OPENBOR_PROFILE_FRAMES=1001 OPENBOR_PROFILE_WARMUP=-1 \
    "$test_build_dir/test_engine_profile" --environment-defaults
run_test "$test_build_dir/test_engine_profile"

# Sample whole script frames and retain exact exclusive native/interpreter
# attribution. Fresh processes verify environment parsing separately from the
# deterministic clock, scope, registry and slow-frame accounting fixtures.
"$test_compiler" "${test_flags[@]}" tests/test_script_profile.c \
    "${test_profile_sources[@]}" "${test_link_flags[@]}" \
    -o "$test_build_dir/test_script_profile"
env -u OPENBOR_PROFILE_SCRIPT -u OPENBOR_PROFILE_SCRIPT_INTERVAL \
    OPENBOR_PROFILE=1 OPENBOR_PROFILE_FRAMES=5 OPENBOR_PROFILE_WARMUP=0 \
    "$test_build_dir/test_script_profile" --environment-defaults
OPENBOR_PROFILE=1 OPENBOR_PROFILE_FRAMES=5 OPENBOR_PROFILE_WARMUP=0 \
    OPENBOR_PROFILE_SCRIPT=0 OPENBOR_PROFILE_SCRIPT_INTERVAL=16 \
    "$test_build_dir/test_script_profile" --environment-disabled
OPENBOR_PROFILE=1 OPENBOR_PROFILE_FRAMES=5 OPENBOR_PROFILE_WARMUP=0 \
    OPENBOR_PROFILE_SCRIPT=1 OPENBOR_PROFILE_SCRIPT_INTERVAL=2 \
    "$test_build_dir/test_script_profile" --environment-custom
for test_script_interval in -1 0 1001 invalid; do
    OPENBOR_PROFILE=1 OPENBOR_PROFILE_FRAMES=5 OPENBOR_PROFILE_WARMUP=0 \
        OPENBOR_PROFILE_SCRIPT=1 OPENBOR_PROFILE_SCRIPT_INTERVAL=$test_script_interval \
        "$test_build_dir/test_script_profile" --environment-defaults
done
run_test "$test_build_dir/test_script_profile"

# Validate descriptor eligibility, optional allocation failure and ownership
# against the real instruction implementation. Allocator wrapping stays in
# this isolated fixture; no test hooks enter the production call path.
"$test_compiler" "${test_flags[@]}" -UNDEBUG \
    -Iengine/source/scriptlib -Iengine/source/preprocessorlib \
    tests/test_native_call_descriptor.c \
    engine/source/scriptlib/Instruction.c engine/source/scriptlib/List.c \
    engine/source/scriptlib/ScriptVariant.c engine/source/scriptlib/ScriptVariantInteger.c \
    "${test_link_flags[@]}" -Wl,--wrap=malloc -Wl,--wrap=free \
    -o "$test_build_dir/test_native_call_descriptor"
run_test "$test_build_dir/test_native_call_descriptor"

# Execute actual compiled scripts with native callbacks, imports and errors.
# PP_TEST=0 retains test-safe logging declarations while selecting the normal
# packed-file preprocessor path supplied by the fixture's in-memory file API.
test_script_sources=(engine/source/scriptlib/Interpreter.c
    engine/source/scriptlib/Instruction.c engine/source/scriptlib/Parser.c
    engine/source/scriptlib/ParserSet.c engine/source/scriptlib/Lexer.c
    engine/source/scriptlib/List.c engine/source/scriptlib/Stack.c
    engine/source/scriptlib/StackedSymbolTable.c engine/source/scriptlib/SymbolTable.c
    engine/source/scriptlib/ScriptVariant.c engine/source/scriptlib/ScriptVariantInteger.c
    engine/source/scriptlib/ImportCache.c engine/source/preprocessorlib/pp_parser.c
    engine/source/preprocessorlib/pp_lexer.c engine/source/preprocessorlib/pp_expr.c)
"$test_compiler" "${test_flags[@]}" -UNDEBUG -UPP_TEST -DPP_TEST=0 \
    -Iengine/source/scriptlib -Iengine/source/preprocessorlib \
    -include tests/script_profile_fixture_globals.h \
    tests/test_script_profile_integration.c "${test_script_sources[@]}" \
    "${test_profile_sources[@]}" "${test_link_flags[@]}" \
    -o "$test_build_dir/test_script_profile_integration"
run_test "$test_build_dir/test_script_profile_integration"

# Verify that both normal execution modes contain their opcode bodies, not
# per-opcode C calls. Test debug and release optimization levels explicitly;
# correctness alone cannot catch a compiler declining ordinary inline hints.
test_dispatch_objects=()
for test_dispatch_level in 0 3; do
    test_dispatch_object="$test_build_dir/Interpreter_dispatch_O$test_dispatch_level.o"
    test_dispatch_mode=(-UNDEBUG)
    if [[ $test_dispatch_level == 3 ]]; then
        test_dispatch_mode=(-DNDEBUG)
    fi
    "$test_compiler" "${test_flags[@]}" "-O$test_dispatch_level" \
        "${test_dispatch_mode[@]}" -UPP_TEST -DPP_TEST=0 \
        -Iengine/source/scriptlib -Iengine/source/preprocessorlib \
        -include tests/script_profile_fixture_globals.h \
        -c engine/source/scriptlib/Interpreter.c -o "$test_dispatch_object"
    test_dispatch_objects+=("$test_dispatch_object")
done
bash tests/test_interpreter_dispatch_codegen.sh "${test_dispatch_objects[@]}"

# Keep unrelated renderer fixtures independent of the diagnostic branch's
# default-on setting. The native profiling suite explicitly enables its own
# collection windows and exercises GPU query opt-out separately.
export OPENBOR_PROFILE=0

# Compile the actual production log path without PP_TEST. Renderer capability
# records must still reach writeToLogFile when profiling is disabled.
"$test_compiler" "${test_flags[@]}" -UPP_TEST tests/test_gpu_logging.c \
    engine/source/gamelib/gpu.c "${test_profile_sources[@]}" \
    "${test_link_flags[@]}" -o "$test_build_dir/test_gpu_logging"
run_test "$test_build_dir/test_gpu_logging"

OPENBOR_TEST_BUILD_DIR="$test_build_dir" bash tests/run_gpu_curve_geometry_tests.sh

# Run context-independent contracts first so hosts without SDL still verify
# stable ordering, transform arithmetic, and CPU/GPU ownership transitions.
"$test_compiler" "${test_flags[@]}" tests/test_spriteq_sort.c \
    "${test_profile_sources[@]}" "${test_link_flags[@]}" \
    -o "$test_build_dir/test_spriteq_sort"
run_test "$test_build_dir/test_spriteq_sort"

"$test_compiler" "${test_flags[@]}" tests/test_gpu_transform.c \
    engine/source/gamelib/gpu_transform.c engine/source/gamelib/gpu_curve.c engine/source/gamelib/gpu_shader.c \
    engine/source/gamelib/pixelformat.c "${test_profile_sources[@]}" "${test_link_flags[@]}" \
    -o "$test_build_dir/test_gpu_transform"
run_test "$test_build_dir/test_gpu_transform"

# Public scene roots only dispatch native work; explicit CPU image access and
# indexed asset maintenance are tested with separate mock GPU pixel storage.
test_scene_sources=(engine/source/gamelib/gpu.c engine/source/gamelib/screen.c
    engine/source/gamelib/draw.c engine/source/gamelib/sprite.c
    engine/source/gamelib/spriteq.c engine/source/gamelib/bitmap.c
    engine/source/gamelib/pixelformat.c)
"$test_compiler" "${test_flags[@]}" tests/test_gpu_coherence.c \
    "${test_scene_sources[@]}" "${test_profile_sources[@]}" "${test_link_flags[@]}" \
    -o "$test_build_dir/test_gpu_coherence"
run_test "$test_build_dir/test_gpu_coherence"

# Inspect uncollected production objects so dead-code elimination cannot hide
# an accidentally reintroduced public software renderer from this gate.
"$test_compiler" "${test_flags[@]}" -r "${test_scene_sources[@]}" engine/source/gamelib/gpu_shader.c \
    -o "$test_build_dir/scene_dispatch.o"
bash tests/check_no_software_renderer.sh "$test_build_dir/scene_dispatch.o"

# Honor explicit dependency paths before consulting installed SDL metadata.
# Both flag sets are required together to avoid mixing separate installations.
if [[ -n ${OPENBOR_TEST_SDL_CFLAGS:-} && -n ${OPENBOR_TEST_SDL_LIBS:-} ]]; then
    read -r -a test_sdl_flags <<< "$OPENBOR_TEST_SDL_CFLAGS"
    read -r -a test_sdl_libs <<< "$OPENBOR_TEST_SDL_LIBS"
elif command -v sdl2-config >/dev/null 2>&1; then
    read -r -a test_sdl_flags <<< "$(sdl2-config --cflags)"
    read -r -a test_sdl_libs <<< "$(sdl2-config --libs)"
elif command -v pkg-config >/dev/null 2>&1 && pkg-config --exists sdl2; then
    read -r -a test_sdl_flags <<< "$(pkg-config --cflags sdl2)"
    read -r -a test_sdl_libs <<< "$(pkg-config --libs sdl2)"
else
    printf 'SDL2 development files were not found; native GL tests were not built.\n' >&2
    if [[ ${OPENBOR_REQUIRE_GPU_TESTS:-0} == 1 ]]; then
        exit 1
    fi
    printf 'Driver-independent scene and ownership tests passed. Test binaries: %s\n' "$test_build_dir"
    exit 0
fi

test_gpu_flags=("${test_flags[@]}" "${test_sdl_flags[@]}"
    -DSDL=1 -DLINUX=1 -DOPENGL=1 -DLOADGL=1 -Iengine/sdl
    -Iengine/source/pnglib -Iengine/source/webmlib)
test_gpu_sources=(engine/source/gamelib/gpu.c engine/source/gamelib/gpu_shader.c
    engine/source/gamelib/gpu_transform.c engine/source/gamelib/gpu_curve.c engine/sdl/gpu_renderer.c engine/sdl/loadgl.c
    "${test_profile_sources[@]}")
export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen}

# The script entry points need SDL headers but no graphics context. Reuse the
# standalone runner so local focused checks and the full suite test one path.
OPENBOR_TEST_BUILD_DIR="$test_build_dir" \
    OPENBOR_TEST_SDL_CFLAGS="${test_sdl_flags[*]}" \
    bash tests/run_drawmethod_color_script_tests.sh

# Capability failures must be prominent before the first scene draw even when
# profiling is disabled. Loader interception belongs only to this fixture.
"$test_compiler" "${test_gpu_flags[@]}" tests/test_gpu_capabilities.c \
    "${test_gpu_sources[@]}" "${test_link_flags[@]}" "${test_sdl_libs[@]}" \
    -Wl,--wrap=SDL_GL_GetProcAddress -o "$test_build_dir/test_gpu_capabilities"
run_test "$test_build_dir/test_gpu_capabilities"

# Inspect real buffer uploads and indexed submissions while injecting failures
# through the existing GL loader. No production-only diagnostics are required.
"$test_compiler" "${test_gpu_flags[@]}" tests/test_gpu_submission.c \
    "${test_gpu_sources[@]}" "${test_link_flags[@]}" "${test_sdl_libs[@]}" \
    -Wl,--wrap=SDL_GL_GetProcAddress -o "$test_build_dir/test_gpu_submission"
run_test "$test_build_dir/test_gpu_submission"

# Native suites require a real GL context. Keep lifecycle/capture checks,
# pixel-level transform comparisons, and presentation transitions separate
# so a failure identifies the affected boundary.
for test_name in test_gpu_renderer test_gpu_shutdown test_gpu_palette_cache test_gpu_state_cache test_gpu_profile; do
    "$test_compiler" "${test_gpu_flags[@]}" "tests/$test_name.c" \
        "${test_gpu_sources[@]}" "${test_link_flags[@]}" "${test_sdl_libs[@]}" \
        -o "$test_build_dir/$test_name"
    run_test "$test_build_dir/$test_name"
done

"$test_compiler" "${test_gpu_flags[@]}" tests/test_gpu_transform_gl.c \
    "${test_gpu_sources[@]}" engine/source/gamelib/pixelformat.c \
    "${test_link_flags[@]}" "${test_sdl_libs[@]}" \
    -o "$test_build_dir/test_gpu_transform_gl"
run_test "$test_build_dir/test_gpu_transform_gl"

# Link real dispatch and queue code while checking native color effects against
# independent expected pixels. The fixture rejects any software draw or fallback.
test_color_native_sources=("${test_gpu_sources[@]}")
for test_color_source in "${test_scene_sources[@]}"; do
    if [[ $test_color_source != engine/source/gamelib/gpu.c ]]; then
        test_color_native_sources+=("$test_color_source")
    fi
done
"$test_compiler" "${test_gpu_flags[@]}" tests/test_gpu_colors.c \
    "${test_color_native_sources[@]}" "${test_link_flags[@]}" "${test_sdl_libs[@]}" \
    -o "$test_build_dir/test_gpu_colors"
run_test "$test_build_dir/test_gpu_colors"

# Reconstruct all static filters against independent kernel/mapping oracles,
# including palette coverage, material operations and GPU-only source reuse.
OPENBOR_TEST_BUILD_DIR="$test_build_dir" \
    OPENBOR_TEST_SDL_CFLAGS="${test_sdl_flags[*]}" \
    OPENBOR_TEST_SDL_LIBS="${test_sdl_libs[*]}" \
    bash tests/run_gpu_resample_tests.sh

# Check analytic bends and perspective with native material/screen ownership.
OPENBOR_TEST_BUILD_DIR="$test_build_dir" \
    OPENBOR_TEST_SDL_CFLAGS="${test_sdl_flags[*]}" \
    OPENBOR_TEST_SDL_LIBS="${test_sdl_libs[*]}" \
    bash tests/run_gpu_curve_tests.sh

# Compare every display-filter pixel with independent RGB8 kernels, including
# source ownership, tiny edges, allocation rejection and context replacement.
OPENBOR_TEST_BUILD_DIR="$test_build_dir" \
    OPENBOR_TEST_SDL_CFLAGS="${test_sdl_flags[*]}" \
    OPENBOR_TEST_SDL_LIBS="${test_sdl_libs[*]}" \
    bash tests/run_gpu_output_filter_tests.sh

# Keep the desktop development launcher's bitmap text GPU-resident while
# preserving its byte repertoire, formatting, clipping and native draw order.
OPENBOR_TEST_BUILD_DIR="$test_build_dir" \
    OPENBOR_TEST_SDL_CFLAGS="${test_sdl_flags[*]}" \
    OPENBOR_TEST_SDL_LIBS="${test_sdl_libs[*]}" \
    bash tests/run_gpu_launcher_text_tests.sh

# Preserve movie color conversion, per-plane reconstruction, opaque clipping,
# resident source caching and native ownership across real context replacement.
OPENBOR_TEST_BUILD_DIR="$test_build_dir" \
    OPENBOR_TEST_SDL_CFLAGS="${test_sdl_flags[*]}" \
    OPENBOR_TEST_SDL_LIBS="${test_sdl_libs[*]}" \
    bash tests/run_gpu_movie_tests.sh

# Keep GIF palette expansion and retained channel composition native while
# checking real decoder streams, exact colors and explicit CPU-read coherence.
OPENBOR_TEST_BUILD_DIR="$test_build_dir" \
    OPENBOR_TEST_SDL_CFLAGS="${test_sdl_flags[*]}" \
    OPENBOR_TEST_SDL_LIBS="${test_sdl_libs[*]}" \
    bash tests/run_gpu_anigif_tests.sh

# Verify movie controller generation, presentation acknowledgement and script
# property validation without media workers or a software drawing substitute.
OPENBOR_TEST_BUILD_DIR="$test_build_dir" \
    OPENBOR_TEST_SDL_CFLAGS="${test_sdl_flags[*]}" \
    bash tests/run_movie_gpu_controller_tests.sh

# Reuse native pixel/ownership oracles with an actual GLES3 context, then test
# the production Android video lifecycle, final composition and failure gates.
OPENBOR_TEST_BUILD_DIR="$test_build_dir" \
    OPENBOR_TEST_SDL_CFLAGS="${test_sdl_flags[*]}" \
    OPENBOR_TEST_SDL_LIBS="${test_sdl_libs[*]}" \
    bash tests/run_gles_scene_tests.sh

OPENBOR_TEST_BUILD_DIR="$test_build_dir" \
    OPENBOR_TEST_SDL_CFLAGS="${test_sdl_flags[*]}" \
    OPENBOR_TEST_SDL_LIBS="${test_sdl_libs[*]}" \
    bash tests/run_android_scene_tests.sh

test_presentation_sources=("${test_gpu_sources[@]}" engine/sdl/opengl.c engine/sdl/video.c
    engine/sdl/videocommon.c engine/sdl/timer.c engine/source/gfxlib/gfx.c
    engine/source/video_settings.c)
"$test_compiler" "${test_gpu_flags[@]}" tests/test_gpu_presentation.c \
    "${test_presentation_sources[@]}" \
    "${test_link_flags[@]}" "${test_sdl_libs[@]}" \
    -o "$test_build_dir/test_gpu_presentation"
run_test "$test_build_dir/test_gpu_presentation"

# Initialization must reject a failed final-display program before reporting
# an active native mode. Only the status queries are deliberately intercepted.
"$test_compiler" "${test_gpu_flags[@]}" tests/test_gpu_presentation_failures.c \
    "${test_presentation_sources[@]}" \
    "${test_link_flags[@]}" "${test_sdl_libs[@]}" \
    -Wl,--wrap=SDL_GL_GetProcAddress -o "$test_build_dir/test_gpu_presentation_failures"
run_test "$test_build_dir/test_gpu_presentation_failures"

printf 'Renderer regression tests completed. Test binaries: %s\n' "$test_build_dir"

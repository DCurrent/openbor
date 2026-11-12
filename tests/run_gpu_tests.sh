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

# Keep unrelated renderer fixtures independent of the diagnostic branch's
# default-on setting. The native profiling suite explicitly enables its own
# collection windows and exercises GPU query opt-out separately.
export OPENBOR_PROFILE=0

# Run context-independent contracts first so hosts without SDL still verify
# stable ordering, transform arithmetic, and CPU/GPU ownership transitions.
"$test_compiler" "${test_flags[@]}" tests/test_spriteq_sort.c \
    "${test_profile_sources[@]}" "${test_link_flags[@]}" \
    -o "$test_build_dir/test_spriteq_sort"
run_test "$test_build_dir/test_spriteq_sort"

"$test_compiler" "${test_flags[@]}" tests/test_gpu_transform.c \
    engine/source/gamelib/gpu_transform.c engine/source/gamelib/gpu_shader.c \
    engine/source/gamelib/pixelformat.c "${test_profile_sources[@]}" "${test_link_flags[@]}" \
    -o "$test_build_dir/test_gpu_transform"
run_test "$test_build_dir/test_gpu_transform"

test_software_sources=(engine/source/gamelib/gpu.c
    engine/source/gamelib/screen.c engine/source/gamelib/screen16.c
    engine/source/gamelib/screen32.c engine/source/gamelib/draw.c
    engine/source/gamelib/draw16.c engine/source/gamelib/draw32.c
    engine/source/gamelib/sprite.c engine/source/gamelib/spritex8p16.c
    engine/source/gamelib/spritex8p32.c engine/source/gamelib/spriteq.c
    engine/source/gamelib/bitmap.c engine/source/gamelib/transform.c
    engine/source/gamelib/pixelformat.c engine/source/omath.c)
"$test_compiler" "${test_flags[@]}" tests/test_gpu_coherence.c \
    "${test_software_sources[@]}" "${test_profile_sources[@]}" "${test_link_flags[@]}" \
    -o "$test_build_dir/test_gpu_coherence"
run_test "$test_build_dir/test_gpu_coherence"

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
    printf 'CPU renderer tests passed. Test binaries: %s\n' "$test_build_dir"
    exit 0
fi

test_gpu_flags=("${test_flags[@]}" "${test_sdl_flags[@]}"
    -DSDL=1 -DLINUX=1 -DOPENGL=1 -DLOADGL=1 -Iengine/sdl
    -Iengine/source/pnglib -Iengine/source/webmlib)
test_gpu_sources=(engine/source/gamelib/gpu.c engine/source/gamelib/gpu_shader.c
    engine/source/gamelib/gpu_transform.c engine/sdl/gpu_renderer.c engine/sdl/loadgl.c
    "${test_profile_sources[@]}")
export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen}

# Native suites require a real GL context. Keep lifecycle/capture checks,
# pixel-level transform comparisons, and presentation transitions separate
# so a failure identifies the affected boundary.
for test_name in test_gpu_renderer test_gpu_shutdown test_gpu_palette_cache test_gpu_profile; do
    "$test_compiler" "${test_gpu_flags[@]}" "tests/$test_name.c" \
        "${test_gpu_sources[@]}" "${test_link_flags[@]}" "${test_sdl_libs[@]}" \
        -o "$test_build_dir/$test_name"
    run_test "$test_build_dir/$test_name"
done

"$test_compiler" "${test_gpu_flags[@]}" tests/test_gpu_transform_gl.c \
    "${test_gpu_sources[@]}" engine/source/gamelib/transform.c \
    engine/source/gamelib/pixelformat.c engine/source/gamelib/draw32.c \
    engine/source/omath.c "${test_link_flags[@]}" "${test_sdl_libs[@]}" \
    -o "$test_build_dir/test_gpu_transform_gl"
run_test "$test_build_dir/test_gpu_transform_gl"

"$test_compiler" "${test_gpu_flags[@]}" tests/test_gpu_presentation.c \
    "${test_gpu_sources[@]}" engine/sdl/opengl.c engine/sdl/video.c \
    engine/sdl/videocommon.c engine/sdl/timer.c \
    "${test_link_flags[@]}" "${test_sdl_libs[@]}" \
    -o "$test_build_dir/test_gpu_presentation"
run_test "$test_build_dir/test_gpu_presentation"

printf 'Renderer regression tests completed. Test binaries: %s\n' "$test_build_dir"

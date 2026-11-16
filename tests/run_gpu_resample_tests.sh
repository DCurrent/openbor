#!/usr/bin/env bash
# Damon V. Caskey
# 2026-11-15
# Build the independent resampling oracle against the production OpenGL backend.
set -euo pipefail

test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-resample-tests.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=c99 -O2 -Wall -Werror -D_DEFAULT_SOURCE -DPP_TEST
    -DSDL=1 -DLINUX=1 -DOPENGL=1 -DLOADGL=1
    -ffunction-sections -fdata-sections -Iengine -Iengine/sdl -Iengine/source
    -Iengine/source/gamelib -Iengine/source/gfxlib
    -Iengine/source/pnglib -Iengine/source/webmlib)
if [[ -n ${OPENBOR_TEST_CFLAGS:-} ]]; then
    read -r -a test_extra_flags <<< "$OPENBOR_TEST_CFLAGS"
    test_flags+=("${test_extra_flags[@]}")
fi

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
    printf 'SDL2 development files were not found; resampling GL tests were not built.\n' >&2
    if [[ ${OPENBOR_REQUIRE_GPU_TESTS:-0} == 1 ]]; then exit 1; fi
    exit 0
fi
test_flags+=("${test_sdl_flags[@]}" -UNDEBUG)

"$test_compiler" "${test_flags[@]}" tests/test_gpu_resample.c \
    engine/source/gamelib/gpu.c engine/source/gamelib/gpu_shader.c \
    engine/source/gamelib/gpu_transform.c engine/sdl/gpu_renderer.c \
    engine/sdl/loadgl.c engine/source/gamelib/engine_profile.c \
    -Wl,--gc-sections -lm "${test_sdl_libs[@]}" \
    -o "$test_build_dir/test_gpu_resample"

export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen}
export OPENBOR_PROFILE=0
if "$test_build_dir/test_gpu_resample"; then
    exit 0
else
    test_status=$?
    if [[ $test_status == 77 && ${OPENBOR_REQUIRE_GPU_TESTS:-0} != 1 ]]; then
        printf 'SKIP: resampling tests require an SDL/OpenGL context.\n'
        exit 0
    fi
    exit "$test_status"
fi

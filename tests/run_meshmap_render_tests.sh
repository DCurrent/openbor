#!/usr/bin/env bash
# Damon V. Caskey - 2026-11-27
set -euo pipefail
test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-meshmap-render.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=c99 -O2 -Wall -Werror -D_DEFAULT_SOURCE -DPP_TEST
    -DSDL=1 -DLINUX=1 -DOPENGL=1 -DLOADGL=1
    -ffunction-sections -fdata-sections -Iengine -Iengine/sdl -Iengine/source
    -Iengine/source/gamelib -Iengine/source/gfxlib -Iengine/source/pnglib
    -Iengine/source/webmlib -UNDEBUG)
if [[ -n ${OPENBOR_TEST_CFLAGS:-} ]]; then
    read -r -a test_extra_flags <<< "$OPENBOR_TEST_CFLAGS"
    test_flags+=("${test_extra_flags[@]}")
fi
if [[ -n ${OPENBOR_TEST_SDL_CFLAGS:-} && -n ${OPENBOR_TEST_SDL_LIBS:-} ]]; then
    read -r -a test_sdl_flags <<< "$OPENBOR_TEST_SDL_CFLAGS"
    read -r -a test_sdl_libs <<< "$OPENBOR_TEST_SDL_LIBS"
else
    read -r -a test_sdl_flags <<< "$(pkg-config --cflags sdl2)"
    read -r -a test_sdl_libs <<< "$(pkg-config --libs sdl2)"
fi
"$test_compiler" "${test_flags[@]}" "${test_sdl_flags[@]}" tests/test_meshmap_render.c \
    engine/source/gamelib/meshmap.c engine/source/gamelib/meshmap_render.c \
    engine/source/gamelib/gpu.c engine/source/gamelib/gpu_shader.c \
    engine/source/gamelib/gpu_transform.c engine/source/gamelib/gpu_curve.c \
    engine/sdl/gpu_renderer.c engine/sdl/loadgl.c engine/source/gamelib/engine_profile.c \
    -Wl,--gc-sections -lm "${test_sdl_libs[@]}" -o "$test_build_dir/test_meshmap_render"
export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen}
export OPENBOR_PROFILE=0
if "$test_build_dir/test_meshmap_render"; then exit 0; else
    test_status=$?
    if [[ $test_status == 77 && ${OPENBOR_REQUIRE_GPU_TESTS:-0} != 1 ]]; then
        printf 'SKIP: meshmap rendering requires an SDL/OpenGL context.\n'
        exit 0
    fi
    exit "$test_status"
fi

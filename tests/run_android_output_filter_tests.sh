#!/usr/bin/env bash
# Damon V. Caskey
# 2026-11-16
# Exercise the production Android filter adapter in a real host GLES2 renderer.
set -euo pipefail

test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-android-output-filter.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=c99 -O2 -Wall -Wextra -Werror -D_DEFAULT_SOURCE -DPP_TEST
    -DSDL=1 -DOPENBOR_ANDROID_FILTER_TEST -Iengine -Iengine/source
    -Iengine/source/gamelib -Iengine/source/gfxlib -Iengine/sdl
    -Iengine/android/app/jni/openbor/include/sdl)
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
    printf 'SDL2 development files are required for Android output-filter host tests.\n' >&2
    exit 77
fi

"$test_compiler" "${test_flags[@]}" "${test_sdl_flags[@]}" -UNDEBUG \
    tests/test_android_output_filter.c engine/sdl/gpu_output_filter_android.c \
    engine/source/gamelib/gpu_shader.c "${test_sdl_libs[@]}" -lm \
    -Wl,--wrap=SDL_GL_GetProcAddress -Wl,--wrap=SDL_RenderReadPixels \
    -Wl,--wrap=SDL_UpdateTexture -o "$test_build_dir/test_android_output_filter"

export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen}
# Linux hosts can expose the GLES API through GLVND's libGL even when a separate
# libGLESv2 loader is absent. Select only the loader in this host fixture; SDL
# still requests and verifies a real ES context, with no desktop shader retry.
if [[ -z ${SDL_VIDEO_GL_DRIVER:-} ]]; then
    test_gl_library=$(python3 - <<'PY'
import ctypes.util
print('' if ctypes.util.find_library('GLESv2') else (ctypes.util.find_library('GL') or ''))
PY
    )
    if [[ -n $test_gl_library ]]; then
        export SDL_VIDEO_GL_DRIVER=$test_gl_library
    fi
fi
export MESA_GLES_VERSION_OVERRIDE=${MESA_GLES_VERSION_OVERRIDE:-2.0}
if "$test_build_dir/test_android_output_filter"; then
    exit 0
else
    test_status=$?
    if [[ $test_status == 77 && ${OPENBOR_REQUIRE_GPU_TESTS:-0} == 1 ]]; then
        printf 'Required Android output-filter GLES2 host renderer is unavailable.\n' >&2
        exit 1
    fi
    exit "$test_status"
fi

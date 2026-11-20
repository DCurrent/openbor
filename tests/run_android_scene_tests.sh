#!/usr/bin/env bash
# Damon V. Caskey
# 2026-11-20
# Compile the actual Android video boundary against real GLES3 and the shared
# scene renderer. Only platform assets, clock and final swap observation differ.
set -euo pipefail

test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-android-scene.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=c99 -O2 -Wall -Wextra -Werror -D_DEFAULT_SOURCE -DPP_TEST
    -DSDL=1 -DLINUX=1 -DOPENGL=1 -DOPENBOR_GLES=1 -DOPENBOR_ANDROID_SCENE_TEST=1
    -ffunction-sections -fdata-sections -Iengine -Iengine/sdl -Iengine/source
    -Iengine/source/gamelib -Iengine/source/gfxlib -Iengine/source/pnglib
    -Iengine/source/webmlib "-I$test_build_dir")
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
    printf 'SDL2 development files are required for Android scene tests.\n' >&2
    if [[ ${OPENBOR_REQUIRE_GPU_TESTS:-0} == 1 ]]; then exit 1; fi
    exit 77
fi
test_gl_library=$(python3 - <<'PY'
import ctypes.util
print(ctypes.util.find_library('GLESv2') or ctypes.util.find_library('GL') or '')
PY
)
if [[ -z $test_gl_library ]]; then
    printf 'A GLES3 runtime is required for Android scene tests.\n' >&2
    if [[ ${OPENBOR_REQUIRE_GPU_TESTS:-0} == 1 ]]; then exit 1; fi
    exit 77
fi

python3 - "$test_build_dir" <<'PY'
from pathlib import Path
import sys
destination = Path(sys.argv[1])
video = Path('engine/android/app/jni/openbor/video.c').read_text()
start = video.index('SDL_Window *window = NULL;')
end = video.index('#include "button_png_800x480.h"', start)
(destination / 'android_scene_video_globals.inc').write_text(video[start:end])
start = video.index('/* Android native GPU presentation boundary. */')
(destination / 'android_scene_video_functions.inc').write_text(video[start:])
def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        if source[end] == '{': depth += 1
        elif source[end] == '}': depth -= 1
        end += 1
    return source[start:end] + '\n'
menu = Path('engine/sdl/menu.c').read_text()
(destination / 'android_scene_menu_functions.inc').write_text(
    function(menu, 'static void printText(') + '\n' +
    function(menu, 'static void blit_video_menu('))
screen = Path('engine/source/gamelib/screen.c').read_text()
(destination / 'android_scene_allocation_functions.inc').write_text(
    function(screen, 's_screen *allocscreen(') + '\n' +
    function(screen, 'void freescreen('))
PY

"$test_compiler" "${test_flags[@]}" "${test_sdl_flags[@]}" -UNDEBUG \
    tests/test_android_scene.c engine/source/gamelib/gpu.c \
    engine/source/gamelib/gpu_shader.c engine/source/gamelib/gpu_transform.c \
    engine/sdl/gpu_renderer.c engine/source/gamelib/engine_profile.c \
    engine/sdl/launcher_text.c "${test_sdl_libs[@]}" "-l:$test_gl_library" -lm \
    -Wl,--gc-sections -Wl,--wrap=SDL_GL_SwapWindow \
    -Wl,--wrap=SDL_GL_CreateContext -Wl,--wrap=SDL_GL_DeleteContext \
    -Wl,--wrap=SDL_GL_GetDrawableSize \
    -Wl,--wrap=SDL_UpdateTexture -Wl,--wrap=SDL_CreateRenderer \
    -Wl,--wrap=glGetShaderiv -Wl,--wrap=glCompileShader \
    -Wl,--wrap=glDrawArrays -Wl,--wrap=malloc -Wl,--wrap=gpu_log \
    -o "$test_build_dir/test_android_scene"

export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen}
export SDL_VIDEO_GL_DRIVER=${SDL_VIDEO_GL_DRIVER:-$test_gl_library}
export MESA_GLES_VERSION_OVERRIDE=${MESA_GLES_VERSION_OVERRIDE:-3.0}
export OPENBOR_PROFILE=0
if "$test_build_dir/test_android_scene"; then
    printf 'Android production scene/presentation/lifecycle integration passed.\n'
else
    test_status=$?
    if [[ $test_status == 77 && ${OPENBOR_REQUIRE_GPU_TESTS:-0} == 1 ]]; then
        printf 'Required Android GLES3 context is unavailable.\n' >&2
        exit 1
    fi
    exit "$test_status"
fi

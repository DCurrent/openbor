#!/usr/bin/env bash
# Damon V. Caskey
# 2026-11-20
# Reuse independent native pixel/ownership oracles against the Android GLES3
# implementation. Only context selection and unsupported legacy GL references
# differ; expected scene pixels and resource assertions remain shared.
set -euo pipefail

test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-gles-tests.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=c99 -O2 -Wall -Werror -D_DEFAULT_SOURCE -DPP_TEST
    -DSDL=1 -DLINUX=1 -DOPENGL=1 -DLOADGL=1 -DOPENBOR_GLES=1
    -ffunction-sections -fdata-sections -Iengine -Iengine/sdl -Iengine/source
    -Iengine/source/gamelib -Iengine/source/gfxlib -Iengine/source/pnglib
    -Iengine/source/webmlib)
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
    printf 'SDL2 development files are required for Android GLES3 scene tests.\n' >&2
    if [[ ${OPENBOR_REQUIRE_GPU_TESTS:-0} == 1 ]]; then exit 1; fi
    exit 77
fi
test_flags+=("${test_sdl_flags[@]}" -UNDEBUG)
test_sources=(engine/source/gamelib/gpu.c engine/source/gamelib/gpu_shader.c
    engine/source/gamelib/gpu_transform.c engine/source/gamelib/gpu_curve.c engine/sdl/gpu_renderer.c
    engine/sdl/loadgl.c engine/source/gamelib/engine_profile.c)
test_link_flags=(-Wl,--gc-sections -lm "${test_sdl_libs[@]}")

export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen}
export OPENBOR_PROFILE=0
# GLVND's desktop library also exposes ES on hosts without a separate GLES
# library. Every executable verifies the actual context is OpenGL ES >= 3.
if [[ -z ${SDL_VIDEO_GL_DRIVER:-} ]]; then
    test_gl_library=$(python3 - <<'PY'
import ctypes.util
print('' if ctypes.util.find_library('GLESv2') else (ctypes.util.find_library('GL') or ''))
PY
    )
    if [[ -n $test_gl_library ]]; then export SDL_VIDEO_GL_DRIVER=$test_gl_library; fi
fi
export MESA_GLES_VERSION_OVERRIDE=${MESA_GLES_VERSION_OVERRIDE:-3.0}

# Focused local reruns can select a subset after an observed failure. The full
# integration runner leaves this unset and executes every listed contract.
read -r -a test_cases <<< "${OPENBOR_GLES_TEST_CASES:-renderer shutdown palette_cache state_cache transform_gl colors resample curve output_filter launcher_text movie movie_failures anigif submission capabilities lifecycle}"
for test_case in "${test_cases[@]}"; do
    test_extra_sources=()
    test_wrappers=()
    test_source="tests/test_gpu_$test_case.c"
    case "$test_case" in
        renderer|shutdown|palette_cache|state_cache) ;;
        transform_gl)
            test_extra_sources=(engine/source/gamelib/pixelformat.c) ;;
        colors)
            test_extra_sources=(engine/source/gamelib/screen.c engine/source/gamelib/draw.c
                engine/source/gamelib/sprite.c engine/source/gamelib/spriteq.c
                engine/source/gamelib/bitmap.c engine/source/gamelib/pixelformat.c) ;;
        resample|curve|movie|movie_failures) ;;
        anigif)
            test_extra_sources=(engine/source/gamelib/anigif.c
                engine/source/gamelib/screen.c engine/source/gamelib/pixelformat.c) ;;
        output_filter|submission|capabilities)
            test_wrappers=(-Wl,--wrap=SDL_GL_GetProcAddress) ;;
        lifecycle)
            test_source=tests/test_gles_lifecycle.c
            test_wrappers=(-Wl,--wrap=SDL_GL_GetProcAddress) ;;
        launcher_text)
            test_extra_sources=(engine/sdl/launcher_text.c)
            test_wrappers=(-Wl,--wrap=SDL_GL_GetProcAddress -Wl,--wrap=malloc) ;;
        *) printf 'Unknown GLES scene test: %s\n' "$test_case" >&2; exit 1 ;;
    esac
    test_binary="$test_build_dir/test_gles_$test_case"
    "$test_compiler" "${test_flags[@]}" "$test_source" \
        "${test_sources[@]}" "${test_extra_sources[@]}" \
        "${test_wrappers[@]}" "${test_link_flags[@]}" -o "$test_binary"
    if "$test_binary"; then
        printf 'Android GLES3 scene contract passed: %s\n' "$test_case"
    else
        test_status=$?
        if [[ $test_status == 77 && ${OPENBOR_REQUIRE_GPU_TESTS:-0} != 1 ]]; then
            printf 'SKIP: Android GLES3 context unavailable for %s.\n' "$test_case"
            exit 0
        fi
        exit "$test_status"
    fi
done
printf 'Android GLES3 scene parity suites completed.\n'

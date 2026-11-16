#!/usr/bin/env bash
# Damon V. Caskey
# 2026-11-15
# Exercise real movie control and script properties without SDL runtime or codecs.
set -euo pipefail

test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-movie-controller.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=gnu99 -O1 -D_DEFAULT_SOURCE -DPP_TEST -DWEBM=1 -DSDL=1 -DLINUX=1
    -ffunction-sections -fdata-sections -Iengine -Iengine/sdl -Iengine/source
    -Iengine/source/gamelib -Iengine/source/gfxlib -Iengine/source/openborscript
    -Iengine/source/pnglib -Iengine/source/preprocessorlib -Iengine/source/ramlib
    -Iengine/source/randlib -Iengine/source/scriptlib -Iengine/source/webmlib
    -Iengine/source/webmlib/halloc -Iengine/source/webmlib/nestegg)
if [[ -n ${OPENBOR_TEST_CFLAGS:-} ]]; then
    read -r -a test_extra_flags <<< "$OPENBOR_TEST_CFLAGS"
    test_flags+=("${test_extra_flags[@]}")
fi
if [[ -n ${OPENBOR_TEST_SDL_CFLAGS:-} ]]; then
    read -r -a test_sdl_flags <<< "$OPENBOR_TEST_SDL_CFLAGS"
elif command -v sdl2-config >/dev/null 2>&1; then
    read -r -a test_sdl_flags <<< "$(sdl2-config --cflags)"
elif command -v pkg-config >/dev/null 2>&1 && pkg-config --exists sdl2; then
    read -r -a test_sdl_flags <<< "$(pkg-config --cflags sdl2)"
else
    printf 'SDL2 headers are required to compile production script APIs.\n' >&2
    exit 1
fi
test_flags+=("${test_sdl_flags[@]}")
test_flags+=(-UNDEBUG)
if [[ ! -f engine/version.h ]]; then
    mkdir -p "$test_build_dir/include"
    cat > "$test_build_dir/include/version.h" <<'VERSION_HEADER'
/* Standalone movie controller test; normal builds generate engine/version.h. */
#ifndef VERSION_H
#define VERSION_H
#define VERSION_NAME "OpenBOR"
#define VERSION_MAJOR "4"
#define VERSION_MINOR "0"
#define VERSION_BUILD "0"
#define VERSION_BUILD_INT 0
#define VERSION_COMMIT "movie-controller-test"
#define VERSION "movie-controller-test"
#endif
VERSION_HEADER
    test_flags+=("-I$test_build_dir/include")
fi

"$test_compiler" "${test_flags[@]}" \
    tests/test_movie_gpu_controller.c engine/source/webmlib/yuv.c \
    engine/source/openborscript/movie.c engine/source/openborscript/constants.c \
    engine/source/openborscript/common_property.c engine/source/utils.c \
    engine/source/gamelib/sound_channel.c \
    engine/source/scriptlib/ScriptVariant.c engine/source/scriptlib/ScriptVariantInteger.c \
    engine/source/scriptlib/List.c -Wl,--gc-sections -Wl,--wrap=checkAlloc -lm \
    -o "$test_build_dir/test_movie_gpu_controller"

test_log="$test_build_dir/test_movie_gpu_controller.log"
if "$test_build_dir/test_movie_gpu_controller" > "$test_log" 2>&1; then
    tail -n 1 "$test_log"
else
    test_status=$?
    cat "$test_log"
    exit "$test_status"
fi

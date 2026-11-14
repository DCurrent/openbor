#!/usr/bin/env bash
# Damon V. Caskey
# 2026-11-14
# Link real script entry points without starting the game or creating GL state.
set -euo pipefail

test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-color-script.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}

# These translation units contain unrelated legacy warnings. Keep assertions
# enabled while leaving the renderer runner's strict warning policy unchanged.
test_flags=(-std=gnu99 -O1 -D_DEFAULT_SOURCE -DPP_TEST -DSDL=1 -DLINUX=1
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
test_flags+=("${test_sdl_flags[@]}" -UNDEBUG)

# Normal builds generate this header. A standalone fixture does not consume its
# version values, so use a clearly labelled build-directory header if absent.
if [[ ! -f engine/version.h ]]; then
    mkdir -p "$test_build_dir/include"
    cat > "$test_build_dir/include/version.h" <<'VERSION_HEADER'
/* Standalone script test only; generated game builds use engine/version.h. */
#ifndef VERSION_H
#define VERSION_H
#define VERSION_NAME "OpenBOR"
#define VERSION_MAJOR "4"
#define VERSION_MINOR "0"
#define VERSION_BUILD "0"
#define VERSION_BUILD_INT 0
#define VERSION_COMMIT "script-test"
#define VERSION "script-test"
#endif
VERSION_HEADER
    test_flags+=("-I$test_build_dir/include")
fi

"$test_compiler" "${test_flags[@]}" \
    tests/test_drawmethod_color_script.c engine/openborscript.c \
    engine/source/openborscript/drawmethod.c engine/source/openborscript/constants.c \
    engine/source/openborscript/common_property.c engine/source/utils.c \
    engine/source/scriptlib/ScriptVariant.c engine/source/scriptlib/ScriptVariantInteger.c \
    engine/source/scriptlib/List.c -Wl,--gc-sections \
    -Wl,--wrap=searchList -Wl,--wrap=checkAlloc -lm \
    -o "$test_build_dir/test_drawmethod_color_script"

# Invalid input is deliberate coverage. Retain its production diagnostics in a
# log, and show the complete log if any contract assertion fails.
test_log="$test_build_dir/test_drawmethod_color_script.log"
if "$test_build_dir/test_drawmethod_color_script" > "$test_log" 2>&1; then
    tail -n 1 "$test_log"
else
    test_status=$?
    cat "$test_log"
    exit "$test_status"
fi

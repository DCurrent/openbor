#!/usr/bin/env bash
# Compile real VM-facing terrain APIs without starting SDL or a GL context.
set -euo pipefail

test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-meshmap-script.XXXXXX")}
mkdir -p "$test_build_dir/fixtures"
test_compiler=${CC:-cc}
test_flags=(-std=gnu99 -O1 -D_DEFAULT_SOURCE -DPP_TEST -DSDL=1 -DLINUX=1
    -ffunction-sections -fdata-sections -Iengine -Iengine/sdl -Iengine/source
    -Iengine/source/gamelib -Iengine/source/gfxlib -Iengine/source/openborscript
    -Iengine/source/pnglib -Iengine/source/preprocessorlib -Iengine/source/ramlib
    -Iengine/source/randlib -Iengine/source/scriptlib -Iengine/source/webmlib)

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
test_link_flags=(-lpng -lz -lm)
if [[ -n ${OPENBOR_TEST_LDFLAGS:-} ]]; then
    read -r -a test_extra_link <<< "$OPENBOR_TEST_LDFLAGS"
    test_link_flags+=("${test_extra_link[@]}")
fi
if [[ ! -f engine/version.h ]]; then
    mkdir -p "$test_build_dir/include"
    cat > "$test_build_dir/include/version.h" <<'VERSION_HEADER'
#ifndef VERSION_H
#define VERSION_H
#define VERSION_NAME "OpenBOR"
#define VERSION_MAJOR "4"
#define VERSION_MINOR "0"
#define VERSION_BUILD "0"
#define VERSION_BUILD_INT 0
#define VERSION_COMMIT "meshmap-script-test"
#define VERSION "meshmap-script-test"
#endif
VERSION_HEADER
    test_flags+=("-I$test_build_dir/include")
fi

# Generate tiny deterministic PNGs using only the standard library. Numerical
# samples include both endpoints and an exact midpoint; artwork has a different
# width to catch accidental source-size coupling and four-pixel truncation.
python3 - "$test_build_dir/fixtures" <<'PY'
import pathlib, struct, sys, zlib
out = pathlib.Path(sys.argv[1])
def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
def png(name, width, height, depth, color, rows, palette=b''):
    data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, depth, color, 0, 0, 0))
    if palette: data += chunk(b'PLTE', palette)
    data += chunk(b'IDAT', zlib.compress(b''.join(b'\0' + row for row in rows))) + chunk(b'IEND', b'')
    (out / name).write_bytes(data)
samples = [1, 1000, 65535, 12000, 32768, 45000, 65535, 50000, 1]
png('geometry.png', 3, 3, 16, 0, [struct.pack('>3H', *samples[i:i+3]) for i in range(0, 9, 3)])
png('indexed.png', 5, 3, 8, 3, [bytes([0, 1, 1, 0, 1])] * 3, bytes([5, 10, 15, 210, 220, 230]))
png('rgb.png', 5, 3, 8, 2, [bytes([12, 50, 90] * 5)] * 3)
PY

"$test_compiler" "${test_flags[@]}" \
    tests/test_meshmap_script.c \
    engine/source/openborscript/meshmap.c engine/source/openborscript/level.c \
    engine/source/openborscript/entity.c engine/source/openborscript/constants.c \
    engine/source/openborscript/common_property.c engine/source/utils.c \
    engine/source/gamelib/meshmap.c engine/source/gamelib/meshmap_image.c \
    engine/source/gamelib/pixelformat.c \
    engine/source/scriptlib/ScriptVariant.c engine/source/scriptlib/ScriptVariantInteger.c \
    engine/source/scriptlib/List.c -Wl,--gc-sections -Wl,--wrap=checkAlloc \
    "${test_link_flags[@]}" -o "$test_build_dir/test_meshmap_script"

test_log="$test_build_dir/test_meshmap_script.log"
if "$test_build_dir/test_meshmap_script" "$test_build_dir/fixtures" > "$test_log" 2>&1; then
    tail -n 1 "$test_log"
else
    test_status=$?
    cat "$test_log"
    exit "$test_status"
fi

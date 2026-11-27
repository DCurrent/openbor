#!/usr/bin/env bash
# Link the real movement adapter and numerical terrain core without a window.
set -euo pipefail
test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-meshmap-entity.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=gnu99 -O2 -Wall -Werror -D_DEFAULT_SOURCE -DPP_TEST -DSDL=1 -DLINUX=1
    -Iengine -Iengine/sdl -Iengine/source -Iengine/source/gamelib -Iengine/source/gfxlib
    -Iengine/source/openborscript -Iengine/source/pnglib -Iengine/source/preprocessorlib
    -Iengine/source/ramlib -Iengine/source/randlib -Iengine/source/scriptlib
    -Iengine/source/webmlib -UNDEBUG)
if [[ -n ${OPENBOR_TEST_CFLAGS:-} ]]; then
    read -r -a test_extra_flags <<< "$OPENBOR_TEST_CFLAGS"
    test_flags+=("${test_extra_flags[@]}")
fi
if [[ -n ${OPENBOR_TEST_SDL_CFLAGS:-} ]]; then
    read -r -a test_sdl_flags <<< "$OPENBOR_TEST_SDL_CFLAGS"
else
    read -r -a test_sdl_flags <<< "$(pkg-config --cflags sdl2)"
fi
test_flags+=("${test_sdl_flags[@]}")
if [[ ! -f engine/version.h ]]; then
    mkdir -p "$test_build_dir/include"
    cat > "$test_build_dir/include/version.h" <<'VERSION_HEADER'
#define VERSION_NAME "OpenBOR"
#define VERSION_MAJOR "4"
#define VERSION_MINOR "0"
#define VERSION_BUILD "0"
#define VERSION_BUILD_INT 0
#define VERSION_COMMIT "meshmap-test"
#define VERSION "meshmap-test"
VERSION_HEADER
    test_flags+=("-I$test_build_dir/include")
fi
"$test_compiler" "${test_flags[@]}" tests/test_meshmap_entity.c \
    engine/source/gamelib/meshmap.c engine/source/gamelib/meshmap_entity.c \
    -lm -o "$test_build_dir/test_meshmap_entity"
"$test_build_dir/test_meshmap_entity"

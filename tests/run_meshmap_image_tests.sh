#!/usr/bin/env bash
# Damon V. Caskey - 2026-11-27
set -euo pipefail
test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-meshmap-image.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=c99 -O2 -Wall -Werror -D_DEFAULT_SOURCE -DPP_TEST
    -Iengine -Iengine/source -Iengine/source/gamelib -UNDEBUG)
if [[ -n ${OPENBOR_TEST_CFLAGS:-} ]]; then
    read -r -a test_extra_flags <<< "$OPENBOR_TEST_CFLAGS"
    test_flags+=("${test_extra_flags[@]}")
fi
if [[ -n ${OPENBOR_TEST_PNG_CFLAGS:-} && -n ${OPENBOR_TEST_PNG_LIBS:-} ]]; then
    read -r -a test_png_flags <<< "$OPENBOR_TEST_PNG_CFLAGS"
    read -r -a test_png_libs <<< "$OPENBOR_TEST_PNG_LIBS"
else
    read -r -a test_png_flags <<< "$(pkg-config --cflags libpng)"
    read -r -a test_png_libs <<< "$(pkg-config --libs libpng)"
fi
"$test_compiler" "${test_flags[@]}" "${test_png_flags[@]}" tests/test_meshmap_image.c \
    engine/source/gamelib/meshmap.c engine/source/gamelib/meshmap_image.c \
    "${test_png_libs[@]}" -lm -o "$test_build_dir/test_meshmap_image"
"$test_build_dir/test_meshmap_image" "$test_build_dir"

#!/usr/bin/env bash
# Damon V. Caskey - 2026-11-26.
# Geometry, sparse ownership and solid contacts need no SDL or graphics context.
set -euo pipefail

test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-meshmap.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=c99 -O2 -Wall -Wextra -Werror -pedantic -D_DEFAULT_SOURCE
    -Iengine/source/gamelib -UNDEBUG)
if [[ -n ${OPENBOR_TEST_CFLAGS:-} ]]; then
    read -r -a test_extra_flags <<< "$OPENBOR_TEST_CFLAGS"
    test_flags+=("${test_extra_flags[@]}")
fi
"$test_compiler" "${test_flags[@]}" tests/test_meshmap.c \
    engine/source/gamelib/meshmap.c -lm -o "$test_build_dir/test_meshmap"
"$test_build_dir/test_meshmap"

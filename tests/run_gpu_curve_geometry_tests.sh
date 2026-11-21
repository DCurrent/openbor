#!/usr/bin/env bash
# Damon V. Caskey
# 2026-11-20
# Native curve preparation checks do not require a graphics context.
set -euo pipefail

test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-curve-geometry.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=c99 -O2 -Wall -Werror -D_DEFAULT_SOURCE -DPP_TEST
    -Iengine/source -Iengine/source/gamelib -UNDEBUG)
if [[ -n ${OPENBOR_TEST_CFLAGS:-} ]]; then
    read -r -a test_extra_flags <<< "$OPENBOR_TEST_CFLAGS"
    test_flags+=("${test_extra_flags[@]}")
fi
"$test_compiler" "${test_flags[@]}" tests/test_gpu_curve_geometry.c \
    engine/source/gamelib/gpu_curve.c -lm -o "$test_build_dir/test_gpu_curve_geometry"
"$test_build_dir/test_gpu_curve_geometry"

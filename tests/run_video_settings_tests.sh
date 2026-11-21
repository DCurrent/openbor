#!/usr/bin/env bash
# Damon V. Caskey
# 2026-11-20
# Exercise saved display controls and the actual menu actions without a window.
set -euo pipefail

test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-video-settings.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=c99 -O2 -Wall -Wextra -Werror -D_DEFAULT_SOURCE -DSDL=1
    -Iengine/source -Iengine/source/gamelib -Iengine/source/gfxlib "-I$test_build_dir")
if [[ -n ${OPENBOR_TEST_CFLAGS:-} ]]; then
    read -r -a test_extra_flags <<< "$OPENBOR_TEST_CFLAGS"
    test_flags+=("${test_extra_flags[@]}")
fi
test_flags+=(-UNDEBUG)

# The full engine owns these entry points. Extract their bodies, following the
# Android launcher fixture, so the test observes production menu/save behavior
# instead of maintaining a second implementation of either state machine.
python3 - "$test_build_dir" <<'PY'
from pathlib import Path
import re
import sys

destination = Path(sys.argv[1])
source = Path('engine/openbor.c').read_text()

def function(signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        if source[end] == '{':
            depth += 1
        elif source[end] == '}':
            depth -= 1
        end += 1
    return source[start:end] + '\n'

header = Path('engine/openbor.h').read_text()
version = re.search(r'^#define\s+COMPATIBLEVERSION\s+\S+', header, re.M)
if not version:
    raise RuntimeError('Missing production settings compatibility version')
(destination / 'video_settings_engine.inc').write_text(
    version.group(0) + '\n\n' + '\n'.join(function(signature) for signature in (
        'void clearsettings()',
        'void savesettings()',
        'void saveasdefault()',
        'void loadsettings()',
        'void loadfromdefault()',
        'static int menu_video_option_enabled(',
        'static void menu_video_adjust(',
    )))
PY

for test_platform in desktop android; do
    test_platform_flags=()
    if [[ $test_platform == android ]]; then
        test_platform_flags+=(-DANDROID=1)
    fi
    "$test_compiler" "${test_flags[@]}" "${test_platform_flags[@]}" \
        tests/test_video_settings.c engine/source/video_settings.c -lm \
        -o "$test_build_dir/test_video_settings_$test_platform"
    mkdir -p "$test_build_dir/video-settings-$test_platform"
    "$test_build_dir/test_video_settings_$test_platform" \
        "$test_build_dir/video-settings-$test_platform"
done

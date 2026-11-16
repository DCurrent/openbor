#!/usr/bin/env bash
# Damon V. Caskey
# 2026-11-15
# Test exact Android movie adapter functions on a host accelerated SDL renderer.
set -euo pipefail

test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-android-movie.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=c99 -O2 -Wall -Wextra -Werror -D_DEFAULT_SOURCE -DPP_TEST -DSDL=1
    -Iengine -Iengine/source -Iengine/source/gamelib -Iengine/source/webmlib
    "-I$test_build_dir")
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
    printf 'SDL2 development files are required for Android adapter host tests.\n' >&2
    exit 77
fi

python3 - "$test_build_dir/android_movie_functions.inc" <<'PY'
from pathlib import Path
import sys
source = Path('engine/android/app/jni/openbor/video.c').read_text()
restore = source[source.index('int video_restore_mode(void)'):source.index('void video_fullscreen_flip()')]
movies = source[source.index('static SDL_Texture *video_android_movie_texture('):source.index('int video_current_refresh_rate()')]
Path(sys.argv[1]).write_text(restore + '\n' + movies)
PY

"$test_compiler" "${test_flags[@]}" "${test_sdl_flags[@]}" -UNDEBUG \
    tests/test_android_movie.c "${test_sdl_libs[@]}" -lm \
    -o "$test_build_dir/test_android_movie"

if "$test_build_dir/test_android_movie"; then
    exit 0
else
    test_status=$?
    if [[ $test_status == 77 && ${OPENBOR_REQUIRE_GPU_TESTS:-0} == 1 ]]; then
        printf 'Required Android adapter host acceleration is unavailable.\n' >&2
        exit 1
    fi
    exit "$test_status"
fi

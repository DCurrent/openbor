#!/usr/bin/env bash
# Exercise unchanged production movement hooks without linking the full game.
set -euo pipefail
test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-meshmap-hooks.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=gnu99 -O2 -Wall -Werror -D_DEFAULT_SOURCE -DPP_TEST -DSDL=1 -DLINUX=1
    -Iengine -Iengine/sdl -Iengine/source -Iengine/source/gamelib -Iengine/source/gfxlib
    -Iengine/source/openborscript -Iengine/source/pnglib -Iengine/source/preprocessorlib
    -Iengine/source/ramlib -Iengine/source/randlib -Iengine/source/scriptlib
    -Iengine/source/webmlib -I"$test_build_dir" -UNDEBUG)
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
#define VERSION_COMMIT "meshmap-hooks-test"
#define VERSION "meshmap-hooks-test"
VERSION_HEADER
    test_flags+=("-I$test_build_dir/include")
fi

# openbor.c is a single large translation unit. Copy the exact selected
# functions into a temporary include, preserving source locations, so unrelated
# platform/event dependencies can be stubbed without replacing the code under
# test. Function bodies are never maintained separately in this fixture.
python3 - "$test_build_dir/meshmap_native_hooks.inc" <<'PY'
import pathlib, re, sys
source = pathlib.Path('engine/openbor.c').read_text()
tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*[\s\S]*?\*/|//[^\n]*|[{}]')
parts = []
for name in ('diff', 'inair', 'check_basemap', 'check_basemap_index', 'checkbase',
             'adjust_base', 'check_move', 'check_gravity'):
    matches = list(re.finditer(r'(?m)^(?:float|int|void) ' + re.escape(name) + r'\([^;{}]*\)\s*\{', source))
    if len(matches) != 1:
        raise SystemExit(f'Expected exactly one production definition of {name}')
    match = matches[0]
    depth = 0
    for token in tokens.finditer(source, match.end() - 1):
        if token.group() == '{': depth += 1
        elif token.group() == '}':
            depth -= 1
            if depth == 0:
                line = source.count('\n', 0, match.start()) + 1
                parts.append(f'#line {line} "engine/openbor.c"\n' + source[match.start():token.end()] + '\n')
                break
    else:
        raise SystemExit(f'Unterminated production definition of {name}')
pathlib.Path(sys.argv[1]).write_text('\n'.join(parts))
PY

"$test_compiler" "${test_flags[@]}" tests/test_meshmap_native_hooks.c \
    engine/source/gamelib/meshmap.c engine/source/gamelib/meshmap_entity.c \
    -lm -o "$test_build_dir/test_meshmap_native_hooks"
"$test_build_dir/test_meshmap_native_hooks"

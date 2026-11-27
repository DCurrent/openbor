#!/usr/bin/env bash
# Exercise the production level-sheet path without starting the game runtime.
set -euo pipefail
test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$test_root"
test_build_dir=${OPENBOR_TEST_BUILD_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/openbor-meshmap-level.XXXXXX")}
mkdir -p "$test_build_dir"
test_compiler=${CC:-cc}
test_flags=(-std=gnu99 -O1 -D_DEFAULT_SOURCE -DPP_TEST -DSDL=1 -DLINUX=1
    -ffunction-sections -fdata-sections -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
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
elif command -v sdl2-config >/dev/null 2>&1; then
    read -r -a test_sdl_flags <<< "$(sdl2-config --cflags)"
elif command -v pkg-config >/dev/null 2>&1 && pkg-config --exists sdl2; then
    read -r -a test_sdl_flags <<< "$(pkg-config --cflags sdl2)"
else
    printf 'SDL2 headers are required to compile production level authoring.\n' >&2
    exit 1
fi
test_flags+=("${test_sdl_flags[@]}")
test_link_flags=(-lm)
if [[ -n ${OPENBOR_TEST_LDFLAGS:-} ]]; then
    read -r -a test_extra_link <<< "$OPENBOR_TEST_LDFLAGS"
    test_link_flags+=("${test_extra_link[@]}")
fi
if [[ ! -f engine/version.h ]]; then
    mkdir -p "$test_build_dir/include"
    cat > "$test_build_dir/include/version.h" <<'VERSION_HEADER'
#define VERSION_NAME "OpenBOR"
#define VERSION_MAJOR "4"
#define VERSION_MINOR "0"
#define VERSION_BUILD "0"
#define VERSION_BUILD_INT 0
#define VERSION_COMMIT "meshmap-level-test"
#define VERSION "meshmap-level-test"
VERSION_HEADER
    test_flags+=("-I$test_build_dir/include")
fi

# Follow the native-hook fixture's source extraction approach. Tokenization and
# dispatch adapter bodies come from openbor.c on each run, never a test copy.
python3 - "$test_build_dir/meshmap_level_loader.inc" <<'PY'
import pathlib, re, sys
source = pathlib.Path('engine/openbor.c').read_text()
start = source.index('typedef struct s_command_token\n')
end = source.index('static void clear_saved_allowselect_arguments', start)
parts = [f'#line {source.count(chr(10), 0, start) + 1} "engine/openbor.c"\n' + source[start:end]]
tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*[\s\S]*?\*/|//[^\n]*|[{}]')
for name in ('ParseArgs', 'load_meshmap_command'):
    matches = list(re.finditer(r'(?m)^(?:static )?(?:int|size_t) ' + name + r'\([^;{}]*\)\s*\{', source))
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
dispatch_start = source.index('        case CMD_LEVEL_MESHMAP_GEOMETRY:', source.index('void load_level('))
dispatch = source[dispatch_start:source.index('        case CMD_LEVEL_BASEMAP:', dispatch_start)]
load_level_start = source.index('void load_level(')
load_level_prefix = source[load_level_start:dispatch_start]
context = re.search(r'(?m)^\s*s_meshmap_authoring_context\s+(\w+)\s*=\s*\{\s*0\s*\}\s*;', load_level_prefix)
if not context:
    raise SystemExit('Each load_level call must initialize its own authoring context to zero')
adapter_call = r'load_meshmap_command\(\s*&' + re.escape(context.group(1)) + r'\s*,\s*cmd\s*,\s*buf\s*\+\s*pos\s*,'
if ('case CMD_LEVEL_MESHMAP_CONFIG:' not in dispatch
        or 'case CMD_LEVEL_MESHMAP_TEXTURE:' not in dispatch
        or 'case CMD_LEVEL_MESHMAP_SIDE:' not in dispatch
        or 'case CMD_LEVEL_MESHMAP_SUPPORT_GROUP:' not in dispatch
        or 'case CMD_LEVEL_MESHMAP_SLOPE_LIMIT:' not in dispatch
        or 'case CMD_LEVEL_MESHMAP_LAYER:' not in dispatch
        or 'case CMD_LEVEL_MESHMAP_INDEX:' not in dispatch
        or not re.search(adapter_call, dispatch)):
    raise SystemExit('Meshmap level dispatch must use the complete source-line adapter')
pathlib.Path(sys.argv[1]).write_text('\n'.join(parts))
PY

"$test_compiler" "${test_flags[@]}" tests/test_meshmap_level_commands.c \
    engine/source/openborscript/meshmap.c engine/source/openborscript/commands.c \
    engine/source/gamelib/meshmap.c engine/source/scriptlib/List.c engine/source/utils.c \
    -Wl,--gc-sections -Wl,--wrap=checkAlloc "${test_link_flags[@]}" \
    -o "$test_build_dir/test_meshmap_level_commands"
"$test_build_dir/test_meshmap_level_commands"

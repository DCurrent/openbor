#!/usr/bin/env bash
# Damon V. Caskey
# 2026-11-20
# Reject retired CPU scene entry points in production objects or an engine
# binary. Inspect uncollected objects in the renderer runner, so linker garbage
# collection cannot conceal dead legacy rasterizers still being compiled.
set -euo pipefail

if [[ $# == 0 ]]; then
    printf 'Usage: %s production-object-or-binary [...]\n' "$0" >&2
    exit 2
fi

test_symbols=$(${NM:-nm} --defined-only "$@")
test_retired='^(line|line32|drawbox|drawbox32|_putpixel|_putpixel32|circle|putsprite_8|putsprite_ex|putsprite_x8p32|putscreenx8p32|putscreen32|scalescreen|scalescreen32|zoomscreen|copyscreen_o|copyscreen_trans|copyscreen_remap|blendscreen|blendscreen32|putbitmap|gfx_draw_.*|drawmethod_global_init|getblendfunction32|blend_.*32|blendfunctions32|channelr|channelg|channelb|tintmode|tintcolor|usechannel|useclip|clip[xy][12]|tint32fp[12]|gpu_set_native_required|gpu_allow_cpu_operation|gpu_allow_cpu_image_operation|gpu_shader_(blend|channel)_reference)$'
test_found=$(awk '{ print $NF }' <<< "$test_symbols" | grep -E "$test_retired" || true)
if [[ -n $test_found ]]; then
    printf 'Retired software renderer symbols remain in production objects:\n%s\n' "$test_found" >&2
    exit 1
fi
printf 'Production objects contain no retired software scene-renderer entry points.\n'

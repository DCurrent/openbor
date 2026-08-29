#!/usr/bin/env bash
set -euo pipefail

# Compatibility entry point for the original OpenBOR Android helper.
SCRIPT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_ROOT}/build.sh" key

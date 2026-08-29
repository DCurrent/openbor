#!/usr/bin/env bash
set -euo pipefail

# -----------------------------------------------------------------------------
# Caskey, Damon V.
# 2026-08-29
#
# Provide the non-Windows counterpart to build.bat without downloading an old
# JDK or bypassing transport security. JDK 17 and the Android SDK must exist.
# -----------------------------------------------------------------------------

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENGINE_ROOT="$(cd "${PROJECT_ROOT}/.." && pwd)"
REPOSITORY_ROOT="$(cd "${PROJECT_ROOT}/../.." && pwd)"
ACTION="${1:-debug}"

cd "${PROJECT_ROOT}"

if [[ ! -f game.properties ]]; then
    cp game.properties.example game.properties
    echo "Created ${PROJECT_ROOT}/game.properties. Enter the game settings, then run this helper again."
    exit 1
fi

if [[ -d "${REPOSITORY_ROOT}/.git" ]]; then
    (cd "${ENGINE_ROOT}" && bash version.sh)
else
    cp "${ENGINE_ROOT}/version.tmp" "${ENGINE_ROOT}/version.h"
fi

case "${ACTION}" in
    debug)
        ./gradlew --no-daemon --console=plain clean packageStandaloneDebug
        ;;
    release)
        if [[ ! -f keystore.properties ]]; then
            ./gradlew --no-daemon --console=plain createReleaseKey
        fi
        ./gradlew --no-daemon --console=plain clean packageStandaloneRelease
        ;;
    key)
        ./gradlew --no-daemon --console=plain createReleaseKey
        ;;
    *)
        echo "Usage: ./build.sh [debug|release|key]"
        exit 1
        ;;
esac

echo "Finished. Packages are available in ${PROJECT_ROOT}/output"

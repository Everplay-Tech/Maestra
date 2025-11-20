#!/usr/bin/env bash
set -euo pipefail

# Enterprise-grade macOS build and packaging pipeline for OrchestraSynth
# - Builds universal arm64/x86_64 binaries
# - Generates JUCE standalone app + plugin
# - Produces a signed-ready .dmg via CPack (DragNDrop)

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-"${ROOT_DIR}/build/macos-universal-release"}"
GENERATOR="${GENERATOR:-}"  # If empty we'll auto-select
CONFIG="${CONFIG:-Release}"

banner() {
    printf "\n[orchestrasynth] %s\n" "$1"
}

# Auto-select a modern generator that supports multi-arch builds
select_generator() {
    if [[ -n "${GENERATOR}" ]]; then
        echo "${GENERATOR}"
        return
    fi

    if command -v ninja >/dev/null 2>&1; then
        echo "Ninja"
    else
        # Fallback works on stock Xcode command line tools
        echo "Unix Makefiles"
    fi
}

# JUCE is now automatically fetched by CMake's FetchContent
mkdir -p "${BUILD_DIR}"
SELECTED_GENERATOR="$(select_generator)"

banner "Configuring CMake (${SELECTED_GENERATOR})"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -G "${SELECTED_GENERATOR}" \
    -DCMAKE_BUILD_TYPE="${CONFIG}" \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

banner "Building targets (app + plugin)"
cmake --build "${BUILD_DIR}" --config "${CONFIG}" --target OrchestraSynth OrchestraSynthPlugin

banner "Producing .dmg via CPack"
cmake --build "${BUILD_DIR}" --config "${CONFIG}" --target package

banner "Artifacts"
DMG_PATH="${BUILD_DIR}/OrchestraSynth-${CONFIG}.dmg"
if [[ -f "${DMG_PATH}" ]]; then
    printf "\nSigned-ready DMG located at: %s\n" "${DMG_PATH}"
else
    # CPack DragNDrop uses CPACK_PACKAGE_FILE_NAME
    ALT_DMG="${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.dmg"
    if [[ -f "${ALT_DMG}" ]]; then
        printf "\nSigned-ready DMG located at: %s\n" "${ALT_DMG}"
    else
        printf "\nDMG not found yet. Check build logs in %s for CPack output.\n" "${BUILD_DIR}"
    fi
fi

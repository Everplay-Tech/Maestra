#!/bin/bash
set -e

# ============================================================================
# OrchestraSynth Code Signing Script
# ============================================================================
# Automatically signs the standalone app, VST3, and AU plugins with
# hardened runtime and proper entitlements for production distribution.
#
# Usage:
#   ./scripts/macos/codesign.sh [BUILD_DIR]
#
# Environment Variables (REQUIRED):
#   CODESIGN_IDENTITY    - Developer ID Application certificate name
#                          Example: "Developer ID Application: Company (TEAM123)"
#
# Environment Variables (OPTIONAL):
#   CODESIGN_KEYCHAIN    - Path to keychain containing signing certificate
#                          (defaults to login keychain)
#   SKIP_PLUGIN_SIGNING  - Set to "1" to skip plugin signing (for testing)
#
# ============================================================================

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="${1:-build/macos-universal-release}"
ENTITLEMENTS_DIR="config/entitlements"
APP_ENTITLEMENTS="${ENTITLEMENTS_DIR}/app.entitlements"
PLUGIN_ENTITLEMENTS="${ENTITLEMENTS_DIR}/plugin.entitlements"

echo -e "${BLUE}=== OrchestraSynth Code Signing ===${NC}"
echo ""

# ============================================================================
# Validate Environment
# ============================================================================

if [ -z "$CODESIGN_IDENTITY" ]; then
    echo -e "${RED}ERROR: CODESIGN_IDENTITY environment variable not set${NC}"
    echo ""
    echo "Please set the code signing identity:"
    echo "  export CODESIGN_IDENTITY=\"Developer ID Application: Your Company (TEAM123)\""
    echo ""
    echo "To list available identities:"
    echo "  security find-identity -v -p codesigning"
    exit 1
fi

echo -e "${GREEN}✓${NC} Code signing identity: $CODESIGN_IDENTITY"

# Validate build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}ERROR: Build directory not found: $BUILD_DIR${NC}"
    echo "Please build the project first or specify correct build directory"
    exit 1
fi

# Validate entitlements files exist
if [ ! -f "$APP_ENTITLEMENTS" ]; then
    echo -e "${RED}ERROR: App entitlements file not found: $APP_ENTITLEMENTS${NC}"
    exit 1
fi

if [ ! -f "$PLUGIN_ENTITLEMENTS" ]; then
    echo -e "${RED}ERROR: Plugin entitlements file not found: $PLUGIN_ENTITLEMENTS${NC}"
    exit 1
fi

echo -e "${GREEN}✓${NC} Build directory: $BUILD_DIR"
echo -e "${GREEN}✓${NC} Entitlements files found"
echo ""

# ============================================================================
# Code Signing Function
# ============================================================================

sign_bundle() {
    local bundle_path="$1"
    local entitlements="$2"
    local bundle_name=$(basename "$bundle_path")

    echo -e "${BLUE}Signing:${NC} $bundle_name"

    # Build codesign command
    local codesign_cmd=(
        codesign
        --force
        --options runtime
        --timestamp
        --entitlements "$entitlements"
        --sign "$CODESIGN_IDENTITY"
    )

    # Add keychain if specified
    if [ -n "$CODESIGN_KEYCHAIN" ]; then
        codesign_cmd+=(--keychain "$CODESIGN_KEYCHAIN")
    fi

    codesign_cmd+=("$bundle_path")

    # Execute signing
    if "${codesign_cmd[@]}"; then
        echo -e "${GREEN}✓${NC} Successfully signed: $bundle_name"

        # Verify signature
        if codesign --verify --deep --strict "$bundle_path" 2>/dev/null; then
            echo -e "${GREEN}✓${NC} Signature verified: $bundle_name"
        else
            echo -e "${YELLOW}⚠${NC}  Warning: Signature verification failed for $bundle_name"
        fi
        echo ""
        return 0
    else
        echo -e "${RED}✗${NC} Failed to sign: $bundle_name"
        echo ""
        return 1
    fi
}

# ============================================================================
# Sign Standalone Application
# ============================================================================

APP_PATH="${BUILD_DIR}/OrchestraSynth.app"

if [ -d "$APP_PATH" ]; then
    echo -e "${BLUE}=== Signing Standalone Application ===${NC}"
    echo ""
    sign_bundle "$APP_PATH" "$APP_ENTITLEMENTS" || exit 1
else
    echo -e "${YELLOW}⚠${NC}  Standalone app not found: $APP_PATH"
    echo ""
fi

# ============================================================================
# Sign Plugins
# ============================================================================

if [ "$SKIP_PLUGIN_SIGNING" = "1" ]; then
    echo -e "${YELLOW}⚠${NC}  Skipping plugin signing (SKIP_PLUGIN_SIGNING=1)"
    echo ""
else
    echo -e "${BLUE}=== Signing Audio Plugins ===${NC}"
    echo ""

    # Sign VST3
    VST3_PATH="${BUILD_DIR}/Plug-Ins/VST3/OrchestraSynth.vst3"
    if [ -d "$VST3_PATH" ]; then
        sign_bundle "$VST3_PATH" "$PLUGIN_ENTITLEMENTS" || exit 1
    else
        echo -e "${YELLOW}⚠${NC}  VST3 plugin not found: $VST3_PATH"
        echo ""
    fi

    # Sign AU
    AU_PATH="${BUILD_DIR}/Plug-Ins/AU/OrchestraSynth.component"
    if [ -d "$AU_PATH" ]; then
        sign_bundle "$AU_PATH" "$PLUGIN_ENTITLEMENTS" || exit 1
    else
        echo -e "${YELLOW}⚠${NC}  AU plugin not found: $AU_PATH"
        echo ""
    fi
fi

# ============================================================================
# Summary
# ============================================================================

echo -e "${GREEN}=== Code Signing Complete ===${NC}"
echo ""
echo "Next steps:"
echo "  1. Package the signed binaries: cmake --build $BUILD_DIR --target package"
echo "  2. Notarize the DMG: ./scripts/macos/notarize.sh"
echo "  3. Validate distribution: ./scripts/macos/validate.sh"
echo ""

#!/bin/bash
set -e

# ============================================================================
# OrchestraSynth Notarization Script
# ============================================================================
# Automatically notarizes the DMG package with Apple's notary service
# and staples the notarization ticket for offline verification.
#
# Usage:
#   ./scripts/macos/notarize.sh [DMG_PATH]
#
# Environment Variables (REQUIRED):
#   NOTARIZE_APPLE_ID       - Apple ID email for notarization
#   NOTARIZE_TEAM_ID        - Apple Developer Team ID
#   NOTARIZE_PASSWORD       - App-specific password or keychain profile name
#
# Environment Variables (OPTIONAL):
#   NOTARIZE_WAIT_TIMEOUT   - Max wait time in seconds (default: 3600)
#   NOTARIZE_USE_KEYCHAIN   - Set to "1" to use keychain profile instead of password
#
# Setup App-Specific Password:
#   1. Go to https://appleid.apple.com/account/manage
#   2. Generate app-specific password
#   3. Store in keychain:
#      xcrun notarytool store-credentials "AC_PASSWORD" \
#        --apple-id "you@example.com" \
#        --team-id "TEAM123" \
#        --password "xxxx-xxxx-xxxx-xxxx"
#
# ============================================================================

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build/macos-universal-release"
DEFAULT_DMG="${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.dmg"
DMG_PATH="${1:-$DEFAULT_DMG}"
WAIT_TIMEOUT="${NOTARIZE_WAIT_TIMEOUT:-3600}"
USE_KEYCHAIN="${NOTARIZE_USE_KEYCHAIN:-0}"

echo -e "${BLUE}=== OrchestraSynth Notarization ===${NC}"
echo ""

# ============================================================================
# Validate Environment
# ============================================================================

if [ -z "$NOTARIZE_APPLE_ID" ]; then
    echo -e "${RED}ERROR: NOTARIZE_APPLE_ID environment variable not set${NC}"
    echo ""
    echo "Please set your Apple ID:"
    echo "  export NOTARIZE_APPLE_ID=\"you@example.com\""
    exit 1
fi

if [ -z "$NOTARIZE_TEAM_ID" ]; then
    echo -e "${RED}ERROR: NOTARIZE_TEAM_ID environment variable not set${NC}"
    echo ""
    echo "Please set your Team ID:"
    echo "  export NOTARIZE_TEAM_ID=\"TEAM123\""
    echo ""
    echo "Find your Team ID at: https://developer.apple.com/account"
    exit 1
fi

if [ -z "$NOTARIZE_PASSWORD" ]; then
    echo -e "${RED}ERROR: NOTARIZE_PASSWORD environment variable not set${NC}"
    echo ""
    echo "Please set your app-specific password or keychain profile:"
    echo "  export NOTARIZE_PASSWORD=\"xxxx-xxxx-xxxx-xxxx\""
    echo "  OR"
    echo "  export NOTARIZE_PASSWORD=\"AC_PASSWORD\" (keychain profile)"
    echo "  export NOTARIZE_USE_KEYCHAIN=\"1\""
    echo ""
    echo "Generate app-specific password at: https://appleid.apple.com/account/manage"
    exit 1
fi

# Validate DMG exists
if [ ! -f "$DMG_PATH" ]; then
    echo -e "${RED}ERROR: DMG file not found: $DMG_PATH${NC}"
    echo ""
    echo "Please build and package the project first:"
    echo "  ./scripts/macos/build_and_package.sh"
    echo "  ./scripts/macos/codesign.sh"
    echo "  cmake --build $BUILD_DIR --target package"
    exit 1
fi

echo -e "${GREEN}✓${NC} DMG file: $DMG_PATH"
echo -e "${GREEN}✓${NC} Apple ID: $NOTARIZE_APPLE_ID"
echo -e "${GREEN}✓${NC} Team ID: $NOTARIZE_TEAM_ID"

# Get DMG file size
DMG_SIZE=$(du -h "$DMG_PATH" | cut -f1)
echo -e "${GREEN}✓${NC} DMG size: $DMG_SIZE"
echo ""

# ============================================================================
# Validate DMG Signing
# ============================================================================

echo -e "${BLUE}=== Validating DMG Signature ===${NC}"
echo ""

if codesign -dv --verbose=4 "$DMG_PATH" 2>&1 | grep -q "Developer ID"; then
    echo -e "${GREEN}✓${NC} DMG is properly signed"
else
    echo -e "${YELLOW}⚠${NC}  Warning: DMG may not be properly signed"
    echo "  This could cause notarization to fail"
fi
echo ""

# ============================================================================
# Submit for Notarization
# ============================================================================

echo -e "${BLUE}=== Submitting to Apple Notary Service ===${NC}"
echo ""
echo "This may take several minutes depending on file size and Apple's queue..."
echo ""

# Build notarytool command
NOTARYTOOL_CMD=(
    xcrun notarytool submit "$DMG_PATH"
    --apple-id "$NOTARIZE_APPLE_ID"
    --team-id "$NOTARIZE_TEAM_ID"
    --wait
    --timeout "$WAIT_TIMEOUT"
)

# Add password or keychain profile
if [ "$USE_KEYCHAIN" = "1" ]; then
    echo -e "${BLUE}Using keychain profile:${NC} $NOTARIZE_PASSWORD"
    NOTARYTOOL_CMD+=(--keychain-profile "$NOTARIZE_PASSWORD")
else
    NOTARYTOOL_CMD+=(--password "$NOTARIZE_PASSWORD")
fi

echo ""

# Submit and capture output
NOTARIZE_OUTPUT=$(mktemp)
if "${NOTARYTOOL_CMD[@]}" 2>&1 | tee "$NOTARIZE_OUTPUT"; then
    echo ""
    echo -e "${GREEN}✓${NC} Notarization submitted successfully"

    # Extract submission ID for reference
    SUBMISSION_ID=$(grep "id:" "$NOTARIZE_OUTPUT" | head -1 | awk '{print $2}')
    if [ -n "$SUBMISSION_ID" ]; then
        echo -e "${GREEN}✓${NC} Submission ID: $SUBMISSION_ID"
    fi
else
    echo ""
    echo -e "${RED}✗${NC} Notarization submission failed"
    echo ""
    echo "Common issues:"
    echo "  - Invalid app-specific password"
    echo "  - Unsigned or improperly signed DMG"
    echo "  - Missing hardened runtime entitlements"
    echo "  - Network connectivity issues"
    echo ""
    echo "To check submission status manually:"
    echo "  xcrun notarytool history --apple-id $NOTARIZE_APPLE_ID --team-id $NOTARIZE_TEAM_ID"
    rm -f "$NOTARIZE_OUTPUT"
    exit 1
fi

rm -f "$NOTARIZE_OUTPUT"
echo ""

# ============================================================================
# Staple Notarization Ticket
# ============================================================================

echo -e "${BLUE}=== Stapling Notarization Ticket ===${NC}"
echo ""

if xcrun stapler staple "$DMG_PATH"; then
    echo ""
    echo -e "${GREEN}✓${NC} Notarization ticket stapled successfully"
    echo ""

    # Verify stapling
    if xcrun stapler validate "$DMG_PATH"; then
        echo ""
        echo -e "${GREEN}✓${NC} Stapled ticket validated"
    fi
else
    echo ""
    echo -e "${YELLOW}⚠${NC}  Warning: Failed to staple notarization ticket"
    echo "  The DMG is notarized but won't work offline"
fi

echo ""

# ============================================================================
# Summary
# ============================================================================

echo -e "${GREEN}=== Notarization Complete ===${NC}"
echo ""
echo "Notarized DMG: $DMG_PATH"
echo ""
echo "Next steps:"
echo "  1. Validate distribution: ./scripts/macos/validate.sh"
echo "  2. Test on clean macOS system"
echo "  3. Distribute to users"
echo ""
echo "To verify notarization status:"
echo "  spctl -a -vv -t install \"$DMG_PATH\""
echo ""

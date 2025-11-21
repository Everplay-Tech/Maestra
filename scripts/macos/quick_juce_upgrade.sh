#!/usr/bin/env bash
set -euo pipefail

# ============================================================================
# Quick JUCE Version Upgrade Script
# ============================================================================
# Quickly updates JUCE version in CMakeLists.txt for M5 compatibility
#
# Usage:
#   ./scripts/macos/quick_juce_upgrade.sh [VERSION]
#
# Examples:
#   ./scripts/macos/quick_juce_upgrade.sh           # Upgrade to 8.0.4
#   ./scripts/macos/quick_juce_upgrade.sh 8.0.3     # Upgrade to specific version
#   ./scripts/macos/quick_juce_upgrade.sh --restore # Restore backup
#
# ============================================================================

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CMAKELISTS="${ROOT_DIR}/CMakeLists.txt"
BACKUP="${CMAKELISTS}.backup"
NEW_VERSION="${1:-8.0.4}"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo ""
echo -e "${BLUE}OrchestraSynth - Quick JUCE Upgrade${NC}"
echo "===================================="
echo ""

# Handle restore
if [[ "$NEW_VERSION" == "--restore" ]]; then
    if [[ -f "$BACKUP" ]]; then
        echo -e "${YELLOW}Restoring from backup...${NC}"
        mv "$BACKUP" "$CMAKELISTS"
        echo -e "${GREEN}✓${NC} Restored CMakeLists.txt from backup"
        echo ""
        exit 0
    else
        echo -e "${RED}✗${NC} No backup found at: ${BACKUP}"
        exit 1
    fi
fi

# Validate version format
if ! [[ "$NEW_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo -e "${RED}✗${NC} Invalid version format: ${NEW_VERSION}"
    echo "Expected format: X.Y.Z (e.g., 8.0.4)"
    exit 1
fi

# Check if CMakeLists.txt exists
if [[ ! -f "$CMAKELISTS" ]]; then
    echo -e "${RED}✗${NC} CMakeLists.txt not found at: ${CMAKELISTS}"
    exit 1
fi

# Get current version
CURRENT_VERSION=$(grep "GIT_TAG" "$CMAKELISTS" | head -1 | awk '{print $2}')

echo "Current JUCE version: ${CURRENT_VERSION}"
echo "New JUCE version:     ${NEW_VERSION}"
echo ""

if [[ "$CURRENT_VERSION" == "$NEW_VERSION" ]]; then
    echo -e "${YELLOW}⚠${NC}  Version already set to ${NEW_VERSION}"
    exit 0
fi

# Create backup
if [[ ! -f "$BACKUP" ]]; then
    cp "$CMAKELISTS" "$BACKUP"
    echo -e "${GREEN}✓${NC} Created backup: ${BACKUP##*/}"
else
    echo -e "${YELLOW}⚠${NC}  Backup already exists: ${BACKUP##*/}"
fi

# Update version
if [[ "$OSTYPE" == "darwin"* ]]; then
    sed -i '' "s/GIT_TAG        ${CURRENT_VERSION}/GIT_TAG        ${NEW_VERSION}/" "$CMAKELISTS"
else
    sed -i "s/GIT_TAG        ${CURRENT_VERSION}/GIT_TAG        ${NEW_VERSION}/" "$CMAKELISTS"
fi

# Verify update
NEW_VERSION_CHECK=$(grep "GIT_TAG" "$CMAKELISTS" | head -1 | awk '{print $2}')

if [[ "$NEW_VERSION_CHECK" == "$NEW_VERSION" ]]; then
    echo -e "${GREEN}✓${NC} Updated JUCE version to ${NEW_VERSION}"
    echo ""
    echo -e "${BLUE}Next steps:${NC}"
    echo "  1. Clean build cache:   rm -rf build/_deps"
    echo "  2. Build:              ./scripts/macos/build_and_package.sh"
    echo "  3. Or use full setup:  ./scripts/macos/setup_m5_build.sh"
    echo ""
    echo "To restore backup:       ./scripts/macos/quick_juce_upgrade.sh --restore"
    echo ""
else
    echo -e "${RED}✗${NC} Update failed - version mismatch"
    echo "Expected: ${NEW_VERSION}"
    echo "Got:      ${NEW_VERSION_CHECK}"
    exit 1
fi

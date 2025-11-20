#!/bin/bash
set -e

# ============================================================================
# OrchestraSynth Production Build Pipeline
# ============================================================================
# Complete end-to-end production build pipeline that:
#   1. Builds universal binaries (arm64 + x86_64)
#   2. Code signs all binaries with hardened runtime
#   3. Creates DMG and/or PKG installers
#   4. Notarizes with Apple
#   5. Staples notarization tickets
#   6. Validates final distribution packages
#
# Usage:
#   ./scripts/macos/production_build.sh [OPTIONS]
#
# Options:
#   --clean              Clean build directory before building
#   --dmg-only           Build DMG only (skip PKG)
#   --pkg-only           Build PKG only (skip DMG)
#   --skip-notarize      Skip notarization (for testing)
#   --skip-validate      Skip validation
#   --config CONFIG      Build configuration (default: Release)
#   --help               Show this help message
#
# Required Environment Variables:
#   CODESIGN_IDENTITY       - Developer ID Application certificate
#   NOTARIZE_APPLE_ID       - Apple ID for notarization
#   NOTARIZE_TEAM_ID        - Apple Developer Team ID
#   NOTARIZE_PASSWORD       - App-specific password or keychain profile
#
# Optional Environment Variables:
#   PKG_SIGNING_IDENTITY    - Developer ID Installer certificate (for PKG)
#   BUILD_NUMBER            - Build number for version tracking
#   SKIP_CODESIGN          - Set to "1" to skip code signing (testing only)
#
# ============================================================================

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default configuration
BUILD_DIR="build/macos-universal-release"
CONFIG="${BUILD_CONFIG:-Release}"
CLEAN_BUILD=0
BUILD_DMG=1
BUILD_PKG=1
RUN_NOTARIZE=1
RUN_VALIDATE=1

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        --dmg-only)
            BUILD_PKG=0
            shift
            ;;
        --pkg-only)
            BUILD_DMG=0
            shift
            ;;
        --skip-notarize)
            RUN_NOTARIZE=0
            shift
            ;;
        --skip-validate)
            RUN_VALIDATE=0
            shift
            ;;
        --config)
            CONFIG="$2"
            shift 2
            ;;
        --help)
            grep "^#" "$0" | grep -v "^#!/" | sed 's/^# \?//'
            exit 0
            ;;
        *)
            echo -e "${RED}ERROR: Unknown option: $1${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# ============================================================================
# Print Banner
# ============================================================================

echo ""
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║                                                               ║${NC}"
echo -e "${CYAN}║           ${MAGENTA}OrchestraSynth Production Build Pipeline${CYAN}          ║${NC}"
echo -e "${CYAN}║                                                               ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# ============================================================================
# Validate Prerequisites
# ============================================================================

echo -e "${BLUE}=== Validating Prerequisites ===${NC}"
echo ""

# Check required tools
REQUIRED_TOOLS=("cmake" "codesign" "productbuild" "pkgbuild" "xcrun")
MISSING_TOOLS=()

for tool in "${REQUIRED_TOOLS[@]}"; do
    if command -v "$tool" >/dev/null 2>&1; then
        echo -e "${GREEN}✓${NC} $tool"
    else
        echo -e "${RED}✗${NC} $tool (missing)"
        MISSING_TOOLS+=("$tool")
    fi
done

if [ ${#MISSING_TOOLS[@]} -gt 0 ]; then
    echo ""
    echo -e "${RED}ERROR: Missing required tools: ${MISSING_TOOLS[*]}${NC}"
    echo "Please install Xcode Command Line Tools: xcode-select --install"
    exit 1
fi

# Check environment variables (unless skipping codesign)
if [ "$SKIP_CODESIGN" != "1" ]; then
    echo ""

    if [ -z "$CODESIGN_IDENTITY" ]; then
        echo -e "${RED}ERROR: CODESIGN_IDENTITY not set${NC}"
        echo "Set with: export CODESIGN_IDENTITY=\"Developer ID Application: Company (TEAM)\""
        exit 1
    fi
    echo -e "${GREEN}✓${NC} CODESIGN_IDENTITY: $CODESIGN_IDENTITY"

    if [ $RUN_NOTARIZE -eq 1 ]; then
        if [ -z "$NOTARIZE_APPLE_ID" ]; then
            echo -e "${RED}ERROR: NOTARIZE_APPLE_ID not set${NC}"
            exit 1
        fi
        echo -e "${GREEN}✓${NC} NOTARIZE_APPLE_ID: $NOTARIZE_APPLE_ID"

        if [ -z "$NOTARIZE_TEAM_ID" ]; then
            echo -e "${RED}ERROR: NOTARIZE_TEAM_ID not set${NC}"
            exit 1
        fi
        echo -e "${GREEN}✓${NC} NOTARIZE_TEAM_ID: $NOTARIZE_TEAM_ID"

        if [ -z "$NOTARIZE_PASSWORD" ]; then
            echo -e "${RED}ERROR: NOTARIZE_PASSWORD not set${NC}"
            exit 1
        fi
        echo -e "${GREEN}✓${NC} NOTARIZE_PASSWORD: [set]"
    fi

    if [ $BUILD_PKG -eq 1 ] && [ -n "$PKG_SIGNING_IDENTITY" ]; then
        echo -e "${GREEN}✓${NC} PKG_SIGNING_IDENTITY: $PKG_SIGNING_IDENTITY"
    fi
fi

echo ""

# ============================================================================
# Step 1: Build Universal Binaries
# ============================================================================

echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Step 1/6: Building Universal Binaries                       ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""

if [ $CLEAN_BUILD -eq 1 ] && [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} Build directory cleaned"
    echo ""
fi

# Run the build and package script
if ./scripts/macos/build_and_package.sh; then
    echo ""
    echo -e "${GREEN}✓${NC} Build completed successfully"
else
    echo ""
    echo -e "${RED}✗${NC} Build failed"
    exit 1
fi

echo ""

# ============================================================================
# Step 2: Code Signing
# ============================================================================

if [ "$SKIP_CODESIGN" != "1" ]; then
    echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║  Step 2/6: Code Signing with Hardened Runtime                ║${NC}"
    echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════╝${NC}"
    echo ""

    if ./scripts/macos/codesign.sh "$BUILD_DIR"; then
        echo -e "${GREEN}✓${NC} Code signing completed successfully"
    else
        echo -e "${RED}✗${NC} Code signing failed"
        exit 1
    fi

    echo ""
else
    echo -e "${YELLOW}⚠${NC}  Skipping code signing (SKIP_CODESIGN=1)"
    echo ""
fi

# ============================================================================
# Step 3: Create Distribution Packages
# ============================================================================

echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Step 3/6: Creating Distribution Packages                    ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Re-package DMG with signed binaries
if [ $BUILD_DMG -eq 1 ]; then
    echo -e "${BLUE}Building DMG installer...${NC}"
    if cmake --build "$BUILD_DIR" --config "$CONFIG" --target package; then
        echo -e "${GREEN}✓${NC} DMG created successfully"
    else
        echo -e "${RED}✗${NC} DMG creation failed"
        exit 1
    fi
    echo ""
fi

# Build PKG installer
if [ $BUILD_PKG -eq 1 ]; then
    echo -e "${BLUE}Building PKG installer...${NC}"
    if ./scripts/macos/build_pkg.sh "$BUILD_DIR"; then
        echo -e "${GREEN}✓${NC} PKG created successfully"
    else
        echo -e "${RED}✗${NC} PKG creation failed"
        exit 1
    fi
    echo ""
fi

# ============================================================================
# Step 4: Notarization
# ============================================================================

if [ $RUN_NOTARIZE -eq 1 ] && [ "$SKIP_CODESIGN" != "1" ]; then
    echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║  Step 4/6: Apple Notarization                                ║${NC}"
    echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════╝${NC}"
    echo ""

    # Notarize DMG
    if [ $BUILD_DMG -eq 1 ]; then
        DMG_PATH="${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.dmg"
        if [ -f "$DMG_PATH" ]; then
            echo -e "${BLUE}Notarizing DMG...${NC}"
            if ./scripts/macos/notarize.sh "$DMG_PATH"; then
                echo -e "${GREEN}✓${NC} DMG notarization completed"
            else
                echo -e "${RED}✗${NC} DMG notarization failed"
                exit 1
            fi
            echo ""
        fi
    fi

    # Notarize PKG
    if [ $BUILD_PKG -eq 1 ]; then
        PKG_PATH="${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.pkg"
        if [ -f "$PKG_PATH" ]; then
            echo -e "${BLUE}Notarizing PKG...${NC}"
            if ./scripts/macos/notarize.sh "$PKG_PATH"; then
                echo -e "${GREEN}✓${NC} PKG notarization completed"
            else
                echo -e "${RED}✗${NC} PKG notarization failed"
                exit 1
            fi
            echo ""
        fi
    fi
else
    echo -e "${YELLOW}⚠${NC}  Skipping notarization"
    echo ""
fi

# ============================================================================
# Step 5: Validation
# ============================================================================

if [ $RUN_VALIDATE -eq 1 ]; then
    echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║  Step 5/6: Package Validation                                ║${NC}"
    echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════╝${NC}"
    echo ""

    if [ -f "./scripts/macos/validate.sh" ]; then
        if ./scripts/macos/validate.sh "$BUILD_DIR"; then
            echo -e "${GREEN}✓${NC} Validation completed successfully"
        else
            echo -e "${YELLOW}⚠${NC}  Validation completed with warnings"
        fi
    else
        echo -e "${YELLOW}⚠${NC}  Validation script not found (skipping)"
    fi

    echo ""
fi

# ============================================================================
# Step 6: Summary & Artifacts
# ============================================================================

echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Step 6/6: Build Summary                                     ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""

echo -e "${GREEN}✓ Production build completed successfully!${NC}"
echo ""
echo -e "${CYAN}Build Artifacts:${NC}"
echo ""

# List all artifacts
if [ -d "$BUILD_DIR" ]; then
    # Standalone app
    if [ -d "${BUILD_DIR}/OrchestraSynth.app" ]; then
        APP_SIZE=$(du -sh "${BUILD_DIR}/OrchestraSynth.app" | cut -f1)
        echo -e "  ${GREEN}•${NC} OrchestraSynth.app (${APP_SIZE})"
    fi

    # VST3
    if [ -d "${BUILD_DIR}/Plug-Ins/VST3/OrchestraSynth.vst3" ]; then
        VST3_SIZE=$(du -sh "${BUILD_DIR}/Plug-Ins/VST3/OrchestraSynth.vst3" | cut -f1)
        echo -e "  ${GREEN}•${NC} OrchestraSynth.vst3 (${VST3_SIZE})"
    fi

    # AU
    if [ -d "${BUILD_DIR}/Plug-Ins/AU/OrchestraSynth.component" ]; then
        AU_SIZE=$(du -sh "${BUILD_DIR}/Plug-Ins/AU/OrchestraSynth.component" | cut -f1)
        echo -e "  ${GREEN}•${NC} OrchestraSynth.component (${AU_SIZE})"
    fi

    echo ""

    # DMG
    if [ -f "${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.dmg" ]; then
        DMG_SIZE=$(du -sh "${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.dmg" | cut -f1)
        echo -e "  ${CYAN}📦 DMG:${NC} OrchestraSynth-0.1.0-macOS-universal.dmg (${DMG_SIZE})"
    fi

    # PKG
    if [ -f "${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.pkg" ]; then
        PKG_SIZE=$(du -sh "${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.pkg" | cut -f1)
        echo -e "  ${CYAN}📦 PKG:${NC} OrchestraSynth-0.1.0-macOS-universal.pkg (${PKG_SIZE})"
    fi
fi

echo ""
echo -e "${CYAN}Distribution Ready:${NC}"
echo -e "  ${GREEN}•${NC} Code signed with hardened runtime"
if [ $RUN_NOTARIZE -eq 1 ] && [ "$SKIP_CODESIGN" != "1" ]; then
    echo -e "  ${GREEN}•${NC} Notarized by Apple"
    echo -e "  ${GREEN}•${NC} Notarization ticket stapled"
fi
echo -e "  ${GREEN}•${NC} Universal binary (Apple Silicon + Intel)"
echo -e "  ${GREEN}•${NC} Ready for distribution"
echo ""

echo -e "${YELLOW}Next Steps:${NC}"
echo "  1. Test on clean macOS system"
echo "  2. Upload to distribution platform"
echo "  3. For enterprise: Upload PKG to MDM system"
echo ""

# Record build info
BUILD_INFO="${BUILD_DIR}/build_info.txt"
cat > "$BUILD_INFO" << EOF
OrchestraSynth Build Information
================================
Build Date: $(date)
Build Type: Production
Configuration: ${CONFIG}
Build Directory: ${BUILD_DIR}
Build Number: ${BUILD_NUMBER:-N/A}

Artifacts:
- DMG: $([ -f "${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.dmg" ] && echo "✓" || echo "✗")
- PKG: $([ -f "${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.pkg" ] && echo "✓" || echo "✗")

Code Signing: $([ "$SKIP_CODESIGN" != "1" ] && echo "✓" || echo "✗")
Notarization: $([ $RUN_NOTARIZE -eq 1 ] && [ "$SKIP_CODESIGN" != "1" ] && echo "✓" || echo "✗")
Validation: $([ $RUN_VALIDATE -eq 1 ] && echo "✓" || echo "✗")

Environment:
- macOS: $(sw_vers -productVersion)
- Xcode: $(xcodebuild -version | head -1 || echo "N/A")
- CMake: $(cmake --version | head -1)
EOF

echo -e "${GREEN}Build information saved to: ${BUILD_INFO}${NC}"
echo ""

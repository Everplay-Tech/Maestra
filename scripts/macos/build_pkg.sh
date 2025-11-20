#!/bin/bash
set -e

# ============================================================================
# OrchestraSynth PKG Installer Builder
# ============================================================================
# Creates enterprise-ready PKG installer for distribution via MDM systems
# (Jamf, Intune, etc.) or direct installation.
#
# The PKG installer includes:
#   - Standalone application installed to /Applications
#   - VST3 plugin installed to /Library/Audio/Plug-Ins/VST3
#   - AU plugin installed to /Library/Audio/Plug-Ins/Components
#
# Usage:
#   ./scripts/macos/build_pkg.sh [BUILD_DIR]
#
# Environment Variables (OPTIONAL):
#   PKG_SIGNING_IDENTITY  - Developer ID Installer certificate for signing PKG
#                           Example: "Developer ID Installer: Company (TEAM123)"
#   PKG_VERSION          - Override version (default: from CMakeLists.txt)
#   PKG_OUTPUT_DIR       - Custom output directory (default: BUILD_DIR)
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
VERSION="${PKG_VERSION:-0.1.0}"
PKG_OUTPUT_DIR="${PKG_OUTPUT_DIR:-$BUILD_DIR}"
PKG_NAME="OrchestraSynth-${VERSION}-macOS-universal.pkg"
PKG_PATH="${PKG_OUTPUT_DIR}/${PKG_NAME}"

# Component identifiers
BUNDLE_ID_PREFIX="com.yourcompany.orchestrasynth"
PKG_IDENTIFIER="${BUNDLE_ID_PREFIX}.pkg"

# Temporary working directory
WORK_DIR=$(mktemp -d)
trap 'rm -rf "$WORK_DIR"' EXIT

echo -e "${BLUE}=== OrchestraSynth PKG Installer Builder ===${NC}"
echo ""

# ============================================================================
# Validate Environment
# ============================================================================

if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}ERROR: Build directory not found: $BUILD_DIR${NC}"
    echo "Please build the project first"
    exit 1
fi

echo -e "${GREEN}✓${NC} Build directory: $BUILD_DIR"
echo -e "${GREEN}✓${NC} Version: $VERSION"
echo -e "${GREEN}✓${NC} Output: $PKG_PATH"

if [ -n "$PKG_SIGNING_IDENTITY" ]; then
    echo -e "${GREEN}✓${NC} PKG signing identity: $PKG_SIGNING_IDENTITY"
else
    echo -e "${YELLOW}⚠${NC}  No PKG_SIGNING_IDENTITY set (PKG will be unsigned)"
fi
echo ""

# ============================================================================
# Prepare Component Payloads
# ============================================================================

echo -e "${BLUE}=== Preparing Component Payloads ===${NC}"
echo ""

# Create payload directories
PAYLOAD_ROOT="${WORK_DIR}/payload"
SCRIPTS_DIR="${WORK_DIR}/scripts"

mkdir -p "${PAYLOAD_ROOT}/Applications"
mkdir -p "${PAYLOAD_ROOT}/Library/Audio/Plug-Ins/VST3"
mkdir -p "${PAYLOAD_ROOT}/Library/Audio/Plug-Ins/Components"
mkdir -p "${SCRIPTS_DIR}"

# Copy standalone app
APP_PATH="${BUILD_DIR}/OrchestraSynth.app"
if [ -d "$APP_PATH" ]; then
    echo -e "${BLUE}Copying:${NC} Standalone application"
    cp -R "$APP_PATH" "${PAYLOAD_ROOT}/Applications/"
    echo -e "${GREEN}✓${NC} Standalone app prepared"
else
    echo -e "${YELLOW}⚠${NC}  Warning: Standalone app not found at $APP_PATH"
fi

# Copy VST3 plugin
VST3_PATH="${BUILD_DIR}/Plug-Ins/VST3/OrchestraSynth.vst3"
if [ -d "$VST3_PATH" ]; then
    echo -e "${BLUE}Copying:${NC} VST3 plugin"
    cp -R "$VST3_PATH" "${PAYLOAD_ROOT}/Library/Audio/Plug-Ins/VST3/"
    echo -e "${GREEN}✓${NC} VST3 plugin prepared"
else
    echo -e "${YELLOW}⚠${NC}  Warning: VST3 plugin not found at $VST3_PATH"
fi

# Copy AU plugin
AU_PATH="${BUILD_DIR}/Plug-Ins/AU/OrchestraSynth.component"
if [ -d "$AU_PATH" ]; then
    echo -e "${BLUE}Copying:${NC} AU plugin"
    cp -R "$AU_PATH" "${PAYLOAD_ROOT}/Library/Audio/Plug-Ins/Components/"
    echo -e "${GREEN}✓${NC} AU plugin prepared"
else
    echo -e "${YELLOW}⚠${NC}  Warning: AU plugin not found at $AU_PATH"
fi

echo ""

# ============================================================================
# Create Post-Install Script
# ============================================================================

echo -e "${BLUE}=== Creating Installation Scripts ===${NC}"
echo ""

cat > "${SCRIPTS_DIR}/postinstall" << 'SCRIPT_EOF'
#!/bin/bash

# Post-installation script for OrchestraSynth
# Resets plugin cache to ensure DAWs recognize new plugins

echo "OrchestraSynth installation complete"

# Reset AU cache (forces Logic Pro, GarageBand to rescan)
echo "Resetting Audio Unit cache..."
killall -9 AudioComponentRegistrar 2>/dev/null || true

# Notify DAW cache reset (VST3)
echo "Clearing VST3 cache..."
rm -f ~/Library/Preferences/com.steinberg.vst3.cache 2>/dev/null || true

# Reset permission cache
echo "Resetting permissions..."
xattr -cr /Applications/OrchestraSynth.app 2>/dev/null || true
xattr -cr /Library/Audio/Plug-Ins/VST3/OrchestraSynth.vst3 2>/dev/null || true
xattr -cr /Library/Audio/Plug-Ins/Components/OrchestraSynth.component 2>/dev/null || true

echo "Installation successful!"
echo "Please restart your DAW to load OrchestraSynth plugins"

exit 0
SCRIPT_EOF

chmod +x "${SCRIPTS_DIR}/postinstall"
echo -e "${GREEN}✓${NC} Post-install script created"
echo ""

# ============================================================================
# Build Component Package
# ============================================================================

echo -e "${BLUE}=== Building PKG Installer ===${NC}"
echo ""

# Create component package
COMPONENT_PKG="${WORK_DIR}/OrchestraSynth-component.pkg"

pkgbuild \
    --root "${PAYLOAD_ROOT}" \
    --identifier "$PKG_IDENTIFIER" \
    --version "$VERSION" \
    --scripts "${SCRIPTS_DIR}" \
    --install-location "/" \
    "$COMPONENT_PKG"

if [ ! -f "$COMPONENT_PKG" ]; then
    echo -e "${RED}✗${NC} Failed to create component package"
    exit 1
fi

echo -e "${GREEN}✓${NC} Component package created"
echo ""

# ============================================================================
# Create Distribution XML
# ============================================================================

DISTRIBUTION_XML="${WORK_DIR}/distribution.xml"

cat > "$DISTRIBUTION_XML" << XML_EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>OrchestraSynth ${VERSION}</title>
    <organization>${BUNDLE_ID_PREFIX}</organization>
    <domains enable_localSystem="true"/>
    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>

    <welcome file="welcome.html" mime-type="text/html"/>
    <license file="license.txt" mime-type="text/plain"/>
    <readme file="readme.html" mime-type="text/html"/>

    <pkg-ref id="$PKG_IDENTIFIER"/>

    <options customize="never" require-scripts="true" rootVolumeOnly="true"/>

    <choices-outline>
        <line choice="default">
            <line choice="$PKG_IDENTIFIER"/>
        </line>
    </choices-outline>

    <choice id="default"/>

    <choice id="$PKG_IDENTIFIER" visible="false">
        <pkg-ref id="$PKG_IDENTIFIER"/>
    </choice>

    <pkg-ref id="$PKG_IDENTIFIER" version="$VERSION" onConclusion="none">OrchestraSynth-component.pkg</pkg-ref>
</installer-gui-script>
XML_EOF

echo -e "${GREEN}✓${NC} Distribution XML created"
echo ""

# ============================================================================
# Create Resource Files
# ============================================================================

RESOURCES_DIR="${WORK_DIR}/resources"
mkdir -p "$RESOURCES_DIR"

# Welcome message
cat > "${RESOURCES_DIR}/welcome.html" << 'HTML_EOF'
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Helvetica Neue", sans-serif; }
        h1 { color: #333; }
        p { color: #666; line-height: 1.6; }
    </style>
</head>
<body>
    <h1>Welcome to OrchestraSynth</h1>
    <p>This installer will install OrchestraSynth and its audio plugins on your Mac.</p>

    <p><strong>What will be installed:</strong></p>
    <ul>
        <li>OrchestraSynth.app (Standalone Application)</li>
        <li>OrchestraSynth.vst3 (VST3 Plugin)</li>
        <li>OrchestraSynth.component (Audio Unit Plugin)</li>
    </ul>

    <p><strong>Requirements:</strong></p>
    <ul>
        <li>macOS 14.0 or later</li>
        <li>Apple Silicon (M1/M2/M3) or Intel processor</li>
    </ul>
</body>
</html>
HTML_EOF

# License
cat > "${RESOURCES_DIR}/license.txt" << 'LICENSE_EOF'
OrchestraSynth Software License Agreement

Copyright (c) 2024 YourCompany. All rights reserved.

[Add your license terms here]

This is a placeholder license. Replace with your actual license agreement.
LICENSE_EOF

# Readme
cat > "${RESOURCES_DIR}/readme.html" << 'README_EOF'
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Helvetica Neue", sans-serif; }
        h2 { color: #333; }
        p { color: #666; line-height: 1.6; }
    </style>
</head>
<body>
    <h2>Installation Complete</h2>

    <p><strong>Installed Locations:</strong></p>
    <ul>
        <li>Standalone App: /Applications/OrchestraSynth.app</li>
        <li>VST3 Plugin: /Library/Audio/Plug-Ins/VST3/OrchestraSynth.vst3</li>
        <li>AU Plugin: /Library/Audio/Plug-Ins/Components/OrchestraSynth.component</li>
    </ul>

    <p><strong>Next Steps:</strong></p>
    <ol>
        <li>Restart your DAW (Logic Pro, Ableton Live, etc.)</li>
        <li>Rescan plugins in your DAW if necessary</li>
        <li>Launch OrchestraSynth from Applications or load as plugin</li>
    </ol>

    <p><strong>Support:</strong></p>
    <p>For support and documentation, visit: https://yourcompany.com/support</p>
</body>
</html>
README_EOF

echo -e "${GREEN}✓${NC} Resource files created"
echo ""

# ============================================================================
# Build Product Archive (Final PKG)
# ============================================================================

echo -e "${BLUE}=== Creating Final Product Package ===${NC}"
echo ""

# Ensure output directory exists
mkdir -p "$PKG_OUTPUT_DIR"

# Build productbuild command
PRODUCTBUILD_CMD=(
    productbuild
    --distribution "$DISTRIBUTION_XML"
    --resources "$RESOURCES_DIR"
    --package-path "$WORK_DIR"
)

# Add signing if identity provided
if [ -n "$PKG_SIGNING_IDENTITY" ]; then
    PRODUCTBUILD_CMD+=(--sign "$PKG_SIGNING_IDENTITY")
    echo -e "${BLUE}Signing with:${NC} $PKG_SIGNING_IDENTITY"
fi

PRODUCTBUILD_CMD+=("$PKG_PATH")

# Build the final PKG
if "${PRODUCTBUILD_CMD[@]}"; then
    echo ""
    echo -e "${GREEN}✓${NC} Product package created successfully"
else
    echo ""
    echo -e "${RED}✗${NC} Failed to create product package"
    exit 1
fi

# ============================================================================
# Validate Package
# ============================================================================

echo ""
echo -e "${BLUE}=== Validating Package ===${NC}"
echo ""

# Check if PKG exists and get size
if [ -f "$PKG_PATH" ]; then
    PKG_SIZE=$(du -h "$PKG_PATH" | cut -f1)
    echo -e "${GREEN}✓${NC} Package created: $PKG_PATH"
    echo -e "${GREEN}✓${NC} Package size: $PKG_SIZE"

    # Verify package signature if signed
    if [ -n "$PKG_SIGNING_IDENTITY" ]; then
        if pkgutil --check-signature "$PKG_PATH" > /dev/null 2>&1; then
            echo -e "${GREEN}✓${NC} Package signature valid"
        else
            echo -e "${YELLOW}⚠${NC}  Warning: Package signature validation failed"
        fi
    fi
else
    echo -e "${RED}✗${NC} Package file not found"
    exit 1
fi

echo ""

# ============================================================================
# Summary
# ============================================================================

echo -e "${GREEN}=== PKG Installer Build Complete ===${NC}"
echo ""
echo "Output: $PKG_PATH"
echo ""
echo "Installation targets:"
echo "  • /Applications/OrchestraSynth.app"
echo "  • /Library/Audio/Plug-Ins/VST3/OrchestraSynth.vst3"
echo "  • /Library/Audio/Plug-Ins/Components/OrchestraSynth.component"
echo ""
echo "To test installation:"
echo "  sudo installer -pkg \"$PKG_PATH\" -target /"
echo ""
echo "For enterprise deployment:"
echo "  1. Sign the PKG with Developer ID Installer certificate"
echo "  2. Notarize the PKG: ./scripts/macos/notarize.sh \"$PKG_PATH\""
echo "  3. Upload to your MDM system (Jamf, Intune, etc.)"
echo ""

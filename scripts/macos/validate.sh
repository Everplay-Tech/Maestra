#!/bin/bash

# ============================================================================
# OrchestraSynth Validation & Verification Script
# ============================================================================
# Comprehensive validation of build artifacts for production readiness.
# Checks code signing, notarization, architecture, entitlements, and more.
#
# Usage:
#   ./scripts/macos/validate.sh [BUILD_DIR]
#
# Exit Codes:
#   0 - All validations passed
#   1 - Critical failures found
#   2 - Warnings found (non-critical)
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

# Configuration
BUILD_DIR="${1:-build/macos-universal-release}"
VALIDATION_PASSED=0
WARNINGS_FOUND=0
ERRORS_FOUND=0

echo ""
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║                                                               ║${NC}"
echo -e "${CYAN}║         ${MAGENTA}OrchestraSynth Validation & Verification${CYAN}          ║${NC}"
echo -e "${CYAN}║                                                               ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# ============================================================================
# Helper Functions
# ============================================================================

print_section() {
    echo ""
    echo -e "${BLUE}=== $1 ===${NC}"
    echo ""
}

print_check() {
    echo -e "${BLUE}Checking:${NC} $1"
}

print_pass() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}⚠${NC}  $1"
    WARNINGS_FOUND=$((WARNINGS_FOUND + 1))
}

print_fail() {
    echo -e "${RED}✗${NC} $1"
    ERRORS_FOUND=$((ERRORS_FOUND + 1))
}

validate_bundle() {
    local bundle_path="$1"
    local bundle_name=$(basename "$bundle_path")
    local bundle_type="$2" # "app" or "plugin"

    echo ""
    echo -e "${CYAN}━━━ Validating: $bundle_name ━━━${NC}"
    echo ""

    # Check existence
    if [ ! -d "$bundle_path" ]; then
        print_fail "$bundle_name not found"
        return 1
    fi
    print_pass "$bundle_name exists"

    # Check architecture (universal binary)
    print_check "Architecture (universal binary)"
    local executable="${bundle_path}/Contents/MacOS/$(basename "$bundle_path" | sed 's/\.[^.]*$//')"
    if [ -f "$executable" ]; then
        local archs=$(lipo -archs "$executable" 2>/dev/null)
        if echo "$archs" | grep -q "x86_64" && echo "$archs" | grep -q "arm64"; then
            print_pass "Universal binary: $archs"
        elif echo "$archs" | grep -q "arm64"; then
            print_warn "Apple Silicon only: $archs (missing Intel)"
        elif echo "$archs" | grep -q "x86_64"; then
            print_warn "Intel only: $archs (missing Apple Silicon)"
        else
            print_fail "Unknown architecture: $archs"
        fi
    else
        print_fail "Executable not found: $executable"
    fi

    # Check code signature
    print_check "Code signature"
    if codesign --verify --deep --strict "$bundle_path" 2>/dev/null; then
        print_pass "Code signature valid"

        # Check for Developer ID
        local signing_info=$(codesign -dv "$bundle_path" 2>&1)
        if echo "$signing_info" | grep -q "Developer ID"; then
            print_pass "Signed with Developer ID (distribution)"
        elif echo "$signing_info" | grep -q "Apple Development"; then
            print_warn "Signed with Development certificate (not for distribution)"
        else
            print_warn "Signed with unknown certificate type"
        fi

        # Check hardened runtime
        if echo "$signing_info" | grep -q "flags=.*runtime"; then
            print_pass "Hardened runtime enabled"
        else
            print_fail "Hardened runtime NOT enabled (required for notarization)"
        fi

        # Check timestamp
        if echo "$signing_info" | grep -q "Timestamp"; then
            print_pass "Signature includes secure timestamp"
        else
            print_warn "Signature missing secure timestamp"
        fi
    else
        print_fail "Code signature invalid or missing"
    fi

    # Check entitlements
    print_check "Entitlements"
    local entitlements=$(codesign -d --entitlements :- "$bundle_path" 2>/dev/null)
    if [ -n "$entitlements" ]; then
        print_pass "Entitlements present"

        # Check for key entitlements
        if echo "$entitlements" | grep -q "com.apple.security.device.audio-input"; then
            print_pass "Audio input entitlement present"
        else
            print_warn "Audio input entitlement missing"
        fi

        if echo "$entitlements" | grep -q "com.apple.security.device.usb"; then
            print_pass "USB (MIDI) entitlement present"
        else
            print_warn "USB entitlement missing (may affect MIDI)"
        fi
    else
        print_fail "No entitlements found"
    fi

    # Check bundle structure
    print_check "Bundle structure"
    if [ -f "${bundle_path}/Contents/Info.plist" ]; then
        print_pass "Info.plist present"

        # Validate Info.plist
        if /usr/libexec/PlistBuddy -c "Print CFBundleIdentifier" "${bundle_path}/Contents/Info.plist" >/dev/null 2>&1; then
            local bundle_id=$(/usr/libexec/PlistBuddy -c "Print CFBundleIdentifier" "${bundle_path}/Contents/Info.plist")
            print_pass "Bundle ID: $bundle_id"

            # Check for placeholder IDs
            if echo "$bundle_id" | grep -qi "yourcompany"; then
                print_warn "Bundle ID contains placeholder 'yourcompany' - should be customized"
            fi
        else
            print_fail "Invalid Info.plist"
        fi
    else
        print_fail "Info.plist missing"
    fi

    # Check for extended attributes (quarantine)
    print_check "Extended attributes"
    local xattrs=$(xattr "$bundle_path" 2>/dev/null)
    if [ -z "$xattrs" ]; then
        print_pass "No quarantine attributes"
    else
        if echo "$xattrs" | grep -q "com.apple.quarantine"; then
            print_warn "Quarantine attribute present (expected for downloaded files)"
        fi
    fi

    echo ""
}

# ============================================================================
# Validate Build Directory
# ============================================================================

print_section "Build Directory"

if [ ! -d "$BUILD_DIR" ]; then
    print_fail "Build directory not found: $BUILD_DIR"
    echo ""
    echo -e "${RED}ERROR: Cannot validate - build directory does not exist${NC}"
    echo "Please build the project first"
    exit 1
fi

print_pass "Build directory exists: $BUILD_DIR"

# ============================================================================
# Validate Standalone Application
# ============================================================================

print_section "Standalone Application"

APP_PATH="${BUILD_DIR}/OrchestraSynth.app"
validate_bundle "$APP_PATH" "app"

# ============================================================================
# Validate VST3 Plugin
# ============================================================================

print_section "VST3 Plugin"

VST3_PATH="${BUILD_DIR}/Plug-Ins/VST3/OrchestraSynth.vst3"
if [ -d "$VST3_PATH" ]; then
    validate_bundle "$VST3_PATH" "plugin"
else
    print_warn "VST3 plugin not found (optional)"
fi

# ============================================================================
# Validate AU Plugin
# ============================================================================

print_section "Audio Unit Plugin"

AU_PATH="${BUILD_DIR}/Plug-Ins/AU/OrchestraSynth.component"
if [ -d "$AU_PATH" ]; then
    validate_bundle "$AU_PATH" "plugin"
else
    print_warn "AU plugin not found (optional)"
fi

# ============================================================================
# Validate DMG
# ============================================================================

print_section "DMG Installer"

DMG_PATH="${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.dmg"
if [ -f "$DMG_PATH" ]; then
    print_pass "DMG exists: $(basename "$DMG_PATH")"

    # Check DMG size
    DMG_SIZE=$(du -h "$DMG_PATH" | cut -f1)
    print_pass "DMG size: $DMG_SIZE"

    # Check DMG signature (if signed)
    print_check "DMG signature"
    if codesign -dv "$DMG_PATH" 2>&1 | grep -q "Developer ID"; then
        print_pass "DMG is signed with Developer ID"
    else
        print_warn "DMG is not signed (optional for DMG)"
    fi

    # Check notarization
    print_check "Notarization status"
    if spctl -a -vv -t install "$DMG_PATH" 2>&1 | grep -q "accepted"; then
        print_pass "DMG is notarized and accepted by Gatekeeper"
    else
        print_warn "DMG is not notarized or not accepted by Gatekeeper"
    fi

    # Check stapled ticket
    print_check "Notarization ticket"
    if xcrun stapler validate "$DMG_PATH" 2>&1 | grep -q "validated"; then
        print_pass "Notarization ticket is stapled"
    else
        print_warn "Notarization ticket is not stapled (requires online verification)"
    fi

    # Try to mount DMG
    print_check "DMG integrity"
    if hdiutil verify "$DMG_PATH" >/dev/null 2>&1; then
        print_pass "DMG passes integrity check"
    else
        print_fail "DMG failed integrity check"
    fi
else
    print_warn "DMG not found (optional)"
fi

# ============================================================================
# Validate PKG
# ============================================================================

print_section "PKG Installer"

PKG_PATH="${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.pkg"
if [ -f "$PKG_PATH" ]; then
    print_pass "PKG exists: $(basename "$PKG_PATH")"

    # Check PKG size
    PKG_SIZE=$(du -h "$PKG_PATH" | cut -f1)
    print_pass "PKG size: $PKG_SIZE"

    # Check PKG signature
    print_check "PKG signature"
    PKG_SIG_INFO=$(pkgutil --check-signature "$PKG_PATH" 2>&1)
    if echo "$PKG_SIG_INFO" | grep -q "signed by a developer certificate"; then
        print_pass "PKG is signed with Developer ID Installer"
    elif echo "$PKG_SIG_INFO" | grep -q "signed"; then
        print_warn "PKG is signed but not with Developer ID Installer"
    else
        print_warn "PKG is not signed"
    fi

    # Check notarization
    print_check "PKG notarization"
    if spctl -a -vv -t install "$PKG_PATH" 2>&1 | grep -q "accepted"; then
        print_pass "PKG is notarized and accepted by Gatekeeper"
    else
        print_warn "PKG is not notarized"
    fi

    # Validate PKG structure
    print_check "PKG structure"
    if pkgutil --payload-files "$PKG_PATH" >/dev/null 2>&1; then
        print_pass "PKG structure is valid"

        # Check installation targets
        local payload=$(pkgutil --payload-files "$PKG_PATH" 2>/dev/null)
        if echo "$payload" | grep -q "Applications/OrchestraSynth.app"; then
            print_pass "PKG installs standalone app"
        fi
        if echo "$payload" | grep -q "Library/Audio/Plug-Ins/VST3"; then
            print_pass "PKG installs VST3 plugin"
        fi
        if echo "$payload" | grep -q "Library/Audio/Plug-Ins/Components"; then
            print_pass "PKG installs AU plugin"
        fi
    else
        print_fail "PKG structure is invalid"
    fi
else
    print_warn "PKG not found (optional)"
fi

# ============================================================================
# System Compatibility Checks
# ============================================================================

print_section "System Compatibility"

# Check macOS version
MACOS_VERSION=$(sw_vers -productVersion)
print_pass "Validator running on macOS $MACOS_VERSION"

# Check minimum deployment target
print_check "Deployment target compatibility"
if [ -f "$APP_PATH" ]; then
    local min_version=$(otool -l "${APP_PATH}/Contents/MacOS/OrchestraSynth" 2>/dev/null | grep -A 3 "LC_VERSION_MIN_MACOSX" | grep "version" | head -1 | awk '{print $2}')
    if [ -n "$min_version" ]; then
        print_pass "Minimum macOS version: $min_version"
    else
        # Try alternate method for newer builds
        min_version=$(otool -l "${APP_PATH}/Contents/MacOS/OrchestraSynth" 2>/dev/null | grep -A 4 "LC_BUILD_VERSION" | grep "minos" | awk '{print $2}')
        if [ -n "$min_version" ]; then
            print_pass "Minimum macOS version: $min_version"
        fi
    fi
fi

# ============================================================================
# Security Checks
# ============================================================================

print_section "Security Validation"

# Check for library validation
print_check "Library validation"
if [ -f "$APP_PATH" ]; then
    local flags=$(codesign -d --entitlements :- "$APP_PATH" 2>&1)
    if echo "$flags" | grep -q "com.apple.security.cs.disable-library-validation"; then
        if echo "$flags" | grep -A 1 "disable-library-validation" | grep -q "true"; then
            print_warn "Library validation is disabled (may be required for plugins)"
        else
            print_pass "Library validation is enabled"
        fi
    else
        print_pass "Library validation is enabled (default)"
    fi
fi

# Check for dangerous entitlements
print_check "Dangerous entitlements"
if [ -f "$APP_PATH" ]; then
    local ents=$(codesign -d --entitlements :- "$APP_PATH" 2>&1)

    if echo "$ents" | grep -q "com.apple.security.cs.allow-jit.*true"; then
        print_warn "JIT is enabled (may fail App Store review)"
    fi

    if echo "$ents" | grep -q "com.apple.security.cs.allow-unsigned-executable-memory.*true"; then
        print_warn "Unsigned executable memory allowed (may fail App Store review)"
    fi

    if echo "$ents" | grep -q "com.apple.security.cs.allow-dyld-environment-variables.*true"; then
        print_warn "DYLD environment variables allowed (security risk)"
    fi

    if echo "$ents" | grep -q "com.apple.security.cs.disable-library-validation.*true"; then
        print_warn "Library validation disabled (security risk)"
    fi
fi

# ============================================================================
# Final Summary
# ============================================================================

echo ""
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  Validation Summary                                          ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""

if [ $ERRORS_FOUND -eq 0 ] && [ $WARNINGS_FOUND -eq 0 ]; then
    echo -e "${GREEN}✓ All validations passed!${NC}"
    echo ""
    echo "The build is ready for production distribution."
    EXIT_CODE=0
elif [ $ERRORS_FOUND -eq 0 ]; then
    echo -e "${YELLOW}⚠  Validation completed with $WARNINGS_FOUND warning(s)${NC}"
    echo ""
    echo "The build is usable but has some non-critical issues."
    echo "Review the warnings above and address if necessary."
    EXIT_CODE=2
else
    echo -e "${RED}✗ Validation failed with $ERRORS_FOUND error(s) and $WARNINGS_FOUND warning(s)${NC}"
    echo ""
    echo "The build has critical issues that must be addressed before distribution."
    echo "Review the errors above and rebuild."
    EXIT_CODE=1
fi

echo ""
echo -e "${CYAN}Statistics:${NC}"
echo -e "  Errors:   ${RED}$ERRORS_FOUND${NC}"
echo -e "  Warnings: ${YELLOW}$WARNINGS_FOUND${NC}"
echo ""

exit $EXIT_CODE

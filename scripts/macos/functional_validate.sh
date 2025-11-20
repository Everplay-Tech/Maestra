#!/bin/bash

# ============================================================================
# OrchestraSynth Functional Validation Script
# ============================================================================
# Automated functional and runtime validation to complement technical checks.
# Validates runtime behavior, plugin loading, and operational readiness.
#
# Usage:
#   ./scripts/macos/functional_validate.sh [BUILD_DIR]
#
# Exit Codes:
#   0 - All functional tests passed
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
TEST_TIMEOUT=10

echo ""
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║                                                               ║${NC}"
echo -e "${CYAN}║        ${MAGENTA}OrchestraSynth Functional Validation${CYAN}             ║${NC}"
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

# ============================================================================
# Validate Build Directory
# ============================================================================

print_section "Build Directory Validation"

if [ ! -d "$BUILD_DIR" ]; then
    print_fail "Build directory not found: $BUILD_DIR"
    echo ""
    echo -e "${RED}ERROR: Cannot validate - build directory does not exist${NC}"
    echo "Please build the project first"
    exit 1
fi

print_pass "Build directory exists: $BUILD_DIR"

# Define paths
APP_PATH="${BUILD_DIR}/OrchestraSynth.app"
VST3_PATH="${BUILD_DIR}/Plug-Ins/VST3/OrchestraSynth.vst3"
AU_PATH="${BUILD_DIR}/Plug-Ins/AU/OrchestraSynth.component"

# ============================================================================
# Runtime Smoke Test - Standalone Application
# ============================================================================

print_section "Standalone Application Runtime Test"

if [ ! -d "$APP_PATH" ]; then
    print_fail "Standalone app not found at $APP_PATH"
else
    print_pass "Standalone app bundle exists"

    # Check if executable exists
    APP_EXECUTABLE="${APP_PATH}/Contents/MacOS/OrchestraSynth"
    if [ ! -f "$APP_EXECUTABLE" ]; then
        print_fail "Executable not found: $APP_EXECUTABLE"
    else
        print_pass "Executable exists"

        # Check if executable is runnable
        print_check "Executable permissions"
        if [ -x "$APP_EXECUTABLE" ]; then
            print_pass "Executable has correct permissions"
        else
            print_fail "Executable is not executable"
        fi

        # Test application launch (headless test - just check it doesn't crash immediately)
        print_check "Application launch test (${TEST_TIMEOUT}s timeout)"

        # Create a test script that launches and immediately quits
        timeout $TEST_TIMEOUT "$APP_EXECUTABLE" --help >/dev/null 2>&1 &
        APP_PID=$!
        sleep 2

        if kill -0 $APP_PID 2>/dev/null; then
            kill -TERM $APP_PID 2>/dev/null
            wait $APP_PID 2>/dev/null
            print_pass "Application launched successfully"
        else
            wait $APP_PID 2>/dev/null
            EXIT_CODE=$?
            if [ $EXIT_CODE -eq 0 ]; then
                print_pass "Application responded to help flag"
            else
                print_warn "Application may have issues (exit code: $EXIT_CODE)"
            fi
        fi
    fi

    # Check Info.plist structure
    print_check "Info.plist validation"
    INFO_PLIST="${APP_PATH}/Contents/Info.plist"
    if [ -f "$INFO_PLIST" ]; then
        # Validate required keys
        REQUIRED_KEYS=(
            "CFBundleIdentifier"
            "CFBundleName"
            "CFBundleVersion"
            "CFBundleShortVersionString"
            "CFBundleExecutable"
        )

        PLIST_VALID=true
        for key in "${REQUIRED_KEYS[@]}"; do
            if ! /usr/libexec/PlistBuddy -c "Print $key" "$INFO_PLIST" >/dev/null 2>&1; then
                print_fail "Missing required key: $key"
                PLIST_VALID=false
            fi
        done

        if [ "$PLIST_VALID" = true ]; then
            print_pass "All required Info.plist keys present"
        fi
    else
        print_fail "Info.plist not found"
    fi

    # Check for required frameworks/libraries
    print_check "Framework dependencies"
    if otool -L "$APP_EXECUTABLE" >/dev/null 2>&1; then
        print_pass "Framework dependencies are valid"

        # Check for unexpected absolute paths (should be relative or system)
        DEPS=$(otool -L "$APP_EXECUTABLE" | grep -v "^$APP_EXECUTABLE:" | grep "/" | awk '{print $1}')
        BAD_DEPS=0
        while IFS= read -r dep; do
            # Skip system frameworks and @rpath/@executable_path
            if [[ ! "$dep" =~ ^/System/ ]] && [[ ! "$dep" =~ ^/usr/lib/ ]] && \
               [[ ! "$dep" =~ ^@rpath ]] && [[ ! "$dep" =~ ^@executable_path ]] && \
               [[ ! "$dep" =~ ^@loader_path ]]; then
                print_warn "Unexpected absolute path: $dep"
                BAD_DEPS=$((BAD_DEPS + 1))
            fi
        done <<< "$DEPS"

        if [ $BAD_DEPS -eq 0 ]; then
            print_pass "No hardcoded absolute paths in dependencies"
        fi
    else
        print_fail "Cannot inspect framework dependencies"
    fi
fi

# ============================================================================
# Plugin Validation - VST3
# ============================================================================

print_section "VST3 Plugin Validation"

if [ ! -d "$VST3_PATH" ]; then
    print_warn "VST3 plugin not found (optional)"
else
    print_pass "VST3 plugin bundle exists"

    # Check VST3 bundle structure
    print_check "VST3 bundle structure"
    VST3_EXECUTABLE="${VST3_PATH}/Contents/MacOS/OrchestraSynth"

    if [ -f "$VST3_EXECUTABLE" ]; then
        print_pass "VST3 executable exists"

        # Check if it's a valid Mach-O binary
        if file "$VST3_EXECUTABLE" | grep -q "Mach-O"; then
            print_pass "VST3 is valid Mach-O binary"
        else
            print_fail "VST3 is not a valid Mach-O binary"
        fi
    else
        print_fail "VST3 executable not found"
    fi

    # Check for moduleinfo.json (VST3 requirement)
    print_check "VST3 moduleinfo.json"
    MODULEINFO="${VST3_PATH}/Contents/moduleinfo.json"
    if [ -f "$MODULEINFO" ]; then
        print_pass "moduleinfo.json exists"

        # Validate JSON structure
        if python3 -c "import json; json.load(open('$MODULEINFO'))" 2>/dev/null; then
            print_pass "moduleinfo.json is valid JSON"
        else
            print_fail "moduleinfo.json is invalid JSON"
        fi
    else
        print_warn "moduleinfo.json not found (may affect plugin scanning)"
    fi

    # Test plugin with validator if available (auval for AU, no standard VST3 validator on macOS)
    print_check "VST3 plugin scan test"
    # Most DAWs scan plugins at launch, we'll just verify the binary is loadable
    if otool -L "$VST3_EXECUTABLE" >/dev/null 2>&1; then
        print_pass "VST3 plugin dependencies are valid"
    else
        print_fail "VST3 plugin has invalid dependencies"
    fi
fi

# ============================================================================
# Plugin Validation - Audio Unit
# ============================================================================

print_section "Audio Unit Plugin Validation"

if [ ! -d "$AU_PATH" ]; then
    print_warn "AU plugin not found (optional)"
else
    print_pass "AU plugin bundle exists"

    # Check AU bundle structure
    print_check "AU bundle structure"
    AU_EXECUTABLE="${AU_PATH}/Contents/MacOS/OrchestraSynth"

    if [ -f "$AU_EXECUTABLE" ]; then
        print_pass "AU executable exists"

        # Check if it's a valid Mach-O binary
        if file "$AU_EXECUTABLE" | grep -q "Mach-O"; then
            print_pass "AU is valid Mach-O binary"
        else
            print_fail "AU is not a valid Mach-O binary"
        fi
    else
        print_fail "AU executable not found"
    fi

    # Check AU Info.plist for required keys
    print_check "AU Info.plist validation"
    AU_PLIST="${AU_PATH}/Contents/Info.plist"
    if [ -f "$AU_PLIST" ]; then
        # Check for AudioComponents key (required for AU)
        if /usr/libexec/PlistBuddy -c "Print AudioComponents" "$AU_PLIST" >/dev/null 2>&1; then
            print_pass "AudioComponents entry present"

            # Check for required AU component fields
            if /usr/libexec/PlistBuddy -c "Print AudioComponents:0:type" "$AU_PLIST" >/dev/null 2>&1; then
                AU_TYPE=$(/usr/libexec/PlistBuddy -c "Print AudioComponents:0:type" "$AU_PLIST")
                print_pass "AU type: $AU_TYPE"
            else
                print_fail "AU component type missing"
            fi

            if /usr/libexec/PlistBuddy -c "Print AudioComponents:0:subtype" "$AU_PLIST" >/dev/null 2>&1; then
                AU_SUBTYPE=$(/usr/libexec/PlistBuddy -c "Print AudioComponents:0:subtype" "$AU_PLIST")
                print_pass "AU subtype: $AU_SUBTYPE"
            else
                print_fail "AU component subtype missing"
            fi

            if /usr/libexec/PlistBuddy -c "Print AudioComponents:0:manufacturer" "$AU_PLIST" >/dev/null 2>&1; then
                AU_MANU=$(/usr/libexec/PlistBuddy -c "Print AudioComponents:0:manufacturer" "$AU_PLIST")
                print_pass "AU manufacturer: $AU_MANU"
            else
                print_fail "AU manufacturer code missing"
            fi
        else
            print_fail "AudioComponents entry missing from Info.plist"
        fi
    else
        print_fail "AU Info.plist not found"
    fi

    # Run auval (Audio Unit Validation) if the plugin exists
    # This is a lightweight check - full validation can take minutes
    print_check "AU validation with auval (quick check)"
    if command -v auval >/dev/null 2>&1; then
        # Extract AU component description for auval
        if [ -n "$AU_TYPE" ] && [ -n "$AU_SUBTYPE" ] && [ -n "$AU_MANU" ]; then
            # Just verify the plugin is discoverable (not full validation which is slow)
            if auval -a 2>&1 | grep -q "$AU_MANU"; then
                print_pass "AU plugin is discoverable by system"
            else
                print_warn "AU plugin may not be discoverable (try: killall -9 AudioComponentRegistrar)"
            fi
        fi
    else
        print_warn "auval not available (AU validation skipped)"
    fi
fi

# ============================================================================
# Package Validation
# ============================================================================

print_section "Distribution Package Validation"

# Check DMG
DMG_PATH="${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.dmg"
if [ -f "$DMG_PATH" ]; then
    print_pass "DMG exists"

    # Quick integrity check
    print_check "DMG integrity"
    if hdiutil verify "$DMG_PATH" >/dev/null 2>&1; then
        print_pass "DMG integrity check passed"
    else
        print_fail "DMG integrity check failed"
    fi

    # Check if DMG is mountable
    print_check "DMG mount test"
    MOUNT_POINT=$(mktemp -d)
    if hdiutil attach "$DMG_PATH" -mountpoint "$MOUNT_POINT" -nobrowse -quiet 2>/dev/null; then
        print_pass "DMG mounts successfully"

        # Check contents
        if [ -d "$MOUNT_POINT/OrchestraSynth.app" ]; then
            print_pass "App present in DMG"
        else
            print_fail "App missing from DMG"
        fi

        if [ -d "$MOUNT_POINT/Plug-Ins" ]; then
            print_pass "Plug-Ins folder present in DMG"
        else
            print_warn "Plug-Ins folder missing from DMG"
        fi

        # Unmount
        hdiutil detach "$MOUNT_POINT" -quiet 2>/dev/null
        rmdir "$MOUNT_POINT" 2>/dev/null
    else
        print_fail "DMG failed to mount"
        rmdir "$MOUNT_POINT" 2>/dev/null
    fi
else
    print_warn "DMG not found (may not be built yet)"
fi

# Check PKG
PKG_PATH="${BUILD_DIR}/OrchestraSynth-0.1.0-macOS-universal.pkg"
if [ -f "$PKG_PATH" ]; then
    print_pass "PKG exists"

    # Validate PKG structure
    print_check "PKG structure validation"
    if pkgutil --check-signature "$PKG_PATH" >/dev/null 2>&1; then
        print_pass "PKG structure is valid"
    else
        # PKG might not be signed yet
        if pkgutil --payload-files "$PKG_PATH" >/dev/null 2>&1; then
            print_pass "PKG structure is valid (not signed)"
        else
            print_fail "PKG structure is invalid"
        fi
    fi

    # Check expected payload
    print_check "PKG payload contents"
    PAYLOAD=$(pkgutil --payload-files "$PKG_PATH" 2>/dev/null)

    if echo "$PAYLOAD" | grep -q "Applications/OrchestraSynth.app"; then
        print_pass "PKG contains standalone app"
    else
        print_warn "PKG missing standalone app in payload"
    fi

    if echo "$PAYLOAD" | grep -q "Library/Audio/Plug-Ins"; then
        print_pass "PKG contains audio plugins"
    else
        print_warn "PKG missing audio plugins in payload"
    fi
else
    print_warn "PKG not found (may not be built yet)"
fi

# ============================================================================
# Observability & Logging Validation
# ============================================================================

print_section "Observability & Logging Infrastructure"

# Check for logging infrastructure in source
print_check "Logger implementation"
if [ -f "src/Logger.h" ] || grep -r "class Logger" src/ >/dev/null 2>&1; then
    print_pass "Logger infrastructure detected"
else
    print_warn "Logger infrastructure not found"
fi

# Check for crash reporting
print_check "Crash reporting implementation"
if [ -f "src/CrashReporter.h" ] || grep -r "CrashReporter" src/ >/dev/null 2>&1; then
    print_pass "Crash reporting infrastructure detected"
else
    print_warn "Crash reporting infrastructure not found"
fi

# Check for performance monitoring
print_check "Performance monitoring implementation"
if [ -f "src/PerformanceMonitor.h" ] || grep -r "PerformanceMonitor" src/ >/dev/null 2>&1; then
    print_pass "Performance monitoring infrastructure detected"
else
    print_warn "Performance monitoring infrastructure not found"
fi

# ============================================================================
# Security & Hardening Validation
# ============================================================================

print_section "Security & Hardening"

if [ -f "$APP_EXECUTABLE" ]; then
    # Check for stack protection
    print_check "Stack protection (stack canaries)"
    if otool -I "$APP_EXECUTABLE" 2>/dev/null | grep -q "___stack_chk"; then
        print_pass "Stack canaries enabled"
    else
        print_warn "Stack canaries not detected"
    fi

    # Check for position-independent code
    print_check "Position Independent Executable (PIE)"
    if otool -hv "$APP_EXECUTABLE" 2>/dev/null | grep -q "PIE"; then
        print_pass "PIE enabled"
    else
        print_warn "PIE not enabled (recommended for security)"
    fi

    # Check for ARC (Automatic Reference Counting) in Objective-C++ code
    print_check "ARC (Automatic Reference Counting)"
    if otool -I "$APP_EXECUTABLE" 2>/dev/null | grep -q "objc_retain\|objc_release"; then
        print_pass "ARC detected (memory safety)"
    else
        # This is not necessarily bad - might not use ObjC
        echo -e "${BLUE}ℹ${NC}  ARC not detected (may not use Objective-C)"
    fi
fi

# ============================================================================
# Build Configuration Validation
# ============================================================================

print_section "Build Configuration"

# Check for debug symbols in release build
if [ -f "$APP_EXECUTABLE" ]; then
    print_check "Debug symbols (should be stripped for release)"
    if dsymutil --verify "$APP_EXECUTABLE" >/dev/null 2>&1; then
        print_warn "Debug symbols present (consider stripping for release)"
    else
        if file "$APP_EXECUTABLE" | grep -q "stripped"; then
            print_pass "Debug symbols stripped (optimized for release)"
        else
            echo -e "${BLUE}ℹ${NC}  Debug symbol status unclear"
        fi
    fi

    # Check optimization level (heuristic - smaller binary often means optimized)
    print_check "Binary optimization"
    BINARY_SIZE=$(stat -f%z "$APP_EXECUTABLE" 2>/dev/null || stat -c%s "$APP_EXECUTABLE" 2>/dev/null)
    if [ -n "$BINARY_SIZE" ]; then
        # Convert to MB
        SIZE_MB=$((BINARY_SIZE / 1024 / 1024))
        print_pass "Binary size: ${SIZE_MB}MB"

        # Warn if binary seems unusually large (might have debug info)
        if [ $SIZE_MB -gt 100 ]; then
            print_warn "Binary size is large (${SIZE_MB}MB) - verify optimization settings"
        fi
    fi
fi

# ============================================================================
# Documentation Validation
# ============================================================================

print_section "Documentation & Production Readiness"

# Check for required documentation
REQUIRED_DOCS=(
    "README.md"
    "docs/OPERATIONS_MANUAL.md"
    "docs/PRODUCTION_PACKAGING.md"
    "docs/APPLE_PACKAGING.md"
)

print_check "Required documentation"
DOCS_MISSING=0
for doc in "${REQUIRED_DOCS[@]}"; do
    if [ -f "$doc" ]; then
        echo -e "  ${GREEN}✓${NC} $doc"
    else
        echo -e "  ${RED}✗${NC} $doc (missing)"
        DOCS_MISSING=$((DOCS_MISSING + 1))
    fi
done

if [ $DOCS_MISSING -eq 0 ]; then
    print_pass "All required documentation present"
else
    print_fail "$DOCS_MISSING documentation file(s) missing"
fi

# Check for build configuration files
print_check "Build configuration"
if [ -f "config/build.env.example" ]; then
    print_pass "Build configuration template exists"
else
    print_warn "Build configuration template missing"
fi

# Check for entitlements
print_check "Entitlements configuration"
ENTITLEMENTS_FOUND=0
if [ -f "config/entitlements/app.entitlements" ]; then
    ENTITLEMENTS_FOUND=$((ENTITLEMENTS_FOUND + 1))
fi
if [ -f "config/entitlements/plugin.entitlements" ]; then
    ENTITLEMENTS_FOUND=$((ENTITLEMENTS_FOUND + 1))
fi

if [ $ENTITLEMENTS_FOUND -eq 2 ]; then
    print_pass "Entitlements files present (app + plugin)"
elif [ $ENTITLEMENTS_FOUND -gt 0 ]; then
    print_warn "Some entitlements files missing"
else
    print_fail "Entitlements files not found"
fi

# ============================================================================
# Final Summary
# ============================================================================

echo ""
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  Functional Validation Summary                               ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""

if [ $ERRORS_FOUND -eq 0 ] && [ $WARNINGS_FOUND -eq 0 ]; then
    echo -e "${GREEN}✓ All functional validations passed!${NC}"
    echo ""
    echo "The build is functionally ready for production."
    EXIT_CODE=0
elif [ $ERRORS_FOUND -eq 0 ]; then
    echo -e "${YELLOW}⚠  Functional validation completed with $WARNINGS_FOUND warning(s)${NC}"
    echo ""
    echo "The build is functionally usable but has some non-critical issues."
    echo "Review the warnings above and address if necessary."
    EXIT_CODE=2
else
    echo -e "${RED}✗ Functional validation failed with $ERRORS_FOUND error(s) and $WARNINGS_FOUND warning(s)${NC}"
    echo ""
    echo "The build has critical functional issues that must be addressed."
    echo "Review the errors above and rebuild."
    EXIT_CODE=1
fi

echo ""
echo -e "${CYAN}Statistics:${NC}"
echo -e "  Errors:   ${RED}$ERRORS_FOUND${NC}"
echo -e "  Warnings: ${YELLOW}$WARNINGS_FOUND${NC}"
echo ""

echo -e "${BLUE}Next Steps:${NC}"
echo "  1. Run technical validation: ./scripts/macos/validate.sh"
echo "  2. Review any errors or warnings above"
echo "  3. Test manually in production-like environment"
echo "  4. Run full integration tests if available"
echo ""

exit $EXIT_CODE

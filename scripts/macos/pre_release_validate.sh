#!/bin/bash

# ============================================================================
# OrchestraSynth Pre-Release Validation Suite
# ============================================================================
# Comprehensive validation suite that runs all checks before release.
# Combines technical, functional, and production readiness validation.
#
# Usage:
#   ./scripts/macos/pre_release_validate.sh [BUILD_DIR] [REPORT_FILE]
#
# Arguments:
#   BUILD_DIR    - Build directory (default: build/macos-universal-release)
#   REPORT_FILE  - Output report file (default: validation-report.txt)
#
# Exit Codes:
#   0 - All validations passed (production ready)
#   1 - Critical failures found (not ready for release)
#   2 - Warnings found (review required)
#
# ============================================================================

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="${1:-build/macos-universal-release}"
REPORT_FILE="${2:-validation-report.txt}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Counters
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0
WARNING_CHECKS=0

# Timestamp
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
REPORT_DATE=$(date '+%Y%m%d_%H%M%S')

# ============================================================================
# Header
# ============================================================================

echo ""
echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║                                                               ║${NC}"
echo -e "${CYAN}║        ${BOLD}${MAGENTA}OrchestraSynth Pre-Release Validation${NC}${CYAN}           ║${NC}"
echo -e "${CYAN}║                                                               ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${BLUE}Validation Suite:${NC}"
echo "  • Technical validation (code signing, notarization, security)"
echo "  • Functional validation (runtime, plugins, observability)"
echo "  • Production readiness checks"
echo "  • Documentation completeness"
echo ""
echo -e "${BLUE}Build Directory:${NC} $BUILD_DIR"
echo -e "${BLUE}Report File:${NC}     $REPORT_FILE"
echo -e "${BLUE}Timestamp:${NC}       $TIMESTAMP"
echo ""

# Initialize report
cat > "$REPORT_FILE" << EOF
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║         OrchestraSynth Pre-Release Validation Report         ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝

Generated: $TIMESTAMP
Build Directory: $BUILD_DIR
Validation Suite Version: 1.0.0

═══════════════════════════════════════════════════════════════

EOF

# ============================================================================
# Helper Functions
# ============================================================================

print_section() {
    local section_name="$1"
    echo ""
    echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${CYAN}  $section_name${NC}"
    echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""

    # Add to report
    echo "" >> "$REPORT_FILE"
    echo "━━━ $section_name ━━━" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
}

run_validation_script() {
    local script_name="$1"
    local script_path="$SCRIPT_DIR/$script_name"
    local description="$2"

    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))

    echo -e "${BLUE}Running:${NC} $description"
    echo ""

    if [ ! -f "$script_path" ]; then
        echo -e "${RED}✗ Validation script not found: $script_path${NC}"
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
        echo "FAILED: $description - Script not found" >> "$REPORT_FILE"
        return 1
    fi

    # Run the script and capture output and exit code
    local temp_output=$(mktemp)
    set +e
    "$script_path" "$BUILD_DIR" > "$temp_output" 2>&1
    local exit_code=$?
    set -e

    # Display output
    cat "$temp_output"

    # Add to report
    echo "--- $description ---" >> "$REPORT_FILE"
    cat "$temp_output" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"

    # Clean up
    rm -f "$temp_output"

    # Evaluate result
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓ $description: PASSED${NC}"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
        echo "RESULT: PASSED" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        return 0
    elif [ $exit_code -eq 2 ]; then
        echo -e "${YELLOW}⚠ $description: PASSED WITH WARNINGS${NC}"
        WARNING_CHECKS=$((WARNING_CHECKS + 1))
        echo "RESULT: PASSED WITH WARNINGS" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        return 0
    else
        echo -e "${RED}✗ $description: FAILED${NC}"
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
        echo "RESULT: FAILED (exit code: $exit_code)" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        return 1
    fi
}

# ============================================================================
# Pre-flight Checks
# ============================================================================

print_section "Pre-flight Checks"

echo -e "${BLUE}Checking prerequisites...${NC}"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}✗ Build directory not found: $BUILD_DIR${NC}"
    echo "CRITICAL: Build directory not found" >> "$REPORT_FILE"
    echo ""
    echo "Please build the project first:"
    echo "  ./scripts/macos/build_and_package.sh"
    echo ""
    exit 1
fi

echo -e "${GREEN}✓ Build directory exists${NC}"

# Check for required tools
REQUIRED_TOOLS=(
    "codesign"
    "pkgutil"
    "lipo"
    "otool"
    "hdiutil"
    "spctl"
)

TOOLS_MISSING=0
for tool in "${REQUIRED_TOOLS[@]}"; do
    if command -v "$tool" >/dev/null 2>&1; then
        echo -e "${GREEN}✓${NC} $tool available"
    else
        echo -e "${RED}✗${NC} $tool not found"
        TOOLS_MISSING=$((TOOLS_MISSING + 1))
    fi
done

if [ $TOOLS_MISSING -gt 0 ]; then
    echo ""
    echo -e "${RED}ERROR: $TOOLS_MISSING required tool(s) missing${NC}"
    echo "CRITICAL: Required tools missing" >> "$REPORT_FILE"
    echo "Please install Xcode Command Line Tools:"
    echo "  xcode-select --install"
    echo ""
    exit 1
fi

echo ""
echo -e "${GREEN}✓ All prerequisites met${NC}"
echo ""

# ============================================================================
# Run Technical Validation
# ============================================================================

print_section "Technical Validation Suite"

run_validation_script "validate.sh" "Technical Validation (Code Signing, Notarization, Architecture)" || true

# ============================================================================
# Run Functional Validation
# ============================================================================

print_section "Functional Validation Suite"

run_validation_script "functional_validate.sh" "Functional Validation (Runtime, Plugins, Observability)" || true

# ============================================================================
# Production Readiness Checklist
# ============================================================================

print_section "Production Readiness Checklist"

echo -e "${BLUE}Checking production readiness criteria...${NC}"
echo ""

READINESS_SCORE=0
READINESS_TOTAL=0

check_criterion() {
    local name="$1"
    local check_command="$2"

    READINESS_TOTAL=$((READINESS_TOTAL + 1))

    echo -n "  [ ] $name ... "

    if eval "$check_command" >/dev/null 2>&1; then
        echo -e "${GREEN}✓${NC}"
        echo "  [✓] $name" >> "$REPORT_FILE"
        READINESS_SCORE=$((READINESS_SCORE + 1))
        return 0
    else
        echo -e "${RED}✗${NC}"
        echo "  [✗] $name" >> "$REPORT_FILE"
        return 1
    fi
}

# Code signing checks
check_criterion "App is code signed" \
    "codesign --verify --deep --strict '$BUILD_DIR/OrchestraSynth.app' 2>/dev/null"

check_criterion "App has hardened runtime" \
    "codesign -dv '$BUILD_DIR/OrchestraSynth.app' 2>&1 | grep -q 'flags=.*runtime'"

check_criterion "App has valid entitlements" \
    "codesign -d --entitlements :- '$BUILD_DIR/OrchestraSynth.app' 2>/dev/null | grep -q 'plist'"

# Architecture checks
check_criterion "App is universal binary (arm64 + x86_64)" \
    "lipo -info '$BUILD_DIR/OrchestraSynth.app/Contents/MacOS/OrchestraSynth' 2>/dev/null | grep -q 'arm64' && lipo -info '$BUILD_DIR/OrchestraSynth.app/Contents/MacOS/OrchestraSynth' 2>/dev/null | grep -q 'x86_64'"

# Notarization checks (optional but recommended)
check_criterion "App is notarized (Gatekeeper approved)" \
    "spctl -a -vv -t install '$BUILD_DIR/OrchestraSynth.app' 2>&1 | grep -q 'accepted'" || true

check_criterion "DMG is notarized (Gatekeeper approved)" \
    "[ -f '$BUILD_DIR/OrchestraSynth-0.1.0-macOS-universal.dmg' ] && spctl -a -vv -t install '$BUILD_DIR/OrchestraSynth-0.1.0-macOS-universal.dmg' 2>&1 | grep -q 'accepted'" || true

# Package checks
check_criterion "DMG installer exists" \
    "[ -f '$BUILD_DIR/OrchestraSynth-0.1.0-macOS-universal.dmg' ]"

check_criterion "PKG installer exists" \
    "[ -f '$BUILD_DIR/OrchestraSynth-0.1.0-macOS-universal.pkg' ]"

# Documentation checks
check_criterion "README.md exists" \
    "[ -f 'README.md' ]"

check_criterion "Production packaging guide exists" \
    "[ -f 'docs/PRODUCTION_PACKAGING.md' ]"

check_criterion "Operations manual exists" \
    "[ -f 'docs/OPERATIONS_MANUAL.md' ]"

# Configuration checks
check_criterion "Build configuration template exists" \
    "[ -f 'config/build.env.example' ]"

check_criterion "Entitlements configured" \
    "[ -f 'config/entitlements/app.entitlements' ] && [ -f 'config/entitlements/plugin.entitlements' ]"

echo ""
echo -e "${BLUE}Production Readiness Score: ${BOLD}$READINESS_SCORE/$READINESS_TOTAL${NC}"
echo ""

READINESS_PERCENTAGE=$((READINESS_SCORE * 100 / READINESS_TOTAL))

if [ $READINESS_PERCENTAGE -eq 100 ]; then
    echo -e "${GREEN}✓ 100% production ready!${NC}"
    echo "Production Readiness: 100% - READY FOR RELEASE" >> "$REPORT_FILE"
elif [ $READINESS_PERCENTAGE -ge 80 ]; then
    echo -e "${YELLOW}⚠ $READINESS_PERCENTAGE% production ready (review required)${NC}"
    echo "Production Readiness: $READINESS_PERCENTAGE% - REVIEW REQUIRED" >> "$REPORT_FILE"
else
    echo -e "${RED}✗ $READINESS_PERCENTAGE% production ready (not ready for release)${NC}"
    echo "Production Readiness: $READINESS_PERCENTAGE% - NOT READY" >> "$REPORT_FILE"
fi

# ============================================================================
# Security & Compliance Audit
# ============================================================================

print_section "Security & Compliance Audit"

echo -e "${BLUE}Running security checks...${NC}"
echo ""

SECURITY_ISSUES=0

# Check for common security issues
if [ -f "$BUILD_DIR/OrchestraSynth.app/Contents/MacOS/OrchestraSynth" ]; then
    # Check for insecure RPATHs
    if otool -l "$BUILD_DIR/OrchestraSynth.app/Contents/MacOS/OrchestraSynth" 2>/dev/null | grep -i "rpath" | grep -v "@loader_path" | grep -v "@executable_path" >/dev/null; then
        echo -e "${YELLOW}⚠${NC}  Potentially insecure RPATH detected"
        echo "WARNING: Insecure RPATH detected" >> "$REPORT_FILE"
        SECURITY_ISSUES=$((SECURITY_ISSUES + 1))
    else
        echo -e "${GREEN}✓${NC}  RPATH configuration is secure"
    fi

    # Check for writable segments
    if otool -l "$BUILD_DIR/OrchestraSynth.app/Contents/MacOS/OrchestraSynth" 2>/dev/null | grep -A 5 "__TEXT" | grep -q "maxprot rwx"; then
        echo -e "${YELLOW}⚠${NC}  Writable code segments detected (security risk)"
        echo "WARNING: Writable code segments detected" >> "$REPORT_FILE"
        SECURITY_ISSUES=$((SECURITY_ISSUES + 1))
    else
        echo -e "${GREEN}✓${NC}  Code segments are properly protected"
    fi

    # Check for stack protection
    if otool -I "$BUILD_DIR/OrchestraSynth.app/Contents/MacOS/OrchestraSynth" 2>/dev/null | grep -q "___stack_chk"; then
        echo -e "${GREEN}✓${NC}  Stack protection enabled"
    else
        echo -e "${YELLOW}⚠${NC}  Stack protection not detected"
        echo "WARNING: Stack protection not detected" >> "$REPORT_FILE"
        SECURITY_ISSUES=$((SECURITY_ISSUES + 1))
    fi

    # Check PIE (Position Independent Executable)
    if otool -hv "$BUILD_DIR/OrchestraSynth.app/Contents/MacOS/OrchestraSynth" 2>/dev/null | grep -q "PIE"; then
        echo -e "${GREEN}✓${NC}  Position Independent Executable (PIE) enabled"
    else
        echo -e "${YELLOW}⚠${NC}  PIE not enabled (recommended for security)"
        echo "WARNING: PIE not enabled" >> "$REPORT_FILE"
        SECURITY_ISSUES=$((SECURITY_ISSUES + 1))
    fi
fi

echo ""
if [ $SECURITY_ISSUES -eq 0 ]; then
    echo -e "${GREEN}✓ No security issues detected${NC}"
    echo "Security Audit: PASSED" >> "$REPORT_FILE"
else
    echo -e "${YELLOW}⚠ $SECURITY_ISSUES security issue(s) detected (review recommended)${NC}"
    echo "Security Audit: $SECURITY_ISSUES issues found" >> "$REPORT_FILE"
fi

# ============================================================================
# Release Artifact Inventory
# ============================================================================

print_section "Release Artifact Inventory"

echo -e "${BLUE}Cataloging release artifacts...${NC}"
echo ""

echo "Release Artifacts:" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

list_artifact() {
    local path="$1"
    local name="$2"

    if [ -e "$path" ]; then
        local size=$(du -h "$path" | cut -f1)
        local checksum=$(shasum -a 256 "$path" | cut -d' ' -f1)
        echo -e "${GREEN}✓${NC} $name"
        echo "    Size: $size"
        echo "    SHA256: $checksum"
        echo ""

        echo "$name:" >> "$REPORT_FILE"
        echo "  Path: $path" >> "$REPORT_FILE"
        echo "  Size: $size" >> "$REPORT_FILE"
        echo "  SHA256: $checksum" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
    else
        echo -e "${YELLOW}⚠${NC}  $name (not found)"
        echo ""
        echo "$name: NOT FOUND" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
    fi
}

list_artifact "$BUILD_DIR/OrchestraSynth.app" "Standalone Application"
list_artifact "$BUILD_DIR/Plug-Ins/VST3/OrchestraSynth.vst3" "VST3 Plugin"
list_artifact "$BUILD_DIR/Plug-Ins/AU/OrchestraSynth.component" "Audio Unit Plugin"
list_artifact "$BUILD_DIR/OrchestraSynth-0.1.0-macOS-universal.dmg" "DMG Installer"
list_artifact "$BUILD_DIR/OrchestraSynth-0.1.0-macOS-universal.pkg" "PKG Installer"

# ============================================================================
# Final Summary
# ============================================================================

print_section "Validation Summary"

echo "" >> "$REPORT_FILE"
echo "═══════════════════════════════════════════════════════════════" >> "$REPORT_FILE"
echo "FINAL SUMMARY" >> "$REPORT_FILE"
echo "═══════════════════════════════════════════════════════════════" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

echo -e "${BOLD}${CYAN}Validation Results:${NC}"
echo ""
echo -e "  Total Checks:    ${BOLD}$TOTAL_CHECKS${NC}"
echo -e "  Passed:          ${GREEN}$PASSED_CHECKS${NC}"
echo -e "  Warnings:        ${YELLOW}$WARNING_CHECKS${NC}"
echo -e "  Failed:          ${RED}$FAILED_CHECKS${NC}"
echo ""

echo "Total Checks: $TOTAL_CHECKS" >> "$REPORT_FILE"
echo "Passed: $PASSED_CHECKS" >> "$REPORT_FILE"
echo "Warnings: $WARNING_CHECKS" >> "$REPORT_FILE"
echo "Failed: $FAILED_CHECKS" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

# Determine final status
if [ $FAILED_CHECKS -eq 0 ] && [ $WARNING_CHECKS -eq 0 ]; then
    echo -e "${BOLD}${GREEN}╔═══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${GREEN}║                                                               ║${NC}"
    echo -e "${BOLD}${GREEN}║                  ✓ READY FOR RELEASE ✓                       ║${NC}"
    echo -e "${BOLD}${GREEN}║                                                               ║${NC}"
    echo -e "${BOLD}${GREEN}╚═══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "All validation checks passed. Build is ready for production deployment."
    echo ""
    echo "FINAL STATUS: READY FOR RELEASE" >> "$REPORT_FILE"
    EXIT_CODE=0

elif [ $FAILED_CHECKS -eq 0 ]; then
    echo -e "${BOLD}${YELLOW}╔═══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${YELLOW}║                                                               ║${NC}"
    echo -e "${BOLD}${YELLOW}║              ⚠ REVIEW REQUIRED BEFORE RELEASE ⚠              ║${NC}"
    echo -e "${BOLD}${YELLOW}║                                                               ║${NC}"
    echo -e "${BOLD}${YELLOW}╚═══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "Validation completed with $WARNING_CHECKS warning(s)."
    echo "Review the warnings and validation report before release."
    echo ""
    echo "FINAL STATUS: REVIEW REQUIRED" >> "$REPORT_FILE"
    EXIT_CODE=2

else
    echo -e "${BOLD}${RED}╔═══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${RED}║                                                               ║${NC}"
    echo -e "${BOLD}${RED}║                 ✗ NOT READY FOR RELEASE ✗                    ║${NC}"
    echo -e "${BOLD}${RED}║                                                               ║${NC}"
    echo -e "${BOLD}${RED}╚═══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "Validation failed with $FAILED_CHECKS critical error(s)."
    echo "Address all errors before attempting release."
    echo ""
    echo "FINAL STATUS: NOT READY FOR RELEASE" >> "$REPORT_FILE"
    EXIT_CODE=1
fi

echo -e "${BLUE}Validation Report:${NC} $REPORT_FILE"
echo ""

# Add recommendations
echo "" >> "$REPORT_FILE"
echo "═══════════════════════════════════════════════════════════════" >> "$REPORT_FILE"
echo "RECOMMENDATIONS" >> "$REPORT_FILE"
echo "═══════════════════════════════════════════════════════════════" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

if [ $EXIT_CODE -eq 0 ]; then
    cat >> "$REPORT_FILE" << EOF
✓ All validation checks passed
✓ Build is production-ready

Next Steps:
1. Archive this validation report with the release
2. Upload artifacts to distribution platform
3. Update release notes with checksums
4. Monitor initial deployment for issues
EOF

elif [ $EXIT_CODE -eq 2 ]; then
    cat >> "$REPORT_FILE" << EOF
⚠ Review all warnings in this report
⚠ Verify non-critical issues are acceptable

Recommended Actions:
1. Review each warning and assess impact
2. Document any accepted warnings in release notes
3. Consider addressing warnings before release
4. Re-run validation after fixes
EOF

else
    cat >> "$REPORT_FILE" << EOF
✗ Critical issues must be resolved before release

Required Actions:
1. Review all FAILED checks in this report
2. Fix critical issues identified
3. Rebuild the project
4. Re-run complete validation suite
5. Do NOT release until all critical issues are resolved
EOF
fi

echo "" >> "$REPORT_FILE"
echo "Report generated: $TIMESTAMP" >> "$REPORT_FILE"
echo "═══════════════════════════════════════════════════════════════" >> "$REPORT_FILE"

exit $EXIT_CODE

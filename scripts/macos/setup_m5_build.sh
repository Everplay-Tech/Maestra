#!/usr/bin/env bash
set -euo pipefail

# ============================================================================
# OrchestraSynth M5 Chip Build Setup Script
# ============================================================================
# Comprehensive setup and build script for Apple M5 chip MacBooks
# Handles JUCE installation, dependency management, and build configuration
#
# Usage:
#   ./scripts/macos/setup_m5_build.sh [OPTIONS]
#
# Options:
#   --upgrade-juce           Upgrade to JUCE 8.x for better M5 support
#   --offline JUCE_PATH      Use local JUCE installation
#   --clean                  Clean all build artifacts and caches
#   --diagnose               Run diagnostics only, don't build
#   --force                  Force rebuild even if checks pass
#   --skip-build             Setup only, don't build
#   --debug                  Enable debug output
#   --help                   Show this help message
#
# Environment Variables:
#   JUCE_VERSION            - Override JUCE version (default: 8.0.4)
#   CMAKE_GENERATOR         - Override generator (default: auto-detect)
#   BUILD_JOBS              - Number of parallel build jobs (default: auto)
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
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/macos-universal-release"
LOG_FILE="${ROOT_DIR}/m5_build_setup.log"
UPGRADE_JUCE=0
OFFLINE_MODE=0
OFFLINE_JUCE_PATH=""
CLEAN_BUILD=0
DIAGNOSE_ONLY=0
FORCE_BUILD=0
SKIP_BUILD=0
DEBUG_MODE=0
JUCE_VERSION="${JUCE_VERSION:-8.0.4}"  # M5-compatible version
MIN_CMAKE_VERSION="3.22.0"
MIN_XCODE_VERSION="15.0"
MIN_MACOS_VERSION="14.0"

# Error trap for debugging
trap 'echo -e "${RED}✗${NC} Script failed at line $LINENO with exit code $?" | tee -a "$LOG_FILE"; exit 1' ERR

# ============================================================================
# Utility Functions
# ============================================================================

log() {
    echo -e "$@" | tee -a "$LOG_FILE"
}

log_header() {
    log ""
    log "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
    log "${CYAN}║$(printf '%63s' | tr ' ' ' ')║${NC}"
    log "${CYAN}║  $(printf '%-60s' "$1")║${NC}"
    log "${CYAN}║$(printf '%63s' | tr ' ' ' ')║${NC}"
    log "${CYAN}╚═══════════════════════════════════════════════════════════════╝${NC}"
    log ""
}

log_section() {
    log ""
    log "${BLUE}=== $1 ===${NC}"
    log ""
}

log_success() {
    log "${GREEN}✓${NC} $1"
}

log_warning() {
    log "${YELLOW}⚠${NC}  $1"
}

log_error() {
    log "${RED}✗${NC} $1"
}

log_info() {
    log "${BLUE}ℹ${NC}  $1"
}

log_debug() {
    if [[ $DEBUG_MODE -eq 1 ]]; then
        log "${MAGENTA}[DEBUG]${NC} $1"
    fi
}

version_compare() {
    # Compare version strings - returns -1 if $1 < $2, 0 if equal, 1 if $1 > $2
    # This function is safe with set -e

    # Handle empty or invalid inputs
    if [[ -z "$1" || -z "$2" ]]; then
        echo "0"
        return 0
    fi

    if [[ "$1" == "$2" ]]; then
        echo "0"
        return 0
    fi

    # Try sort -V first (GNU sort) - use explicit error handling
    local sorted=""
    sorted=$(printf '%s\n' "$1" "$2" | sort -V 2>/dev/null | head -n1) || sorted=""

    if [[ -n "$sorted" ]]; then
        if [[ "$sorted" == "$1" ]]; then
            echo "-1"
        else
            echo "1"
        fi
        return 0
    fi

    # Fallback to manual version comparison for BSD sort
    # Convert version strings to arrays
    local IFS='.'
    local ver1=($1)
    local ver2=($2)
    local len=${#ver1[@]}

    if [[ ${#ver2[@]} -gt $len ]]; then
        len=${#ver2[@]}
    fi

    for ((i=0; i<len; i++)); do
        local num1=${ver1[i]:-0}
        local num2=${ver2[i]:-0}

        # Remove non-numeric characters for comparison
        num1=${num1//[^0-9]/}
        num2=${num2//[^0-9]/}

        # Default to 0 if empty after cleaning
        num1=${num1:-0}
        num2=${num2:-0}

        if [[ $num1 -lt $num2 ]]; then
            echo "-1"
            return 0
        elif [[ $num1 -gt $num2 ]]; then
            echo "1"
            return 0
        fi
    done

    echo "0"
    return 0
}

# ============================================================================
# Parse Arguments
# ============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        --upgrade-juce)
            UPGRADE_JUCE=1
            shift
            ;;
        --offline)
            OFFLINE_MODE=1
            OFFLINE_JUCE_PATH="$2"
            shift 2
            ;;
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        --diagnose)
            DIAGNOSE_ONLY=1
            shift
            ;;
        --force)
            FORCE_BUILD=1
            shift
            ;;
        --skip-build)
            SKIP_BUILD=1
            shift
            ;;
        --debug)
            DEBUG_MODE=1
            set -x  # Enable bash debug mode
            shift
            ;;
        --help)
            grep "^#" "$0" | grep -v "^#!/" | sed 's/^# \?//'
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Initialize log file
echo "OrchestraSynth M5 Build Setup - $(date)" > "$LOG_FILE"
echo "========================================" >> "$LOG_FILE"

# ============================================================================
# Print Banner
# ============================================================================

log_header "OrchestraSynth M5 Chip Build Setup"

log_info "Log file: ${LOG_FILE}"
log ""

# ============================================================================
# System Detection
# ============================================================================

log_section "System Detection"

# Detect macOS version
MACOS_VERSION=$(sw_vers -productVersion)
log_info "macOS Version: ${MACOS_VERSION}"

# Detect chip architecture
CHIP_ARCH=$(uname -m)
CHIP_BRAND=$(sysctl -n machdep.cpu.brand_string 2>/dev/null || echo "Unknown")
log_info "Architecture: ${CHIP_ARCH}"
log_info "CPU: ${CHIP_BRAND}"

# Detect if M5 chip
IS_M5=0
if echo "$CHIP_BRAND" | grep -qi "Apple M5"; then
    IS_M5=1
    log_success "Apple M5 chip detected"
elif echo "$CHIP_BRAND" | grep -qi "Apple M[0-9]"; then
    log_info "Apple Silicon detected ($(echo "$CHIP_BRAND" | grep -o "Apple M[0-9]"))"
else
    log_warning "Non-Apple Silicon chip detected"
fi

# Check if running under Rosetta
if [[ "$CHIP_ARCH" == "x86_64" ]] && sysctl -n sysctl.proc_translated 2>/dev/null | grep -q "1"; then
    log_warning "Running under Rosetta 2 emulation"
    log_info "Consider using native arm64 terminal for better performance"
fi

# ============================================================================
# Prerequisite Checks
# ============================================================================

log_section "Prerequisite Validation"

ERRORS=0
WARNINGS=0

log_debug "Starting prerequisite checks..."

# Check Xcode Command Line Tools
log_debug "Checking for xcodebuild..."
if ! command -v xcodebuild &> /dev/null; then
    log_error "Xcode Command Line Tools not found"
    log_info "Install with: xcode-select --install"
    ((ERRORS++))
else
    log_debug "xcodebuild found, getting version..."
    # Get Xcode version with better error handling
    if XCODE_VERSION=$(xcodebuild -version 2>&1 | head -1 | awk '{print $2}'); then
        log_debug "Xcode version: ${XCODE_VERSION}"

        # Validate we got a version number
        if [[ -n "$XCODE_VERSION" && "$XCODE_VERSION" =~ ^[0-9]+\.[0-9]+ ]]; then
            log_success "Xcode Command Line Tools: ${XCODE_VERSION}"

            log_debug "Comparing Xcode version ${XCODE_VERSION} with minimum ${MIN_XCODE_VERSION}..."
            XCODE_CMP=$(version_compare "$XCODE_VERSION" "$MIN_XCODE_VERSION") || XCODE_CMP="-1"
            log_debug "Version comparison result: ${XCODE_CMP}"

            if [[ $XCODE_CMP -lt 0 ]]; then
                log_warning "Xcode ${XCODE_VERSION} is older than recommended ${MIN_XCODE_VERSION}"
                log_info "Update Xcode for best M5 chip support"
                ((WARNINGS++))
            fi
        else
            log_warning "Could not parse Xcode version: '${XCODE_VERSION}'"
            log_success "Xcode Command Line Tools: present (version unknown)"
            ((WARNINGS++))
        fi
    else
        log_warning "xcodebuild command failed - Command Line Tools may need reinstalling"
        log_info "Try: sudo xcode-select --reset"
        ((WARNINGS++))
    fi
fi

# Check CMake
log_debug "Checking for cmake..."
if ! command -v cmake &> /dev/null; then
    log_error "CMake not found"
    log_info "Install with: brew install cmake"
    ((ERRORS++))
else
    log_debug "cmake found, getting version..."
    # Get CMake version with better error handling
    if CMAKE_VERSION=$(cmake --version 2>&1 | head -1 | awk '{print $3}'); then
        log_debug "CMake version: ${CMAKE_VERSION}"

        # Validate we got a version number
        if [[ -n "$CMAKE_VERSION" && "$CMAKE_VERSION" =~ ^[0-9]+\.[0-9]+ ]]; then
            log_success "CMake: ${CMAKE_VERSION}"

            log_debug "Comparing CMake version ${CMAKE_VERSION} with minimum ${MIN_CMAKE_VERSION}..."
            CMAKE_CMP=$(version_compare "$CMAKE_VERSION" "$MIN_CMAKE_VERSION") || CMAKE_CMP="-1"
            log_debug "Version comparison result: ${CMAKE_CMP}"

            if [[ $CMAKE_CMP -lt 0 ]]; then
                log_error "CMake ${CMAKE_VERSION} is older than required ${MIN_CMAKE_VERSION}"
                log_info "Update with: brew upgrade cmake"
                ((ERRORS++))
            fi
        else
            log_warning "Could not parse CMake version: '${CMAKE_VERSION}'"
            log_success "CMake: present (version unknown)"
            ((WARNINGS++))
        fi
    else
        log_error "cmake command failed"
        ((ERRORS++))
    fi
fi

# Check Ninja (optional but recommended)
if command -v ninja &> /dev/null; then
    if NINJA_VERSION=$(ninja --version 2>&1); then
        log_success "Ninja: ${NINJA_VERSION} (recommended)"
    else
        log_warning "Ninja found but version check failed"
        ((WARNINGS++))
    fi
else
    log_warning "Ninja not found (optional but recommended for faster builds)"
    log_info "Install with: brew install ninja"
    ((WARNINGS++))
fi

# Check Git
if ! command -v git &> /dev/null; then
    log_error "Git not found"
    log_info "Install Xcode Command Line Tools: xcode-select --install"
    ((ERRORS++))
else
    if GIT_VERSION=$(git --version 2>&1 | awk '{print $3}'); then
        log_success "Git: ${GIT_VERSION}"
    else
        log_warning "Git found but version check failed"
        ((WARNINGS++))
    fi
fi

# Check available disk space
if AVAILABLE_SPACE=$(df -h "$ROOT_DIR" 2>&1 | awk 'NR==2 {print $4}'); then
    log_info "Available disk space: ${AVAILABLE_SPACE}"
else
    log_warning "Could not check disk space"
fi

# Check for Homebrew (helpful but not required)
if command -v brew &> /dev/null; then
    if BREW_VERSION=$(brew --version 2>&1 | head -1 | awk '{print $2}'); then
        log_success "Homebrew: ${BREW_VERSION}"
    else
        log_warning "Homebrew found but version check failed"
        ((WARNINGS++))
    fi
else
    log_warning "Homebrew not found (recommended for easy dependency management)"
    log_info "Install from: https://brew.sh"
    ((WARNINGS++))
fi

log ""
log_info "Prerequisite Summary:"
log "  Errors: ${ERRORS}"
log "  Warnings: ${WARNINGS}"

if [[ $ERRORS -gt 0 ]]; then
    log ""
    log_error "Cannot proceed due to ${ERRORS} error(s)"
    log_info "Please fix the errors above and run again"
    exit 1
fi

if [[ $DIAGNOSE_ONLY -eq 1 ]]; then
    log ""
    log_success "Diagnostics complete"
    exit 0
fi

# ============================================================================
# Clean Build (if requested)
# ============================================================================

if [[ $CLEAN_BUILD -eq 1 ]]; then
    log_section "Cleaning Build Directory"

    if [[ -d "$BUILD_DIR" ]]; then
        log_info "Removing: ${BUILD_DIR}"
        rm -rf "$BUILD_DIR"
        log_success "Build directory cleaned"
    else
        log_info "Build directory doesn't exist, nothing to clean"
    fi

    # Clean CMake cache
    if [[ -f "${ROOT_DIR}/CMakeCache.txt" ]]; then
        log_info "Removing CMake cache"
        rm -f "${ROOT_DIR}/CMakeCache.txt"
    fi

    # Clean FetchContent cache for JUCE
    FETCHCONTENT_BASE="${ROOT_DIR}/build/_deps"
    if [[ -d "$FETCHCONTENT_BASE" ]]; then
        log_info "Cleaning FetchContent cache"
        rm -rf "$FETCHCONTENT_BASE"
        log_success "FetchContent cache cleaned"
    fi
fi

# ============================================================================
# JUCE Configuration
# ============================================================================

log_section "JUCE Configuration"

JUCE_MODIFIED=0

if [[ $UPGRADE_JUCE -eq 1 ]]; then
    log_info "Upgrading to JUCE ${JUCE_VERSION} for enhanced M5 support"

    # Backup current CMakeLists.txt
    cp "${ROOT_DIR}/CMakeLists.txt" "${ROOT_DIR}/CMakeLists.txt.backup"
    log_success "Created backup: CMakeLists.txt.backup"

    # Update JUCE version in CMakeLists.txt
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sed -i '' "s/GIT_TAG        [0-9.]\+/GIT_TAG        ${JUCE_VERSION}/" "${ROOT_DIR}/CMakeLists.txt"
    else
        sed -i "s/GIT_TAG        [0-9.]\+/GIT_TAG        ${JUCE_VERSION}/" "${ROOT_DIR}/CMakeLists.txt"
    fi

    log_success "Updated JUCE version to ${JUCE_VERSION}"
    JUCE_MODIFIED=1

elif [[ $OFFLINE_MODE -eq 1 ]]; then
    log_info "Using offline JUCE installation: ${OFFLINE_JUCE_PATH}"

    if [[ ! -d "$OFFLINE_JUCE_PATH" ]]; then
        log_error "JUCE path does not exist: ${OFFLINE_JUCE_PATH}"
        exit 1
    fi

    # Backup current CMakeLists.txt
    cp "${ROOT_DIR}/CMakeLists.txt" "${ROOT_DIR}/CMakeLists.txt.backup"
    log_success "Created backup: CMakeLists.txt.backup"

    # Replace FetchContent with local path
    cat > "${ROOT_DIR}/CMakeLists.juce.local" << EOF
# Use local JUCE installation
set(JUCE_DIR "${OFFLINE_JUCE_PATH}")
add_subdirectory(\${JUCE_DIR} juce)
EOF

    # Update CMakeLists.txt to use local JUCE
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sed -i '' '/FetchContent_Declare/,/FetchContent_MakeAvailable/c\
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.juce.local)
' "${ROOT_DIR}/CMakeLists.txt"
    else
        sed -i '/FetchContent_Declare/,/FetchContent_MakeAvailable/c\include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.juce.local)' "${ROOT_DIR}/CMakeLists.txt"
    fi

    log_success "Configured to use local JUCE installation"
    JUCE_MODIFIED=1

else
    log_info "Using default JUCE configuration from CMakeLists.txt"
fi

# ============================================================================
# M5-Specific Optimizations
# ============================================================================

log_section "M5-Specific Build Optimizations"

# Create M5 optimization flags file
M5_FLAGS_FILE="${ROOT_DIR}/cmake/M5Optimizations.cmake"
mkdir -p "$(dirname "$M5_FLAGS_FILE")"

cat > "$M5_FLAGS_FILE" << 'EOF'
# M5 Chip Optimizations for OrchestraSynth
# Automatically included during build configuration

if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    message(STATUS "Applying M5 Apple Silicon optimizations")

    # Target M5 microarchitecture
    set(M5_COMPILE_FLAGS
        -mcpu=apple-m5          # Target M5 specifically
        -mtune=apple-m5         # Optimize for M5
        -march=armv9-a          # ARMv9 instruction set
    )

    # Check if compiler supports M5 flags
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-mcpu=apple-m5" COMPILER_SUPPORTS_M5)

    if(COMPILER_SUPPORTS_M5)
        message(STATUS "  [OK] M5-specific optimizations enabled")
        foreach(target OrchestraSynth OrchestraSynthPlugin)
            if(TARGET ${target})
                target_compile_options(${target} PRIVATE ${M5_COMPILE_FLAGS})
            endif()
        endforeach()
    else()
        message(STATUS "  [WARN] Compiler doesn't support M5-specific flags, using general ARM64 optimizations")
        foreach(target OrchestraSynth OrchestraSynthPlugin)
            if(TARGET ${target})
                target_compile_options(${target} PRIVATE -mcpu=apple-latest)
            endif()
        endforeach()
    endif()

    # Enable SVE/NEON optimizations
    foreach(target OrchestraSynth OrchestraSynthPlugin)
        if(TARGET ${target})
            target_compile_definitions(${target} PRIVATE
                JUCE_USE_ARM_NEON=1
                JUCE_ARM_NEON_OPTIMIZATIONS=1
            )
        endif()
    endforeach()

    # Link-time optimizations
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

    message(STATUS "  [OK] NEON optimizations enabled")
    message(STATUS "  [OK] Link-time optimization enabled")
endif()
EOF

log_success "Created M5 optimization configuration"

# ============================================================================
# Environment Setup
# ============================================================================

log_section "Environment Configuration"

# Determine optimal build parallelism
if [[ -z "${BUILD_JOBS:-}" ]]; then
    BUILD_JOBS=$(sysctl -n hw.ncpu)
    log_info "Auto-detected ${BUILD_JOBS} CPU cores"
else
    log_info "Using ${BUILD_JOBS} build jobs (from BUILD_JOBS env)"
fi

# Select generator
if [[ -z "${CMAKE_GENERATOR:-}" ]]; then
    if command -v ninja &> /dev/null; then
        CMAKE_GENERATOR="Ninja"
    else
        CMAKE_GENERATOR="Unix Makefiles"
    fi
fi
log_info "CMake Generator: ${CMAKE_GENERATOR}"

# Set M5-specific environment variables
export MACOSX_DEPLOYMENT_TARGET="${MIN_MACOS_VERSION}"
export CMAKE_OSX_ARCHITECTURES="arm64;x86_64"
export CMAKE_BUILD_TYPE="Release"

log_success "Environment configured for M5 build"

# ============================================================================
# CMake Configuration
# ============================================================================

if [[ $SKIP_BUILD -eq 0 ]]; then
    log_section "CMake Configuration"

    mkdir -p "$BUILD_DIR"

    CMAKE_ARGS=(
        -S "${ROOT_DIR}"
        -B "${BUILD_DIR}"
        -G "${CMAKE_GENERATOR}"
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
        -DCMAKE_OSX_DEPLOYMENT_TARGET="${MIN_MACOS_VERSION}"
    )

    # Add M5 optimizations if available
    if [[ -f "$M5_FLAGS_FILE" ]]; then
        CMAKE_ARGS+=(-DCMAKE_PROJECT_INCLUDE="${M5_FLAGS_FILE}")
        log_info "Including M5 optimizations"
    fi

    log_info "Running CMake configuration..."
    log ""

    if cmake "${CMAKE_ARGS[@]}" 2>&1 | tee -a "$LOG_FILE"; then
        log ""
        log_success "CMake configuration successful"
    else
        log ""
        log_error "CMake configuration failed"
        log_info "Check the log file for details: ${LOG_FILE}"

        # Common error diagnostics
        log ""
        log_section "Troubleshooting Suggestions"
        log_info "1. Check network connection (JUCE download may have failed)"
        log_info "2. Try with --clean flag to remove cached files"
        log_info "3. Try with --upgrade-juce flag for newer JUCE version"
        log_info "4. Check log file for specific errors: ${LOG_FILE}"

        exit 1
    fi

    # ============================================================================
    # Build
    # ============================================================================

    log_section "Building OrchestraSynth"

    log_info "Building with ${BUILD_JOBS} parallel jobs..."
    log ""

    if cmake --build "$BUILD_DIR" --config Release --parallel "$BUILD_JOBS" 2>&1 | tee -a "$LOG_FILE"; then
        log ""
        log_success "Build successful!"
    else
        log ""
        log_error "Build failed"
        log_info "Check the log file for details: ${LOG_FILE}"

        # Offer retry with single job (for debugging)
        log ""
        log_warning "Parallel builds can sometimes have race conditions"
        log_info "Retry with single job? This will be slower but more reliable."
        read -p "Retry with -j1? (y/N): " -n 1 -r
        log ""

        if [[ $REPLY =~ ^[Yy]$ ]]; then
            log_info "Retrying with single job..."
            if cmake --build "$BUILD_DIR" --config Release --parallel 1 2>&1 | tee -a "$LOG_FILE"; then
                log ""
                log_success "Build successful on retry!"
            else
                log ""
                log_error "Build failed on retry"
                exit 1
            fi
        else
            exit 1
        fi
    fi

    # ============================================================================
    # Create Package
    # ============================================================================

    log_section "Creating DMG Package"

    if cmake --build "$BUILD_DIR" --config Release --target package 2>&1 | tee -a "$LOG_FILE"; then
        log ""
        log_success "Package created successfully"
    else
        log ""
        log_warning "Package creation failed (not critical)"
    fi
fi

# ============================================================================
# Validation
# ============================================================================

if [[ $SKIP_BUILD -eq 0 ]]; then
    log_section "Build Validation"

    # Check if app was built
    APP_PATH="${BUILD_DIR}/OrchestraSynth_artefacts/Release/OrchestraSynth.app"
    if [[ ! -d "$APP_PATH" ]]; then
        APP_PATH="${BUILD_DIR}/OrchestraSynth.app"
    fi

    if [[ -d "$APP_PATH" ]]; then
        log_success "Standalone app built: OrchestraSynth.app"

        # Check architectures
        APP_BINARY="${APP_PATH}/Contents/MacOS/OrchestraSynth"
        if [[ -f "$APP_BINARY" ]]; then
            ARCHS=$(lipo -archs "$APP_BINARY" 2>/dev/null || echo "unknown")
            log_info "Binary architectures: ${ARCHS}"

            if echo "$ARCHS" | grep -q "arm64"; then
                log_success "ARM64 (Apple Silicon) architecture present"
            else
                log_warning "ARM64 architecture missing"
            fi

            if echo "$ARCHS" | grep -q "x86_64"; then
                log_success "x86_64 (Intel) architecture present"
            else
                log_warning "x86_64 architecture missing"
            fi
        fi
    else
        log_warning "Standalone app not found at expected location"
    fi

    # Check plugins
    VST3_PATH="${BUILD_DIR}/OrchestraSynthPlugin_artefacts/Release/VST3/OrchestraSynth.vst3"
    if [[ ! -d "$VST3_PATH" ]]; then
        VST3_PATH="${BUILD_DIR}/VST3/OrchestraSynth.vst3"
    fi

    if [[ -d "$VST3_PATH" ]]; then
        log_success "VST3 plugin built: OrchestraSynth.vst3"
    else
        log_warning "VST3 plugin not found"
    fi

    AU_PATH="${BUILD_DIR}/OrchestraSynthPlugin_artefacts/Release/AU/OrchestraSynth.component"
    if [[ ! -d "$AU_PATH" ]]; then
        AU_PATH="${BUILD_DIR}/AU/OrchestraSynth.component"
    fi

    if [[ -d "$AU_PATH" ]]; then
        log_success "AU plugin built: OrchestraSynth.component"
    else
        log_warning "AU plugin not found"
    fi
fi

# ============================================================================
# Summary
# ============================================================================

log_header "Build Setup Complete"

if [[ $SKIP_BUILD -eq 0 ]]; then
    log_success "OrchestraSynth built successfully for M5 chip"
    log ""
    log_info "Build artifacts located in:"
    log "  ${BUILD_DIR}"
    log ""
    log_info "Next steps:"
    log "  1. Test the standalone app: open '${APP_PATH}'"
    log "  2. Run validation: ./scripts/macos/validate.sh"
    log "  3. For production: ./scripts/macos/production_build.sh"
else
    log_success "Setup complete, build skipped (--skip-build)"
fi

log ""
log_info "Full build log saved to: ${LOG_FILE}"

# Restore CMakeLists.txt if modified and there were errors
if [[ $JUCE_MODIFIED -eq 1 && -f "${ROOT_DIR}/CMakeLists.txt.backup" ]]; then
    log ""
    log_info "CMakeLists.txt was modified during this run"
    log_info "Backup available at: CMakeLists.txt.backup"
    log_info "To restore: mv CMakeLists.txt.backup CMakeLists.txt"
fi

log ""
log_success "All done!"
log ""

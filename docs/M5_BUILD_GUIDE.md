# M5 Chip Build Guide

## Overview

This guide provides comprehensive instructions for building OrchestraSynth on Apple M5 chip MacBooks. The M5 chip is Apple's latest generation Apple Silicon processor, and while it's fully compatible with existing ARM64 software, optimal performance requires specific build configurations.

## Common Issues with M5 Chips

The standard JUCE 7.0.12 configuration may fail on M5 chips due to:

1. **Newer SDK requirements** - M5 chips require macOS 15+ SDKs
2. **Compiler flag compatibility** - Older compiler flags may not recognize M5
3. **CMake detection issues** - CMake may not properly detect M5 capabilities
4. **JUCE version limitations** - JUCE 7.x has limited M5-specific optimizations
5. **Network/Git issues** - FetchContent download failures

## Quick Start

### Option 1: Automatic Setup (Recommended)

The `setup_m5_build.sh` script handles everything automatically:

```bash
# Clean build with JUCE upgrade (recommended for M5)
./scripts/macos/setup_m5_build.sh --upgrade-juce --clean

# Quick build with diagnostics
./scripts/macos/setup_m5_build.sh

# Diagnostics only (no build)
./scripts/macos/setup_m5_build.sh --diagnose
```

### Option 2: Manual JUCE Version Update

If you prefer to update JUCE version manually:

```bash
# Edit CMakeLists.txt
# Change line 57 from:
#   GIT_TAG        7.0.12
# To:
#   GIT_TAG        8.0.4

# Then build normally
./scripts/macos/build_and_package.sh
```

### Option 3: Offline Build (No Internet)

If you have JUCE installed locally:

```bash
# Download JUCE 8.0.4 separately
git clone --branch 8.0.4 --depth 1 https://github.com/juce-framework/JUCE.git ~/JUCE

# Use offline mode
./scripts/macos/setup_m5_build.sh --offline ~/JUCE
```

## Prerequisites

### Required Software

1. **Xcode 15.0+** with Command Line Tools
   ```bash
   xcode-select --install
   # Verify installation
   xcodebuild -version
   ```

2. **CMake 3.22+**
   ```bash
   brew install cmake
   # Or download from: https://cmake.org/download/
   ```

3. **Git** (usually included with Xcode)
   ```bash
   git --version
   ```

### Recommended Software

1. **Ninja Build System** (faster builds)
   ```bash
   brew install ninja
   ```

2. **Homebrew** (package management)
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

## Setup Script Usage

### Command Line Options

```bash
./scripts/macos/setup_m5_build.sh [OPTIONS]
```

| Option | Description |
|--------|-------------|
| `--upgrade-juce` | Upgrade to JUCE 8.x for enhanced M5 support |
| `--offline PATH` | Use local JUCE installation at PATH |
| `--clean` | Clean all build artifacts and caches |
| `--diagnose` | Run diagnostics only, don't build |
| `--force` | Force rebuild even if checks pass |
| `--skip-build` | Setup only, don't build |
| `--help` | Show help message |

### Environment Variables

```bash
# Override JUCE version (default: 8.0.4)
export JUCE_VERSION="8.0.4"

# Force specific generator
export CMAKE_GENERATOR="Ninja"

# Control build parallelism
export BUILD_JOBS=8
```

## Build Process Details

### What the Script Does

1. **System Detection**
   - Detects M5 chip and validates compatibility
   - Checks macOS version and architecture
   - Detects Rosetta 2 emulation (warns if active)

2. **Prerequisite Validation**
   - Verifies Xcode installation and version
   - Checks CMake version (minimum 3.22.0)
   - Validates Git availability
   - Checks disk space and optional tools

3. **JUCE Configuration**
   - Updates to M5-compatible JUCE version (8.x)
   - Or configures offline/local JUCE installation
   - Backs up original CMakeLists.txt

4. **M5 Optimizations**
   - Creates M5-specific compiler flags
   - Enables ARM NEON optimizations
   - Configures link-time optimization (LTO)
   - Sets M5 microarchitecture targeting

5. **Build Execution**
   - Configures CMake with optimal settings
   - Builds with parallel jobs (auto-detected cores)
   - Creates DMG package
   - Validates binary architectures

6. **Validation**
   - Verifies app and plugin builds
   - Checks universal binary architectures (arm64 + x86_64)
   - Generates detailed build log

### Build Artifacts

After successful build, you'll find:

```
build/macos-universal-release/
├── OrchestraSynth.app              # Standalone application
├── OrchestraSynth.vst3             # VST3 plugin
├── OrchestraSynth.component        # AU plugin
└── OrchestraSynth-*.dmg            # DMG installer
```

## M5-Specific Optimizations

The script automatically applies these optimizations:

### Compiler Flags

```cmake
-mcpu=apple-m5          # Target M5 CPU specifically
-mtune=apple-m5         # Tune optimizations for M5
-march=armv9-a          # Use ARMv9 instruction set
```

### JUCE Definitions

```cmake
JUCE_USE_ARM_NEON=1              # Enable NEON SIMD
JUCE_ARM_NEON_OPTIMIZATIONS=1    # Use NEON optimizations
```

### Build Settings

- **Link-Time Optimization (LTO)**: Enabled for Release builds
- **Universal Binary**: arm64 (native) + x86_64 (compatibility)
- **Deployment Target**: macOS 14.0 minimum

## Troubleshooting

### Issue: "JUCE download failed"

**Symptoms:** CMake fails during FetchContent_MakeAvailable

**Solutions:**
1. Check internet connection
2. Use `--offline` mode with local JUCE
3. Clear CMake cache: `./scripts/macos/setup_m5_build.sh --clean`
4. Check firewall/proxy settings

### Issue: "Compiler doesn't support M5 flags"

**Symptoms:** Warning about M5-specific optimizations

**Solutions:**
1. Update Xcode: `softwareupdate --install -a`
2. Update Command Line Tools: `xcode-select --install`
3. Verify Xcode version: `xcodebuild -version` (should be 15.0+)

### Issue: "Running under Rosetta 2"

**Symptoms:** Warning about Rosetta emulation

**Solutions:**
1. Use native Terminal app (not third-party terminals in x86_64 mode)
2. Check terminal architecture: `arch`
3. Force arm64: `arch -arm64 zsh` then run script

### Issue: "Build fails with parallel jobs"

**Symptoms:** Random build errors, race conditions

**Solutions:**
1. Script will automatically offer single-job retry
2. Or manually: `export BUILD_JOBS=1` and rebuild
3. Update Ninja: `brew upgrade ninja`

### Issue: "Universal binary missing architecture"

**Symptoms:** `lipo -archs` shows only one architecture

**Solutions:**
1. Check CMAKE_OSX_ARCHITECTURES setting
2. Ensure Xcode supports both architectures
3. Clean rebuild: `--clean` flag

### Issue: "App won't open - damaged or incomplete"

**Symptoms:** macOS Gatekeeper blocks app

**Solutions:**
1. Code sign the app: `./scripts/macos/codesign.sh`
2. For testing: `xattr -cr /path/to/OrchestraSynth.app`
3. Full production build: `./scripts/macos/production_build.sh`

## Performance Optimization

### Best Practices for M5 Builds

1. **Use Ninja** for fastest builds
   ```bash
   brew install ninja
   # Script will auto-detect and use Ninja
   ```

2. **Maximize parallelism** (M5 has many cores)
   ```bash
   export BUILD_JOBS=$(sysctl -n hw.ncpu)
   ```

3. **Use latest Xcode** for newest optimizations
   ```bash
   softwareupdate --list
   softwareupate --install "Xcode-*"
   ```

4. **Enable JUCE 8.x** for M5-specific optimizations
   ```bash
   ./scripts/macos/setup_m5_build.sh --upgrade-juce
   ```

5. **Clean builds** for production
   ```bash
   ./scripts/macos/setup_m5_build.sh --clean --upgrade-juce
   ```

## Verification

### Verify M5 Optimizations

Check that M5 optimizations were applied:

```bash
# Check binary info
otool -l build/macos-universal-release/OrchestraSynth.app/Contents/MacOS/OrchestraSynth | grep -A5 "cmd LC_VERSION_MIN"

# Check architectures
lipo -info build/macos-universal-release/OrchestraSynth.app/Contents/MacOS/OrchestraSynth

# Check dependencies
otool -L build/macos-universal-release/OrchestraSynth.app/Contents/MacOS/OrchestraSynth
```

### Run Validation Suite

```bash
# Technical validation
./scripts/macos/validate.sh

# Functional validation
./scripts/macos/functional_validate.sh

# Full pre-release validation
./scripts/macos/pre_release_validate.sh
```

### Performance Testing

Test audio performance on M5:

```bash
# Open standalone app
open build/macos-universal-release/OrchestraSynth.app

# Monitor CPU usage
# Activity Monitor > CPU > Show OrchestraSynth

# Test with high polyphony:
# 1. Play dense MIDI chords
# 2. Check CPU usage (should be <30% on M5)
# 3. Check for audio dropouts (should be none)
```

## Advanced Configuration

### Custom JUCE Version

```bash
# Use specific JUCE version
export JUCE_VERSION="8.0.3"
./scripts/macos/setup_m5_build.sh --upgrade-juce

# Or edit CMakeLists.txt directly
# Line 57: GIT_TAG 8.0.3
```

### Custom Build Configuration

```bash
# Debug build
export CMAKE_BUILD_TYPE=Debug
./scripts/macos/setup_m5_build.sh

# Arm64 only (no universal)
# Edit CMakeLists.txt line 46:
# CMAKE_OSX_ARCHITECTURES "arm64"
```

### Integration with Production Build

```bash
# After successful M5 setup build:
source config/build.env  # Configure signing credentials
./scripts/macos/production_build.sh
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: M5 Build

on: [push, pull_request]

jobs:
  build-m5:
    runs-on: macos-14  # or macos-15 for M5
    steps:
      - uses: actions/checkout@v4

      - name: Install dependencies
        run: |
          brew install cmake ninja

      - name: Setup M5 build
        run: |
          ./scripts/macos/setup_m5_build.sh --upgrade-juce --skip-build

      - name: Build
        run: |
          cmake --build build/macos-universal-release --config Release

      - name: Validate
        run: |
          ./scripts/macos/validate.sh
```

## FAQ

### Q: Do I need to upgrade JUCE?

**A:** For best M5 support, yes. JUCE 8.x has M5-specific optimizations. However, JUCE 7.0.12 may work with the right configuration.

### Q: Will this work on Intel Macs?

**A:** Yes! The script builds universal binaries (arm64 + x86_64) that work on both Apple Silicon and Intel Macs.

### Q: How long does the build take?

**A:** On M5 with Ninja: ~5-15 minutes. First build downloads JUCE (~2-5 min), subsequent builds are faster.

### Q: Can I use this for non-M5 Apple Silicon?

**A:** Yes! Works on M1, M2, M3, M4, and M5 chips. M5-specific optimizations gracefully fall back on older chips.

### Q: What if I want JUCE 7.x?

**A:** Don't use `--upgrade-juce` flag. The script will use the version in CMakeLists.txt (7.0.12).

### Q: Can I modify the M5 optimizations?

**A:** Yes! Edit `cmake/M5Optimizations.cmake` created by the script. Customize compiler flags as needed.

## References

- [JUCE Documentation](https://juce.com/learn/documentation)
- [CMake Documentation](https://cmake.org/documentation/)
- [Apple Silicon Guide](https://developer.apple.com/documentation/apple-silicon)
- [Xcode Release Notes](https://developer.apple.com/documentation/xcode-release-notes)

## Support

For issues specific to M5 builds:

1. Check build log: `m5_build_setup.log`
2. Run diagnostics: `./scripts/macos/setup_m5_build.sh --diagnose`
3. Review [OPERATIONS_MANUAL.md](./OPERATIONS_MANUAL.md)
4. Check [GitHub Issues](https://github.com/Everplay-Tech/Maestra/issues)

## Changelog

### v1.0.0 (2025-11-21)
- Initial M5 build support
- Automatic JUCE upgrade to 8.x
- M5-specific compiler optimizations
- Comprehensive diagnostics and validation
- Offline build mode
- Universal binary support

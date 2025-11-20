# OrchestraSynth

OrchestraSynth is a macOS orchestral synthesizer featuring:

- Five orchestral sections: **Strings, Brass, Woodwinds, Percussion, Choir**
- Shared audio engine with deterministic MIDI handling and centralized DSP
- Convolution reverb and oversampling / anti-aliasing utilities
- Preset serialization via JUCE `ValueTree` / `DynamicObject`
- Enterprise-oriented logging, crash reporting, and performance monitoring
- A JUCE standalone app and a JUCE-based plugin (VST3, AU) sharing the same core

This repository is organized for modern JUCE + CMake builds on current macOS toolchains (macOS 14, AppleClang 17+).

---

## 1. Cloning and setup

```bash
git clone <your-orchestra-synth-repo> OrchestraSynth
cd OrchestraSynth
```

**Note:** JUCE 7.0.12 is automatically downloaded by CMake's FetchContent during the build process. No manual setup required.

## 2. Development Build (Quick Start)

For a development build with universal binary and DMG:

```bash
scripts/macos/build_and_package.sh
```

The script enforces `arm64;x86_64` architectures, builds both JUCE targets in **Release** mode, and invokes **CPack DragNDrop** to generate `OrchestraSynth-0.1.0-macOS-universal.dmg`.

## 3. Production Build (Enterprise-Ready)

For production-ready, code-signed, and notarized packages:

### One-Command Production Build

```bash
# 1. Configure credentials (first time only)
cp config/build.env.example config/build.env
# Edit build.env with your Apple Developer credentials

# 2. Load configuration
source config/build.env

# 3. Build everything
./scripts/macos/production_build.sh
```

This automated pipeline:
1. ✅ Builds universal binaries (Apple Silicon + Intel)
2. ✅ Code signs with hardened runtime and entitlements
3. ✅ Creates both DMG and PKG installers
4. ✅ Notarizes with Apple's notary service
5. ✅ Staples notarization tickets
6. ✅ Validates all packages

**Output:**
- `OrchestraSynth-0.1.0-macOS-universal.dmg` - Drag-and-drop installer
- `OrchestraSynth-0.1.0-macOS-universal.pkg` - Enterprise installer for MDM

### Individual Production Scripts

```bash
# Code signing
./scripts/macos/codesign.sh

# Notarization
./scripts/macos/notarize.sh [path-to-dmg-or-pkg]

# PKG installer creation
./scripts/macos/build_pkg.sh

# Validation
./scripts/macos/validate.sh
```

---

## Documentation

- **Production Packaging:** [docs/PRODUCTION_PACKAGING.md](docs/PRODUCTION_PACKAGING.md) - Complete guide for enterprise deployment
- **Apple Packaging:** [docs/APPLE_PACKAGING.md](docs/APPLE_PACKAGING.md) - Notarization and verification
- **Operations Manual:** [docs/OPERATIONS_MANUAL.md](docs/OPERATIONS_MANUAL.md) - Development workflow and troubleshooting

## Distribution Formats

| Format | Use Case | Installation |
|--------|----------|--------------|
| **DMG** | Direct downloads, user installations | Drag-and-drop to Applications |
| **PKG** | Enterprise deployment, MDM systems | Automated system-wide installation |

---

## Project Structure

```
OrchestraSynth/
├── src/                    # Source code
├── config/                 # Production configuration
│   ├── build.env.example  # Environment template
│   └── entitlements/      # macOS entitlements
├── scripts/macos/         # Build automation
│   ├── production_build.sh
│   ├── codesign.sh
│   ├── notarize.sh
│   ├── build_pkg.sh
│   └── validate.sh
├── docs/                  # Documentation
└── CMakeLists.txt        # CMake build configuration
```

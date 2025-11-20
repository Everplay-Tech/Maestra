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

## 2. macOS build + packaging (universal binary + DMG)

For a production-ready macOS bundle (standalone + VST3 + AU) and a distributable `.dmg`, run:

```bash
scripts/macos/build_and_package.sh
```

The script enforces `arm64;x86_64` architectures, builds both JUCE targets in **Release** mode, and invokes **CPack DragNDrop** to generate `OrchestraSynth-0.1.0-macOS-universal.dmg`. See [docs/APPLE_PACKAGING.md](docs/APPLE_PACKAGING.md) for notarization and verification steps.

---

- **Packaging guide:** [docs/APPLE_PACKAGING.md](docs/APPLE_PACKAGING.md)
- **Build automation:** [`scripts/macos/build_and_package.sh`](scripts/macos/build_and_package.sh)
- **DMG layout script:** [`scripts/macos/dmg_setup.scpt`](scripts/macos/dmg_setup.scpt)

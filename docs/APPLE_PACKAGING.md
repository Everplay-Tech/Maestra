# macOS Delivery Guide (universal app + plugins + DMG)

This guide describes how to produce a production-ready, Apple-compliant build of OrchestraSynth for both Apple Silicon and Intel Macs. It covers prerequisites, repeatable build commands, DMG generation, and notarization/codesigning hand-offs for release engineering teams.

> **TL;DR**: Run `scripts/macos/build_and_package.sh` on macOS with Xcode command line tools installed. The script builds universal binaries (app + plugin) and emits a DragNDrop `.dmg` ready for signing and notarization.

## 1. Prerequisites

- **macOS 14+** with the latest Xcode Command Line Tools (or full Xcode) installed.
- **CMake 3.22+** and optionally **Ninja** for faster builds.
- **JUCE 7.0.12** cloned to `external/JUCE` (required by CMake). Clone command:
  ```bash
  mkdir -p external
  git clone --branch 7.0.12 --depth 1 https://github.com/juce-framework/JUCE.git external/JUCE
  ```
- **Apple Developer ID** and access to your team signing certificates for final distribution.

## 2. Fast path: build + DMG in one command

Run the orchestrated build/package script from the repo root:

```bash
scripts/macos/build_and_package.sh
```

What the script does:

1. Selects a generator (`Ninja` if present, otherwise Unix Makefiles) and enforces a universal `arm64;x86_64` build.
2. Configures CMake in `build/macos-universal-release` (overridable via `BUILD_DIR`).
3. Builds **OrchestraSynth.app** and **OrchestraSynth.plugin** targets in **Release** mode.
4. Invokes `cpack` to produce a compressed DMG named `OrchestraSynth-0.1.0-macOS-universal.dmg` (or `<config>.dmg`).

Environment variables you can override:

- `BUILD_DIR` – custom build output directory (default: `build/macos-universal-release`).
- `GENERATOR` – force a specific generator (e.g., `Xcode`).
- `CONFIG` – build configuration (`Release` recommended for distribution).
- `JUCE_DIR` – explicit JUCE path if not under `external/JUCE`.

## 3. Manual CMake invocation (if you need finer control)

```bash
cmake -S . -B build/macos-universal-release \
  -G "Ninja" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

cmake --build build/macos-universal-release --config Release --target OrchestraSynth OrchestraSynthPlugin
cmake --build build/macos-universal-release --config Release --target package
```

The `package` target uses **CPack DragNDrop** to assemble a `.dmg` with the app bundle and plugin bundles staged in dedicated directories (see `CMakeLists.txt` for install destinations).

## 4. Contents of the DMG

- `OrchestraSynth.app` — the standalone application bundle.
- `Plug-Ins/VST3/OrchestraSynth.vst3` — universal VST3 plugin bundle.
- `Plug-Ins/AU/OrchestraSynth.component` — universal AU plugin bundle.

You can safely codesign/notarize the DMG without altering its structure. The DMG uses a deterministic Finder layout via `scripts/macos/dmg_setup.scpt`.

## 5. Codesigning and notarization (recommended sequence)

Replace the signing identities with values from your Apple Developer account.

```bash
# Sign app + plugins inside the build staging tree
codesign --force --options runtime --timestamp \
  --sign "Developer ID Application: Your Company (TEAMID)" \
  "build/macos-universal-release/OrchestraSynth.app"

codesign --force --options runtime --timestamp \
  --sign "Developer ID Application: Your Company (TEAMID)" \
  "build/macos-universal-release/Plug-Ins/VST3/OrchestraSynth.vst3"

codesign --force --options runtime --timestamp \
  --sign "Developer ID Application: Your Company (TEAMID)" \
  "build/macos-universal-release/Plug-Ins/AU/OrchestraSynth.component"

# (Optional) re-run packaging if you sign after the first CPack pass
cmake --build build/macos-universal-release --config Release --target package

# Notarize the DMG
xcrun notarytool submit build/macos-universal-release/OrchestraSynth-0.1.0-macOS-universal.dmg \
  --apple-id "you@example.com" --team-id "TEAMID" --keychain-profile "AC_PASSWORD" --wait

# Staple notarization ticket
xcrun stapler staple build/macos-universal-release/OrchestraSynth-0.1.0-macOS-universal.dmg
```

## 6. Verification checklist

- **Lipo validation** (confirm universal slices):
  ```bash
  lipo -info build/macos-universal-release/OrchestraSynth.app/Contents/MacOS/OrchestraSynth
  lipo -info build/macos-universal-release/Plug-Ins/VST3/OrchestraSynth.vst3/Contents/MacOS/OrchestraSynth
  lipo -info build/macos-universal-release/Plug-Ins/AU/OrchestraSynth.component/Contents/MacOS/OrchestraSynth
  ```
- **Codesign validation**:
  ```bash
  codesign --verify --deep --strict --verbose=2 build/macos-universal-release/OrchestraSynth.app
  spctl --assess --verbose build/macos-universal-release/OrchestraSynth.app
  ```
- **DMG smoke test**: mount the DMG and launch the app from the mounted volume; confirm the plugin bundles copy cleanly to `/Library/Audio/Plug-Ins`.

## 7. CI/CD integration notes

- The CPack configuration lives in `CMakeLists.txt` under the "Installation + packaging" section and uses `DragNDrop` to emit `.dmg` artifacts.
- Preserve the `scripts/macos/dmg_setup.scpt` when moving build pipelines; it pins Finder layout and icon sizing for consistent user experience.
- Artifacts are deterministic across runs as long as JUCE assets and CMake inputs are unchanged.

## 8. Support contact hand-off

Release engineering teams should capture the following when reporting issues:
- Full `cmake --system-information` output and `cmake --build` logs.
- Exact SHA of the repo and JUCE submodule reference.
- macOS version and Xcode version.
- Whether codesigning/notarization was performed before or after DMG assembly.

With this guide and the provided scripts, OrchestraSynth can be shipped as a universal macOS `.dmg` with enterprise-grade repeatability and auditability.

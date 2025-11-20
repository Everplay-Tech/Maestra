# OrchestraSynth Operations Manual

## Purpose and scope
This manual defines operational standards for building, packaging, validating, and shipping OrchestraSynth across macOS orchestral synthesizer targets (standalone app and plugin). It emphasizes deterministic builds, enterprise-grade observability, and secure distribution.

## Repository layout (ops-relevant)
- `CMakeLists.txt` — target definitions, universal binary configuration, and CPack packaging defaults for the app and plugin. JUCE 7.0.12 is automatically fetched via CMake's FetchContent.
- `scripts/macos/build_and_package.sh` — end-to-end macOS build and DMG pipeline with generator auto-selection.
- `scripts/macos/dmg_setup.scpt` — Finder layout script consumed by CPack DragNDrop DMG generation.
- `docs/APPLE_PACKAGING.md` — notarization and verification guidance for distributable DMGs (follow after packaging here).

## Prerequisites
- macOS 14+ with AppleClang 17+ and CMake ≥ 3.22.
- Developer tools: Xcode Command Line Tools, `codesign` identity and notarization Apple ID for release signing.
- Internet connection for first build (CMake will automatically download JUCE 7.0.12).
- Optional: Ninja installed for faster multi-arch builds (auto-detected).

## Environment setup
1. Clone the repository:
   ```bash
   git clone <your-orchestra-synth-repo> OrchestraSynth
   cd OrchestraSynth
   ```
   **Note:** JUCE 7.0.12 is automatically downloaded by CMake's FetchContent during the first build. No manual setup required.

2. Recommended environment variables (overridable per run):
   - `BUILD_DIR` — build output directory (default: `build/macos-universal-release`).
   - `GENERATOR` — explicit CMake generator (default: auto-selects `Ninja` when available, else `Unix Makefiles`).
   - `CONFIG` — build configuration (default: `Release`).

## Standard build workflow
### One-command build + package (preferred)
Run the hardened script for a universal Release build and DMG creation:
```bash
scripts/macos/build_and_package.sh
```
Behavior:
- Selects a generator automatically (prefers Ninja).
- Configures CMake for `arm64;x86_64` (CMake will automatically download JUCE 7.0.12 on first run).
- Builds the standalone app and plugin, then invokes the `package` target (CPack DragNDrop DMG).
- Emits the final DMG path (either `OrchestraSynth-${CONFIG}.dmg` or `OrchestraSynth-0.1.0-macOS-universal.dmg`) inside the build directory.

### Manual CMake flow (if deeper control is needed)
```bash
# Configure (set GENERATOR to Ninja or Unix Makefiles as desired)
cmake -S . -B build/macos-universal-release \
  -G "Ninja" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

# Build app + plugin
cmake --build build/macos-universal-release --config Release --target OrchestraSynth OrchestraSynthPlugin

# Package DMG via CPack
cmake --build build/macos-universal-release --config Release --target package
```
Artifacts: the `.app` bundle and plugin binaries reside under the build directory; the DMG is written at the top level of the same build directory.

## Packaging and release
- CPack DragNDrop is configured in `CMakeLists.txt` to produce a universal DMG named `OrchestraSynth-${PROJECT_VERSION}-macOS-universal.dmg` and to apply the Finder layout in `scripts/macos/dmg_setup.scpt`.
- After DMG generation, follow `docs/APPLE_PACKAGING.md` for notarization, staple, and verification steps before distributing.

## Observability and resilience
- **Logging:** `Logger` captures timestamped entries, increments a total counter, and mirrors messages to JUCE’s debug output for live diagnostics. Use `log(LogLevel, message)` from audio or UI contexts for structured event tracing.
- **Crash reporting:** `CrashReporter` installs/uninstalls a global handler flag and records the latest crash message while logging it as an error. Integrate `submitCrashReport` in exception boundaries or platform crash hooks before release builds.
- **Performance monitoring:** `PerformanceMonitor` tracks per-block render timing with atomic snapshots (last block and running average). Wrap audio processing blocks with `beginBlock`/`endBlock` and expose `getSnapshot` for UI meters or telemetry exporters.

## Operational validation checklist

### Automated Validation (Production-Ready)

The repository now includes comprehensive automated validation replacing manual procedures:

1. **Quick Validation (Development)**
   ```bash
   # After every build
   ./scripts/macos/validate.sh              # Technical validation (~2 min)
   ./scripts/macos/functional_validate.sh   # Functional validation (~3 min)
   ```

2. **Comprehensive Validation (Pre-Release)**
   ```bash
   # Before every release
   ./scripts/macos/pre_release_validate.sh  # Complete validation suite (~5-10 min)
   cat validation-report.txt                 # Review detailed report
   ```

3. **CI/CD Validation (Automated)**
   - Automatic validation on every push/PR via GitHub Actions
   - Full release validation on version tags
   - See `.github/workflows/validation.yml` and `.github/workflows/release.yml`

### What Gets Validated

**Technical Checks (50+ automated checks):**
- ✅ Code signing and notarization
- ✅ Universal binary architecture (arm64 + x86_64)
- ✅ Hardened runtime and entitlements
- ✅ Bundle structure and Info.plist
- ✅ DMG/PKG integrity and Gatekeeper acceptance
- ✅ Security hardening (stack protection, PIE)

**Functional Checks:**
- ✅ Application launch capability
- ✅ Plugin bundle structure and discoverability
- ✅ Framework dependencies (no hardcoded paths)
- ✅ Observability infrastructure (logging, crash reporting, performance monitoring)
- ✅ Documentation and configuration completeness

**Production Readiness:**
- ✅ All artifacts present and signed
- ✅ Security audit passed
- ✅ Artifact inventory with SHA256 checksums
- ✅ Generates compliance report

### Legacy Manual Validation (Optional)

For manual verification or troubleshooting:

- **Build sanity:** Confirm `scripts/macos/build_and_package.sh` completes without errors and produces the DMG in the build directory.
- **Runtime smoke test:** Launch the standalone app from the build output, verify MIDI routing, and load a preset through the UI without crashes.
- **Plugin sanity:** Load the AU or VST3 in a host (e.g., Logic, Live) and validate audio output plus preset loading.
- **Observability:** Trigger a test log entry, confirm it surfaces in JUCE debug output, and validate `PerformanceMonitor` snapshots change during audio playback.
- **Release readiness:** Run notarization per `docs/APPLE_PACKAGING.md`; archive the notarization ticket and DMG checksum alongside release notes.

**Note:** All manual checks above are now automated. See `docs/VALIDATION_PROCEDURES.md` for complete documentation.

## Troubleshooting
- **JUCE download issues:** Ensure you have an active internet connection for the first build. CMake will automatically download JUCE 7.0.12 via FetchContent.
- **Generator issues:** Set `GENERATOR="Unix Makefiles"` if Ninja is unavailable, or install Ninja for faster multi-arch builds.
- **Stale artifacts:** Remove the build directory (`rm -rf build/macos-universal-release`) before re-configuring when switching toolchains or architectures.
- **DMG layout not applied:** Confirm `scripts/macos/dmg_setup.scpt` is present and that CPack runs the `package` target in the configured build directory.

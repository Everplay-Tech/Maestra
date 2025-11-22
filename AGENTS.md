# Repository Guidelines

## Project Structure & Module Organization
Source lives under `src/` with feature-focused folders: `App/` bootstraps JUCE, `Engine/` manages orchestral voices, `DSP/` hosts processing utilities, `UI/` renders mixers and preset tools, and `Plugin/` shares code with the AU/VST3 targets. Automation lives in `scripts/macos/`, configuration and entitlements in `config/`, documentation in `docs/`, and shared assets under `resources/`. Generated artefacts land in `build/`; keep the directory untracked.

## Build, Test, and Development Commands
- `./DoubleClickBuild.command` or `Build OrchestraSynth.app` runs the universal Release build with the expected env flags (`JUCE_SKIP_BUNDLE_SIGNING_CHECKS=1`, `ORCHESTRASYNTH_SKIP_PLUGIN_INSTALL=1`).  
- `scripts/macos/build_and_package.sh` performs the developer build plus DMG packaging; use it for day-to-day verification.  
- `./scripts/macos/setup_m5_build.sh --upgrade-juce --clean` upgrades JUCE and patches flags for Apple M5 hardware.  
- Production flows require `cp config/build.env.example config/build.env`, populate credentials, `source config/build.env`, then run `./scripts/macos/production_build.sh` for signed DMG/PKG artefacts.  
- Use `InstallOrchestraSynth.command` / `UninstallOrchestraSynth.command` to move artefacts on and off the system without manual Finder steps.

## Coding Style & Naming Conventions
Follow the JUCE-oriented C++17 style already in `src/UI/MixerComponent.cpp`: four-space indentation, K&R braces, explicit `juce::` prefixes, and `constexpr` tables for immutable data. Classes remain PascalCase, members camelCase (`performanceMonitor`, `stringsStrip`), and helper functions verb-based camelCase inside anonymous namespaces. Use `auto` sparingly, respect RAII ownership, and mirror neighbouring files because no `.clang-format` is enforced.

## Testing Guidelines
The project relies on scripted validation rather than standalone unit tests. After meaningful changes run `./scripts/macos/build_and_package.sh`, then `./scripts/macos/functional_validate.sh` for runtime smoke checks. Before committing, execute `./scripts/macos/validate.sh` to verify codesign, architectures, and plugin manifests. Each pull request should attach the report from `./scripts/macos/pre_release_validate.sh` (optionally pointing to a custom output path) to prove release readiness.

## Commit & Pull Request Guidelines
Recent history favors concise, imperative subjects that call out the subsystem (e.g., `UI: Add virtual keyboard window`, `Engine: Qualify Logger usages`). Keep body text focused on motivation and risks, and list whichever validation commands were executed. PRs must include a short summary, linked issue or task ID, validation output (or log excerpts), and screenshots whenever JUCE UI changes occur. Never commit generated artefacts or `config/build.env`; call out exceptions directly in the PR.

## Security & Configuration Tips
Secrets live solely in `config/build.env` and are ignored by git—copy from the example, fill in Apple IDs, and restrict permissions. When sharing build logs, redact credential echoes, and prefer temporary override files (`config/overrides/*.cmake`) for machine-specific tweaks instead of editing `CMakeLists.txt`.

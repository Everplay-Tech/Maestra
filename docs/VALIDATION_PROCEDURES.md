# OrchestraSynth Validation Procedures

**Comprehensive Validation Guide for Production Readiness**

This document describes the complete automated validation infrastructure for OrchestraSynth, replacing manual validation procedures with automated, repeatable checks.

---

## Table of Contents

1. [Overview](#overview)
2. [Validation Layers](#validation-layers)
3. [Quick Start](#quick-start)
4. [Validation Scripts](#validation-scripts)
5. [CI/CD Integration](#cicd-integration)
6. [Interpreting Results](#interpreting-results)
7. [Troubleshooting](#troubleshooting)

---

## Overview

The OrchestraSynth validation infrastructure provides **three layers of validation**:

1. **Technical Validation** - Code signing, notarization, architecture, security
2. **Functional Validation** - Runtime behavior, plugin loading, observability
3. **Production Readiness** - Comprehensive pre-release checks

### Automation Benefits

✅ **Consistent** - Same checks every time, no human error
✅ **Fast** - Automated checks run in minutes vs. hours
✅ **Comprehensive** - 50+ validation points across all layers
✅ **Documented** - Detailed reports for compliance and debugging
✅ **CI/CD Ready** - Integrates with GitHub Actions and other platforms

---

## Validation Layers

### Layer 1: Technical Validation

**Script:** `scripts/macos/validate.sh`
**Purpose:** Validates build artifacts for Apple platform compliance
**Runtime:** ~2-3 minutes

**Checks:**
- ✅ Code signature validity
- ✅ Notarization status
- ✅ Universal binary architecture (arm64 + x86_64)
- ✅ Entitlements configuration
- ✅ Bundle structure (Info.plist, frameworks, resources)
- ✅ DMG/PKG integrity
- ✅ Gatekeeper acceptance
- ✅ Security hardening (hardened runtime, timestamps)

**When to run:**
- After every build
- Before code signing/notarization
- After making changes to build configuration

### Layer 2: Functional Validation

**Script:** `scripts/macos/functional_validate.sh`
**Purpose:** Validates runtime behavior and operational readiness
**Runtime:** ~3-5 minutes

**Checks:**
- ✅ Application launch capability
- ✅ Executable permissions and structure
- ✅ Framework dependencies (no hardcoded paths)
- ✅ Plugin bundle structure (VST3, AU)
- ✅ Plugin discoverability
- ✅ Info.plist completeness
- ✅ DMG mountability and contents
- ✅ PKG payload validation
- ✅ Observability infrastructure (logging, crash reporting, performance monitoring)
- ✅ Security hardening (stack protection, PIE, ARC)
- ✅ Documentation completeness
- ✅ Build configuration files

**When to run:**
- After every build
- Before deployment
- When troubleshooting runtime issues

### Layer 3: Production Readiness Validation

**Script:** `scripts/macos/pre_release_validate.sh`
**Purpose:** Comprehensive pre-release validation suite
**Runtime:** ~5-10 minutes

**Checks:**
- ✅ All technical validation checks
- ✅ All functional validation checks
- ✅ Production readiness score (13 criteria)
- ✅ Security audit (RPATH, code segments, stack protection, PIE)
- ✅ Release artifact inventory with checksums
- ✅ Generates detailed validation report

**When to run:**
- Before every release
- When creating release candidates
- For compliance/audit purposes

---

## Quick Start

### Running All Validations (Recommended)

```bash
# Build the project
./scripts/macos/build_and_package.sh

# Run comprehensive pre-release validation
./scripts/macos/pre_release_validate.sh

# Review the validation report
cat validation-report.txt
```

### Running Individual Validations

```bash
# Technical validation only
./scripts/macos/validate.sh

# Functional validation only
./scripts/macos/functional_validate.sh

# With custom build directory
./scripts/macos/validate.sh build/custom-directory
```

---

## Validation Scripts

### 1. Technical Validation Script

**Location:** `scripts/macos/validate.sh`

**Usage:**
```bash
./scripts/macos/validate.sh [BUILD_DIR]
```

**Arguments:**
- `BUILD_DIR` - Build directory path (default: `build/macos-universal-release`)

**Exit Codes:**
- `0` - All validations passed
- `1` - Critical failures found
- `2` - Warnings found (non-critical)

**Output:**
- Colored terminal output with ✓/✗/⚠ indicators
- Detailed error/warning messages
- Statistics summary

**Example Output:**
```
=== Build Directory ===
✓ Build directory exists: build/macos-universal-release

=== Standalone Application ===
━━━ Validating: OrchestraSynth.app ━━━
✓ OrchestraSynth.app exists
✓ Universal binary: x86_64 arm64
✓ Code signature valid
✓ Signed with Developer ID (distribution)
✓ Hardened runtime enabled

╔═══════════════════════════════════════╗
║  Validation Summary                   ║
╚═══════════════════════════════════════╝

✓ All validations passed!

Statistics:
  Errors:   0
  Warnings: 0
```

---

### 2. Functional Validation Script

**Location:** `scripts/macos/functional_validate.sh`

**Usage:**
```bash
./scripts/macos/functional_validate.sh [BUILD_DIR]
```

**Arguments:**
- `BUILD_DIR` - Build directory path (default: `build/macos-universal-release`)

**Exit Codes:**
- `0` - All functional tests passed
- `1` - Critical failures found
- `2` - Warnings found (non-critical)

**Validation Categories:**
1. **Standalone Application Runtime**
   - Executable existence and permissions
   - Application launch test
   - Info.plist validation
   - Framework dependency checks

2. **VST3 Plugin**
   - Bundle structure
   - Mach-O binary validation
   - moduleinfo.json validation
   - Dependency checks

3. **Audio Unit Plugin**
   - Bundle structure
   - Mach-O binary validation
   - AudioComponents validation
   - auval discoverability check

4. **Distribution Packages**
   - DMG integrity and mount test
   - PKG structure and payload

5. **Observability Infrastructure**
   - Logger implementation
   - Crash reporting
   - Performance monitoring

6. **Security & Hardening**
   - Stack protection
   - Position Independent Executable (PIE)
   - ARC (Automatic Reference Counting)

7. **Build Configuration**
   - Debug symbols check
   - Binary optimization
   - Size analysis

8. **Documentation**
   - Required docs present
   - Build configuration files
   - Entitlements configuration

**Example Output:**
```
=== Standalone Application Runtime Test ===

✓ Standalone app bundle exists
✓ Executable exists
✓ Executable has correct permissions
✓ Application launched successfully
✓ All required Info.plist keys present
✓ Framework dependencies are valid
✓ No hardcoded absolute paths in dependencies

╔═══════════════════════════════════════╗
║  Functional Validation Summary        ║
╚═══════════════════════════════════════╝

✓ All functional validations passed!

Statistics:
  Errors:   0
  Warnings: 1

Next Steps:
  1. Run technical validation: ./scripts/macos/validate.sh
  2. Review any errors or warnings above
  3. Test manually in production-like environment
```

---

### 3. Pre-Release Validation Script

**Location:** `scripts/macos/pre_release_validate.sh`

**Usage:**
```bash
./scripts/macos/pre_release_validate.sh [BUILD_DIR] [REPORT_FILE]
```

**Arguments:**
- `BUILD_DIR` - Build directory path (default: `build/macos-universal-release`)
- `REPORT_FILE` - Output report file (default: `validation-report.txt`)

**Exit Codes:**
- `0` - All validations passed (production ready)
- `1` - Critical failures found (not ready for release)
- `2` - Warnings found (review required)

**Validation Phases:**

1. **Pre-flight Checks**
   - Build directory existence
   - Required tools availability

2. **Technical Validation Suite**
   - Runs complete `validate.sh` script

3. **Functional Validation Suite**
   - Runs complete `functional_validate.sh` script

4. **Production Readiness Checklist** (13 criteria)
   - [ ] App is code signed
   - [ ] App has hardened runtime
   - [ ] App has valid entitlements
   - [ ] App is universal binary (arm64 + x86_64)
   - [ ] App is notarized (Gatekeeper approved)
   - [ ] DMG is notarized
   - [ ] DMG installer exists
   - [ ] PKG installer exists
   - [ ] README.md exists
   - [ ] Production packaging guide exists
   - [ ] Operations manual exists
   - [ ] Build configuration template exists
   - [ ] Entitlements configured

5. **Security & Compliance Audit**
   - RPATH security
   - Code segment protection
   - Stack protection
   - PIE (Position Independent Executable)

6. **Release Artifact Inventory**
   - Lists all artifacts with sizes and SHA256 checksums
   - Standalone app, plugins, installers

**Generated Report:**

The script generates a comprehensive validation report including:
- Complete validation results from all layers
- Production readiness score
- Security audit findings
- Artifact inventory with checksums
- Final status and recommendations

**Example Report Structure:**
```
╔═══════════════════════════════════════╗
║ OrchestraSynth Pre-Release Report    ║
╚═══════════════════════════════════════╝

Generated: 2024-11-20 15:30:00
Build Directory: build/macos-universal-release

━━━ Technical Validation Suite ━━━
[Full technical validation output]

━━━ Functional Validation Suite ━━━
[Full functional validation output]

━━━ Production Readiness Checklist ━━━
  [✓] App is code signed
  [✓] App has hardened runtime
  [✓] App has valid entitlements
  [✓] App is universal binary
  ...

Production Readiness: 100% - READY FOR RELEASE

━━━ Security & Compliance Audit ━━━
✓ RPATH configuration is secure
✓ Code segments are properly protected
✓ Stack protection enabled
✓ PIE enabled

Security Audit: PASSED

━━━ Release Artifact Inventory ━━━
Standalone Application:
  Path: build/macos-universal-release/OrchestraSynth.app
  Size: 45M
  SHA256: abc123...

FINAL SUMMARY
═══════════════════════════════════════
Total Checks: 2
Passed: 2
Warnings: 0
Failed: 0

FINAL STATUS: READY FOR RELEASE

RECOMMENDATIONS
═══════════════════════════════════════
✓ All validation checks passed
✓ Build is production-ready

Next Steps:
1. Archive this validation report with the release
2. Upload artifacts to distribution platform
3. Update release notes with checksums
```

---

## CI/CD Integration

### GitHub Actions Workflows

Two workflows are provided for automated validation in CI/CD:

#### 1. Validation Workflow

**File:** `.github/workflows/validation.yml`

**Triggers:**
- Push to `main`, `develop`, or `claude/**` branches
- Pull requests to `main` or `develop`
- Manual trigger

**Jobs:**

1. **validate-build**
   - Builds project
   - Runs technical validation
   - Runs functional validation
   - Uploads artifacts

2. **pre-release-validation** (main branch/tags only)
   - Runs comprehensive pre-release validation
   - Uploads validation report
   - Comments on PRs with results

3. **code-quality**
   - Checks documentation completeness
   - Validates build configuration
   - Checks build scripts

4. **security-scan**
   - Scans for sensitive data
   - Verifies .gitignore configuration

**Usage:**
```yaml
# Automatically runs on push/PR

# Manual trigger:
gh workflow run validation.yml
```

#### 2. Release Workflow

**File:** `.github/workflows/release.yml`

**Triggers:**
- Push tags matching `v*.*.*` (e.g., `v1.0.0`)
- Manual trigger with version input

**Jobs:**

1. **build-release**
   - Builds project
   - Code signs (if certificates configured)
   - Notarizes (if credentials configured)
   - Runs comprehensive validation
   - Generates checksums
   - Creates GitHub release

2. **post-release-validation**
   - Downloads release artifacts
   - Verifies artifact integrity
   - Validates checksums

**Usage:**
```bash
# Create and push a tag
git tag v1.0.0
git push origin v1.0.0

# Or trigger manually
gh workflow run release.yml -f version=1.0.0
```

### Required Secrets for CI/CD

Configure these secrets in GitHub repository settings:

**Code Signing:**
- `DEVELOPER_ID_APPLICATION` - Base64-encoded .p12 certificate
- `CERT_PASSWORD` - Certificate password
- `KEYCHAIN_PASSWORD` - Temporary keychain password
- `CODESIGN_IDENTITY` - e.g., "Developer ID Application: Company (TEAM)"
- `PKG_SIGNING_IDENTITY` - e.g., "Developer ID Installer: Company (TEAM)"

**Notarization:**
- `NOTARIZE_APPLE_ID` - Apple ID email
- `NOTARIZE_TEAM_ID` - Team ID
- `NOTARIZE_PASSWORD` - App-specific password

**Converting Certificate to Base64:**
```bash
base64 -i certificate.p12 -o certificate.txt
# Copy contents of certificate.txt to DEVELOPER_ID_APPLICATION secret
```

---

## Interpreting Results

### Exit Codes

All validation scripts use consistent exit codes:

- **0** - Success (all checks passed)
- **1** - Critical failure (must fix before release)
- **2** - Warnings (review recommended)

### Output Indicators

- ✅ `✓` - Check passed
- ❌ `✗` - Check failed (critical)
- ⚠️  `⚠` - Warning (non-critical)
- ℹ️  `ℹ` - Information only

### Result Interpretation

#### All Green (Exit Code 0)
```
✓ All validations passed!
FINAL STATUS: READY FOR RELEASE
```
**Action:** Safe to release

#### Warnings (Exit Code 2)
```
⚠ Validation completed with 3 warning(s)
FINAL STATUS: REVIEW REQUIRED
```
**Action:** Review warnings, assess risk, decide on release

Common warnings:
- Missing optional plugins
- Non-critical entitlements
- Development certificates (not for production)

#### Errors (Exit Code 1)
```
✗ Validation failed with 2 error(s)
FINAL STATUS: NOT READY FOR RELEASE
```
**Action:** Must fix errors before release

Common errors:
- Code signature invalid
- Bundle structure broken
- Required files missing
- Architecture not universal

---

## Troubleshooting

### Validation Script Not Found

**Error:**
```
✗ Validation script not found: scripts/macos/validate.sh
```

**Solution:**
```bash
# Ensure you're in the repository root
cd /path/to/OrchestraSynth

# Check script exists
ls -la scripts/macos/

# Make executable if needed
chmod +x scripts/macos/*.sh
```

### Build Directory Not Found

**Error:**
```
✗ Build directory not found: build/macos-universal-release
```

**Solution:**
```bash
# Build the project first
./scripts/macos/build_and_package.sh

# Or specify custom build directory
./scripts/macos/validate.sh /path/to/custom/build
```

### Code Signing Validation Fails

**Error:**
```
✗ Code signature invalid or missing
```

**Solution:**
```bash
# Check current signature
codesign -dv build/macos-universal-release/OrchestraSynth.app

# Sign the app
./scripts/macos/codesign.sh

# Run validation again
./scripts/macos/validate.sh
```

### Notarization Validation Fails

**Error:**
```
⚠ DMG is not notarized or not accepted by Gatekeeper
```

**Solution:**
```bash
# Notarize the DMG
./scripts/macos/notarize.sh build/macos-universal-release/OrchestraSynth-0.1.0-macOS-universal.dmg

# Check notarization status
xcrun notarytool history --apple-id you@example.com --team-id TEAM123

# Run validation again
./scripts/macos/validate.sh
```

### Universal Binary Check Fails

**Error:**
```
✗ App is not universal binary
```

**Solution:**
```bash
# Check current architectures
lipo -info build/macos-universal-release/OrchestraSynth.app/Contents/MacOS/OrchestraSynth

# Rebuild with correct architectures
rm -rf build/
CMAKE_OSX_ARCHITECTURES="arm64;x86_64" ./scripts/macos/build_and_package.sh
```

### Plugin Not Discoverable

**Error:**
```
⚠ AU plugin may not be discoverable
```

**Solution:**
```bash
# Reset Audio Component Registrar
killall -9 AudioComponentRegistrar

# Re-scan plugins
auval -a

# Check if plugin appears
auval -a | grep OrchestraSynth
```

### High Warning Count

If you see many warnings but no critical errors:

1. **Review each warning** - Some may be expected (e.g., development builds)
2. **Check context** - Development builds have different requirements than production
3. **Document accepted warnings** - Create a list of known/acceptable warnings
4. **Fix what you can** - Address warnings that are easy to fix

### Validation Report Not Generated

**Error:**
```
Report file not created
```

**Solution:**
```bash
# Check write permissions
ls -la validation-report.txt

# Specify custom location
./scripts/macos/pre_release_validate.sh build/macos-universal-release /tmp/report.txt

# Check disk space
df -h
```

---

## Best Practices

### Development Workflow

1. **After every code change:**
   ```bash
   ./scripts/macos/build_and_package.sh
   ./scripts/macos/functional_validate.sh
   ```

2. **Before committing:**
   ```bash
   ./scripts/macos/validate.sh
   ```

3. **Before creating PR:**
   ```bash
   ./scripts/macos/pre_release_validate.sh
   ```

### Release Workflow

1. **Create release candidate:**
   ```bash
   # Build and sign
   ./scripts/macos/production_build.sh

   # Comprehensive validation
   ./scripts/macos/pre_release_validate.sh build/macos-universal-release release-v1.0.0-report.txt

   # Review report
   cat release-v1.0.0-report.txt
   ```

2. **Archive validation report:**
   ```bash
   # Save with release artifacts
   cp release-v1.0.0-report.txt releases/v1.0.0/
   ```

3. **Create release:**
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   # CI/CD will run full validation automatically
   ```

### Continuous Improvement

- **Monitor validation failures** - Track common issues
- **Update thresholds** - Adjust warning/error criteria as needed
- **Add new checks** - Expand validation coverage over time
- **Document exceptions** - Keep a list of known acceptable warnings

---

## Validation Checklist

Use this checklist to ensure complete validation:

### Pre-Build
- [ ] Latest code pulled from repository
- [ ] Dependencies installed (CMake, Ninja)
- [ ] Build configuration reviewed

### Build
- [ ] Build completes without errors
- [ ] Universal binary created (arm64 + x86_64)
- [ ] All targets built (app + plugins)
- [ ] Installers generated (DMG + PKG)

### Technical Validation
- [ ] Code signatures valid
- [ ] Hardened runtime enabled
- [ ] Entitlements configured correctly
- [ ] Notarization complete (for release)
- [ ] Gatekeeper acceptance verified

### Functional Validation
- [ ] Application launches
- [ ] Plugins load in DAWs
- [ ] No hardcoded paths in dependencies
- [ ] Observability infrastructure present
- [ ] Documentation complete

### Pre-Release Validation
- [ ] All technical checks pass
- [ ] All functional checks pass
- [ ] Production readiness score > 80%
- [ ] Security audit passes
- [ ] Validation report generated
- [ ] Checksums calculated

### Release
- [ ] Validation report archived
- [ ] Artifacts uploaded
- [ ] Release notes updated
- [ ] Checksums published

---

## Support and Resources

### Documentation
- [Operations Manual](OPERATIONS_MANUAL.md) - Build and deployment workflows
- [Production Packaging](PRODUCTION_PACKAGING.md) - Enterprise packaging guide
- [Apple Packaging](APPLE_PACKAGING.md) - Notarization and verification

### Scripts
- `scripts/macos/validate.sh` - Technical validation
- `scripts/macos/functional_validate.sh` - Functional validation
- `scripts/macos/pre_release_validate.sh` - Comprehensive validation
- `scripts/macos/production_build.sh` - Full production build + validation

### Workflows
- `.github/workflows/validation.yml` - Automated validation
- `.github/workflows/release.yml` - Release automation

---

**Last Updated:** 2024
**Version:** 1.0.0

For questions or issues, consult the Operations Manual or create an issue in the repository.

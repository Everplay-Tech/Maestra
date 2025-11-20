# OrchestraSynth Production Packaging Guide

**Enterprise-Grade macOS Packaging for Production Deployment**

This guide covers everything you need to create production-ready, notarized macOS packages for OrchestraSynth suitable for enterprise distribution, App Store submission, or direct download.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Quick Start](#quick-start)
3. [Configuration](#configuration)
4. [Build Process](#build-process)
5. [Distribution Formats](#distribution-formats)
6. [CI/CD Integration](#cicd-integration)
7. [Troubleshooting](#troubleshooting)
8. [Security Best Practices](#security-best-practices)

---

## Prerequisites

### Required Software

- **macOS 14.0 or later**
- **Xcode Command Line Tools** (14.0+)
  ```bash
  xcode-select --install
  ```
- **CMake 3.22+**
  ```bash
  brew install cmake
  ```
- **Ninja** (optional, for faster builds)
  ```bash
  brew install ninja
  ```

### Apple Developer Account Requirements

1. **Apple Developer Program Membership** ($99/year)
   - Sign up at: https://developer.apple.com/programs/

2. **Developer ID Certificates**
   - **Developer ID Application** (for code signing apps/plugins)
   - **Developer ID Installer** (for signing PKG installers)
   - Download from: https://developer.apple.com/account/resources/certificates

3. **Notarization Credentials**
   - Apple ID with 2FA enabled
   - App-specific password generated at: https://appleid.apple.com/account/manage
   - Team ID from: https://developer.apple.com/account

### Installing Certificates

```bash
# Import certificates into keychain
security import DeveloperIDApplication.cer -k ~/Library/Keychains/login.keychain-db
security import DeveloperIDInstaller.cer -k ~/Library/Keychains/login.keychain-db

# Verify installation
security find-identity -v -p codesigning
```

---

## Quick Start

### 1. Configure Environment

Copy the example configuration and customize:

```bash
cp config/build.env.example config/build.env
```

Edit `config/build.env` with your credentials:

```bash
export CODESIGN_IDENTITY="Developer ID Application: Your Company (TEAM123)"
export NOTARIZE_APPLE_ID="you@yourcompany.com"
export NOTARIZE_TEAM_ID="TEAM123ABC"
export NOTARIZE_PASSWORD="AC_PASSWORD"  # Keychain profile name
export NOTARIZE_USE_KEYCHAIN="1"
```

**Important:** Store notarization password in keychain (recommended):

```bash
xcrun notarytool store-credentials "AC_PASSWORD" \
  --apple-id "you@yourcompany.com" \
  --team-id "TEAM123ABC" \
  --password "xxxx-xxxx-xxxx-xxxx"
```

### 2. Load Configuration

```bash
source config/build.env
```

### 3. Run Production Build

```bash
./scripts/macos/production_build.sh
```

This will:
1. ✅ Build universal binaries (Apple Silicon + Intel)
2. ✅ Code sign with hardened runtime
3. ✅ Create DMG and PKG installers
4. ✅ Notarize with Apple
5. ✅ Staple notarization tickets
6. ✅ Validate packages

---

## Configuration

### Environment Variables

All configuration is done via environment variables in `config/build.env`:

#### Code Signing (Required)

```bash
# Application signing certificate
export CODESIGN_IDENTITY="Developer ID Application: Company (TEAM)"

# Optional: Specific keychain
export CODESIGN_KEYCHAIN="/path/to/keychain.keychain-db"

# PKG signing certificate (optional but recommended)
export PKG_SIGNING_IDENTITY="Developer ID Installer: Company (TEAM)"
```

#### Notarization (Required)

```bash
export NOTARIZE_APPLE_ID="you@company.com"
export NOTARIZE_TEAM_ID="TEAM123ABC"
export NOTARIZE_PASSWORD="AC_PASSWORD"  # Keychain profile
export NOTARIZE_USE_KEYCHAIN="1"
```

#### Build Customization (Optional)

```bash
# Company/Product branding
export COMPANY_NAME="Your Company"
export PRODUCT_NAME="OrchestraSynth"
export BUNDLE_ID_PREFIX="com.yourcompany.orchestrasynth"

# Build configuration
export BUILD_CONFIG="Release"
export BUILD_NUMBER="$(date +%Y%m%d%H%M%S)"

# CMake generator
export GENERATOR="Ninja"
```

### Entitlements

Entitlements control app permissions and are critical for notarization.

**Location:** `config/entitlements/`

- `app.entitlements` - Standalone application entitlements
- `plugin.entitlements` - Audio plugin entitlements

**Key Entitlements:**

```xml
<!-- Required for audio I/O -->
<key>com.apple.security.device.audio-input</key>
<true/>

<!-- Required for MIDI devices -->
<key>com.apple.security.device.usb</key>
<true/>

<!-- For plugin compatibility with DAWs -->
<key>com.apple.security.cs.disable-library-validation</key>
<true/>  <!-- Only in plugin.entitlements -->
```

### CMake Customization

Edit `CMakeLists.txt` to customize:

**Plugin Identifiers (IMPORTANT - Must be unique!):**

```cmake
PLUGIN_MANUFACTURER_CODE "Ymco"  # Change to your 4-char code
PLUGIN_CODE             "Orch"  # Change to your 4-char code
```

**Register your codes at:** https://developer.apple.com/library/archive/documentation/General/Reference/InfoPlistKeyReference/Articles/CoreFoundationKeys.html

---

## Build Process

### Individual Build Steps

While `production_build.sh` automates everything, you can run steps individually:

#### Step 1: Build Binaries

```bash
./scripts/macos/build_and_package.sh
```

**Output:**
- `build/macos-universal-release/OrchestraSynth.app`
- `build/macos-universal-release/Plug-Ins/VST3/OrchestraSynth.vst3`
- `build/macos-universal-release/Plug-Ins/AU/OrchestraSynth.component`

#### Step 2: Code Sign

```bash
./scripts/macos/codesign.sh
```

Signs all binaries with hardened runtime and entitlements.

#### Step 3: Create DMG

```bash
cmake --build build/macos-universal-release --target package
```

#### Step 4: Create PKG (Optional)

```bash
./scripts/macos/build_pkg.sh
```

**PKG installs to:**
- `/Applications/OrchestraSynth.app`
- `/Library/Audio/Plug-Ins/VST3/OrchestraSynth.vst3`
- `/Library/Audio/Plug-Ins/Components/OrchestraSynth.component`

#### Step 5: Notarize

```bash
# DMG
./scripts/macos/notarize.sh build/macos-universal-release/OrchestraSynth-0.1.0-macOS-universal.dmg

# PKG
./scripts/macos/notarize.sh build/macos-universal-release/OrchestraSynth-0.1.0-macOS-universal.pkg
```

**Note:** Notarization typically takes 2-10 minutes.

#### Step 6: Validate

```bash
./scripts/macos/validate.sh
```

Checks:
- ✅ Code signatures
- ✅ Notarization status
- ✅ Universal binary architecture
- ✅ Entitlements
- ✅ Bundle structure
- ✅ Security compliance

---

## Distribution Formats

### DMG (Drag-and-Drop Installer)

**Best for:** Direct downloads, user-initiated installations

**Contents:**
- Standalone app
- VST3 plugin (in Plug-Ins folder)
- AU plugin (in Plug-Ins folder)

**User Installation:**
1. Mount DMG
2. Drag app to Applications
3. Manually copy plugins to system folders (if needed)

**Advantages:**
- User-friendly
- No admin password required (for app)
- Easy to distribute

**Disadvantages:**
- Users must manually install plugins
- No automatic uninstall

### PKG (macOS Installer Package)

**Best for:** Enterprise deployment, MDM systems (Jamf, Intune)

**Contents:**
- All components installed to correct system locations
- Post-install scripts (AU cache reset, permissions)

**Installation:**
```bash
# GUI
open OrchestraSynth-0.1.0-macOS-universal.pkg

# Command line (for automation)
sudo installer -pkg OrchestraSynth-0.1.0-macOS-universal.pkg -target /
```

**Advantages:**
- Automatic installation to correct locations
- Works with MDM systems
- Post-install automation
- Can include uninstaller

**Disadvantages:**
- Requires admin password
- Less familiar to some users

### Which Format to Use?

| Use Case | Format | Reason |
|----------|--------|--------|
| Website downloads | DMG | User-friendly, familiar |
| Enterprise/Corporate | PKG | MDM compatible, automated |
| App Store | Neither | Use App Store build process |
| Beta testing | DMG | Quick distribution |
| CI/CD deployment | PKG | Scriptable installation |

**Recommendation:** Provide both formats for maximum flexibility.

---

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Build and Release

on:
  push:
    tags:
      - 'v*'

jobs:
  build-macos:
    runs-on: macos-14
    steps:
      - uses: actions/checkout@v3

      - name: Setup environment
        run: |
          echo "CODESIGN_IDENTITY=${{ secrets.CODESIGN_IDENTITY }}" >> $GITHUB_ENV
          echo "NOTARIZE_APPLE_ID=${{ secrets.NOTARIZE_APPLE_ID }}" >> $GITHUB_ENV
          echo "NOTARIZE_TEAM_ID=${{ secrets.NOTARIZE_TEAM_ID }}" >> $GITHUB_ENV
          echo "NOTARIZE_PASSWORD=${{ secrets.NOTARIZE_PASSWORD }}" >> $GITHUB_ENV
          echo "BUILD_NUMBER=${{ github.run_number }}" >> $GITHUB_ENV

      - name: Import certificates
        run: |
          echo "${{ secrets.DEVELOPER_ID_APPLICATION }}" | base64 -d > cert.p12
          security create-keychain -p actions build.keychain
          security import cert.p12 -k build.keychain -P "${{ secrets.CERT_PASSWORD }}" -T /usr/bin/codesign
          security set-key-partition-list -S apple-tool:,apple: -k actions build.keychain
          security list-keychains -s build.keychain
          export CODESIGN_KEYCHAIN=build.keychain

      - name: Build production packages
        run: ./scripts/macos/production_build.sh

      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: macos-packages
          path: |
            build/macos-universal-release/*.dmg
            build/macos-universal-release/*.pkg

      - name: Create release
        uses: softprops/action-gh-release@v1
        with:
          files: |
            build/macos-universal-release/*.dmg
            build/macos-universal-release/*.pkg
```

### Required Secrets

Store these in your CI/CD system:

- `CODESIGN_IDENTITY` - Developer ID Application certificate name
- `NOTARIZE_APPLE_ID` - Apple ID email
- `NOTARIZE_TEAM_ID` - Team ID
- `NOTARIZE_PASSWORD` - App-specific password
- `PKG_SIGNING_IDENTITY` - Developer ID Installer certificate name
- `DEVELOPER_ID_APPLICATION` - Base64-encoded .p12 certificate
- `CERT_PASSWORD` - Certificate password

---

## Troubleshooting

### Code Signing Issues

**Problem:** `errSecInternalComponent` error

**Solution:**
```bash
# Unlock keychain
security unlock-keychain ~/Library/Keychains/login.keychain-db

# Set keychain search list
security list-keychains -s ~/Library/Keychains/login.keychain-db
```

**Problem:** `identity not found`

**Solution:**
```bash
# List available identities
security find-identity -v -p codesigning

# Use exact name shown
export CODESIGN_IDENTITY="Developer ID Application: Company Name (TEAM123)"
```

### Notarization Failures

**Problem:** `Invalid bundle` error

**Solution:** Ensure hardened runtime is enabled:
```bash
codesign -dv --verbose=4 YourApp.app 2>&1 | grep -i runtime
# Should show: flags=0x10000(runtime)
```

**Problem:** Notarization stuck at "In Progress"

**Solution:**
```bash
# Check status manually
xcrun notarytool history --apple-id "you@company.com" --team-id "TEAM123"

# Get detailed log
xcrun notarytool log <submission-id> --apple-id "you@company.com" --team-id "TEAM123"
```

**Problem:** `Invalid entitlements`

**Solution:**
- Verify entitlements file is valid XML
- Check for typos in entitlement keys
- Ensure no conflicting entitlements

### Build Failures

**Problem:** JUCE download fails

**Solution:**
```bash
# Clear CMake cache
rm -rf build/
rm -rf ~/.cmake/

# Rebuild
./scripts/macos/build_and_package.sh
```

**Problem:** Universal binary not created

**Solution:**
```bash
# Verify architectures
lipo -info build/macos-universal-release/OrchestraSynth.app/Contents/MacOS/OrchestraSynth

# Should show: x86_64 arm64
```

### Validation Warnings

**Warning:** `Bundle ID contains placeholder`

**Solution:** Update in `config/build.env`:
```bash
export BUNDLE_ID_PREFIX="com.yourcompany.orchestrasynth"
```

**Warning:** `Library validation disabled`

**Solution:** This is expected for plugins (required for DAW compatibility).

---

## Security Best Practices

### 1. Protect Credentials

✅ **DO:**
- Store `build.env` in `.gitignore`
- Use keychain for notarization password
- Use environment variables in CI/CD
- Rotate app-specific passwords regularly

❌ **DON'T:**
- Commit credentials to git
- Share app-specific passwords
- Use plain text passwords in scripts

### 2. Hardened Runtime

Always enable hardened runtime (automatically done by scripts):

```bash
codesign --options runtime ...
```

### 3. Minimal Entitlements

Use only the entitlements you need:
- Audio input: Required for audio I/O
- USB: Required for MIDI devices
- Disable library validation: Only for plugins

Avoid:
- JIT compilation (unless required)
- Unsigned executable memory
- DYLD environment variables

### 4. Notarization

Always notarize production builds:
- Required for macOS 10.15+
- Prevents Gatekeeper warnings
- Essential for enterprise trust

### 5. Certificate Management

- Renew certificates before expiration
- Keep certificate private keys secure
- Use separate certificates for development/distribution
- Document certificate locations

---

## Advanced Topics

### Custom Icons

Create a 1024x1024 PNG icon, then convert:

```bash
# Create iconset
mkdir OrchestraSynth.iconset
sips -z 16 16     icon.png --out OrchestraSynth.iconset/icon_16x16.png
sips -z 32 32     icon.png --out OrchestraSynth.iconset/icon_16x16@2x.png
sips -z 32 32     icon.png --out OrchestraSynth.iconset/icon_32x32.png
sips -z 64 64     icon.png --out OrchestraSynth.iconset/icon_32x32@2x.png
sips -z 128 128   icon.png --out OrchestraSynth.iconset/icon_128x128.png
sips -z 256 256   icon.png --out OrchestraSynth.iconset/icon_128x128@2x.png
sips -z 256 256   icon.png --out OrchestraSynth.iconset/icon_256x256.png
sips -z 512 512   icon.png --out OrchestraSynth.iconset/icon_256x256@2x.png
sips -z 512 512   icon.png --out OrchestraSynth.iconset/icon_512x512.png
sips -z 1024 1024 icon.png --out OrchestraSynth.iconset/icon_512x512@2x.png

# Convert to ICNS
iconutil -c icns OrchestraSynth.iconset

# Move to resources
mv OrchestraSynth.icns resources/icon.icns
```

### DMG Background Image

Create a 540x380 PNG background:

```bash
# Save as resources/dmg_background.png
# CMakeLists.txt will automatically use it
```

### Version Management

Update version in `CMakeLists.txt`:

```cmake
project(OrchestraSynth VERSION 1.0.0 LANGUAGES C CXX)
```

Set build number:

```bash
export BUILD_NUMBER="$(date +%Y%m%d%H%M%S)"
```

Full version will be: `1.0.0.20240315120000`

---

## Support and Resources

### Documentation

- [Apple Code Signing Guide](https://developer.apple.com/support/code-signing/)
- [Notarization Documentation](https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution)
- [JUCE Documentation](https://docs.juce.com/)

### Tools

- **notarytool** - Notarization CLI tool
- **codesign** - Code signing tool
- **pkgutil** - PKG inspection tool
- **spctl** - Gatekeeper policy tool

### Common Commands

```bash
# Verify code signature
codesign --verify --deep --strict YourApp.app

# Check Gatekeeper status
spctl -a -vv -t install YourApp.dmg

# Inspect PKG contents
pkgutil --payload-files YourApp.pkg

# Check notarization status
xcrun notarytool history --apple-id you@company.com
```

---

## Checklist for First Production Build

- [ ] Install Xcode Command Line Tools
- [ ] Join Apple Developer Program
- [ ] Download Developer ID certificates
- [ ] Generate app-specific password
- [ ] Store password in keychain
- [ ] Copy and configure `build.env`
- [ ] Update bundle IDs in CMakeLists.txt
- [ ] Update plugin manufacturer codes
- [ ] Add icon file (optional)
- [ ] Test build with `production_build.sh`
- [ ] Validate with `validate.sh`
- [ ] Test installation on clean system
- [ ] Document distribution process

---

**Last Updated:** 2024
**Version:** 1.0.0

For issues or questions, consult the [Operations Manual](OPERATIONS_MANUAL.md) or [Apple Packaging Guide](APPLE_PACKAGING.md).

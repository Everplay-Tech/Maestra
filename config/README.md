# Configuration Directory

This directory contains configuration files for production macOS packaging.

## Files

### `build.env.example`

Template for build environment configuration. Copy this to `build.env` and customize:

```bash
cp config/build.env.example config/build.env
```

**Important:** `build.env` is in `.gitignore` and should never be committed (contains credentials).

### `entitlements/`

macOS entitlements files that define app permissions:

- **`app.entitlements`** - Standalone application entitlements
- **`plugin.entitlements`** - Audio plugin entitlements (VST3/AU)

#### Key Entitlements

Both files include:
- Audio device access (`com.apple.security.device.audio-input`)
- USB/MIDI access (`com.apple.security.device.usb`)
- Bluetooth MIDI (`com.apple.security.device.bluetooth`)
- User file access (for presets/projects)

Plugin entitlements additionally include:
- Library validation disabled (required for some DAW compatibility)

## Usage

1. **First time setup:**
   ```bash
   cp config/build.env.example config/build.env
   ```

2. **Edit configuration:**
   ```bash
   nano config/build.env
   # or
   code config/build.env
   ```

3. **Load environment:**
   ```bash
   source config/build.env
   ```

4. **Build:**
   ```bash
   ./scripts/macos/production_build.sh
   ```

## Security Notes

- Never commit `build.env` to version control
- Store notarization passwords in keychain (recommended)
- Rotate app-specific passwords regularly
- Keep certificate private keys secure

## Documentation

See [PRODUCTION_PACKAGING.md](../docs/PRODUCTION_PACKAGING.md) for complete documentation.

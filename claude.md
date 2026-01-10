# Claude Code Configuration

## Target Board
- **Primary test board:** ESP32-S3 (`esp32-s3-devkitc-1-n16r8v-LED-32x8`)

## Granted Permissions
- Read/write files in this repository
- Execute bash commands
- Git operations (commit, branch, etc.)
- Build firmware with PlatformIO

## Flashing Firmware
**IMPORTANT:** Always use `-t upload` to flash the device. A plain `pio run` only builds without flashing.

### First-time or clean flash (recommended):
```bash
# Step 1: Erase and upload firmware (includes bootloader, partition table, OTA data, factory, and app)
pio run -e esp32-s3-devkitc-1-n16r8v-LED-32x8 -t erase -t upload

# Step 2: Upload filesystem (REQUIRED - contains web UI, captive portal, icons, etc.)
pio run -e esp32-s3-devkitc-1-n16r8v-LED-32x8 -t uploadfs
```
**Both steps are required!** The firmware will not work properly without the filesystem.

### Regular upload (after initial flash):
```bash
# Upload firmware only (use when code changes but web files haven't changed)
pio run -e esp32-s3-devkitc-1-n16r8v-LED-32x8 -t upload

# Upload filesystem only (use when web files change but code hasn't changed)
pio run -e esp32-s3-devkitc-1-n16r8v-LED-32x8 -t uploadfs
```

### Build only (no flash):
```bash
pio run -e esp32-s3-devkitc-1-n16r8v-LED-32x8
```

### Flash layout (16MB partition table):
| Address    | Content              |
|------------|----------------------|
| 0x00000000 | Bootloader           |
| 0x00008000 | Partition table      |
| 0x0000e000 | OTA data             |
| 0x00010000 | Factory partition    |
| 0x000e0000 | App (OTA_0)          |
| 0x00c90000 | Filesystem (LittleFS)|

## AP Mode / Captive Portal

### Automatic AP Mode
The device **automatically starts in AP mode** when no WiFi credentials are configured (e.g., after a fresh flash). No button press required.

### Manual AP Mode
To force AP mode when WiFi credentials are already configured:
- Hold **GPIO 4** to GND during boot

### Connecting
1. Connect to the `pixelix-XXXXXXXX` WiFi network
2. The captive portal will appear for WiFi configuration
3. Enter your WiFi SSID and passphrase
4. Click "Restart" to connect to your network

## Troubleshooting

When debugging issues, always consider whether **firmware**, **filesystem**, or **both** need to be reflashed:

| Symptom | Likely Cause | Solution |
|---------|--------------|----------|
| Web pages not loading / 404 errors | Missing filesystem | `pio run -t uploadfs` |
| Captive portal shows wrong page | Missing/outdated filesystem | `pio run -t uploadfs` |
| API endpoints not working | Outdated firmware | `pio run -t upload` |
| New features not appearing | Outdated firmware and/or filesystem | Reflash both |
| Device boots to factory/updater | OTA data pointing to wrong partition | `pio run -t upload` (rewrites OTA data) |
| Complete malfunction after erase | Missing firmware or filesystem | Reflash both (firmware first, then filesystem) |

**After a flash erase (`-t erase`), ALWAYS reflash both firmware AND filesystem.**

## Notes
- Build artifacts go to `.pio/build/`
- Release binaries can be copied to `release/` folder
- The `release/` folder is in `.gitignore`
- Web UI files are in `data/` folder and get built into the filesystem image

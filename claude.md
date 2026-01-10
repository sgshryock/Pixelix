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
To enter AP mode for WiFi configuration:
- Hold **GPIO 4** to GND during boot
- Connect to the `pixelix-XXXXXXXX` WiFi network
- The captive portal will appear for WiFi configuration

## Notes
- Build artifacts go to `.pio/build/`
- Release binaries can be copied to `release/` folder
- The `release/` folder is in `.gitignore`
- Web UI files are in `data/` folder and get built into the filesystem image

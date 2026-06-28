# JanOS Pancake + ESP32 Marauder Dual Boot Summary

## Changes Made

### Partition Scheme — `pancake/ESP32C5/partitions.csv`
Replaced the single `factory` partition with a proper dual-boot OTA layout:

| Partition | Type        | Offset   | Size   | Purpose                      |
|-----------|-------------|----------|--------|------------------------------|
| nvs       | data/nvs    | 0x9000   | 24KB   | Settings (NVS)               |
| phy_init  | data/phy    | 0xF000   | 4KB    | RF calibration               |
| otadata   | data/ota    | 0x10000  | 8KB    | Boot selector                |
| ota_0     | app/ota_0   | 0x20000  | 3.4MB  | JanOS Pancake                |
| ota_1     | app/ota_1   | 0x390000 | 3.4MB  | ESP32 Marauder               |
| spiffs    | data/spiffs | 0x700000 | 256KB  | Marauder settings (JSON)     |
| storage   | data/fat    | 0x740000 | 768KB  | JanOS file storage           |

Both apps compile to the same virtual address — the bootloader's MMU maps whichever
OTA partition is active. No special compile-time offsets needed.

---

### Pancake `main/main.c`
- Added `#include "esp_ota_ops.h"`
- Added **Marauder** tile (`LV_SYMBOL_POWER`, `UI_ACCENT_PINK`) to `show_main_tiles()`
- Added handler in `main_tile_event_cb()` that finds `ota_1`, calls
  `esp_ota_set_boot_partition()`, then `esp_restart()`

### Pancake `main/CMakeLists.txt`
- Added `app_update` to REQUIRES so the OTA API links correctly

### Marauder `esp32_marauder/MenuFunctions.cpp`
- Added `#include "esp_ota_ops.h"` at the top
- Added **JanOS** menu node (`TFTMAGENTA`, `FLIPPER` icon) to `mainMenu` in
  `RunSetup()`, boots `ota_0`

---

## Flashing

Flash each component to the correct offset:

| Binary              | Offset    |
|---------------------|-----------|
| bootloader.bin      | 0x2000    |
| partition-table.bin | 0x8000    |
| JanOS Pancake .bin  | 0x20000   |
| Marauder .bin       | 0x390000  |

Example esptool command:
```
esptool.py --chip esp32c5 --port <PORT> --baud 921600 write_flash -z \
  0x2000   bootloader.bin \
  0x8000   partition-table.bin \
  0x20000  janos_pancake.bin \
  0x390000 esp32_marauder.bin
```

---

## Compiling Marauder (Arduino IDE)

### 1. Enable the Pancake target in `configs.h`

Line 36 — uncomment this:
```cpp
#define MARAUDER_PANCAKE
```
Comment out any other board define above it.

### 2. Set the TFT_eSPI display config in `User_Setup_Select.h`

Line 40 — uncomment this:
```cpp
#include <User_Setup_marauder_pancake.h>
```
Make sure all other `#include <User_Setup_...>` lines in that file are commented out.

### 3. Arduino IDE board settings

| Setting           | Value                |
|-------------------|----------------------|
| Board             | ESP32C5 Dev Module   |
| CPU Frequency     | 160 MHz              |
| Flash Mode        | QIO                  |
| Flash Frequency   | 80 MHz               |
| Flash Size        | 8MB (64Mb)           |
| Partition Scheme  | Custom (see below)   |
| PSRAM             | OPI PSRAM            |
| Upload Speed      | 921600               |
| Core Debug Level  | None                 |

### 4. Custom partition scheme

Place a `partitions.csv` directly inside the sketch folder (`esp32_marauder/`) with
the same dual-boot layout used by the pancake project. Arduino IDE will detect and
use it automatically when "Custom" is selected for the partition scheme:

```
# Name,   Type, SubType, Offset,   Size,     Flags
nvs,      data, nvs,     0x9000,   0x6000,
phy_init, data, phy,     0xF000,   0x1000,
otadata,  data, ota,     0x10000,  0x2000,
ota_0,    app,  ota_0,   0x20000,  0x370000,
ota_1,    app,  ota_1,   0x390000, 0x370000,
storage,  data, fat,     0x700000, 0x100000,
```

### 5. After compiling

Do **not** use the normal Upload button — it flashes to `0x10000` (old factory offset).
Instead use **Sketch → Export Compiled Binary** to get the `.bin`, then flash it
manually to `0x390000` (ota_1) as part of the full dual-boot flash command above.

---

## Compiling JanOS (ESP-IDF)

### Prerequisites

- **ESP-IDF v6.0** — the sdkconfig is locked to this version. Install via the
  [Espressif installer](https://dl.espressif.com/dl/esp-idf/) or the ESP-IDF VS Code extension.

### Build

Open an **ESP-IDF terminal** (added to Start menu by the installer on Windows), then:

```bash
cd "pancake/ESP32C5"
idf.py set-target esp32c5
idf.py build
```

`set-target` only needs to be run once. After that, `idf.py build` is all you need.

### Key settings (already configured in `sdkconfig`)

| Setting         | Value                   |
|-----------------|-------------------------|
| Target          | esp32c5                 |
| Flash size      | 8MB                     |
| Flash mode      | **DIO** (not QIO)       |
| Flash frequency | 80 MHz                  |
| Partition table | Custom (`partitions.csv`) |
| PSRAM           | Quad mode, 40 MHz       |
| Monitor baud    | 115200                  |

> Flash mode is **DIO** for JanOS, unlike Marauder which uses QIO. Don't mix these up.

### Output binary

After a successful build, the post-build script automatically copies the binary to:
```
pancake/ESP32C5/binaries-esp32c5/projectZero.bin
```

### Flashing for dual-boot

Do **not** use `idf.py flash` — it flashes to the old single-app offset. Flash
manually to `ota_0` instead:

```bash
esptool.py --chip esp32c5 --port <PORT> --baud 921600 write_flash -z \
  0x2000   build/bootloader/bootloader.bin \
  0x8000   build/partition_table/partition-table.bin \
  0x20000  build/projectZero.bin
```

Or combine with the Marauder binary in the full dual-boot flash command above.

---

## How Boot Switching Works

- Tapping **Marauder** in JanOS sets `ota_1` as the boot partition and restarts.
- Tapping **JanOS** in Marauder sets `ota_0` as the boot partition and restarts.
- The ESP-IDF bootloader reads `otadata` at `0x10000` to decide which app to run.
- If `otadata` is blank/erased, the bootloader defaults to `ota_0` (JanOS Pancake).

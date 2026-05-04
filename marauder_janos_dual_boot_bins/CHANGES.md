# Dual Boot Changes Log

All modifications made to both firmwares to support dual-boot between
JanOS Pancake (ota_0) and ESP32 Marauder (ota_1).

---

## Partition Tables

Both files are identical and must stay in sync:
- `pancake/ESP32C5/partitions.csv`
- `ESP32Marauder-DualBoot/esp32_marauder/partitions.csv`

### What changed
Replaced the original single `factory` partition scheme with a dual-boot OTA layout.

### Original (`pancake/ESP32C5/partitions.csv`)
```
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x700000,
storage,  data, fat,     0x710000, 0xF0000,
```

### New (both files)
```
nvs,      data, nvs,     0x9000,   0x6000,
phy_init, data, phy,     0xF000,   0x1000,
otadata,  data, ota,     0x10000,  0x2000,
ota_0,    app,  ota_0,   0x20000,  0x370000,
ota_1,    app,  ota_1,   0x390000, 0x370000,
spiffs,   data, spiffs,  0x700000, 0x40000,
storage,  data, fat,     0x740000, 0xC0000,
```

### Why
- `factory` replaced with `ota_0` + `ota_1` so the bootloader can switch between
  JanOS and Marauder via the OTA mechanism
- `otadata` at `0x10000` is read by the bootloader to decide which app to run;
  if blank it defaults to `ota_0` (JanOS)
- `spiffs` added at `0x700000` because Marauder stores all settings as
  `/settings.json` on SPIFFS — without this partition `SPIFFS.begin()` fails
  and the settings system is completely non-functional
- `storage` (FAT) shifted to `0x740000` and reduced from 1MB to 768KB to make
  room for the SPIFFS partition

---

## JanOS Pancake

### `pancake/ESP32C5/main/CMakeLists.txt`

**What changed:** Added two components to the REQUIRES list.

| Component   | Added by | Reason                                      |
|-------------|----------|---------------------------------------------|
| `app_update`| Claude   | Provides `esp_ota_ops.h` for boot switching |
| `esp_adc`   | User     | ADC support for battery monitoring          |

**Current REQUIRES line:**
```cmake
idf_component_register(... REQUIRES heap driver esp_adc esp_event console bt esp-tls app_update ...)
```

---

### `pancake/ESP32C5/main/main.c`

#### 1. Added include
```c
// OTA (dual-boot partition switching)
#include "esp_ota_ops.h"
```
Added after the `esp_tls.h` include block (~line 63).

#### 2. Added Marauder tile to `show_main_tiles()` (~line 9878)
```c
create_tile(tiles_container, LV_SYMBOL_POWER, "Marauder", UI_ACCENT_PINK, main_tile_event_cb, "Marauder");
```
Added after the existing Bluetooth tile. Uses `UI_ACCENT_PINK` (`0xE91E63`).

#### 3. Added handler in `main_tile_event_cb()` (~line 9715)
```c
} else if (strcmp(tile_name, "Marauder") == 0) {
    const esp_partition_t *marauder = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    if (marauder) {
        esp_ota_set_boot_partition(marauder);
    }
    esp_restart();
}
```
Finds the `ota_1` partition (Marauder), sets it as the next boot partition,
and restarts. If `ota_1` is not found the device still restarts into JanOS.

---

## ESP32 Marauder

### `ESP32Marauder-DualBoot/esp32_marauder/partitions.csv` *(new file)*

Created in the sketch folder so Arduino IDE uses it when "Custom" is selected
as the partition scheme. Content is identical to the pancake partitions.csv above.

---

### `ESP32Marauder-DualBoot/esp32_marauder/MenuFunctions.cpp`

#### 1. Added include
```cpp
#include "esp_ota_ops.h"
```
Added at the top of the file after the existing includes (~line 3).
Available in Arduino ESP32 core as part of the ESP-IDF integration.

#### 2. Added JanOS menu node to `RunSetup()` (~line 1625)
```cpp
this->addNodes(&mainMenu, "JanOS", TFTMAGENTA, NULL, REBOOT, []() {
    const esp_partition_t *janos = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (janos) {
      esp_ota_set_boot_partition(janos);
    }
    ESP.restart();
});
```
Added after the existing Reboot node in the main menu.

| Property | Value |
|----------|-------|
| Label    | `"JanOS"` |
| Color    | `TFTMAGENTA` (defined as `8` in `configs.h`) |
| Icon     | `REBOOT` (defined as `14` in `MenuFunctions.h`) |
| Action   | Sets `ota_0` as boot partition and restarts |

---

### `ESP32Marauder-DualBoot/esp32_marauder/SDInterface.h`

Added `target_label` parameter (default `"ota_1"`) to both public functions:
```cpp
void runUpdate(String file_name = "", const char* target_label = "ota_1");
void performUpdate(Stream &updateSource, size_t updateSize, const char* target_label = "ota_1");
```

---

### `ESP32Marauder-DualBoot/esp32_marauder/SDInterface.cpp`

#### Problem
Marauder's "Update Firmware" feature uses `esp_ota_get_next_update_partition(NULL)`
which always returns the partition that is NOT currently running. Since Marauder runs
from `ota_1`, this returns `ota_0` — meaning a firmware update would silently
overwrite JanOS.

#### Fix — `performUpdate()`
`Update.begin()` now passes `target_label` so the write goes to the correct partition:
```cpp
if (Update.begin(updateSize, U_FLASH, -1, LOW, target_label)) {
```

#### Fix — `runUpdate()`
- `target_label` is passed through to `performUpdate()`
- Boot partition is always set back to `ota_1` (Marauder) after any update:
  - If Marauder was updated → reboots into new Marauder
  - If JanOS was updated → stays in Marauder, user switches manually via JanOS button

```cpp
// Always boot back to Marauder after any update
const esp_partition_t *marauder = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
esp_ota_set_boot_partition(marauder);
ESP.restart();
```

#### Why overwriting the running partition works
The ESP32 executes code from cache, not directly from flash, so `ota_1` can be
erased and rewritten while Marauder is still running from it. After `ESP.restart()`
the bootloader loads the freshly written image from `ota_1`.

---

### `ESP32Marauder-DualBoot/esp32_marauder/MenuFunctions.h`

Changed `buildSDFileMenu(bool update = false)` to `buildSDFileMenu(int mode = 0)`:
- `mode 0` — delete SD files (unchanged behaviour)
- `mode 1` — update Marauder via `runUpdate(..., "ota_1")`
- `mode 2` — update JanOS via `runUpdate(..., "ota_0")`

`true` still evaluates as `mode 1` so any unmodified callers are unaffected.

---

### `ESP32Marauder-DualBoot/esp32_marauder/MenuFunctions.cpp`

#### `buildSDFileMenu()` changes
- Signature changed from `bool update` to `int mode`
- `update = (mode >= 1)` preserves existing file-list filtering logic
- Menu title now shows `"Update Marauder"` or `"Update JanOS"` instead of `"Bin Files"`
- Mode 2 file items are coloured `TFTMAGENTA` to distinguish them from Marauder updates

#### Device menu — "Update JanOS" item
Added after the existing "Update Firmware" node:
```cpp
this->addNodes(&deviceMenu, "Update JanOS", TFTMAGENTA, NULL, SD_UPDATE, [this]() {
    display_obj.clearScreen();
    ...
    this->buildSDFileMenu(2);
    this->changeMenu(&sdDeleteMenu, true);
});
```
Both "Update Firmware" and "Update JanOS" only appear when an SD card is present
(`sd_obj.supported`).

---

## Boot Switching Summary

| From     | Action                  | Partition written | Boot after restart |
|----------|-------------------------|-------------------|--------------------|
| JanOS    | Tap Marauder tile       | —                 | ota_1 (Marauder)   |
| Marauder | Tap JanOS node          | —                 | ota_0 (JanOS)      |
| Marauder | Update Firmware (SD)    | ota_1             | ota_1 (Marauder)   |
| Marauder | Update JanOS (SD)       | ota_0             | ota_1 (Marauder)   |

Both boot switches use `esp_ota_set_boot_partition()` followed by a restart.
The ESP-IDF bootloader reads `otadata` at `0x10000` on every boot to determine
which app partition to map and execute.

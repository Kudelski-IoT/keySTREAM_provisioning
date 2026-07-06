# ESP32 Firmware — `keystream_ble`

This firmware runs on an **ESP32** target and provides BLE-based KTA provisioning using the Keystream security stack and CryptoAuthLib (ATECC608).

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Installing ESP-IDF](#installing-esp-idf)
3. [Activating the ESP-IDF Environment](#activating-the-esp-idf-environment)
4. [First-Time Python Environment Setup](#first-time-python-environment-setup)
5. [Building the Firmware](#building-the-firmware)
6. [Flashing the Firmware](#flashing-the-firmware)
7. [Full Clean + Rebuild](#full-clean--rebuild)
8. [Troubleshooting](#troubleshooting)

---

## Prerequisites

| Requirement | Version | Notes |
|---|---|---|
| Windows 10/11 | — | Linux/macOS also supported by ESP-IDF |
| [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) | **v5.3.1** | Installed via the Espressif Windows installer |
| Python | 3.12 | Bundled inside the ESP-IDF installer |
| CMake | 3.24+ | Bundled inside the ESP-IDF installer |
| Git | Any | Required by ESP-IDF |
| USB-to-Serial driver | — | CP210x or CH340 driver for your board |

---

## Installing ESP-IDF

### Option A — Windows Installer (Recommended)

1. Download the **ESP-IDF Windows Installer** from the official Espressif page:  
   👉 https://dl.espressif.com/dl/esp-idf/

2. Run the installer (`esp-idf-tools-setup-online-*.exe`).

3. Select **ESP-IDF v5.3.1** when prompted for the version.

4. Let the installer finish. It will place everything under:
   ```
   C:\Espressif\
   ├── frameworks\esp-idf-v5.3.1\   ← IDF source & tools
   ├── python_env\idf5.3_py3.12_env\ ← Python virtual environment
   └── tools\                        ← xtensa toolchain, cmake, ninja, etc.
   ```

5. The installer also creates a shortcut **"ESP-IDF v5.3 PowerShell"** in the Start Menu — use that shortcut for a pre-activated shell.

### Option B — Manual (git clone)

```powershell
git clone --recursive https://github.com/espressif/esp-idf.git C:\Espressif\frameworks\esp-idf-v5.3.1
cd C:\Espressif\frameworks\esp-idf-v5.3.1
git checkout v5.3.1
git submodule update --init --recursive
.\install.ps1 esp32
```

---

## Activating the ESP-IDF Environment

Every new PowerShell session requires sourcing the export script before running `idf.py`.

```powershell
# Run this once at the start of every terminal session
C:\Espressif\frameworks\esp-idf-v5.3.1\export.ps1
```

You should see output ending with:
```
Done! You can now compile ESP-IDF projects.
```

> **Tip:** The "ESP-IDF v5.3 PowerShell" Start Menu shortcut does this automatically.

---

## First-Time Python Environment Setup

If you see this error when activating:

```
ERROR: C:\Espressif\python_env\idf5.3_py3.12_env\Scripts\python.exe doesn't exist!
Please run the install script or "idf_tools.py install-python-env"
```

Run the following command **once** to create the Python virtual environment:

```powershell
python.exe C:\Espressif\frameworks\esp-idf-v5.3.1\tools\idf_tools.py install-python-env
```

Wait for it to finish installing all packages, then re-run `export.ps1`.

---

## Building the Firmware

```powershell
# 1. Navigate to the firmware directory
cd C:\ESP32_Mobile_APP_X\firmware

# 2. Activate ESP-IDF (if not already done)
C:\Espressif\frameworks\esp-idf-v5.3.1\export.ps1

# 3. Build
idf.py build
```

A successful build ends with:
```
Project build complete. To flash, run:
 idf.py flash
```

The compiled binary is placed at:
```
firmware/build/keystream_ble.bin
```

---

## Flashing the Firmware

1. Connect the ESP32 board via USB.
2. Identify the COM port in Device Manager (e.g. `COM9`).

```powershell
# Flash to a specific port
idf.py -p COM9 flash

# Flash and open serial monitor immediately
idf.py -p COM9 flash monitor
```

To find available COM ports:
```powershell
[System.IO.Ports.SerialPort]::GetPortNames()
```

---

## Full Clean + Rebuild

Use `fullclean` when you want to start from scratch (removes all build artifacts):

```powershell
cd C:\ESP32_Mobile_APP_X\firmware
C:\Espressif\frameworks\esp-idf-v5.3.1\export.ps1
idf.py fullclean
idf.py build
idf.py -p COM9 flash
```

> `fullclean` also removes the `managed_components` folder. The next build will re-download any IDF component manager dependencies.

---

## Troubleshooting

### `idf.py` is not recognized

**Cause:** The ESP-IDF environment is not activated in the current terminal.  
**Fix:** Run the export script first:
```powershell
C:\Espressif\frameworks\esp-idf-v5.3.1\export.ps1
```

---

### Python environment missing (`python.exe doesn't exist`)

**Cause:** The Python virtual environment was not created (e.g., the installer was interrupted).  
**Fix:**
```powershell
python.exe C:\Espressif\frameworks\esp-idf-v5.3.1\tools\idf_tools.py install-python-env
```

---

### `ninja failed` — multiple definition of `salSignHash`

**Cause:** A stub definition of `salSignHash` in `main/main.c` conflicts with the real implementation in `kta_provisioning/SOURCE/salapi/k_sal_crypto.c`.  
**Fix:** Remove the stub from `main.c` — only the `#include "k_sal_crypto.h"` line should remain.

---

### `undefined reference to 'ble_sal_enqueue_rx'`

**Cause:** The BLE backend/SAL source files (`backend_ble.c`, `ble_sal.c`) are not listed in `main/CMakeLists.txt`.  
**Fix:** In `main/CMakeLists.txt`, ensure these lines are **not** commented out inside `MAIN_SRCS`:
```cmake
"${MCU_BACKEND_PATH}/backend_${MCU_BACKEND}.c"
"${MCU_SAL_PATH}/${MCU_BACKEND}_sal.c"
```

---

### `mingw32-make: No targets specified and no makefile found`

**Cause:** The CMakeLists.txt `mcu_bridge_build` custom target tries to run `make` in the `kta_provisioning/mcu/` directory, but no `Makefile` exists there.  
**Fix:** In `main/CMakeLists.txt`, the guard `elseif(NOT EXISTS "${MCU_DIR}/Makefile")` skips the custom target automatically. If you see this error, make sure you have the latest version of `CMakeLists.txt`.

---

### Serial port not found / flashing fails

- Check Device Manager → Ports (COM & LPT) to confirm the COM port number.
- Install the correct USB driver:
  - **CP2102** boards → [Silicon Labs CP210x driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
  - **CH340** boards → [CH340 driver](http://www.wch-ic.com/downloads/CH341SER_EXE.html)
- Try a different USB cable (some cables are charge-only).
- Hold the **BOOT** button on the ESP32 while initiating the flash.

---

### Build succeeds but device doesn't run / crashes at boot

- Open the serial monitor to see the boot log:
  ```powershell
  idf.py -p COM9 monitor
  ```
- Press `Ctrl+]` to exit the monitor.
- A reset loop usually means a stack overflow or missing NVS partition — run `idf.py -p COM9 erase_flash` then flash again.

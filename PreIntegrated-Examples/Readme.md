# keySTREAM Pre-Integrated Examples

This directory contains a PowerShell configuration script (`Config.ps1`) and a set of pre-integrated firmware examples that automate provisioning of keySTREAM-enabled devices.

`Config.ps1` copies the correct KTA library sources, patches `ktaConfig.h`, and outputs the path to the ready-to-open project file for your selected device.

**Supported platforms:**

| Target | Board | Build tool | Transport |
|---|---|---|---|
| `D21_X` | ATSAMD21 (EV10E69A kit) | MPLAB X | Wi-Fi (AWS FreeRTOS) |
| `SAME54_X` | ATSAME54 (E54 kit) | MPLAB X | Ethernet (AWS IoT Core) |
| `SG41_X` | PIC32CX-SG41 (WINCS02) | MPLAB X | Wi-Fi — dual-app: FOTA + Boot |
| `SG41_UART_X` | PIC32CX-SG41 (EV89U05A) | MPLAB X + Windows exe | UART closed-network gateway |
| `ESP32_BLE_CLI_X` | ESP32 + ATECC608 | ESP-IDF | BLE (Windows Python CLI) |
| `ESP32_Mobile_APP_X` | ESP32 + ATECC608 | ESP-IDF | BLE (Mobile App) |

See also: the offline documentation (`docs/index.html` → **Fleet Profile Creation Guide**) for the full keySTREAM account and fleet profile setup walkthrough.

---

## Prerequisites

| Requirement | Notes |
|---|---|
| **Windows PC** | Windows 10 or 11 |
| **PowerShell** | Built-in on Windows 10/11 |
| **MPLAB X IDE** | v6.15 or above — required for `D21_X`, `SAME54_X`, `SG41_X`, `SG41_UART_X` |
| **ESP-IDF v5.3.1** | Required for `ESP32_BLE_CLI_X` and `ESP32_Mobile_APP_X` |
| **keySTREAM portal account** | Free registration at [mc.obp.iot.kudelski.com](https://mc.obp.iot.kudelski.com) |
| **Fleet Profile UID** | Obtained from the keySTREAM portal (see offline documentation `docs/index.html` → **Fleet Profile Creation Guide**) |

---

## Important Notes

- **SG41_X (WINCS02):**  
  The SG41_X board uses a WINCS02 Wi-Fi module. Open the serial monitor on the board's COM port before powering on. If Wi-Fi does not connect, reset the board and try again.  
  To avoid path-length issues on Windows, place the repository at a short root path such as `C:\ks\` before running `Config.ps1`.

- **SAME54_X long path on Windows:**  
  The generated SAME54_X project path can exceed Windows' maximum path length (260 chars). If you encounter build or file-copy errors, move the repository to a short root path such as `C:\ks\` and re-run `Config.ps1`.  
  For the full ATSAME54-XPRO + AWS IoT Core walkthrough, see the offline documentation (`docs/index.html` → **Pre-Integrated Examples → ATSAME54-XPRO + AWS IoT Core**).

- **SG41_UART_X — closed-network gateway:**  
  This example pairs a PIC32CX-SG41 MCU (no cloud connectivity on the MCU itself) with a Windows gateway executable that handles all HTTPS communication to keySTREAM. For full build and run instructions, see the offline documentation (`docs/index.html` → **Pre-Integrated Examples → UART Transport Integration - EV89U05A**).  
  To avoid path-length issues on Windows, place the repository at a short root path such as `C:\ks\` before running `Config.ps1`.

- **ESP32 targets — ESP-IDF, not MPLAB X:**  
  `ESP32_BLE_CLI_X` and `ESP32_Mobile_APP_X` are ESP-IDF projects built with `idf.py build`. They are **not** opened in MPLAB X IDE. See the [ESP32 Mobile App firmware README](./TrustMANAGER/ESP32_Mobile_APP_X/firmware/README.md) for full ESP-IDF build instructions.  
  To avoid path-length issues on Windows, place the repository at a short root path such as `C:\ks\` before running `Config.ps1`.

---

## Step-by-Step Instructions

### 1. Download or Clone the Repository

Clone or extract the repository to any location on your PC, for example:
```
C:\Projects\keySTREAM_provisioning\
```

### 2. Open PowerShell

Press `Win + S`, type **PowerShell**, and press `Enter`.

### 3. Navigate to the Script Directory

```powershell
cd "C:\Projects\keySTREAM_provisioning\PreIntegrated-Examples"
```

### 4. Run the Configuration Script

`Config.ps1` guides you through selecting your TrustMANAGER folder and target device, then automatically copies all required KTA library sources and patches configuration headers.

**Set execution policy (once per machine):**
```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

**Run the script:**
```powershell
.\Config.ps1
```

**Alternative if the above is blocked by policy:**
```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\Config.ps1
```

**Revert after use (optional):**
```powershell
Set-ExecutionPolicy -Scope CurrentUser Restricted
```

### 5. Script Prompts

The script will ask you to:

1. **Select a TrustMANAGER folder** — choose the folder that contains your device sub-folders (e.g., `TrustMANAGER`).
2. **Select a device** — choose one of:
   - `D21_X`
   - `SAME54_X`
   - `SG41_X`
   - `SG41_UART_X`
   - `ESP32_BLE_CLI_X`
   - `ESP32_Mobile_APP_X`

The script copies the KTA sources, applies the required patches, and prints the path to the project file to open next.

---

### 6. Configure the Fleet Profile UID

After the script runs, set your **Fleet Profile Public UID** in the configuration header for your device.

> **Where to get it:** Log in to the [keySTREAM portal](https://mc.obp.iot.kudelski.com), navigate to **Fleet & PKI → Fleet & Cert**, and copy the Fleet Profile Public UID. See the offline documentation (`docs/index.html` → **Fleet Profile Creation Guide**) for the full walkthrough.

| Device | Configuration file | Macro to set |
|---|---|---|
| `D21_X` | `TrustMANAGER\D21_X\keystream_connect\App_Config.h` | `KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID` |
| `SAME54_X` | `TrustMANAGER\SAME54_X\keystream_connect\App_Config.h` | `KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID` |
| `SG41_X` | `TrustMANAGER\SG41_X\keystream_connect\fota_app_config.h` | `KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID` |
| `SG41_UART_X` | `TrustMANAGER\SG41_UART_X\App_Config.h` | `KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID` |
| `ESP32_BLE_CLI_X` | `TrustMANAGER\ESP32_BLE_CLI_X\App_Config.h` | `KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID` |
| `ESP32_Mobile_APP_X` | *(managed by the mobile app at runtime)* | — |

---

### 7. Configure Application-Specific Settings

Update the connection and application settings for your device:

#### D21_X
- **File:** `TrustMANAGER\D21_X\keystream_connect\App_Config.h`
- **Settings:** Wi-Fi SSID and password, AWS endpoint

#### SAME54_X
- **File:** `TrustMANAGER\SAME54_X\keystream_connect\App_Config.h`
- **Settings:** AWS endpoint, AWS Thing Name
- **AWS setup guide:** offline documentation (`docs/index.html` → **Pre-Integrated Examples → ATSAME54-XPRO + AWS IoT Core**)

#### SG41_X
- **File:** `TrustMANAGER\SG41_X\keystream_connect\fota_app_config.h`
- **Settings:** Wi-Fi SSID and password
- **Note:** `SG41_X` uses `fota_app_config.h` (not `App_Config.h`) because it contains two applications: `keystream_fota` and `keystream_boot`.

#### SG41_UART_X
- **File:** `TrustMANAGER\SG41_UART_X\App_Config.h`
- **Settings:** UART COM port (`UART_COM_PORT` / `UART_COM_PORT_STR`)
- **Full guide:** offline documentation (`docs/index.html` → **Pre-Integrated Examples → UART Transport Integration - EV89U05A**)

#### ESP32_BLE_CLI_X
- **File:** `TrustMANAGER\ESP32_BLE_CLI_X\App_Config.h`
- **Settings:** Fleet Profile UID
- **Python CLI:** `TrustMANAGER\ESP32_BLE_CLI_X\windows_ble_cli.py`

#### ESP32_Mobile_APP_X
- **Files:** `TrustMANAGER\ESP32_Mobile_APP_X\firmware\` (ESP-IDF project)
- **Settings:** Configured via the paired mobile application at runtime
- **Build guide:** [ESP32 firmware README](./TrustMANAGER/ESP32_Mobile_APP_X/firmware/README.md)

---

### 8. Open and Build the Project

After `Config.ps1` completes, the script prints the full path to your project.

#### MPLAB X targets (D21_X, SAME54_X, SG41_X, SG41_UART_X)

The script output will look like:

```
=====================================================================
Next Step:
Please open the following .X project file in MPLAB X IDE to continue:

    <path-to-repo>\TrustMANAGER\D21_X\keystream_connect\firmware\d21_aws\keystream_aws_d21.X

=====================================================================
```

Open the displayed `.X` file in **MPLAB X IDE**, then use **Clean and Build** (`Shift+F11`).

| Device | MPLAB X project path |
|---|---|
| `D21_X` | `TrustMANAGER\D21_X\keystream_connect\firmware\d21_aws\keystream_aws_d21.X` |
| `SAME54_X` | `TrustMANAGER\SAME54_X\keystream_connect\firmware\e54_aws_freertos\keystream_aws_e54.X` |
| `SG41_X` (FOTA app) | `TrustMANAGER\SG41_X\keystream_connect\keystream_fota\keystream_Fota.X` |
| `SG41_X` (Boot app) | `TrustMANAGER\SG41_X\keystream_connect\keystream_boot\keystream_boot.X` |
| `SG41_UART_X` | `TrustMANAGER\SG41_UART_X\keystream_connect\keystream_connect.X` |

> **SG41_X note:** Both `keystream_Fota.X` and `keystream_boot.X` must be built and flashed. The FOTA application handles provisioning; the boot application handles secure firmware updates.

#### ESP-IDF targets (ESP32_BLE_CLI_X, ESP32_Mobile_APP_X)

These projects are built with ESP-IDF, not MPLAB X. Navigate to the firmware folder and run:

```powershell
idf.py build
idf.py -p <COM_PORT> flash monitor
```

| Device | Firmware folder |
|---|---|
| `ESP32_BLE_CLI_X` | `TrustMANAGER\ESP32_BLE_CLI_X\firmware\` |
| `ESP32_Mobile_APP_X` | `TrustMANAGER\ESP32_Mobile_APP_X\firmware\` |

See the [ESP32 Mobile App firmware README](./TrustMANAGER/ESP32_Mobile_APP_X/firmware/README.md) for the full ESP-IDF setup and build walkthrough.

---

## Folder Structure

```
PreIntegrated-Examples\
├── Config.ps1                          ← Run this to configure your project
├── docs/index.html                     ← Offline documentation (Fleet Profile Creation, AWS Setup)
├── Readme.md                           ← This file
├── images\                             ← Screenshots used in the guides
└── TrustMANAGER\
    ├── D21_X\
    │   └── keystream_connect\
    │       └── firmware\
    │           ├── common\             ← KTA lib sources (populated by Config.ps1)
    │           └── d21_aws\
    │               └── keystream_aws_d21.X   ← MPLAB X project
    ├── SAME54_X\
    │   └── keystream_connect\
    │       └── firmware\
    │           └── e54_aws_freertos\
    │               └── keystream_aws_e54.X   ← MPLAB X project
    ├── SG41_X\
    │   └── keystream_connect\
    │       ├── keystream_fota\
    │       │   └── keystream_Fota.X          ← MPLAB X project (FOTA app)
    │       └── keystream_boot\
    │           └── keystream_boot.X          ← MPLAB X project (Boot app)
    ├── SG41_UART_X\
    │   ├── App_Config.h                      ← Fleet UID + UART COM port (set this before building)
    │   ├── build_gateway.bat                 ← Build Windows gateway executable
    │   ├── gateway\                          ← Windows gateway sources
    │   └── keystream_connect\
    │       └── keystream_connect.X           ← MPLAB X project (MCU side)
    ├── ESP32_BLE_CLI_X\
    │   ├── App_Config.h                      ← Fleet Profile UID (set this before building)
    │   ├── windows_ble_cli.py                ← Windows BLE CLI provisioning tool
    │   └── firmware\                         ← ESP-IDF project (idf.py build)
    └── ESP32_Mobile_APP_X\
        ├── firmware\                         ← ESP-IDF project (idf.py build)
        │   └── README.md                     ← Full ESP-IDF build guide
        └── mobile\                           ← Mobile app sources
```

---

## Notes

- Run `Config.ps1` from any directory — it locates files relative to itself.
- If you encounter missing-file errors, verify that the `TrustMANAGER` folder structure matches the tree above.

---

## Support

For any issues or questions, please contact your keySTREAM support representative.
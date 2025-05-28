# keySTREAM Provisioning Automation

This repository contains a PowerShell script (`Config.ps1`) to automate the configuration and provisioning process for keySTREAM device firmware examples.  
It supports the following platforms:
- `d21_X`
- `same54_X`
- `sg41_X`

---

## Prerequisites

- **Windows PC**
- **PowerShell** (built-in on Windows 10/11)
- **MPLAB X IDE** (for opening the generated `.X` project files)

---

## Step-by-Step Instructions

### 1. Download or Clone the Repository

Download or clone this repository to any location on your PC.

For example, you might place it in your Downloads or Documents folder:
```
keySTREAM_provisioning-main
```
You can use any folder or path that is convenient for you.
### 2. Open PowerShell

Open a PowerShell window:
- Press `Win + S`, type `PowerShell`, and press `Enter`.
### 3. Navigate to the Script Directory

In PowerShell, navigate to the `PreConfigured-Examples` folder inside your cloned or extracted repository.  
For example, if you are in the root of your repository, run:

```powershell
cd .\PreConfigured-Examples
```

Or, if you are elsewhere, provide the full path to the folder:

```powershell
cd "path\to\your\keySTREAM_provisioning-main\PreConfigured-Examples"
```

*(Replace `path\to\your` with the actual path where you placed the repository.)*

### 4. Run the Configuration Script

Execute the script:
```powershell
.\Config.ps1
```

### 5. Follow the On-Screen Prompts

The script will guide you through the following steps:

- **Select a folder**: Choose the main folder (e.g., `TrustMANAGER`).
- **Select a device**: Choose your target device (`d21_X`, `same54_X`, or `sg41_X`).
- **Enter configuration values**:
  - For `d21_X`: Enter WiFi SSID, WiFi password, and Device Public Profile UID.
  - For `same54_X`: Enter only the Device Public Profile UID.
  - For `sg41_X`: Enter WiFi SSID, WiFi password, and Device Public Profile UID (when prompted).

The script will automatically update the relevant configuration files and copy required provisioning files for your selected platform.

### 6. Open the MPLAB X Project

After completion, the script will display the **full path** to the generated `.X` project file for your device.  
Example output:
```
=====================================================================
Next Step:
Please open the following .X project file in MPLAB X IDE to continue:

    C:\Users\<your-username>\Downloads\GitHub_Repo\keySTREAM_provisioning-main\PreConfigured-Examples\TrustMANAGER\d21_X\keystream_connect\firmware\d21_aws\keystream_aws_d21.X

=====================================================================
```
Open this file in **MPLAB X IDE** to continue your development.

---

## Notes

- You can run this script from any location on your PC.
- Make sure all required subfolders and files are present as described in the repository.
- If you encounter errors about missing files or folders, please verify your directory structure.

---

## Support

For any issues or questions, please contact your keySTREAM support representative.

---
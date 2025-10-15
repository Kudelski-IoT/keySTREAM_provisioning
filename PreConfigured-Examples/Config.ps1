# -----------------------------------------------------------------------------
# Config.ps1 - keySTREAM Provisioning Automation Script
#
# Description:
#   This PowerShell script automates the configuration and provisioning process
#   for keySTREAM device firmware examples. It allows you to select your target
#   device folder, update WiFi and device profile settings, and copy required
#   provisioning files for supported platforms (d21_X, same54_X, sg41_X).
#
#   After configuration, the script provides the full path to the MPLAB X
#   project file (.X) for your selected device, ready for you to open in
#   MPLAB X IDE and continue your development.
#
# -----------------------------------------------------------------------------
# Get the path of the current script's directory (Pre-Configured Examples)
$currentPath = Split-Path -Parent $MyInvocation.MyCommand.Definition

# List all directories in the current path, excluding 'images'
$folders = Get-ChildItem -Path $currentPath -Directory | Where-Object { $_.Name -ne 'images' }


if ($folders.Count -eq 0) {
    Write-Host "No folders found in $currentPath."
    exit
}

Write-Host "Select a folder:"
for ($i = 0; $i -lt $folders.Count; $i++) {
    Write-Host "$($i + 1): $($folders[$i].Name)"
}

do {
    $folderSelection = Read-Host "Enter the number of the folder you want to use as TrustMANAGER"
    $isValidFolder = ($folderSelection -as [int]) -and ($folderSelection -ge 1) -and ($folderSelection -le $folders.Count)
    if (-not $isValidFolder) {
        Write-Host "Invalid selection. Please try again."
    }
} until ($isValidFolder)

$trustManagerPath = $folders[$folderSelection - 1].FullName
Write-Host "You selected: $trustManagerPath"

# Get valid subfolders inside TrustMANAGER
$subFolders = Get-ChildItem -Path $trustManagerPath -Directory | Where-Object { $_.Name -in @('d21_X', 'same54_X', 'sg41_X') }

if ($subFolders.Count -eq 0) {
    Write-Host "No subfolders in '$($folders[$folderSelection - 1].Name)'."
    exit
}

Write-Host "Select a Desire Device '$($folders[$folderSelection - 1].Name)':"
for ($j = 0; $j -lt $subFolders.Count; $j++) {
    Write-Host "$($j + 1): $($subFolders[$j].Name)"
}

do {
    $subSelection = Read-Host "Enter the number of the subfolder you want to select"
    $isValidSub = ($subSelection -as [int]) -and ($subSelection -ge 1) -and ($subSelection -le $subFolders.Count)
    if (-not $isValidSub) {
        Write-Host "Invalid selection. Please try again."
    }
} until ($isValidSub)

$selectedSubFolder = $subFolders[$subSelection - 1].Name
Write-Host "You selected Device: $selectedSubFolder"

# Perform operations based on subfolder
if ($selectedSubFolder -eq "d21_X")
{
    Write-Host "Performing d21_X operations..."
    $preConfigRoot = Split-Path $currentPath -Parent
    $ktaLibPath = Join-Path $preConfigRoot "kta_lib\kta_provisioning"
    if (-not (Test-Path $ktaLibPath))
    {
        Write-Host "The 'kta_lib\kta_provisioning' folder was not found in the current directory structure."
        exit
    }

    # 1. Copy SOURCE folder to ...\firmware\common\kta_provisioning
    $sourcePath = Join-Path $ktaLibPath "SOURCE"
    $destPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\common\kta_provisioning\SOURCE"

    # Remove the destination SOURCE folder if it already exists
    if (Test-Path $destPath) {
        Remove-Item $destPath -Recurse -Force
    }

    # Copy all files from SOURCE (including ktaConfig.h)
    Copy-Item $sourcePath -Destination $destPath -Recurse -Force

    # 1a. Copy COMMON folder to ...\firmware\common\kta_provisioning
    $commonSourcePath = Join-Path $preConfigRoot "apps\keystream_late_provisioning_app\src\config\default\library\kta_lib\COMMON"
    $commonDestPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\common\kta_provisioning\COMMON"

    # Remove the destination COMMON folder if it already exists
    if (Test-Path $commonDestPath) {
        Remove-Item $commonDestPath -Recurse -Force
    }
    # Copy all files from COMMON
    Copy-Item $commonSourcePath -Destination $commonDestPath -Recurse -Force

    # Overwrite ktaConfig.h with the latest version
    $srcKtaConfig = Join-Path $preConfigRoot "apps\keystream_late_provisioning_app\src\config\default\library\kta_lib\SOURCE\include\ktaConfig.h"
    $destKtaConfig = Join-Path $destPath "include\ktaConfig.h"
    Copy-Item $srcKtaConfig -Destination $destKtaConfig -Force

    # --- Custom script for ktaConfig.h ---
    $ktaConfigPath = Join-Path $destPath "include\ktaConfig.h"
    if (Test-Path $ktaConfigPath) {
        # Replace the commented or existing define with the required define
        (Get-Content $ktaConfigPath) | ForEach-Object {
            if ($_ -match '^\s*//\s*#define\s+C_KTA_APP__LOG\b' -or $_ -match '^\s*#define\s+C_KTA_APP__LOG\b') {
                '#define C_KTA_APP__LOG               APP_DebugPrintf'
            } else {
                $_
            }
        } | Set-Content $ktaConfigPath

        $lines = Get-Content $ktaConfigPath
        if ($lines.Count -ge 56) {
            $lines = $lines[0..54] + '#include "app.h"' + '#include "App_Config.h"' + $lines[55..($lines.Count-1)]
            $lines | Set-Content $ktaConfigPath
        } else {
            # If file is shorter, just append both at the end
            Add-Content $ktaConfigPath '#include "app.h"'
            Add-Content $ktaConfigPath '#include "App_Config.h"'
        }
            
        # 1. Replace C_KTA_APP__DEVICE_PUBLIC_UID with required macro
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*#define\s+C_KTA_APP__DEVICE_PUBLIC_UID\b') {
                '#define C_KTA_APP__DEVICE_PUBLIC_UID    KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID'
            } else {
                $_
            }
        }

        # 2. Uncomment NETWORK_STACK_AVAILABLE
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*//\s*#define\s+NETWORK_STACK_AVAILABLE\b') {
                ' #define NETWORK_STACK_AVAILABLE'
            } else {
                $_
            }
        }

        $lines | Set-Content $ktaConfigPath
        }


    # 2. Copy other files to ...\firmware\common\
    $destCommon = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\common"
    if (-not (Test-Path $destCommon))
    {
        New-Item -ItemType Directory -Path $destCommon -Force | Out-Null
    }

    Copy-Item "$ktaLibPath\cust_def_device.c"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\cust_def_device.h"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\cust_def_signer.c"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\cust_def_signer.h"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\ktaFieldMgntHook.c"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\ktaFieldMgntHook.h"    -Destination $destCommon -Force

    $preConfigRoot = Split-Path $currentPath -Parent
    $srcCryptoAuthLib = Join-Path $preConfigRoot "apps\keystream_late_provisioning_app\src\config\default\library\cryptoauthlib"

    # Dynamic destination path (based on selected TrustMANAGER and device)
    $dstCryptoAuthLib = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\d21_aws\src\config\keystream_connect\library\cryptoauthlib"

    if (Test-Path $srcCryptoAuthLib) {
        # Ensure destination exists
        if (-not (Test-Path $dstCryptoAuthLib)) {
            New-Item -ItemType Directory -Path $dstCryptoAuthLib -Force | Out-Null
        }

        # Copy all .c and .h files from cryptoauthlib (root and subfolders)
        Get-ChildItem -Path $srcCryptoAuthLib -Include *.c,*.h -File -Recurse | ForEach-Object {
            $dest = Join-Path $dstCryptoAuthLib ($_.FullName.Substring($srcCryptoAuthLib.Length).TrimStart('\'))
            $destDir = Split-Path $dest -Parent
            if (-not (Test-Path $destDir)) {
                New-Item -ItemType Directory -Path $destDir -Force | Out-Null
            }
            Copy-Item $_.FullName -Destination $dest -Force
        }

        # Copy each required subfolder individually
        $subfolders = @('atcacert', 'calib', 'crypto', 'hal', 'host', 'jwt')
        foreach ($folder in $subfolders) {
            $srcSub = Join-Path $srcCryptoAuthLib $folder
            if (Test-Path $srcSub) {
                Copy-Item $srcSub -Destination $dstCryptoAuthLib -Recurse -Force
            } else {
                Write-Host "Folder not found: $folder"
            }
        }
    } else {
        Write-Host "Source cryptoauthlib folder not found at $srcCryptoAuthLib"
    }

    # Professional delivery message for customer
    $xProjPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\d21_aws\keystream_aws_d21.X"
    Write-Host ""
    Write-Host "====================================================================="
    Write-Host "Next Step:"
    Write-Host "Please open the following .X project file in MPLAB X IDE to continue:"
    Write-Host ""
    Write-Host "    $xProjPath"
    Write-Host ""
    Write-Host "====================================================================="

} 
elseif ($selectedSubFolder -eq "same54_X") 
{
    Write-Host "Performing same54 operations..."

    $preConfigRoot = Split-Path $currentPath -Parent
    $ktaLibPath = Join-Path $preConfigRoot "kta_lib\kta_provisioning"

    # 1. Copy SOURCE folder to ...\firmware\common\kta_provisioning
    $sourcePath = Join-Path $ktaLibPath "SOURCE"
    $destPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\common\kta_provisioning\SOURCE"

    # Remove the destination SOURCE folder if it already exists
    if (Test-Path $destPath) {
        Remove-Item $destPath -Recurse -Force
    }

    # Copy all files from SOURCE (including ktaConfig.h)
    Copy-Item $sourcePath -Destination $destPath -Recurse -Force

    # 1a. Copy COMMON folder to ...\firmware\common\kta_provisioning
    $commonSourcePath = Join-Path $preConfigRoot "apps\keystream_late_provisioning_app\src\config\default\library\kta_lib\COMMON"
    $commonDestPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\common\kta_provisioning\COMMON"

    # Remove the destination COMMON folder if it already exists
    if (Test-Path $commonDestPath) {
        Remove-Item $commonDestPath -Recurse -Force
    }
    # Copy all files from COMMON
    Copy-Item $commonSourcePath -Destination $commonDestPath -Recurse -Force

    # Overwrite ktaConfig.h with the latest version
    $srcKtaConfig = Join-Path $preConfigRoot "apps\keystream_late_provisioning_app\src\config\default\library\kta_lib\SOURCE\include\ktaConfig.h"
    $destKtaConfig = Join-Path $destPath "include\ktaConfig.h"
    Copy-Item $srcKtaConfig -Destination $destKtaConfig -Force

    # --- Custom script for ktaConfig.h ---
    $ktaConfigPath = Join-Path $destPath "include\ktaConfig.h"
    if (Test-Path $ktaConfigPath) {

        $lines = Get-Content $ktaConfigPath
        if ($lines.Count -ge 56) {
            $lines = $lines[0..54] + '#include "App_Config.h"' + $lines[55..($lines.Count-1)]
            $lines | Set-Content $ktaConfigPath
        } else {
            # If file is shorter, just append at the end
            Add-Content $ktaConfigPath '#include "App_Config.h"'
        }

        # 1. Replace C_KTA_APP__DEVICE_PUBLIC_UID with required macro
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*#define\s+C_KTA_APP__DEVICE_PUBLIC_UID\b') {
                '#define C_KTA_APP__DEVICE_PUBLIC_UID                 KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID'
            } else {
                $_
            }
        }

        # 2. Uncomment NETWORK_STACK_AVAILABLE
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*//\s*#define\s+NETWORK_STACK_AVAILABLE\b') {
                ' #define NETWORK_STACK_AVAILABLE'
            } else {
                $_
            }
        }

        $lines | Set-Content $ktaConfigPath
    }

    # 2. Copy other files to ...\firmware\common\
    $destCommon = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\common"
    if (-not (Test-Path $destCommon))
    {
        New-Item -ItemType Directory -Path $destCommon -Force | Out-Null
    }

    Copy-Item "$ktaLibPath\cust_def_device.c"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\cust_def_device.h"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\cust_def_signer.c"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\cust_def_signer.h"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\ktaFieldMgntHook.c"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\ktaFieldMgntHook.h"    -Destination $destCommon -Force

    $preConfigRoot = Split-Path $currentPath -Parent
    $srcCryptoAuthLib = Join-Path $preConfigRoot "apps\keystream_late_provisioning_app\src\config\default\library\cryptoauthlib"

    # Dynamic destination path (based on selected TrustMANAGER and device)
    $dstCryptoAuthLib = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\e54_aws_freertos\src\config\keystream_connect\library\cryptoauthlib"

    if (Test-Path $srcCryptoAuthLib) {
        # Ensure destination exists
        if (-not (Test-Path $dstCryptoAuthLib)) {
            New-Item -ItemType Directory -Path $dstCryptoAuthLib -Force | Out-Null
        }

        # Copy all .c and .h files from cryptoauthlib (root and subfolders), EXCLUDING hal
        Get-ChildItem -Path $srcCryptoAuthLib -Include *.c,*.h -File -Recurse | Where-Object {
            $_.FullName -notmatch '\\hal\\'
        } | ForEach-Object {
            $dest = Join-Path $dstCryptoAuthLib ($_.FullName.Substring($srcCryptoAuthLib.Length).TrimStart('\'))
            $destDir = Split-Path $dest -Parent
            if (-not (Test-Path $destDir)) {
                New-Item -ItemType Directory -Path $destDir -Force | Out-Null
            }
            Copy-Item $_.FullName -Destination $dest -Force
        }

        # Copy each required subfolder individually
        $subfolders = @('atcacert', 'calib', 'crypto', 'host', 'jwt')
        foreach ($folder in $subfolders) {
            $srcSub = Join-Path $srcCryptoAuthLib $folder
            if (Test-Path $srcSub) {
                Copy-Item $srcSub -Destination $dstCryptoAuthLib -Recurse -Force
            } else {
                Write-Host "Folder not found: $folder"
            }
        }
    } else {
        Write-Host "Source cryptoauthlib folder not found at $srcCryptoAuthLib"
    }

    $atcaConfigPath    = Join-Path $dstCryptoAuthLib "atca_config.h"

    # 1. atca_config.h: Replace extern atca_plib_i2c_api_t sercom2_plib_i2c_api; with extern atca_plib_i2c_api_t sercom3_plib_i2c_api;
    if (Test-Path $atcaConfigPath) {
        (Get-Content $atcaConfigPath) -replace 'extern atca_plib_i2c_api_t sercom2_plib_i2c_api;', 'extern atca_plib_i2c_api_t sercom7_plib_i2c_api;' | Set-Content $atcaConfigPath
    }

    # Professional delivery message for customer
    $xProjPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\e54_aws_freertos\keystream_aws_e54.X."
    Write-Host ""
    Write-Host "====================================================================="
    Write-Host "Next Step:"
    Write-Host "Please open the following .X project file in MPLAB X IDE to continue:"
    Write-Host ""
    Write-Host "    $xProjPath"
    Write-Host ""
    Write-Host "====================================================================="
} 
elseif ($selectedSubFolder -eq "sg41_X") 
{
    $preConfigRoot = Split-Path $currentPath -Parent
    $ktaLibPath = Join-Path $preConfigRoot "kta_lib\kta_provisioning"
    if (-not (Test-Path $ktaLibPath)) {
        Write-Host "The 'kta_lib\kta_provisioning' folder was not found in the current directory structure."
        exit
    }

    # 1. Copy SOURCE folder to ...\src\config\default\library\kta_lib\SOURCE
    $sourcePath = Join-Path $ktaLibPath "SOURCE"
    $destSourcePath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\src\config\default\library\kta_lib\SOURCE"

    # Remove the destination SOURCE folder if it already exists
    if (Test-Path $destSourcePath) {
        Remove-Item $destSourcePath -Recurse -Force
    }

    # Copy all files from SOURCE (including ktaConfig.h)
    Copy-Item $sourcePath -Destination $destSourcePath -Recurse -Force


    # 1a. Copy COMMON folder to ...\firmware\common\kta_provisioning
    $commonSourcePath = Join-Path $preConfigRoot "apps\keystream_late_provisioning_app\src\config\default\library\kta_lib\COMMON"
    $commonDestPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\src\config\default\library\kta_lib\COMMON"


    # Remove the destination COMMON folder if it already exists
    if (Test-Path $commonDestPath) {
        Remove-Item $commonDestPath -Recurse -Force
    }
    # Copy all files from COMMON
    Copy-Item $commonSourcePath -Destination $commonDestPath -Recurse -Force

    # Overwrite ktaConfig.h with the latest version
    $srcKtaConfig = Join-Path $preConfigRoot "apps\keystream_late_provisioning_app\src\config\default\library\kta_lib\SOURCE\include\ktaConfig.h"
    $destKtaConfig = Join-Path $destSourcePath "include\ktaConfig.h"
    Copy-Item $srcKtaConfig -Destination $destKtaConfig -Force


    # --- Custom script for ktaConfig.h ---
    $ktaConfigPath = $destKtaConfig
    if (Test-Path $ktaConfigPath) {

        $lines = Get-Content $ktaConfigPath
        if ($lines.Count -ge 56) {
            $lines = $lines[0..54] + '#include "App_Config.h"' + $lines[55..($lines.Count-1)]
            $lines | Set-Content $ktaConfigPath
        } else {
            # If file is shorter, just append at the end
            Add-Content $ktaConfigPath '#include "App_Config.h"'
        }
                
        # 1. Replace C_KTA_APP__DEVICE_PUBLIC_UID with required macro
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*#define\s+C_KTA_APP__DEVICE_PUBLIC_UID\b') {
                '#define C_KTA_APP__DEVICE_PUBLIC_UID                 KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID'
            } else {
                $_
            }
        }

        # 2. Uncomment NETWORK_STACK_AVAILABLE
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*//\s*#define\s+NETWORK_STACK_AVAILABLE\b') {
                ' #define NETWORK_STACK_AVAILABLE'
            } else {
                $_
            }
        }

        $lines | Set-Content $ktaConfigPath
    }


    $destcustdef = Join-Path $destSourcePath "..\"
    Copy-Item (Join-Path $ktaLibPath "cust_def_device.c")      -Destination $destcustdef -Force
    Copy-Item (Join-Path $ktaLibPath "cust_def_device.h")      -Destination $destcustdef -Force
    Copy-Item (Join-Path $ktaLibPath "cust_def_signer.c")      -Destination $destcustdef -Force
    Copy-Item (Join-Path $ktaLibPath "cust_def_signer.h")      -Destination $destcustdef -Force
    Copy-Item (Join-Path $ktaLibPath "ktaFieldMgntHook.c")     -Destination $destcustdef -Force
    Copy-Item (Join-Path $ktaLibPath "ktaFieldMgntHook.h")     -Destination $destcustdef -Force

    $preConfigRoot = Split-Path $currentPath -Parent
    $srcCryptoAuthLib = Join-Path $preConfigRoot "apps\keystream_late_provisioning_app\src\config\default\library\cryptoauthlib"

    # Dynamic destination path (based on selected TrustMANAGER and device)
    $dstCryptoAuthLib = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\src\config\default\library\cryptoauthlib"

    if (Test-Path $srcCryptoAuthLib) {
        # Ensure destination exists
        if (-not (Test-Path $dstCryptoAuthLib)) {
            New-Item -ItemType Directory -Path $dstCryptoAuthLib -Force | Out-Null
        }

        # Copy all .c and .h files from cryptoauthlib (root and subfolders), EXCLUDING hal
        Get-ChildItem -Path $srcCryptoAuthLib -Include *.c,*.h -File -Recurse | Where-Object {
            $_.FullName -notmatch '\\hal\\'
        } | ForEach-Object {
            $dest = Join-Path $dstCryptoAuthLib ($_.FullName.Substring($srcCryptoAuthLib.Length).TrimStart('\'))
            $destDir = Split-Path $dest -Parent
            if (-not (Test-Path $destDir)) {
                New-Item -ItemType Directory -Path $destDir -Force | Out-Null
            }
            Copy-Item $_.FullName -Destination $dest -Force
        }

        # Copy each required subfolder individually
        $subfolders = @('atcacert', 'calib', 'crypto', 'host', 'jwt')
        foreach ($folder in $subfolders) {
            $srcSub = Join-Path $srcCryptoAuthLib $folder
            if (Test-Path $srcSub) {
                Copy-Item $srcSub -Destination $dstCryptoAuthLib -Recurse -Force
            } else {
                Write-Host "Folder not found: $folder"
            }
        }
    } else {
        Write-Host "Source cryptoauthlib folder not found at $srcCryptoAuthLib"
    }

    $atcaConfigPath    = Join-Path $dstCryptoAuthLib "atca_config.h"

    # 1. atca_config.h: Replace extern atca_plib_i2c_api_t sercom2_plib_i2c_api; with extern atca_plib_i2c_api_t sercom3_plib_i2c_api;
    if (Test-Path $atcaConfigPath) {
        (Get-Content $atcaConfigPath) -replace 'extern atca_plib_i2c_api_t sercom2_plib_i2c_api;', 'extern atca_plib_i2c_api_t sercom3_plib_i2c_api;' | Set-Content $atcaConfigPath
    }

    Remove-Item (Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware") -Recurse -Force -ErrorAction SilentlyContinue

    $xProjPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\keystream_connect.X"
    Write-Host ""
    Write-Host "======================================================================================"
    Write-Host "Next Step:"
    Write-Host "Please open the following .X project file in MPLAB X IDE to continue:"
    Write-Host ""
    Write-Host "    $xProjPath"
    Write-Host ""
    Write-Host "====================================================================================="
}
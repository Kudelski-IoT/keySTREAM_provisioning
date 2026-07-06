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
$subFolders = Get-ChildItem -Path $trustManagerPath -Directory | Where-Object { $_.Name -in @('d21_X', 'same54_X', 'sg41_X', 'SG41_UART_X', 'ESP32_BLE_CLI_X', 'ESP32_Mobile_APP_X') }

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
    # ═══════════════════════════════════════════════════════════════════════════════
    # DEVICE: d21_X  (ATSAMD21 — D21 AWS FreeRTOS / MPLAB X)
    # ───────────────────────────────────────────────────────────────────────────
    # Configures the D21 Harmony/AWS firmware project for keySTREAM provisioning.
    #   Step 1 : Copies KTA SOURCE  → firmware/common/kta_provisioning/SOURCE/
    #   Step 2 : Copies KTA COMMON  → firmware/common/kta_provisioning/COMMON/
    #   Step 3 : Patches ktaConfig.h
    #              – Enables APP_DebugPrintf log macro
    #              – Injects #include "app.h" and #include "App_Config.h"
    #              – Replaces C_KTA_APP__DEVICE_PUBLIC_UID with portal macro
    #              – Uncomments NETWORK_STACK_AVAILABLE
    #   Step 4 : Copies cust_def_* / ktaFieldMgntHook → firmware/common/
    #   Step 5 : Copies cryptoauthlib (all subfolders incl. hal/)
    #            → firmware/d21_aws/src/config/keystream_connect/library/cryptoauthlib/
    #   Output : Prints MPLAB X .X project path for the user to open
    # ═══════════════════════════════════════════════════════════════════════════════
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
    $commonSourcePath = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\kta_lib\COMMON"
    $commonDestPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\common\kta_provisioning\COMMON"

    # Remove the destination COMMON folder if it already exists
    if (Test-Path $commonDestPath) {
        Remove-Item $commonDestPath -Recurse -Force
    }
    # Copy all files from COMMON
    Copy-Item $commonSourcePath -Destination $commonDestPath -Recurse -Force

    # Overwrite ktaConfig.h with the latest version
    $srcKtaConfig = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\kta_lib\SOURCE\include\ktaConfig.h"
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
            
            # 1. Replace C_KTA_APP__DEVICE_PUBLIC_UID with required macro (handle both commented and uncommented)
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*(?://\s*)?#define\s+C_KTA_APP__DEVICE_PUBLIC_UID\b') {
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
    $srcCryptoAuthLib = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\cryptoauthlib"

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
    # ═══════════════════════════════════════════════════════════════════════════════
    # DEVICE: same54_X  (ATSAME54 — E54 AWS FreeRTOS / MPLAB X)
    # ───────────────────────────────────────────────────────────────────────────
    # Configures the SAME54 Harmony/AWS firmware project for keySTREAM provisioning.
    #   Step 1 : Copies KTA SOURCE  → firmware/common/kta_provisioning/SOURCE/
    #   Step 2 : Copies KTA COMMON  → firmware/common/kta_provisioning/COMMON/
    #   Step 3 : Patches ktaConfig.h
    #              – Injects #include "App_Config.h"
    #              – Replaces C_KTA_APP__DEVICE_PUBLIC_UID with portal macro
    #              – Uncomments NETWORK_STACK_AVAILABLE
    #   Step 4 : Copies cust_def_* / ktaFieldMgntHook → firmware/common/
    #   Step 5 : Copies cryptoauthlib (excluding hal/) and patches atca_config.h
    #              – sercom2 → sercom7  (SAME54 I2C pin mapping)
    #   Output : Prints MPLAB X .X project path for the user to open
    # ═══════════════════════════════════════════════════════════════════════════════
    Write-Host "Performing same54_X operations..."

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
    $commonSourcePath = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\kta_lib\COMMON"
    $commonDestPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\common\kta_provisioning\COMMON"

    # Remove the destination COMMON folder if it already exists
    if (Test-Path $commonDestPath) {
        Remove-Item $commonDestPath -Recurse -Force
    }
    # Copy all files from COMMON
    Copy-Item $commonSourcePath -Destination $commonDestPath -Recurse -Force

    # Overwrite ktaConfig.h with the latest version
    $srcKtaConfig = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\kta_lib\SOURCE\include\ktaConfig.h"
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

        # 1. Replace C_KTA_APP__DEVICE_PUBLIC_UID with required macro (handle both commented and uncommented)
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*(?://\s*)?#define\s+C_KTA_APP__DEVICE_PUBLIC_UID\b') {
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
    $srcCryptoAuthLib = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\cryptoauthlib"

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
    $xProjPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\e54_aws_freertos\keystream_aws_e54.X"
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
    # ═══════════════════════════════════════════════════════════════════════════════
    # DEVICE: sg41_X  (WFI32 / SG41 — MPLAB X)
    # ───────────────────────────────────────────────────────────────────────────
    # Configures the SG41 Harmony projects for keySTREAM provisioning.
    # Note: Uses src/ project layout (not firmware/) — no MPLAB Harmony firmware folder.
    # Two applications inside sg41_X, both configured by this block:
    #   1. keystream_fota  → sg41_X/keystream_connect/keystream_fota/keystream_connect.X
    #   2. keystream_boot  → sg41_X/keystream_connect/keystream_boot/keystream_connect.X
    #   Step 1 : Copies KTA SOURCE  → src/config/default/library/kta_lib/SOURCE/
    #   Step 2 : Copies KTA COMMON  → src/config/default/library/kta_lib/COMMON/
    #   Step 3 : Patches ktaConfig.h
    #              – Injects #include "App_Config.h"
    #              – Replaces C_KTA_APP__DEVICE_PUBLIC_UID with portal macro
    #              – Uncomments NETWORK_STACK_AVAILABLE
    #   Step 4 : Copies cust_def_* / ktaFieldMgntHook          [keystream_fota only]
    #   Step 5 : Copies cryptoauthlib (excluding hal/) and patches atca_config.h
    #              – sercom2 → sercom3  (SG41 I2C pin mapping)  [both apps]
    #   Step 6 : Removes legacy firmware/ folder               [keystream_fota only]
    #   Note  : keystream_boot receives cryptoauthlib update only; all other
    #           sources (KTA lib, cust_def, ktaConfig.h) remain untouched.
    #   Output : Prints MPLAB X .X project path for each application
    # ═══════════════════════════════════════════════════════════════════════════════
    $preConfigRoot = Split-Path $currentPath -Parent
    $ktaLibPath = Join-Path $preConfigRoot "kta_lib\kta_provisioning"
    if (-not (Test-Path $ktaLibPath)) {
        Write-Host "The 'kta_lib\kta_provisioning' folder was not found in the current directory structure."
        exit
    }

    # ── keystream_fota : full KTA + cryptoauthlib setup ──────────────────────
    $appName = 'keystream_fota'
    Write-Host "Performing sg41_X operations..."

    # 1. Copy SOURCE folder to ...\src\config\default\library\kta_lib\SOURCE
    $sourcePath = Join-Path $ktaLibPath "SOURCE"
    $destSourcePath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\$appName\src\config\default\library\kta_lib\SOURCE"

    # Back up any salapi files in the destination that are NOT present in the generic source.
    # These are FOTA-specific files (k_sal_fota.c, fota_storage files, FOTA headers) placed
    # by the packaging script. The Remove-Item below would delete them; backing up here
    # and restoring after the copy keeps them intact without touching generic files.
    $genericSalapiSrc = Join-Path $sourcePath "salapi"
    $destSalapiPath   = Join-Path $destSourcePath "salapi"
    $salapiBackupDir  = $null
    if (Test-Path $destSalapiPath) {
        $salapiBackupDir = Join-Path ([System.IO.Path]::GetTempPath()) ("salapi_fota_bak_" + [System.Guid]::NewGuid().ToString())
        New-Item -ItemType Directory -Path $salapiBackupDir -Force | Out-Null
        Get-ChildItem -Path $destSalapiPath -Recurse -File | ForEach-Object {
            $relPath  = $_.FullName.Substring($destSalapiPath.Length).TrimStart('\')
            $srcEquiv = Join-Path $genericSalapiSrc $relPath
            if (-not (Test-Path $srcEquiv)) {
                $backupTarget    = Join-Path $salapiBackupDir $relPath
                $backupTargetDir = Split-Path $backupTarget -Parent
                if (-not (Test-Path $backupTargetDir)) { New-Item -ItemType Directory -Path $backupTargetDir -Force | Out-Null }
                Copy-Item $_.FullName -Destination $backupTarget -Force
            }
        }
    }

    # Back up existing fotaplatform files — SG41_X maintains its own hardcoded FOTA
    # implementation that must NOT be overwritten by the generic KTA lib SOURCE copy.
    $fotaServiceDestPath = Join-Path $destSourcePath "kta\modules\fotaservice"
    $fotaPlatformCSrc    = Join-Path $fotaServiceDestPath "fotaplatform.c"
    $fotaPlatformHSrc    = Join-Path $fotaServiceDestPath "include\fotaplatform.h"
    $fotaPlatformCBackup = $null
    $fotaPlatformHBackup = $null
    if (Test-Path $fotaPlatformCSrc) {
        $fotaPlatformCBackup = Join-Path ([System.IO.Path]::GetTempPath()) ("fotaplatform_c_bak_" + [System.Guid]::NewGuid().ToString() + ".c")
        Copy-Item $fotaPlatformCSrc -Destination $fotaPlatformCBackup -Force
    }
    if (Test-Path $fotaPlatformHSrc) {
        $fotaPlatformHBackup = Join-Path ([System.IO.Path]::GetTempPath()) ("fotaplatform_h_bak_" + [System.Guid]::NewGuid().ToString() + ".h")
        Copy-Item $fotaPlatformHSrc -Destination $fotaPlatformHBackup -Force
    }

    # Remove the destination SOURCE folder if it already exists
    if (Test-Path $destSourcePath) {
        Remove-Item $destSourcePath -Recurse -Force
    }

    # Copy all files from SOURCE (including ktaConfig.h)
    Copy-Item $sourcePath -Destination $destSourcePath -Recurse -Force

    # Restore the FOTA-specific salapi files that were backed up above
    if ($salapiBackupDir -and (Test-Path $salapiBackupDir)) {
        if (-not (Test-Path $destSalapiPath)) { New-Item -ItemType Directory -Path $destSalapiPath -Force | Out-Null }
        Get-ChildItem -Path $salapiBackupDir -Recurse -File | ForEach-Object {
            $relPath    = $_.FullName.Substring($salapiBackupDir.Length).TrimStart('\')
            $restoreDst = Join-Path $destSalapiPath $relPath
            $restoreDir = Split-Path $restoreDst -Parent
            if (-not (Test-Path $restoreDir)) { New-Item -ItemType Directory -Path $restoreDir -Force | Out-Null }
            Copy-Item $_.FullName -Destination $restoreDst -Force
        }
        Remove-Item $salapiBackupDir -Recurse -Force -ErrorAction SilentlyContinue
    }

    # Restore the SG41_X-specific fotaplatform files backed up above.
    # The generic KTA lib SOURCE copy may have placed stub versions; the project-level
    # implementations are restored here so they are never overwritten.
    if ($fotaPlatformCBackup -and (Test-Path $fotaPlatformCBackup)) {
        if (-not (Test-Path $fotaServiceDestPath)) {
            New-Item -ItemType Directory -Path $fotaServiceDestPath -Force | Out-Null
        }
        Copy-Item $fotaPlatformCBackup -Destination $fotaPlatformCSrc -Force
        Remove-Item $fotaPlatformCBackup -Force -ErrorAction SilentlyContinue
    }
    if ($fotaPlatformHBackup -and (Test-Path $fotaPlatformHBackup)) {
        $fotaIncludeDir = Join-Path $fotaServiceDestPath "include"
        if (-not (Test-Path $fotaIncludeDir)) {
            New-Item -ItemType Directory -Path $fotaIncludeDir -Force | Out-Null
        }
        Copy-Item $fotaPlatformHBackup -Destination $fotaPlatformHSrc -Force
        Remove-Item $fotaPlatformHBackup -Force -ErrorAction SilentlyContinue
    }


    # 1a. Copy COMMON folder to ...\firmware\common\kta_provisioning
    $commonSourcePath = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\kta_lib\COMMON"
    $commonDestPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\$appName\src\config\default\library\kta_lib\COMMON"


    # Remove the destination COMMON folder if it already exists
    if (Test-Path $commonDestPath) {
        Remove-Item $commonDestPath -Recurse -Force
    }
    # Copy all files from COMMON
    Copy-Item $commonSourcePath -Destination $commonDestPath -Recurse -Force

    # Overwrite ktaConfig.h with the latest version
    $srcKtaConfig = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\kta_lib\SOURCE\include\ktaConfig.h"
    $destKtaConfig = Join-Path $destSourcePath "include\ktaConfig.h"
    Copy-Item $srcKtaConfig -Destination $destKtaConfig -Force


    # --- Custom script for ktaConfig.h ---
    $ktaConfigPath = $destKtaConfig
    if (Test-Path $ktaConfigPath) {

        $lines = Get-Content $ktaConfigPath
        if ($lines.Count -ge 56) {
            $lines = $lines[0..54] + '#include "../../../../../../../../App_Config.h"' + $lines[55..($lines.Count-1)]
            $lines | Set-Content $ktaConfigPath
        } else {
            # If file is shorter, just append at the end
            Add-Content $ktaConfigPath '#include "../../../../../../../../App_Config.h"'
        }
                
        # 1. Replace C_KTA_APP__DEVICE_PUBLIC_UID with required macro (handle both commented and uncommented)
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*(?://\s*)?#define\s+C_KTA_APP__DEVICE_PUBLIC_UID\b') {
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

        # 3. Uncomment FOTA_ENABLE (SG41_X only)
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*//\s*#define\s+FOTA_ENABLE\b') {
                '#define FOTA_ENABLE'
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


    # ── cryptoauthlib copy + atca_config.h patch + .X output — same for both apps ──
    # Only the destination path changes between keystream_fota and keystream_boot.
    $srcCryptoAuthLib = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\cryptoauthlib"

    foreach ($appName in @('keystream_fota', 'keystream_boot')) {

        $dstCryptoAuthLib = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\$appName\src\config\default\library\cryptoauthlib"

        if (Test-Path $srcCryptoAuthLib) {
            if (-not (Test-Path $dstCryptoAuthLib)) {
                New-Item -ItemType Directory -Path $dstCryptoAuthLib -Force | Out-Null
            }

            # Copy all .c and .h files (root + subfolders), EXCLUDING hal/
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

        # atca_config.h: sercom2 → sercom3  (SG41 I2C pin mapping)
        $atcaConfigPath = Join-Path $dstCryptoAuthLib "atca_config.h"
        if (Test-Path $atcaConfigPath) {
            (Get-Content $atcaConfigPath) -replace 'extern atca_plib_i2c_api_t sercom2_plib_i2c_api;', 'extern atca_plib_i2c_api_t sercom3_plib_i2c_api;' | Set-Content $atcaConfigPath
        }

        # Copy COMMSTACK/http (for keystream_fota only — bootloader does not use network stack)
        if ($appName -eq 'keystream_fota') {
            $commstackHttpSrc  = Join-Path $preConfigRoot "kta_lib\kta_provisioning\COMMSTACK\http"
            $commstackHttpDest = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\$appName\src\config\default\library\kta_lib\COMMSTACK\http"

            if (Test-Path $commstackHttpSrc) {
                if (-not (Test-Path $commstackHttpDest)) {
                    New-Item -ItemType Directory -Path $commstackHttpDest -Force | Out-Null
                }

                # Back up the SG41_X-specific k_sal_com.c before the generic COMMSTACK/http
                # copy overwrites it.  This file contains the platform-specific HTTP SAL
                # implementation and must NOT be replaced by the generic version.
                $kSalComSrc    = Join-Path $commstackHttpDest "k_sal_com.c"
                $kSalComBackup = $null
                if (Test-Path $kSalComSrc) {
                    $kSalComBackup = Join-Path ([System.IO.Path]::GetTempPath()) ("k_sal_com_bak_" + [System.Guid]::NewGuid().ToString() + ".c")
                    Copy-Item $kSalComSrc -Destination $kSalComBackup -Force
                }

                Copy-Item -Path "$commstackHttpSrc\*" -Destination $commstackHttpDest -Recurse -Force

                # Restore the backed-up k_sal_com.c so the generic copy does not persist.
                if ($kSalComBackup -and (Test-Path $kSalComBackup)) {
                    Copy-Item $kSalComBackup -Destination $kSalComSrc -Force
                    Remove-Item $kSalComBackup -Force -ErrorAction SilentlyContinue
                }

                # Ensure no stale salapi/ subfolder remains
                $staleDir = Join-Path $commstackHttpDest "salapi"
                if (Test-Path $staleDir) { Remove-Item $staleDir -Recurse -Force }
            } else {
                Write-Host "WARNING: COMMSTACK/http source not found at: $commstackHttpSrc" -ForegroundColor Yellow
            }
        }

        # Remove legacy firmware/ folder (keystream_fota only — uses src/ layout)
        if ($appName -eq 'keystream_fota') {
            Remove-Item (Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\$appName\firmware") -Recurse -Force -ErrorAction SilentlyContinue
        }

        $xProjPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\$appName\keystream_connect.X"
        Write-Host ""
        Write-Host "====================================================================="
        Write-Host "Next Step ($appName):"
        Write-Host "Please open the following .X project file in MPLAB X IDE to continue:"
        Write-Host ""
        Write-Host "    $xProjPath"
        Write-Host ""
        Write-Host "====================================================================="
    }

    # Remove App_Config.h — keystream_fota uses fota_app_config.h; App_Config.h is not needed
    $appConfigToRemove = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\fota_app_config.h"
    Remove-Item $appConfigToRemove -Force -ErrorAction SilentlyContinue
}

elseif ($selectedSubFolder -eq "ESP32_BLE_CLI_X")
{
    # ═══════════════════════════════════════════════════════════════════════════════
    # DEVICE: ESP32_BLE_CLI_X  (ESP32 + ATECC608 — Windows Python BLE CLI)
    # ───────────────────────────────────────────────────────────────────────────
    # Configures the ESP32 BLE CLI firmware project for keySTREAM provisioning.
    #   Step 1 : Copies KTA SOURCE → firmware/main/kta_provisioning/SOURCE/
    #              – Removes salSignHash API (not supported on this ESP32 build)
    #   Step 2 : Copies logmodule only from KTA COMMON
    #              (transport_interface files already exist in target — not overwritten)
    #   Step 3 : Patches ktaConfig.h
    #              – Injects #include "App_Config.h"
    #              – Replaces C_KTA_APP__DEVICE_PUBLIC_UID with portal macro
    #              – Uncomments NETWORK_STACK_AVAILABLE
    #   Step 4 : Copies cust_def_* / ktaFieldMgntHook → firmware/main/
    #   Step 5 : Copies cryptoauthlib (excluding hal/ and atca_config.h)
    #   Step 6 : Copies Kta_Unified MCU layer → firmware/main/kta_provisioning/mcu/
    #              • backends/ble/  (root files + sal/esp32/ble_config.h only)
    #              • backends/backend_interface.c / .h
    #              • bridgeKta/  (full)
    #              • examples/common/  (full)
    #              • examples/platform/freertos/  (full)
    #              • examples/platform/kta_mcu_integration.h
    # ═══════════════════════════════════════════════════════════════════════════════
    Write-Host "Performing ESP32_BLE_CLI_X operations..."

    $preConfigRoot = Split-Path $currentPath -Parent
    $ktaLibPath = Join-Path $preConfigRoot "kta_lib\kta_provisioning"

    if (-not (Test-Path $ktaLibPath)) {
        Write-Host "The 'kta_lib\kta_provisioning' folder was not found in the current directory structure."
        exit
    }

    # 1. Copy SOURCE folder to ...\firmware\main\kta_provisioning\SOURCE
    $sourcePath = Join-Path $ktaLibPath "SOURCE"
    $destPath = Join-Path $trustManagerPath "$selectedSubFolder\firmware\main\kta_provisioning\SOURCE"

    if (Test-Path $destPath) {
        Remove-Item $destPath -Recurse -Force
    }
    Copy-Item $sourcePath -Destination $destPath -Recurse -Force

    # Remove salSignHash API from copied k_sal_crypto.c (causes build failure)
    $kSalCryptoPath = Join-Path $destPath "salapi\k_sal_crypto.c"
    if (Test-Path $kSalCryptoPath) {
        $content = Get-Content $kSalCryptoPath -Raw
        $content = [regex]::Replace($content, '(?s)/\*\*\s+\*\s+@brief implement salSignHash\s+\*\s+\*/\s+TKStatus salSignHash[\s\S]+?\n\}', '')
        Set-Content $kSalCryptoPath -Value $content -NoNewline
    }

    # 1a. Copy logmodule from COMMON into ...\firmware\main\kta_provisioning\COMMON
    # (existing transport_interface files are preserved; only logmodule is copied)
    $commonSourcePath = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\kta_lib\COMMON"
    $commonDestPath = Join-Path $trustManagerPath "$selectedSubFolder\firmware\main\kta_provisioning\COMMON"

    if (-not (Test-Path $commonDestPath)) {
        New-Item -ItemType Directory -Path $commonDestPath -Force | Out-Null
    }
    $logModuleSrc = Join-Path $commonSourcePath "logmodule"
    if (Test-Path $logModuleSrc) {
        Copy-Item $logModuleSrc -Destination $commonDestPath -Recurse -Force
    } else {
        Write-Host "logmodule not found at: $logModuleSrc"
    }

    # Overwrite ktaConfig.h with the latest version
    $srcKtaConfig = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\kta_lib\SOURCE\include\ktaConfig.h"
    $destKtaConfig = Join-Path $destPath "include\ktaConfig.h"
    Copy-Item $srcKtaConfig -Destination $destKtaConfig -Force

    # # --- Custom script for ktaConfig.h ---
    $ktaConfigPath = Join-Path $destPath "include\ktaConfig.h"
    if (Test-Path $ktaConfigPath) {
        $lines = Get-Content $ktaConfigPath
        if ($lines.Count -ge 56) {
            $lines = $lines[0..54] + '#include "App_Config.h"' + $lines[55..($lines.Count-1)]
            $lines | Set-Content $ktaConfigPath
        } else {
            Add-Content $ktaConfigPath '#include "App_Config.h"'
        }

        # 1. Replace C_KTA_APP__DEVICE_PUBLIC_UID with required macro (handle both commented and uncommented)
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*(?://\s*)?#define\s+C_KTA_APP__DEVICE_PUBLIC_UID\b') {
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

    # 2. Copy other files to ...\firmware\main\
    $destMain = Join-Path $trustManagerPath "$selectedSubFolder\firmware\main"
    if (-not (Test-Path $destMain)) {
        New-Item -ItemType Directory -Path $destMain -Force | Out-Null
    }

    Copy-Item "$ktaLibPath\cust_def_device.c"    -Destination $destMain -Force
    Copy-Item "$ktaLibPath\cust_def_device.h"    -Destination $destMain -Force
    Copy-Item "$ktaLibPath\cust_def_signer.c"    -Destination $destMain -Force
    Copy-Item "$ktaLibPath\cust_def_signer.h"    -Destination $destMain -Force
    Copy-Item "$ktaLibPath\ktaFieldMgntHook.c"   -Destination $destMain -Force
    Copy-Item "$ktaLibPath\ktaFieldMgntHook.h"   -Destination $destMain -Force

    # 3. Copy cryptoauthlib to ...\firmware\components\cryptoauthlib\lib
    $srcCryptoAuthLib = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\cryptoauthlib"
    $dstCryptoAuthLib = Join-Path $trustManagerPath "$selectedSubFolder\firmware\components\cryptoauthlib\lib"

    if (Test-Path $srcCryptoAuthLib) {
        if (-not (Test-Path $dstCryptoAuthLib)) {
            New-Item -ItemType Directory -Path $dstCryptoAuthLib -Force | Out-Null
        }

        # Copy all .c and .h files from cryptoauthlib (root and subfolders), EXCLUDING hal and atca_config.h
        Get-ChildItem -Path $srcCryptoAuthLib -Include *.c,*.h -File -Recurse | Where-Object {
            $_.FullName -notmatch '\\hal\\' -and $_.Name -ne 'atca_config.h'
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

    # ── KTA Unified MCU ────────────────────────────────────────────────
    $ktaUnifiedMcu = Join-Path $preConfigRoot "Kta_Unified\mcu"
    $destMcuRoot   = Join-Path $trustManagerPath "$selectedSubFolder\firmware\main\kta_provisioning\mcu"

    if (-not (Test-Path $ktaUnifiedMcu)) {
        Write-Host "Kta_Unified\mcu not found at: $ktaUnifiedMcu"
    } else {

        # 1. backends/ — only ble/ (full), but inside ble/sal/ keep only esp32/
        #    + backend_interface.c / backend_interface.h at backends root
        $destBackends = Join-Path $destMcuRoot "backends"

        # 1a. ble/ root files (backend_ble.c, ble_sal.h)
        $srcBle  = Join-Path $ktaUnifiedMcu "backends\ble"
        $destBle = Join-Path $destBackends "ble"
        if (-not (Test-Path $destBle)) { New-Item -ItemType Directory -Path $destBle -Force | Out-Null }
        Get-ChildItem -Path $srcBle -File | ForEach-Object {
            Copy-Item $_.FullName -Destination $destBle -Force
        }

        # 1b. ble/sal/esp32/ only
        $srcEsp32  = Join-Path $srcBle "sal\esp32"
        $destSalEsp32 = Join-Path $destBle "sal\esp32"
        if (-not (Test-Path $destSalEsp32)) { New-Item -ItemType Directory -Path $destSalEsp32 -Force | Out-Null }
        # Copy only ble_config.h from esp32/
        $bleConfigSrc = Join-Path $srcEsp32 "ble_config.h"
        if (Test-Path $bleConfigSrc) {
            Copy-Item $bleConfigSrc -Destination $destSalEsp32 -Force
        } else {
            Write-Host "ble_config.h not found at: $bleConfigSrc"
        }

        # 1c. backend_interface.c and backend_interface.h
        Copy-Item (Join-Path $ktaUnifiedMcu "backends\backend_interface.c") -Destination $destBackends -Force
        Copy-Item (Join-Path $ktaUnifiedMcu "backends\backend_interface.h") -Destination $destBackends -Force

        # 2. bridgeKta/ — copy in full
        $srcBridgeKta  = Join-Path $ktaUnifiedMcu "bridgeKta"
        $destBridgeKta = Join-Path $destMcuRoot "bridgeKta"
        if (Test-Path $destBridgeKta) { Remove-Item $destBridgeKta -Recurse -Force }
        Copy-Item $srcBridgeKta -Destination $destBridgeKta -Recurse -Force

        # 3. examples/ — common/ (full) + platform/freertos/ + platform/kta_mcu_integration.h
        $destExamples = Join-Path $destMcuRoot "examples"

        # 3a. examples/common/ full
        $srcCommonEx  = Join-Path $ktaUnifiedMcu "examples\common"
        $destCommonEx = Join-Path $destExamples "common"
        if (Test-Path $destCommonEx) { Remove-Item $destCommonEx -Recurse -Force }
        Copy-Item $srcCommonEx -Destination $destCommonEx -Recurse -Force

        # 3b. examples/platform/freertos/ full
        $srcFreertos  = Join-Path $ktaUnifiedMcu "examples\platform\freertos"
        $destPlatform = Join-Path $destExamples "platform"
        $destFreertos = Join-Path $destPlatform "freertos"
        if (-not (Test-Path $destPlatform)) { New-Item -ItemType Directory -Path $destPlatform -Force | Out-Null }
        if (Test-Path $destFreertos) { Remove-Item $destFreertos -Recurse -Force }
        Copy-Item $srcFreertos -Destination $destFreertos -Recurse -Force

        # 3c. examples/platform/kta_mcu_integration.h
        $srcIntegH = Join-Path $ktaUnifiedMcu "examples\platform\kta_mcu_integration.h"
        if (Test-Path $srcIntegH) {
            Copy-Item $srcIntegH -Destination $destPlatform -Force
        } else {
            Write-Host "kta_mcu_integration.h not found at: $srcIntegH"
        }

    }

    $esp32ProjPath = Join-Path $trustManagerPath "$selectedSubFolder\firmware"
    Write-Host ""
    Write-Host "====================================================================="
    Write-Host "Next Step:"
    Write-Host "Please open and build the following ESP-IDF firmware project:"
    Write-Host ""
    Write-Host "    $esp32ProjPath"
    Write-Host ""
    Write-Host "====================================================================="
}
elseif ($selectedSubFolder -eq "ESP32_Mobile_APP_X")
{
    # ═══════════════════════════════════════════════════════════════════════════════
    # DEVICE: ESP32_Mobile_APP_X  (ESP32 + ATECC608 — Mobile App BLE provisioning)
    # ───────────────────────────────────────────────────────────────────────────
    # Configures the ESP32 Mobile App BLE firmware project for keySTREAM provisioning.
    # Folder layout mirrors ESP32_BLE_CLI_X (sources go under kta_provisioning/).
    #   Step 1 : Copies KTA SOURCE → firmware/main/kta_provisioning/SOURCE/
    #              – Removes salSignHash API (not supported on this ESP32 build)
    #   Step 2 : Copies logmodule only from KTA COMMON
    #              (transport_interface files already exist in target — not overwritten)
    #   Step 3 : Patches ktaConfig.h
    #              – Replaces C_KTA_APP__DEVICE_PUBLIC_UID with portal macro
    #              – Uncomments NETWORK_STACK_AVAILABLE
    #              – NOTE: #include "App_Config.h" is intentionally NOT injected;
    #                the Mobile App manages its own app-level configuration.
    #   Step 4 : Copies cust_def_* / ktaFieldMgntHook → firmware/main/
    #   Step 5 : Copies cryptoauthlib (excluding hal/ and atca_config.h)
    #   Step 6 : Copies Kta_Unified MCU layer → firmware/main/kta_provisioning/mcu/
    #              • backends/ble/  (root files + sal/esp32/ble_config.h only)
    #              • backends/backend_interface.c / .h
    #              • bridgeKta/  (full)
    #              • examples/common/  (full)
    #              • examples/platform/freertos/  (full)
    #              • examples/platform/kta_mcu_integration.h
    # ═══════════════════════════════════════════════════════════════════════════════
    Write-Host "Performing ESP32_Mobile_APP_X operations..."

    $preConfigRoot = Split-Path $currentPath -Parent
    $ktaLibPath    = Join-Path $preConfigRoot "kta_lib\kta_provisioning"

    if (-not (Test-Path $ktaLibPath)) {
        Write-Host "The 'kta_lib\kta_provisioning' folder was not found in the current directory structure."
        exit
    }

    # ── Step 1 : Copy KTA SOURCE ──────────────────────────────────────────────
    # Destination mirrors the ESP32_BLE_CLI_X layout: firmware/main/kta_provisioning/SOURCE
    $sourcePath = Join-Path $ktaLibPath "SOURCE"
    $destPath   = Join-Path $trustManagerPath "$selectedSubFolder\firmware\main\kta_provisioning\SOURCE"

    if (Test-Path $destPath) {
        Remove-Item $destPath -Recurse -Force
    }
    Copy-Item $sourcePath -Destination $destPath -Recurse -Force

    # Remove salSignHash API — causes a build failure on the Mobile App ESP32 target.
    # The signing-key service is not yet supported on this platform.
    $kSalCryptoPath = Join-Path $destPath "salapi\k_sal_crypto.c"
    if (Test-Path $kSalCryptoPath) {
        $content = Get-Content $kSalCryptoPath -Raw
        $content = [regex]::Replace($content,
            '(?s)/\*\*\s+\*\s+@brief implement salSignHash\s+\*\s+\*/\s+TKStatus salSignHash[\s\S]+?\n\}', '')
        Set-Content $kSalCryptoPath -Value $content -NoNewline
    }

    # Patch ktamgr.c — replace the ktaSignHash function with a not-supported stub.
    # salSignHash was removed from k_sal_crypto.c above; the ktamgr-level wrapper
    # must also be replaced so the Mobile App build does not fail due to an
    # unresolved or broken call to the missing signing-key service.
    $ktaMgrPath = Join-Path $destPath "kta\ktamgr\ktamgr.c"
    if (Test-Path $ktaMgrPath) {
        $ktaSignHashStub = @'
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaSignHash
(
  uint32_t  xKeyId,
  uint8_t*  xpHash,
  size_t    xHashLen,
  uint8_t*  xpSignedHashOutBuff,
  uint32_t  xSignedHashOutBuffLen,
  size_t*   xpActualSignedHashOutLen
)
{
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  // REQ RQ_M-KTA-STRT-FN-0600(1) : Input Parameters Check
  if (
    (NULL == xpHash) ||
    (NULL == xpSignedHashOutBuff) ||
    (NULL == xpActualSignedHashOutLen) ||
    (0u == xHashLen) ||
    (0u == xSignedHashOutBuffLen) ||
    (32u < xHashLen) ||
    (64u < xSignedHashOutBuffLen)
  )
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("[ktaSignHash]Invalid parameter passed");
    goto end;
  }

  // REQ RQ_M-KTA-STRT-FN-0610(1) : Sign hash data - not supported
  M_KTALOG__ERR("ktaSignHash not supported");
  status = E_K_STATUS_ERROR;

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}
#endif
'@
        $content = Get-Content $ktaMgrPath -Raw
        # The optional doc-comment body uses (?:(?!\*/)[\s\S])*? so it cannot span
        # across a '*/' (or '**/') terminator. Without this bound, the lazy match
        # started at the file-header comment and consumed everything down to
        # ktaSignHash — deleting all #include directives and the
        # '#ifdef OBJECT_MANAGEMENT_FEATURE' that the trailing '#endif' belongs to
        # (errors: 'size_t' undefined + '#endif without #if').
        $content = [regex]::Replace($content,
            '(?s)(?:/\*\*(?:(?!\*/)[\s\S])*?\*/\s*)?TKStatus\s+ktaSignHash\s*\([\s\S]+?\n\}(?:\s*\r?\n#endif)?',
            $ktaSignHashStub)
        Set-Content $ktaMgrPath -Value $content -NoNewline
    }

    # Remove salSignHash declaration from k_sal_crypto.h — the Mobile App does not
    # use signing-key services; keeping the declaration causes compiler errors when
    # the implementation is absent.
    $kSalCryptoHPath = Join-Path $destPath "salapi\include\k_sal_crypto.h"
    if (Test-Path $kSalCryptoHPath) {
        $content = Get-Content $kSalCryptoHPath -Raw
        # The optional doc-comment body uses (?:(?!\*/)[\s\S])*? so it cannot span
        # across a '*/' terminator. This ensures only the single doc comment
        # immediately preceding salSignHash is removed — without the bound, the
        # lazy match grabbed everything from the file header down to salSignHash,
        # deleting the '#ifndef/#define K_SAL_CRYPTO_H' include guard and leaving
        # an orphan '#endif' (error: #endif without #if).
        $content = [regex]::Replace($content,
            '(?s)(?:/\*\*(?:(?!\*/)[\s\S])*?\*/\s*)?TKStatus\s+salSignHash\s*\([\s\S]+?\);\s*',
            '')
        Set-Content $kSalCryptoHPath -Value $content -NoNewline
    }

    # ── Step 2 : Copy KTA COMMON (logmodule only) ─────────────────────────────
    # Only the logmodule sub-folder is copied.  The transport_interface files are
    # already present in the Mobile App target firmware and must not be overwritten.
    $commonSourcePath = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\kta_lib\COMMON"
    $commonDestPath   = Join-Path $trustManagerPath "$selectedSubFolder\firmware\main\kta_provisioning\COMMON"

    if (-not (Test-Path $commonDestPath)) {
        New-Item -ItemType Directory -Path $commonDestPath -Force | Out-Null
    }
    $logModuleSrc = Join-Path $commonSourcePath "logmodule"
    if (Test-Path $logModuleSrc) {
        Copy-Item $logModuleSrc -Destination $commonDestPath -Recurse -Force
    } else {
        Write-Host "logmodule not found at: $logModuleSrc"
    }

    # ── Step 3 : Patch ktaConfig.h ────────────────────────────────────────────
    # Overwrite with the latest baseline, then apply Mobile-App-specific patches.
    # IMPORTANT: #include "App_Config.h" is intentionally NOT injected here.
    #            The Mobile App firmware has its own app-level configuration mechanism.
    $srcKtaConfig  = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\kta_lib\SOURCE\include\ktaConfig.h"
    $destKtaConfig = Join-Path $destPath "include\ktaConfig.h"
    Copy-Item $srcKtaConfig -Destination $destKtaConfig -Force

    $ktaConfigPath = Join-Path $destPath "include\ktaConfig.h"
    if (Test-Path $ktaConfigPath) {
        # Patch 3a: Replace C_KTA_APP__DEVICE_PUBLIC_UID with the keySTREAM portal
        #           profile macro so the correct fleet is targeted at runtime.
        #           Handle both commented and uncommented versions.
        $lines = Get-Content $ktaConfigPath
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*(?://\s*)?#define\s+C_KTA_APP__DEVICE_PUBLIC_UID\b') {
                '#define C_KTA_APP__DEVICE_PUBLIC_UID                 KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID'
            } else { $_ }
        }

        # Patch 3b: Uncomment NETWORK_STACK_AVAILABLE so the TCP/IP stack is
        #           activated and the device can reach the keySTREAM cloud.
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*//\s*#define\s+NETWORK_STACK_AVAILABLE\b') {
                ' #define NETWORK_STACK_AVAILABLE'
            } else { $_ }
        }

        $lines | Set-Content $ktaConfigPath
    }

    # ── Step 4 : Copy cust_def and ktaFieldMgntHook support files ────────────
    # These files implement the custom device/signer certificate definitions
    # and the KTA field management hook required by the provisioning flow.
    $destMain = Join-Path $trustManagerPath "$selectedSubFolder\firmware\main"
    if (-not (Test-Path $destMain)) {
        New-Item -ItemType Directory -Path $destMain -Force | Out-Null
    }

    Copy-Item "$ktaLibPath\cust_def_device.c"  -Destination $destMain -Force
    Copy-Item "$ktaLibPath\cust_def_device.h"  -Destination $destMain -Force
    Copy-Item "$ktaLibPath\cust_def_signer.c"  -Destination $destMain -Force
    Copy-Item "$ktaLibPath\cust_def_signer.h"  -Destination $destMain -Force
    Copy-Item "$ktaLibPath\ktaFieldMgntHook.c" -Destination $destMain -Force
    Copy-Item "$ktaLibPath\ktaFieldMgntHook.h" -Destination $destMain -Force

    # ── Step 5 : Copy cryptoauthlib ───────────────────────────────────────────
    # hal/ is excluded — the ESP32 HAL is already part of the ESP-IDF component.
    # atca_config.h is excluded — the target has its own ESP32-specific version.
    $srcCryptoAuthLib = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\cryptoauthlib"
    $dstCryptoAuthLib = Join-Path $trustManagerPath "$selectedSubFolder\firmware\components\cryptoauthlib\lib"

    if (Test-Path $srcCryptoAuthLib) {
        if (-not (Test-Path $dstCryptoAuthLib)) {
            New-Item -ItemType Directory -Path $dstCryptoAuthLib -Force | Out-Null
        }

        Get-ChildItem -Path $srcCryptoAuthLib -Include *.c,*.h -File -Recurse | Where-Object {
            $_.FullName -notmatch '\\hal\\' -and $_.Name -ne 'atca_config.h'
        } | ForEach-Object {
            $dest    = Join-Path $dstCryptoAuthLib ($_.FullName.Substring($srcCryptoAuthLib.Length).TrimStart('\'))
            $destDir = Split-Path $dest -Parent
            if (-not (Test-Path $destDir)) {
                New-Item -ItemType Directory -Path $destDir -Force | Out-Null
            }
            Copy-Item $_.FullName -Destination $dest -Force
        }

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

    # ── Step 6 : Copy Kta_Unified MCU layer ──────────────────────────────────
    # The Kta_Unified MCU layer provides the platform-agnostic abstraction between
    # the KTA library and the underlying BLE / FreeRTOS transport on ESP32.
    # Only the ESP32 BLE / FreeRTOS-specific parts are included:
    #   backends/ble/              – BLE backend implementation + ESP32 SAL config
    #   backend_interface.c / .h   – Common backend contract (shared by all backends)
    #   bridgeKta/                 – Bridge between KTA library and MCU layer (full)
    #   examples/common/           – Common example glue code shared across platforms
    #   examples/platform/freertos/– FreeRTOS task entry points and hooks
    #   examples/platform/kta_mcu_integration.h – MCU/KTA integration contract
    $ktaUnifiedMcu = Join-Path $preConfigRoot "Kta_Unified\mcu"
    $destMcuRoot   = Join-Path $trustManagerPath "$selectedSubFolder\firmware\main\kta_provisioning\mcu"

    if (-not (Test-Path $ktaUnifiedMcu)) {
        Write-Host "Kta_Unified\mcu not found at: $ktaUnifiedMcu"
    } else {

        # 6a. BLE backend root files (e.g. backend_ble.c, ble_sal.h)
        $srcBle  = Join-Path $ktaUnifiedMcu "backends\ble"
        $destBle = Join-Path $destMcuRoot "backends\ble"
        if (-not (Test-Path $destBle)) { New-Item -ItemType Directory -Path $destBle -Force | Out-Null }
        Get-ChildItem -Path $srcBle -File | ForEach-Object {
            Copy-Item $_.FullName -Destination $destBle -Force
        }

        # 6b. BLE SAL — only the ESP32-specific config header (ble_config.h).
        #     Other SAL platform folders are excluded (not needed for Mobile App).
        $srcEsp32     = Join-Path $srcBle "sal\esp32"
        $destSalEsp32 = Join-Path $destBle "sal\esp32"
        if (-not (Test-Path $destSalEsp32)) { New-Item -ItemType Directory -Path $destSalEsp32 -Force | Out-Null }
        $bleConfigSrc = Join-Path $srcEsp32 "ble_config.h"
        if (Test-Path $bleConfigSrc) {
            Copy-Item $bleConfigSrc -Destination $destSalEsp32 -Force
        } else {
            Write-Host "ble_config.h not found at: $bleConfigSrc"
        }

        # 6c. Backend interface — common transport contract used by all backends
        $destBackends = Join-Path $destMcuRoot "backends"
        Copy-Item (Join-Path $ktaUnifiedMcu "backends\backend_interface.c") -Destination $destBackends -Force
        Copy-Item (Join-Path $ktaUnifiedMcu "backends\backend_interface.h") -Destination $destBackends -Force

        # 6d. BridgeKta — KTA library bridge layer (full copy)
        $srcBridgeKta  = Join-Path $ktaUnifiedMcu "bridgeKta"
        $destBridgeKta = Join-Path $destMcuRoot "bridgeKta"
        if (Test-Path $destBridgeKta) { Remove-Item $destBridgeKta -Recurse -Force }
        Copy-Item $srcBridgeKta -Destination $destBridgeKta -Recurse -Force

        # 6e. Examples — common glue code shared across all platforms (full copy)
        $srcCommonEx  = Join-Path $ktaUnifiedMcu "examples\common"
        $destCommonEx = Join-Path $destMcuRoot "examples\common"
        if (Test-Path $destCommonEx) { Remove-Item $destCommonEx -Recurse -Force }
        Copy-Item $srcCommonEx -Destination $destCommonEx -Recurse -Force

        # 6f. Examples — FreeRTOS platform entry points (RTOS tasks, hooks)
        $srcFreertos  = Join-Path $ktaUnifiedMcu "examples\platform\freertos"
        $destPlatform = Join-Path $destMcuRoot "examples\platform"
        $destFreertos = Join-Path $destPlatform "freertos"
        if (-not (Test-Path $destPlatform)) { New-Item -ItemType Directory -Path $destPlatform -Force | Out-Null }
        if (Test-Path $destFreertos) { Remove-Item $destFreertos -Recurse -Force }
        Copy-Item $srcFreertos -Destination $destFreertos -Recurse -Force

        # 6g. Platform integration header — MCU/KTA integration contract
        $srcIntegH = Join-Path $ktaUnifiedMcu "examples\platform\kta_mcu_integration.h"
        if (Test-Path $srcIntegH) {
            Copy-Item $srcIntegH -Destination $destPlatform -Force
        } else {
            Write-Host "kta_mcu_integration.h not found at: $srcIntegH"
        }

    }

    $esp32ProjPath = Join-Path $trustManagerPath "$selectedSubFolder\firmware"
    Write-Host ""
    Write-Host "====================================================================="
    Write-Host "Next Step:"
    Write-Host "Please open and build the following ESP-IDF firmware project:"
    Write-Host ""
    Write-Host "    $esp32ProjPath"
    Write-Host ""
    Write-Host "====================================================================="
}
elseif ($selectedSubFolder -eq "SG41_UART_X")
{
    # ═══════════════════════════════════════════════════════════════════════════════
    # DEVICE: SG41_UART_X  (SG41 MCU ↔ Windows Gateway via UART)
    # ───────────────────────────────────────────────────────────────────────────
    # Configures the SG41 UART bridge project for keySTREAM provisioning.
    # Architecture: MCU connects to a Windows PC gateway over UART; the gateway
    # handles all cloud communication (HTTP to keySTREAM).
    #
    # GATEWAY SIDE (Windows PC executable):
    #   Step 1 : Copies application/main_windows_async_kta.c
    #   Step 2 : Copies backends/ root files (backend_interface.c/h, backend_message.c/h)
    #   Step 3 : Copies backends/uart/ (backend_uart.c, uart_sal.h)
    #   Step 4 : Copies backends/uart/sal/windows/ folder
    #   Step 5 : Copies keyStreamIntegration/COMMON/ folder
    #   Step 6 : Copies keyStreamIntegration/COMMSTACK/http/ (http.c, include/, windows/, comm_if.c)
    #   Step 7 : Copies ktaIntegration/ (ktaFieldMgntHook.c/h)
    #   Step 8 : Copies ktaIntegration/platform/ (windows/, include/)
    #
    # MCU SIDE (SG41 firmware — kta_lib):
    #   Step 1 : Copies mcu/examples/common/
    #   Step 2 : Copies mcu/examples/platform/kta_mcu_integration.h + bare_metal/
    #   Step 3 : Copies mcu/bridgeKta/ (full)
    #   Step 4 : Copies mcu/backends/backend_interface.c/h
    #   Step 5 : Copies mcu/backends/uart/ (backend_uart.c, uart_sal.h)
    #   Step 6 : Copies standard KTA lib (SOURCE, COMMON, COMMSTACK, patches ktaConfig.h)
    #   Step 7 : Patches k_sal_log.c — adds #include "definitions.h" and USB-CDC salPrint
    # ═══════════════════════════════════════════════════════════════════════════════
    Write-Host "Performing SG41_UART_X operations..."

    $preConfigRoot  = Split-Path $currentPath -Parent
    $ktaUnified     = Join-Path $preConfigRoot "Kta_Unified"
    $ktaUnifiedGw   = Join-Path $ktaUnified "gateway"
    $ktaUnifiedMcu  = Join-Path $ktaUnified "mcu"

    if (-not (Test-Path $ktaUnifiedGw)) {
        Write-Host "Kta_Unified\gateway not found at: $ktaUnifiedGw"
        exit
    }
    if (-not (Test-Path $ktaUnifiedMcu)) {
        Write-Host "Kta_Unified\mcu not found at: $ktaUnifiedMcu"
        exit
    }

    # ── GATEWAY SIDE ─────────────────────────────────────────────────────────
    $gwDest = Join-Path $trustManagerPath "$selectedSubFolder\gateway"

    # Step 1: application/main_windows_async_kta.c
    $destApp = Join-Path $gwDest "application"
    if (-not (Test-Path $destApp)) { New-Item -ItemType Directory -Path $destApp -Force | Out-Null }
    Copy-Item (Join-Path $ktaUnifiedGw "application\main_windows_async_kta.c") -Destination $destApp -Force

    # Step 2: backends/ root .c and .h files
    $destBackends = Join-Path $gwDest "backends"
    if (-not (Test-Path $destBackends)) { New-Item -ItemType Directory -Path $destBackends -Force | Out-Null }
    Copy-Item (Join-Path $ktaUnifiedGw "backends\backend_interface.c") -Destination $destBackends -Force
    Copy-Item (Join-Path $ktaUnifiedGw "backends\backend_interface.h") -Destination $destBackends -Force
    Copy-Item (Join-Path $ktaUnifiedGw "backends\backend_message.c")   -Destination $destBackends -Force
    Copy-Item (Join-Path $ktaUnifiedGw "backends\backend_message.h")   -Destination $destBackends -Force

    # Step 3: backends/uart/ (backend_uart.c, uart_sal.h)
    $destUart = Join-Path $destBackends "uart"
    if (-not (Test-Path $destUart)) { New-Item -ItemType Directory -Path $destUart -Force | Out-Null }
    Copy-Item (Join-Path $ktaUnifiedGw "backends\uart\backend_uart.c") -Destination $destUart -Force
    Copy-Item (Join-Path $ktaUnifiedGw "backends\uart\uart_sal.h")     -Destination $destUart -Force

    # Step 4: backends/uart/sal/windows/ folder
    # Note: uart_config.h is intentionally excluded — it must not be overwritten.
    $srcUartSalWin  = Join-Path $ktaUnifiedGw "backends\uart\sal\windows"
    $destUartSalWin = Join-Path $destUart "sal\windows"
    if (-not (Test-Path $destUartSalWin)) { New-Item -ItemType Directory -Path $destUartSalWin -Force | Out-Null }
    Get-ChildItem -Path $srcUartSalWin -File -Recurse | Where-Object { $_.Name -ne 'uart_config.h' } | ForEach-Object {
        $dest    = Join-Path $destUartSalWin ($_.FullName.Substring($srcUartSalWin.Length).TrimStart('\'))
        $destDir = Split-Path $dest -Parent
        if (-not (Test-Path $destDir)) { New-Item -ItemType Directory -Path $destDir -Force | Out-Null }
        Copy-Item $_.FullName -Destination $dest -Force
    }

    # Step 5: keyStreamIntegration/COMMON/ folder
    $srcKsCommon = Join-Path $ktaUnifiedGw "keyStreamIntegration\COMMON"
    $destKsCommon = Join-Path $gwDest "keyStreamIntegration\COMMON"
    if (Test-Path $destKsCommon) { Remove-Item $destKsCommon -Recurse -Force }
    Copy-Item $srcKsCommon -Destination $destKsCommon -Recurse -Force

    # Step 6: keyStreamIntegration/COMMSTACK/http/ (http.c, include/, windows/, comm_if.c)
    $srcHttpBase = Join-Path $ktaUnifiedGw "keyStreamIntegration\COMMSTACK\http"
    $destHttp = Join-Path $gwDest "keyStreamIntegration\COMMSTACK\http"
    if (-not (Test-Path $destHttp)) { New-Item -ItemType Directory -Path $destHttp -Force | Out-Null }
    Copy-Item (Join-Path $srcHttpBase "http.c")    -Destination $destHttp -Force
    Copy-Item (Join-Path $srcHttpBase "comm_if.c") -Destination $destHttp -Force
    # include/ folder
    $srcHttpInclude = Join-Path $srcHttpBase "include"
    $destHttpInclude = Join-Path $destHttp "include"
    if (Test-Path $destHttpInclude) { Remove-Item $destHttpInclude -Recurse -Force }
    Copy-Item $srcHttpInclude -Destination $destHttpInclude -Recurse -Force
    # windows/ folder
    $srcHttpWin = Join-Path $srcHttpBase "windows"
    $destHttpWin = Join-Path $destHttp "windows"
    if (Test-Path $destHttpWin) { Remove-Item $destHttpWin -Recurse -Force }
    Copy-Item $srcHttpWin -Destination $destHttpWin -Recurse -Force

    # Step 7: ktaIntegration/ (ktaFieldMgntHook.c/h)
    $destKtaInteg = Join-Path $gwDest "ktaIntegration"
    if (-not (Test-Path $destKtaInteg)) { New-Item -ItemType Directory -Path $destKtaInteg -Force | Out-Null }
    Copy-Item (Join-Path $ktaUnifiedGw "ktaIntegration\ktaFieldMgntHook.c") -Destination $destKtaInteg -Force
    Copy-Item (Join-Path $ktaUnifiedGw "ktaIntegration\ktaFieldMgntHook.h") -Destination $destKtaInteg -Force

    # Step 8: ktaIntegration/platform/ (windows/, include/)
    $srcPlatform = Join-Path $ktaUnifiedGw "ktaIntegration\platform"
    $destPlatform = Join-Path $destKtaInteg "platform"
    if (-not (Test-Path $destPlatform)) { New-Item -ItemType Directory -Path $destPlatform -Force | Out-Null }
    # windows/ folder
    $srcPlatWin = Join-Path $srcPlatform "windows"
    $destPlatWin = Join-Path $destPlatform "windows"
    if (Test-Path $destPlatWin) { Remove-Item $destPlatWin -Recurse -Force }
    Copy-Item $srcPlatWin -Destination $destPlatWin -Recurse -Force
    # include/ folder
    $srcPlatInclude = Join-Path $srcPlatform "include"
    $destPlatInclude = Join-Path $destPlatform "include"
    if (Test-Path $destPlatInclude) { Remove-Item $destPlatInclude -Recurse -Force }
    Copy-Item $srcPlatInclude -Destination $destPlatInclude -Recurse -Force

    # ── MCU SIDE ─────────────────────────────────────────────────────────────
    $mcuKtaLibDest = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\src\config\default\library\kta_lib"

    # ── COMMSTACK preservation ────────────────────────────────────────────────
    # COMMSTACK is already present in the project template and must NOT be removed
    # or overwritten by any of the copy operations below.  Back it up now so it
    # can be restored automatically if something unexpectedly removes it.
    $destCommStack      = Join-Path $mcuKtaLibDest "COMMSTACK"
    $commStackBackupDir = $null
    if (Test-Path $destCommStack) {
        $commStackBackupDir = Join-Path ([System.IO.Path]::GetTempPath()) ("COMMSTACK_backup_" + [System.Guid]::NewGuid().ToString())
        Copy-Item $destCommStack -Destination $commStackBackupDir -Recurse -Force
    }
    # ─────────────────────────────────────────────────────────────────────────

    # MCU Step 1: mcu/examples/common/
    $srcMcuCommon = Join-Path $ktaUnifiedMcu "examples\common"
    $destMcuCommon = Join-Path $mcuKtaLibDest "mcu\examples\common"
    if (Test-Path $destMcuCommon) { Remove-Item $destMcuCommon -Recurse -Force }
    Copy-Item $srcMcuCommon -Destination $destMcuCommon -Recurse -Force

    # MCU Step 2: mcu/examples/platform/kta_mcu_integration.h + bare_metal/
    $destMcuPlatform = Join-Path $mcuKtaLibDest "mcu\examples\platform"
    if (-not (Test-Path $destMcuPlatform)) { New-Item -ItemType Directory -Path $destMcuPlatform -Force | Out-Null }
    $srcIntegH = Join-Path $ktaUnifiedMcu "examples\platform\kta_mcu_integration.h"
    if (Test-Path $srcIntegH) {
        Copy-Item $srcIntegH -Destination $destMcuPlatform -Force
    }
    $srcBareMetal = Join-Path $ktaUnifiedMcu "examples\platform\bare_metal"
    $destBareMetal = Join-Path $destMcuPlatform "bare_metal"
    if (Test-Path $destBareMetal) { Remove-Item $destBareMetal -Recurse -Force }
    Copy-Item $srcBareMetal -Destination $destBareMetal -Recurse -Force

    # MCU Step 3: mcu/bridgeKta/ (full)
    $srcBridgeKta = Join-Path $ktaUnifiedMcu "bridgeKta"
    $destBridgeKta = Join-Path $mcuKtaLibDest "mcu\bridgeKta"
    if (Test-Path $destBridgeKta) { Remove-Item $destBridgeKta -Recurse -Force }
    Copy-Item $srcBridgeKta -Destination $destBridgeKta -Recurse -Force

    # MCU Step 4: mcu/backends/backend_interface.c/h
    $destMcuBackends = Join-Path $mcuKtaLibDest "mcu\backends"
    if (-not (Test-Path $destMcuBackends)) { New-Item -ItemType Directory -Path $destMcuBackends -Force | Out-Null }
    Copy-Item (Join-Path $ktaUnifiedMcu "backends\backend_interface.c") -Destination $destMcuBackends -Force
    Copy-Item (Join-Path $ktaUnifiedMcu "backends\backend_interface.h") -Destination $destMcuBackends -Force

    # MCU Step 5: mcu/backends/uart/ (backend_uart.c, uart_sal.h)
    $destMcuUart = Join-Path $destMcuBackends "uart"
    if (-not (Test-Path $destMcuUart)) { New-Item -ItemType Directory -Path $destMcuUart -Force | Out-Null }
    Copy-Item (Join-Path $ktaUnifiedMcu "backends\uart\backend_uart.c") -Destination $destMcuUart -Force
    Copy-Item (Join-Path $ktaUnifiedMcu "backends\uart\uart_sal.h")     -Destination $destMcuUart -Force

    # MCU Step 6: Standard KTA lib (SOURCE, COMMON + ktaConfig.h patching)
    # SOURCE and cust_def come from the local kta_lib\kta_provisioning folder.
    # COMMON and ktaConfig.h baseline come from Harmony_Apps\ATEC608_late_provisioning_app.
    # COMMSTACK is NOT copied — it is already present in the project template.
    $ktaProvisioningPath = Join-Path $preConfigRoot "kta_lib\kta_provisioning"
    if (-not (Test-Path $ktaProvisioningPath)) {
        Write-Host "The 'kta_lib\kta_provisioning' folder was not found at: $ktaProvisioningPath"
        exit
    }
    $commonAppLibPath = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\kta_lib"
    if (-not (Test-Path $commonAppLibPath)) {
        Write-Host "The ATEC608_late_provisioning_app kta_lib was not found at: $commonAppLibPath"
        exit
    }

    # Ensure parent kta_lib directory exists before copying
    if (-not (Test-Path $mcuKtaLibDest)) {
        New-Item -ItemType Directory -Path $mcuKtaLibDest -Force | Out-Null
    }

    # Copy SOURCE from local kta_lib\kta_provisioning
    $srcSource = Join-Path $ktaProvisioningPath "SOURCE"
    $destSource = Join-Path $mcuKtaLibDest "SOURCE"
    if (Test-Path $destSource) { Remove-Item $destSource -Recurse -Force }
    Copy-Item $srcSource -Destination $destSource -Recurse -Force

    # Copy COMMON from ATEC608_late_provisioning_app
    $srcCommon = Join-Path $commonAppLibPath "COMMON"
    $destCommon = Join-Path $mcuKtaLibDest "COMMON"
    if (Test-Path $destCommon) { Remove-Item $destCommon -Recurse -Force }
    Copy-Item $srcCommon -Destination $destCommon -Recurse -Force

    # COMMSTACK is intentionally NOT copied — already present in the project template.

    # Copy utility files (ktaFieldMgntHook, cust_def) from local kta_lib\kta_provisioning
    $utilFiles = @("ktaFieldMgntHook.c", "ktaFieldMgntHook.h", "cust_def_device.c", "cust_def_device.h", "cust_def_signer.c", "cust_def_signer.h")
    foreach ($file in $utilFiles) {
        $srcFile = Join-Path $ktaProvisioningPath $file
        if (Test-Path $srcFile) {
            Copy-Item $srcFile -Destination $mcuKtaLibDest -Force
        }
    }

    # Overwrite ktaConfig.h with the latest version from ATEC608_late_provisioning_app
    $srcKtaConfig = Join-Path $commonAppLibPath "SOURCE\include\ktaConfig.h"
    $destKtaConfig = Join-Path $destSource "include\ktaConfig.h"
    Copy-Item $srcKtaConfig -Destination $destKtaConfig -Force

    # Patch ktaConfig.h
    # Note: App_Config.h is located at SG41_UART_X/App_Config.h
    # ktaConfig.h is at SG41_UART_X/keystream_connect/src/config/default/library/kta_lib/SOURCE/include/
    # So the relative path requires 8 levels of ../ to reach App_Config.h
    $ktaConfigPath = $destKtaConfig
    if (Test-Path $ktaConfigPath) {
        $lines = Get-Content $ktaConfigPath
        if ($lines.Count -ge 56) {
            $lines = $lines[0..54] + '#include "../../../../../../../../App_Config.h"' + $lines[55..($lines.Count-1)]
            $lines | Set-Content $ktaConfigPath
        } else {
            Add-Content $ktaConfigPath '#include "../../../../../../../../App_Config.h"'
        }

        # Replace C_KTA_APP__DEVICE_PUBLIC_UID with portal macro (handle both commented and uncommented)
        $lines = Get-Content $ktaConfigPath
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*(?://\s*)?#define\s+C_KTA_APP__DEVICE_PUBLIC_UID\b') {
                '#define C_KTA_APP__DEVICE_PUBLIC_UID                 KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID'
            } else {
                $_
            }
        }

        # Uncomment NETWORK_STACK_AVAILABLE
        $lines = $lines | ForEach-Object {
            if ($_ -match '^\s*//\s*#define\s+NETWORK_STACK_AVAILABLE\b') {
                ' #define NETWORK_STACK_AVAILABLE'
            } else {
                $_
            }
        }

        $lines | Set-Content $ktaConfigPath
    }

    # Patch k_sal_log.c — add #include "definitions.h" and USB-CDC salPrint implementation.
    # On SG41_UART_X, SERCOM6 is reserved for the KTA UART bridge so printf is a no-op.
    # KTA log output is routed to the USB-CDC SYS_CONSOLE instead.
    $kSalLogPath = Join-Path $destSource "salapi\k_sal_log.c"
    if (Test-Path $kSalLogPath) {
        $kSalLogLines = Get-Content $kSalLogPath

        # 1. Inject '#include "definitions.h"' after the last existing #include line
        if (-not ($kSalLogLines | Where-Object { $_ -match '#include\s+"definitions\.h"' })) {
            $lastIncludeIdx = -1
            for ($i = 0; $i -lt $kSalLogLines.Count; $i++) {
                if ($kSalLogLines[$i] -match '^\s*#include\s+') { $lastIncludeIdx = $i }
            }
            if ($lastIncludeIdx -ge 0) {
                $kSalLogLines = $kSalLogLines[0..$lastIncludeIdx] + '#include "definitions.h"' + $kSalLogLines[($lastIncludeIdx + 1)..($kSalLogLines.Count - 1)]
            }
        }

        # 2. Route salPrint to USB-CDC (SYS_CONSOLE).
        # The generic k_sal_log.c ships a printf-based salPrint which is a no-op on
        # this target (SERCOM6 is reserved for the KTA UART bridge). REPLACE the
        # existing salPrint body with the USB-CDC implementation (the previous
        # "append only if missing" logic left the printf version in place because a
        # salPrint already existed). Guarded by SYS_CONSOLE_Message so it is
        # idempotent: replace the printf version, or append if no salPrint exists.
        if (-not ($kSalLogLines | Where-Object { $_ -match 'SYS_CONSOLE_Message' })) {
            $salPrintImpl = @'
/**
 * @brief implement salPrint
 *
 */
void salPrint
(
  const char* xpBuffer
)
{
  if (NULL != xpBuffer)
  {
    /* printf/write() is a no-op stub on this target (SERCOM6 is reserved for
     * the KTA UART bridge). Route KTA logs to the USB-CDC SYS_CONSOLE instead,
     * the same channel used by the [UART-Bridge] status messages. */
    SYS_CONSOLE_Message(SYS_CONSOLE_HandleGet(SYS_CONSOLE_INDEX_0), xpBuffer);
  }
}
'@
            $kSalLogRaw = ($kSalLogLines -join "`r`n")
            # Match an existing salPrint definition (with optional preceding doc
            # comment). '\{[^{}]*\}' matches a single-level brace block, which is
            # correct for the shipped printf implementation (no nested braces).
            $salPrintPattern = '(?s)(?:/\*\*(?:(?!\*/)[\s\S])*?\*/\s*)?void\s+salPrint\s*\([^)]*\)\s*\{[^{}]*\}'
            if ([regex]::IsMatch($kSalLogRaw, $salPrintPattern)) {
                $kSalLogRaw = [regex]::Replace($kSalLogRaw, $salPrintPattern, $salPrintImpl)
            } else {
                $kSalLogRaw = $kSalLogRaw.TrimEnd() + "`r`n`r`n" + $salPrintImpl
            }
            $kSalLogLines = $kSalLogRaw -split "`r`n"
        }

        $kSalLogLines | Set-Content $kSalLogPath
        Write-Host "  Patched k_sal_log.c with definitions.h include and salPrint (USB-CDC)."
    } else {
        Write-Host "  WARNING: k_sal_log.c not found at: $kSalLogPath"
    }


    # Patch App_Config.h — remove '#include "ktaConfig.h"' to break circular include.
    # ktaConfig.h already includes App_Config.h; App_Config.h must NOT include it back
    # or the compiler fails when reaching App_Config.h through any other include chain.
    $appConfigPath = Join-Path $trustManagerPath "$selectedSubFolder\App_Config.h"
    if (Test-Path $appConfigPath) {
        $appConfigLines = Get-Content $appConfigPath
        $appConfigLines = $appConfigLines | Where-Object { $_ -notmatch '^\s*#include\s+"ktaConfig\.h"' }
        $appConfigLines | Set-Content $appConfigPath
    } else {
        Write-Host "  Warning: App_Config.h not found at $appConfigPath"
    }

    # ── cryptoauthlib copy (excluding hal/) ──────────────────────────────────
    # hal/ is already present in the project template and must NOT be touched.
    # All other cryptoauthlib files are refreshed from the ATEC608 reference app.
    $srcCryptoAuthLib = Join-Path $preConfigRoot "Harmony_Apps\ATEC608_late_provisioning_app\src\config\default\library\cryptoauthlib"
    $dstCryptoAuthLib = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\src\config\default\library\cryptoauthlib"

    if (Test-Path $srcCryptoAuthLib) {
        if (-not (Test-Path $dstCryptoAuthLib)) {
            New-Item -ItemType Directory -Path $dstCryptoAuthLib -Force | Out-Null
        }

        # Copy all .c and .h files (root + subfolders), EXCLUDING hal/
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

        # Copy each required subfolder individually (hal is intentionally omitted)
        $subfolders = @('atcacert', 'calib', 'crypto', 'host', 'jwt')
        foreach ($folder in $subfolders) {
            $srcSub = Join-Path $srcCryptoAuthLib $folder
            if (Test-Path $srcSub) {
                Copy-Item $srcSub -Destination $dstCryptoAuthLib -Recurse -Force
            } else {
                Write-Host "  cryptoauthlib subfolder not found: $folder"
            }
        }

        # atca_config.h: sercom2 → sercom3  (SG41 I2C pin mapping)
        $atcaConfigPath = Join-Path $dstCryptoAuthLib "atca_config.h"
        if (Test-Path $atcaConfigPath) {
            (Get-Content $atcaConfigPath) -replace 'extern atca_plib_i2c_api_t sercom2_plib_i2c_api;', 'extern atca_plib_i2c_api_t sercom3_plib_i2c_api;' | Set-Content $atcaConfigPath
        }
    } else {
        Write-Host "  WARNING: Source cryptoauthlib not found at: $srcCryptoAuthLib"
    }
    # ─────────────────────────────────────────────────────────────────────────

    # ── Restore COMMSTACK if it was accidentally removed ────────────────────
    if ($commStackBackupDir -ne $null) {
        if (-not (Test-Path $destCommStack)) {
            Copy-Item $commStackBackupDir -Destination $destCommStack -Recurse -Force
        }
        Remove-Item $commStackBackupDir -Recurse -Force -ErrorAction SilentlyContinue
    }
    # ─────────────────────────────────────────────────────────────────────────

    $xProjPath  = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\keystream_connect.X"
    $gatewayPath = Join-Path $trustManagerPath "$selectedSubFolder\gateway"
    Write-Host ""
    Write-Host "====================================================================="
    Write-Host "Next Step:"
    Write-Host ""
    Write-Host "  1. MCU (SG41) — Open the following .X project in MPLAB X IDE:"
    Write-Host ""
    Write-Host "     $xProjPath"
    Write-Host ""
    Write-Host "  2. Gateway (Windows) — Build and run the gateway from:"
    Write-Host ""
    Write-Host "     $gatewayPath"
    Write-Host ""
    Write-Host "====================================================================="
}

# =============================================================================
# Exclude ktaConfig.h.ftl from the generated Pre-Integrated Example package.
#
# 'ktaConfig.h.ftl' is a Harmony/MPLAB code-generation template that lives in the
# kta_lib SOURCE (SOURCE\include\) and is required there for Harmony package
# generation. It is carried into the example only as a side effect of copying the
# entire kta_provisioning\SOURCE folder. Pre-Integrated Examples ship the
# pre-generated 'ktaConfig.h' (not the template), so the '.ftl' must not appear in
# the packaged example. Remove ONLY this file from the configured example output;
# the original source template and the kta_lib SOURCE copy remain untouched.
# =============================================================================
if ($trustManagerPath -and $selectedSubFolder) {
    $exampleRoot = Join-Path $trustManagerPath $selectedSubFolder
    if (Test-Path $exampleRoot) {
        Get-ChildItem -Path $exampleRoot -Recurse -Filter 'ktaConfig.h.ftl' -ErrorAction SilentlyContinue | ForEach-Object {
            Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
            Write-Host "Excluded ktaConfig.h.ftl from packaged example: $($_.FullName)"
        }
    }
}

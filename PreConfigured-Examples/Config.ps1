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
    # Path to tmg_conf.h
    $tmgConfPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\tmg_conf.h"

    if (Test-Path $tmgConfPath) 
    {
        $ssid = Read-Host "Enter new WIFI SSID"
        $pwd = Read-Host "Enter new WIFI password"
        $uid = Read-Host "Enter new KEYSTREAM Device Public Profile UID"

        $content = Get-Content $tmgConfPath

        $content = $content -replace '(#define\s+WIFI_SSID\s*)".*"', "`$1`"$ssid`""
        $content = $content -replace '(#define\s+WIFI_PWD\s*)".*"', "`$1`"$pwd`""
        $content = $content -replace '(#define\s+KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID\s*)".*"', "`$1`"$uid`""

        Set-Content $tmgConfPath $content

        Write-Host "updated successfully."
    } 
    else 
    {
        Write-Host "update failure $tmgConfPath"
    }

    $preConfigRoot = Split-Path $currentPath -Parent
    $ktaLibPath = Join-Path $preConfigRoot "kta_lib\kta_provisioning"
    if (-not (Test-Path $ktaLibPath))
    {
        Write-Host "The 'kta_lib\kta_provisioning' folder was not found in the current directory structure."
        exit
    }

    # 1. Copy SOURCE folder to ...\firmware\common\kta_provisioning
    $destKtaProv = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\common\kta_provisioning"
    $sourcePath = Join-Path $ktaLibPath "SOURCE"
    Copy-Item $sourcePath -Destination $destKtaProv -Recurse -Force

    # 2. Copy other files to ...\firmware\common\
    $destCommon = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\common"
    if (-not (Test-Path $destCommon))
    {
        New-Item -ItemType Directory -Path $destCommon -Force | Out-Null
    }

    Copy-Item "$ktaLibPath\fota_service"         -Destination $destCommon -Recurse -Force
    Copy-Item "$ktaLibPath\cust_def_device.c"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\cust_def_device.h"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\cust_def_signer.c"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\cust_def_signer.h"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\ktaFieldMgntHook.c"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\ktaFieldMgntHook.h"    -Destination $destCommon -Force

    # Remove only the 'break;' at line 483 in ktaFieldMgntHook.c (1-based line number)
    $ktaFieldMgntHookPath = Join-Path $destCommon "ktaFieldMgntHook.c"
    if (Test-Path $ktaFieldMgntHookPath) {
        $lines = Get-Content $ktaFieldMgntHookPath
        # Remove line 483 (index 482 in 0-based array) if it starts with 'break;'
        if ($lines.Count -ge 483 -and $lines[482].Trim().StartsWith('break;')) {
            $lines = $lines[0..481] + $lines[483..($lines.Count-1)]
            Set-Content $ktaFieldMgntHookPath $lines
        } else {
            Write-Host "'break;' not found at line 483 in ktaFieldMgntHook.c."
        }
    } else {
        Write-Host "ktaFieldMgntHook.c not found at $ktaFieldMgntHookPath"
    }

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

    # Only take UID as input
    $uid = Read-Host "Enter new KEYSTREAM Device Public Profile UID"

    $ktaLibPath = Join-Path (Split-Path $currentPath) "kta_lib\kta_provisioning" 
    $tmgConfPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\tmg_conf.h"
    $content = Get-Content $tmgConfPath
    $content = $content -replace '(#define\s+KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID\s*)".*"', "`$1`"$uid`""
    Set-Content $tmgConfPath $content

    $preConfigRoot = Split-Path $currentPath -Parent
    $ktaLibPath = Join-Path $preConfigRoot "kta_lib\kta_provisioning"

    # 1. Copy SOURCE folder to ...\firmware\common\kta_provisioning
    $destKtaProv = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\common\kta_provisioning"
    $sourcePath = Join-Path $ktaLibPath "SOURCE"
    Copy-Item $sourcePath -Destination $destKtaProv -Recurse -Force

    # 2. Copy other files to ...\firmware\common\
    $destCommon = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\firmware\common"
    if (-not (Test-Path $destCommon))
    {
        New-Item -ItemType Directory -Path $destCommon -Force | Out-Null
    }

    Copy-Item "$ktaLibPath\fota_service"         -Destination $destCommon -Recurse -Force
    Copy-Item "$ktaLibPath\cust_def_device.c"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\cust_def_device.h"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\cust_def_signer.c"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\cust_def_signer.h"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\ktaFieldMgntHook.c"    -Destination $destCommon -Force
    Copy-Item "$ktaLibPath\ktaFieldMgntHook.h"    -Destination $destCommon -Force

    # Remove only the 'break;' at line 483 in ktaFieldMgntHook.c (1-based line number)
    $ktaFieldMgntHookPath = Join-Path $destCommon "ktaFieldMgntHook.c"
    if (Test-Path $ktaFieldMgntHookPath) {
        $lines = Get-Content $ktaFieldMgntHookPath
        # Remove line 483 (index 482 in 0-based array) if it starts with 'break;'
        if ($lines.Count -ge 483 -and $lines[482].Trim().StartsWith('break;')) {
            $lines = $lines[0..481] + $lines[483..($lines.Count-1)]
            Set-Content $ktaFieldMgntHookPath $lines
        } else {
            Write-Host "'break;' not found at line 483 in ktaFieldMgntHook.c."
        }
    } else {
        Write-Host "ktaFieldMgntHook.c not found at $ktaFieldMgntHookPath"
    }

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
    # Path to configuration.h
    $configPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\src\config\default\configuration.h"
    if (Test-Path $configPath) {
        $ssid = Read-Host "Enter new WIFI SSID"
        $pwd = Read-Host "Enter new WIFI password"

        # Update only SSID and PASSWORD in configuration.h
        $configContent = Get-Content $configPath
        $configContent = $configContent -replace '(#define SYS_WINCS_WIFI_STA_SSID\s+)"[^"]*"', "`$1`"$ssid`""
        $configContent = $configContent -replace '(#define SYS_WINCS_WIFI_STA_PWD\s+)"[^"]*"', "`$1`"$pwd`""
        Set-Content $configPath $configContent
    } else {
        Write-Host "configuration.h not found at $configPath"
    }

    # Path to ktaConfig.h
    $ktaConfigPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\src\config\default\library\kta_lib\ktaConfig.h"
    if (Test-Path $ktaConfigPath) {
        $uid = Read-Host "Enter new KEYSTREAM Device Public Profile UID"

        $ktaConfigContent = Get-Content $ktaConfigPath
        $ktaConfigContent = $ktaConfigContent -replace '(\#define C_KTA_APP__DEVICE_PUBLIC_UID\s*)\([^\)]*\)', "`$1(`"$uid`")"
        Set-Content $ktaConfigPath $ktaConfigContent
        Write-Host "ktaConfig.h updated successfully."
    } else {
        Write-Host "ktaConfig.h not found at $ktaConfigPath"
    }


    $preConfigRoot = Split-Path $currentPath -Parent
    $ktaLibPath = Join-Path $preConfigRoot "kta_lib\kta_provisioning"
    if (-not (Test-Path $ktaLibPath)) {
        Write-Host "The 'kta_lib\kta_provisioning' folder was not found in the current directory structure."
        exit
    }

    $destKtaLib = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\src\config\default\library\kta_lib"
    if (-not (Test-Path $destKtaLib)) {
        New-Item -ItemType Directory -Path $destKtaLib -Force | Out-Null
    }

    Copy-Item (Join-Path $ktaLibPath "SOURCE")                -Destination $destKtaLib -Recurse -Force
    Copy-Item (Join-Path $ktaLibPath "fota_service")           -Destination $destKtaLib -Recurse -Force
    Copy-Item (Join-Path $ktaLibPath "cust_def_device.c")      -Destination $destKtaLib -Force
    Copy-Item (Join-Path $ktaLibPath "cust_def_device.h")      -Destination $destKtaLib -Force
    Copy-Item (Join-Path $ktaLibPath "cust_def_signer.c")      -Destination $destKtaLib -Force
    Copy-Item (Join-Path $ktaLibPath "cust_def_signer.h")      -Destination $destKtaLib -Force
    Copy-Item (Join-Path $ktaLibPath "ktaFieldMgntHook.c")     -Destination $destKtaLib -Force
    Copy-Item (Join-Path $ktaLibPath "ktaFieldMgntHook.h")     -Destination $destKtaLib -Force

    # Remove only the 'break;' at line 483 in ktaFieldMgntHook.c (1-based line number)
    $ktaFieldMgntHookPath = Join-Path $destKtaLib "ktaFieldMgntHook.c"
    if (Test-Path $ktaFieldMgntHookPath) {
        $lines = Get-Content $ktaFieldMgntHookPath
        # Remove line 483 (index 482 in 0-based array) if it starts with 'break;'
        if ($lines.Count -ge 483 -and $lines[482].Trim().StartsWith('break;')) {
            $lines = $lines[0..481] + $lines[483..($lines.Count-1)]
            Set-Content $ktaFieldMgntHookPath $lines
        } else {
            Write-Host "'break;' not found at line 483 in ktaFieldMgntHook.c."
        }
    } else {
        Write-Host "ktaFieldMgntHook.c not found at $ktaFieldMgntHookPath"
    }

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


    $xProjPath = Join-Path $trustManagerPath "$selectedSubFolder\keystream_connect\keystream_connect.X"
    Write-Host ""
    Write-Host "====================================================================="
    Write-Host "Next Step:"
    Write-Host "Please open the following .X project file in MPLAB X IDE to continue:"
    Write-Host ""
    Write-Host "    $xProjPath"
    Write-Host ""
    Write-Host "====================================================================="
}
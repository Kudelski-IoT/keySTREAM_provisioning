---
title: "keySTREAM Trusted Agent Release Notes"
date:  "2025.10.15"
author: "Kudelski IoT"
---
# Release v1.4.1 (15/10/2025)

## New features/Changes in this version

## Fixed issues
- Fixed JIRA issues:
  - LPM-33 : KeySTREAM FoTA Usecase - FoTA name mismatch fix
  - LPM-32 : SG41 TPDS Eng release Feedback
  - LPM-28 : KTA delivery/doc improvements
  - LPM-17 : KTA - Coverity report
  - LPM-4  : Slot8 issue after FOTA : KTA version was reading as 0 instead of correct version.

## Features list
| Feature                                                              | ATECC608 |
| ---                                                                  | ---      |
| Onboarding with certificate validation with basic key(E256)          |    1     |
| Onboarding with certificate validation with advance key(E384,E521..) |    0     |
| Onboarding without certificate validation                            |    0     |
| Mutable Fleet Profile                                                |    1     |
| Certificate renewal                                                  |    1     |
| key renewal                                                          |    1     |
| FOTA service                                                         |    1     |

## Compatibility
- This version is compatible with
- CAL Version       - 3.7.8
- keySTREAM Version - 1.30

## Open source information
- NA

## Identifier
- Git Tag: v1.4.1

---

# History

## Release v1.3.0 (22/05/2025)

### New features/Changes in this version
- Symmetric key service
- Signing public key service
- FOTA service

### Compatibility
- This version is compatible with
- CAL Version       - 3.7.8
- keySTREAM Version - 1.28

### Open source information
- NA

### Identifier
- Git Tag: v1.3.0

## Release v1.2.8 (09/03/2024)

## New features/Changes in this version

#### Static Check Fixed using SonarQube Tool
- KTA/SAL, Hook Files and communication files present in Harmony pkg
  were fixed for static check with listed deviation.

## Compatibility
- This version is compatible with
- CAL Version       - 3.7.5
- keySTREAM Version - 1.25.3

## Migrating from Old KTA (v1.2.6) to KTA v1.2.8.

### Scenario 1 :
- Onboarding same device using different Fleet Profile in the same tenant.
#### Preconditions
- Device is already on-boarded to Fleet Profile A, with old KTA (v1.2.6).
#### Steps to migrate
- Create new fleet Profile B in **same tenant**
- Execute TPDS steps for updating the project with new fleet profile and KTA version.
- Once all steps are executed, load project in MPLAB X IDE and compile.
- Once compilation is completed, Flash new KTA hex (Built using latest KTA v1.2.8) on same device with Fleet Profile B created on same tenant.
- From the keystream UI, where your device is already listed. click on **Remove device** option.
- This will refurbish the device .Once reburbish is completed. Manually Claim device in tenant with manifest lite file generated via TPDS Usecase.
#### Expected Results
- Device should be on-boarded to Fleet Profile B
- Device should be provisioned and End to End should work.

### Scenario 2 :
- Onboarding same device using different Fleet Profile created in different tenant.
#### Preconditions
- Device is already on-boarded to Fleet Profile A, with old KTA (v1.2.6) in one tenant.
#### Steps to migrate
- Create new fleet Profile B in **different tenant**
- Execute TPDS steps for updating the project with new fleet profile and KTA version.
- Once all steps are executed, load project in MPLAB X IDE and compile.
- Once compilation is completed, Flash new KTA hex (Built using latest KTA v1.2.8) on same device with Fleet Profile B created on different tenant.
- From the keystream UI, where your device is already listed. click on **Remove device** option.
- This will refurbish the device .Once reburbish is completed. Manually Claim device with manifest lite file generated via TPDS Usecase in tenant where Fleet Profile B is created. 
#### Expected Results
- Device should be on-boarded to Fleet Profile B
- Device should be provisioned and End to End should work.

## Open source information
- NA

## Identifier
- Git Tag: v1.2.8

# Release v1.2.7 (07/12/2024) (Internal released to KUD only)

## New features/Changes in this version

#### UTC to Generalized time format migration
- KTA code was updated to support Generalized Time format.

#### KTA Version Storage
- KTA Version Storage in Slot 8 implemented for handling transition from UTC to Generalized time format.

#### Progressive keySTREAM Server polling
- Polling time to keySTREAM Server changes done with 5 seconds for first 10 minutes and after that hourly once.

#### CAL Update 3.7.5
- KTA code has been integrated with cryptoauthlib (3.7.5).

#### 2PKI Implementation
- KTA Code updated to support 2PKI/1PKI feature.

## Fixed issues / Change Requests
- Fixed HTTP Receive Timeout issue.

## Compatibility
- This version is compatible with
- CAL Version       - 3.7.5
- keySTREAM Version - NA

## Open source information
- NA

## Identifier
- Git Tag: NA

---

# History

# Release v1.2.6 (03/21/2024)

## New features/Changes in this version

#### Device Onboarding
- Device Onboarding is the process by which a device is registered in keySTREAM server with a certain device identity and attached to a certain device vendor. Device onboarding is performed automatically by keySTREAM as part of the in-field device identity provisioning process.

#### Device Certificate Renewal
- A new certificate will be generated and provisioned to the device before the old one expires. Renewing a certificate does not mean changing the associated key pair. **The key pair stays the same.**

#### Key Renewal
- This Option Enables the User to Renew the key pair from keySTREAM UI.

#### Device Refurbish
- A device can be brought back to factory provisioned state at any time using the device refurbishment functionality.

#### Optional Multi Device Profile Public Uid
- Added Mutable Device Profile Public UID Feature in KTA.

#### Slot 8 Config Update
- Storage of Signer Issuer Common Name, Signer Subject Common Name, Leaf Issuer Common Name, Signer Issuer Organisation Name, Signer Issuer Oraganisation Unit Name, Signer Subject Organisation Name, Signer Expiry Date, Leaf Expiry Date and Leaf Issuer Organisation Name has been updated.

#### CAL Update 3.7.4
- KTA code has been integrated with cryptoauthlib (3.7.4).

## Fixed issues / Change Requests
- Changes Done to Support Integration of KTA with Latest Cryptoauthlib (3.7.4).
- Fixed Compilation Warnings.
- Updated Doxygen Header's for KTA Source Files.
- Device Reboot issue fixed.
- Key Renewal issue fixed.
- Expiry Date issue fixed.

## Compatibility
- This version is compatible with
- CAL Version       - 3.7.4
- keySTREAM Version - NA

## Open source information
- NA

## Identifier
- Git Tag: v1.2.6
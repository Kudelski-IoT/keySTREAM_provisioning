---
title: "keySTREAM Trusted Agent Release Notes"
date:  "2024.07.12"
author: "Kudelski IoT"
---

# Release v1.2.7 (07/12/2024)

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
- keySTREAM Version - TBD

## Open source information
- TBD

## Identifier
- Git Tag: TBD

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
- CAL Version       - 3.7.5
- keySTREAM Version - TBD

## Open source information
- TBD

## Identifier
- Git Tag: TBD
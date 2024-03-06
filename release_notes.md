## keySTREAM Trusted Agent Release Notes

## Release v1.2.6

### New Features
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

### Fixes
- Changes Done to Support Integration of KTA with Latest Cryptoauthlib.
- Fixed Compilation Warnings. 
- Updated Doxygen Header's for KTA Source Files.


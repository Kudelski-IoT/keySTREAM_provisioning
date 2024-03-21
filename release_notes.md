## keySTREAM Trusted Agent Release Notes

## Release v1.2.6 (03/21/2024)

### Updates

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

### Fixes
- Changes Done to Support Integration of KTA with Latest Cryptoauthlib (3.7.4).
- Fixed Compilation Warnings. 
- Updated Doxygen Header's for KTA Source Files.
- Device Reboot issue fixed.
- Key Renewal issue fixed.
- Expiry Date issue fixed.
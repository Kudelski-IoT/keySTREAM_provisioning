# keySTREAM Trusted Agent Library (KTA_LIB) Configurations and example application.


# Documentation
Documentation for this library can be found in following path within this repository: docs/index.html

# Building and using the Library
## Prerequisites
### Enable below preprocessor MACRO for as per the requirement.
  - **File Path:**  
  - Header Files\config\default\library\kta_lib\ktaConfig.h
  - NETWORK_STACK_AVAILABLE - Enable this MACRO once network stack is implemented by customer.
  - FOTA_ENABLE - Enable this MACRO once FOTA agent is fully implemented by customer.

### Mandatory API Implementation for FOTA Services

To enable FOTA Services, user needs to implement the following APIs in the specified source file:

- **File Path:**  
  Source Files\config\default\kta_lib\fota_service\Fota_Platform.c

- **Required APIs:**  
  - fotaPlatformGetComponents()
  - fotaStartInstallation()

# Preconfigured Examples

- Preconfigured example applications are provided in the **PreConfigured-Examples** directory of our Git repository. These examples include an integrated communication stack and additional supported features.
- For more information on the preconfigured examples, please refer to the Getting Started Guide available in the **PreConfigured-Examples** directory, or the offline documentation as described later in this README.

### Install the following tools.
   - [Microchip MPLAB X IDE](https://www.microchip.com/mplab/mplab-x-ide)
   - [Microchip MPLAB® Harmony](https://www.microchip.com/mplab/mplab-harmony)

### Enable long path in Windows Machine.
   - In some system, users might see compilation issues due to disabled long path setting.Please follow the below steps to enable long path.
       - Open Command Prompt (CMD) in Administrator Mode.
       - Enter below command to enable long path.

         reg add "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\FileSystem" /v LongPathsEnabled /t REG_DWORD /d 1 /f
       - To verify Whether long path is enabled you can use the below command.

         reg query "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\FileSystem" /v LongPathsEnabled

          - You should be able to see below when long path is enabled
             - LongPathsEnabled    REG_DWORD    0x1


### Please refer the following offline document page for steps to integrate KTA_LIB.
 - Open index.html present in following path of this repository: docs/index.html
 - Click on keySTREAM Trusted Agent Documentation.
 - From left Panel click on MPLAB® Harmony KTA_LIB Configurations and Application and follow the instructions.

# Licensing
License terms for using keySTREAM Trusted Agent library (KTA_LIB) are defined in the [license](./license.md) file of this repo. Please refer to this file for all definitive licensing information.

# Release Notes
Please refer the following page for release notes. [Release Notes](./release_notes.md)

# Contents Summary

| Folder     | Description                                                              |
| ---        | ---                                                                      |
| apps       | Example application to demonstrate usage of KTA_LIB with Harmony           |
| config     | KTA_LIB module configuration files                                          |
| docs       | KTA_LIB help documentation                                                  |
| PreConfigured-Examples       | Pre integrated application to demonstrate usage of KTA_LIB, More details available in [README](./PreConfigured-Examples/Readme.md)
|


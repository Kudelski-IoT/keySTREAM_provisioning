/*******************************************************************************

  File Name:
    metadata.h

  Summary:
    System-wide prototypes for metadata handling.

  Description:
    This file contains the definitions and structures used for managing metadata
    associated with firmware images, such as bootloaders and applications.
    It includes magic numbers, image types, and a structure to hold metadata
    information including versioning, addresses, sizes, and signatures.

 *******************************************************************************/

/*******************************************************************************
* Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/

#ifndef IMAGE_METADATA_H
#define IMAGE_METADATA_H

#include <stdint.h>

#define METADATA_MAGIC        0xAABBCCDD    // Magic number for metadata validation
#define TYPE_BOOTLOADER       0x01          // Image type for bootloader
#define TYPE_APPLICATION      0x02          // Image type for application

// Metadata structure for firmware images
// This structure is used to store metadata about firmware images, such as bootloaders and applications.
typedef struct {
    uint32_t magic;                  		// Magic number for validation
    uint8_t  image_type;             		// 0x01 = bootloader, 0x02 = application

    uint8_t  version_major;			        // Major version
    uint8_t  version_minor;			        // Minor version
    uint8_t  version_patch;			        // Patch version
    uint32_t build_number;        		    // Build number

    uint32_t image_address;          		// Start address of image
    uint32_t image_size;             		// Length in bytes

    uint32_t signature_address;      		// Address of digital signature
    uint32_t signature_size;         		// Signature length in bytes

    uint32_t application_metadata_start;  	// Used by bootloader metadata only, should be 0 for app metadaa

    uint32_t reserved[3];            		// Padding or future fields

    uint32_t metadata_hash;         		// Hash of *this structure* excluding this field
} firmware_metadata_t;

extern const firmware_metadata_t app_meta_data;

#endif /* DEFINITIONS_H */

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

#include "metadata.h"
#include "app_wincs02.h"
#include "peripheral/nvmctrl/plib_nvmctrl.h"
#include <string.h>

__attribute__ ((section(".AppMetaData")))
const firmware_metadata_t app_meta_data =
{
    METADATA_MAGIC,
    TYPE_APPLICATION,
    APP_COMPONENT_NAME,
    APP_COMPONENT_VERSION,
    0x00010000,
    0x0005FC00,
    0x0006FC40,
    0x00000040,
    0x00000000,
    0x00000000
};

uint32_t get_passivebank_metadata_address(void)
{
    // Offset by 512KB
    return ((uint32_t)&app_meta_data) + 0x80000;
}

uint32_t get_passivebank_component_start_address(void)
{
    firmware_metadata_t* app_meta = (firmware_metadata_t*)&app_meta_data;

    // Offset by 512KB to write to the passive bank
    return app_meta->image_address + 0x80000;
}

uint32_t get_passivebank_component_end_address(void)
{
    firmware_metadata_t* app_meta = (firmware_metadata_t*)&app_meta_data;

    // In the current design, 0x400 bytes are reserved for metadata at the end
    // of the image
    return app_meta->image_address + app_meta->image_size + (0x400 - 1) +
            0x80000; // Offset by 512KB to write to the passive bank
}

uint32_t get_passivebank_nvm_blocks_count_to_erase(void)
{
    return ((get_passivebank_component_end_address() - get_passivebank_component_start_address() + 1) /
            NVMCTRL_FLASH_BLOCKSIZE);
}

void get_component_information(uint8_t id, uint8_t* name, uint8_t* version)
{
    firmware_metadata_t* app_meta = (firmware_metadata_t*)&app_meta_data;
    const char *comp_name = (const char*)app_meta->image_name;
    const char *comp_version = (const char*)app_meta->image_version;

    // Assuming 0 is the ID for the application component... With additional
    // components, this can be expanded
    if (id == 0)
    {
        strncpy((char *)name, comp_name, strlen(comp_name));
        strncpy((char *)version, comp_version, strlen(comp_version));
    }
    else
    {
        // Handle other component types if necessary
        name[0] = '\0';
        version[0] = '\0';
    }
}

/**
 * @brief Get all components information at once
 * 
 * This function fills an array of component structures with name and version
 * information for all available components in the system.
 * 
 * @param[out] components Pointer to array of TComponent structures to fill
 * @return Number of components filled (0-COMPONENTS_MAX)
 */
uint8_t get_all_components_information(TComponent* components)
{
    if (components == NULL)
    {
        return 0;
    }
    
    // Component 0: Application
    firmware_metadata_t* app_meta = (firmware_metadata_t*)&app_meta_data;
    
    if (app_meta->magic == METADATA_MAGIC)
    {
        size_t nameLen = strlen((const char*)app_meta->image_name);
        size_t versionLen = strlen((const char*)app_meta->image_version);
        
        if (nameLen > 0 && nameLen < 16)
        {
            memcpy(components[0].componentName, app_meta->image_name, nameLen);
            components[0].componentNameLen = nameLen;
            
            memcpy(components[0].componentVersion, app_meta->image_version, versionLen);
            components[0].componentVersionLen = versionLen;
            
            // TODO: Other component details can be added in future
            
            return 1;
        }
    }

    return 0;
}

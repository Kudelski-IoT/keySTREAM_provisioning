
#include "metadata.h"
#include "app.h"

__attribute__ ((section(".BootMetaData")))
const firmware_metadata_t bootloader_metadata =
{
    METADATA_MAGIC,
    TYPE_BOOTLOADER,
    BOOT_COMPONENT_NAME,
    BOOT_COMPONENT_VERSION,
    0x00000000,
    0x0000FC00,
    0x0000FC40,
    0x00000040,
    0x0006FC00,
    0x00000000
};
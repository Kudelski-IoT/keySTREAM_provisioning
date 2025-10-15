
#include "metadata.h"
#include "app_wincs02.h"

__attribute__ ((section(".AppMetaData")))
const firmware_metadata_t app_meta_data =
{
    METADATA_MAGIC,
    TYPE_APPLICATION,
    (uint8_t)APP_VERSION_MAJOR, (uint8_t)APP_VERSION_MINOR, (uint8_t)APP_VERSION_PATCH,
    0x00000000,
    0x00010000,
    0x0006FC00,
    0x0007FC30,
    0x00000040,
    0x00000000,
    {0x00000000, 0x00000000, 0x00000000},
    0x00000000
};
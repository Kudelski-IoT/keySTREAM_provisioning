/**
 * @file backend_message.c
 * @brief Backend Message Protocol Implementation - Protocol Layer
 * 
 * Platform-independent implementation of TLV message protocol.
 * Works on any MCU (STM32, ESP32, Nordic, Microchip, etc.) and any platform.
 * 
 * This is GENERIC code with no platform-specific dependencies.
 */

#include "backend_message.h"
#include <string.h>

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool g_backend_message_initialized = false;

/* ============================================================================
 * CRC32 Implementation (Generic)
 * ============================================================================ */

/* CRC32 lookup table */
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t backend_message_calculate_crc32(const uint8_t *xpData, size_t xLength)
{
    uint32_t crc = 0xFFFFFFFFU;
    
    for (size_t i = 0U; i < xLength; i++) {
        uint8_t index = (uint8_t)((crc ^ xpData[i]) & 0xFFU);
        crc = (crc >> 8) ^ crc32_table[index];
    }
    
    return ~crc;
}

bool backend_message_verify_crc32(const uint8_t *xpData, size_t xLength, uint32_t xExpectedCrc)
{
    uint32_t calculated_crc = backend_message_calculate_crc32(xpData, xLength);
    return (calculated_crc == xExpectedCrc);
}

/* ============================================================================
 * Protocol Initialization
 * ============================================================================ */

BackendMessageStatus backend_message_init(void)
{
    g_backend_message_initialized = true;
    return BACKEND_MESSAGE_SUCCESS;
}

/* ============================================================================
 * Message Construction (Generic)
 * ============================================================================ */

BackendMessageStatus backend_message_create(BackendMessage *xpMsg, uint8_t xMsgType)
{
    if (NULL == xpMsg) {
        return BACKEND_MESSAGE_ERROR_INVALID_PARAM;
    }
    
    (void)memset(xpMsg, 0, sizeof(BackendMessage));
    xpMsg->message_type = xMsgType;
    xpMsg->command_tag = 0U;
    xpMsg->field_count = 0U;
    
    return BACKEND_MESSAGE_SUCCESS;
}

BackendMessageStatus backend_message_set_command(BackendMessage *xpMsg, uint8_t xCmdTag)
{
    if (NULL == xpMsg) {
        return BACKEND_MESSAGE_ERROR_INVALID_PARAM;
    }
    
    xpMsg->command_tag = xCmdTag;
    xpMsg->field_count = 0U;  /* Reset fields when setting new command */
    
    return BACKEND_MESSAGE_SUCCESS;
}

BackendMessageStatus backend_message_add_field(BackendMessage *xpMsg,
                                            uint16_t xFieldTag,
                                            const uint8_t *xpValue,
                                            uint16_t xLength)
{
    if ((NULL == xpMsg) || (NULL == xpValue)) {
        return BACKEND_MESSAGE_ERROR_INVALID_PARAM;
    }
    
    if (xpMsg->field_count >= BACKEND_MESSAGE_MAX_FIELDS) {
        return BACKEND_MESSAGE_ERROR_BUFFER_FULL;
    }
    
    BackendMessageField *field = &xpMsg->fields[xpMsg->field_count];
    field->tag = xFieldTag;
    field->length = xLength;
    field->value = (uint8_t*)xpValue;  /* Store pointer */
    xpMsg->field_count++;
    
    return BACKEND_MESSAGE_SUCCESS;
}

/* ============================================================================
 * Serialization (Generic TLV Format)
 * ============================================================================ */

BackendMessageStatus backend_message_serialize(const BackendMessage *xpMsg,
                                            uint8_t *xpOutput,
                                            size_t xOutputSize,
                                            size_t *xpOutputLength)
{
    if ((NULL == xpMsg) || (NULL == xpOutput) || (NULL == xpOutputLength)) {
        return BACKEND_MESSAGE_ERROR_INVALID_PARAM;
    }
    
    uint8_t *ptr = xpOutput;
    size_t remaining = xOutputSize;
    
    /* Message header: [MSG_TYPE:1][CMD_TAG:1][FIELD_COUNT:1][RESERVED:1] */
    if (remaining < 4U) return BACKEND_MESSAGE_ERROR_BUFFER_FULL;
    
    *ptr++ = xpMsg->message_type;
    *ptr++ = xpMsg->command_tag;
    *ptr++ = xpMsg->field_count;
    *ptr++ = 0U; /* Reserved */
    remaining -= 4U;
    
    /* Serialize each field: [TAG:2][LENGTH:2][VALUE:LENGTH] */
    for (uint8_t i = 0U; i < xpMsg->field_count; i++) {
        const BackendMessageField *field = &xpMsg->fields[i];
        
        if (remaining < (4U + field->length)) {
            return BACKEND_MESSAGE_ERROR_BUFFER_FULL;
        }
        
        /* Field tag (2 bytes, big-endian) */
        *ptr++ = (uint8_t)((field->tag >> 8) & 0xFFU);
        *ptr++ = (uint8_t)(field->tag & 0xFFU);
        
        /* Field length (2 bytes, big-endian) */
        *ptr++ = (uint8_t)((field->length >> 8) & 0xFFU);
        *ptr++ = (uint8_t)(field->length & 0xFFU);
        
        /* Field value */
        if ((field->length > 0U) && (NULL != field->value)) {
            (void)memcpy(ptr, field->value, field->length);
            ptr += field->length;
        }
        
        remaining -= (4U + field->length);
    }
    
    *xpOutputLength = (size_t)(ptr - xpOutput);
    return BACKEND_MESSAGE_SUCCESS;
}

/* ============================================================================
 * Deserialization (Generic TLV Format)
 * ============================================================================ */

BackendMessageStatus backend_message_deserialize(const uint8_t *xpData,
                                              size_t xLength,
                                              BackendMessage *xpMsg)
{
    if ((NULL == xpData) || (NULL == xpMsg) || (xLength < 4U)) {
        return BACKEND_MESSAGE_ERROR_INVALID_PARAM;
    }
    
    const uint8_t *ptr = xpData;
    size_t remaining = xLength;
    
    /* Clear message */
    (void)memset(xpMsg, 0, sizeof(BackendMessage));
    
    /* Parse message header: [MSG_TYPE:1][CMD_TAG:1][FIELD_COUNT:1][RESERVED:1] */
    xpMsg->message_type = *ptr++;
    xpMsg->command_tag = *ptr++;
    xpMsg->field_count = *ptr++;
    ptr++; /* Reserved */
    remaining -= 4U;
    
    if (xpMsg->field_count > BACKEND_MESSAGE_MAX_FIELDS) {
        return BACKEND_MESSAGE_ERROR_INVALID_MESSAGE;
    }
    
    /* Parse each field */
    for (uint8_t i = 0U; i < xpMsg->field_count; i++) {
        if (remaining < 4U) return BACKEND_MESSAGE_ERROR_DESERIALIZATION;
        
        BackendMessageField *field = &xpMsg->fields[i];
        
        /* Field tag (2 bytes, big-endian) */
        field->tag = (uint16_t)(((uint16_t)ptr[0] << 8) | ptr[1]);
        ptr += 2U;
        
        /* Field length (2 bytes, big-endian) */
        field->length = (uint16_t)(((uint16_t)ptr[0] << 8) | ptr[1]);
        ptr += 2U;
        remaining -= 4U;
        
        /* Field value */
        if (field->length > 0U) {
            if (remaining < field->length) {
                return BACKEND_MESSAGE_ERROR_DESERIALIZATION;
            }
            
            field->value = (uint8_t*)ptr;  /* Point to data in buffer */
            ptr += field->length;
            remaining -= field->length;
        } else {
            field->value = NULL;
        }
    }
    
    return BACKEND_MESSAGE_SUCCESS;
}

/* ============================================================================
 * Utility Functions (Generic)
 * ============================================================================ */

/**
 * @brief Get field value from message by tag
 */
const uint8_t* backend_message_get_field(const BackendMessage *xpMsg, uint16_t xTag, size_t *xpLength)
{
    if ((NULL == xpMsg) || (NULL == xpLength)) {
        return NULL;
    }
    
    for (uint8_t i = 0U; i < xpMsg->field_count; i++) {
        if (xpMsg->fields[i].tag == xTag) {
            *xpLength = xpMsg->fields[i].length;
            return xpMsg->fields[i].value;
        }
    }
    
    *xpLength = 0U;
    return NULL;
}

/**
 * @brief Check if message is valid
 */
bool backend_message_is_valid(const BackendMessage *xpMsg)
{
    if (NULL == xpMsg) return false;
    
    if (xpMsg->field_count > BACKEND_MESSAGE_MAX_FIELDS) {
        return false;
    }
    
    return true;
}

/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2026 Nagravision Sàrl

* Subject to your compliance with these terms, you may use the Nagravision Sàrl
* Software and any derivatives exclusively with Nagravision's products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may accompany
* Nagravision Software.

* Redistribution of this Nagravision Software in source or binary form is allowed
* and must include the above terms of use and the following disclaimer with the
* distribution and accompanying materials.

* THIS SOFTWARE IS SUPPLIED BY NAGRAVISION "AS IS". NO WARRANTIES, WHETHER EXPRESS,
* IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF
* NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE. IN NO
* EVENT WILL NAGRAVISION BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL
* OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO
* THE SOFTWARE, HOWEVER CAUSED, EVEN IF NAGRAVISION HAS BEEN ADVISED OF THE
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW,
* NAGRAVISION 'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS
* SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY
* TO NAGRAVISION FOR THIS SOFTWARE.
********************************************************************************/
/**
 * @file kta_bridge.h
 * @brief Generic KTA integration bridge (Transport -> KTA)
 *
 * This module integrates transport layer requests with KTA APIs on any MCU.
 * Works with UART, USB, BLE, Zigbee, or any other transport backend.
 *
 * Expected include paths (set in build system):
 * - KTA library include directory
 * - Transport layer include directory (../../common)
 */

#ifndef KTA_BRIDGE_H
#define KTA_BRIDGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

/* ============================================================================
 * Transport Interface Types (inline definitions - no separate header needed)
 * ============================================================================ */

/* Transport status codes */
typedef enum {
    TRANSPORT_OK = 0,
    TRANSPORT_SUCCESS = 0,  /* Alias for TRANSPORT_OK */
    TRANSPORT_ERROR = -1,
    TRANSPORT_TIMEOUT = -2,
    TRANSPORT_INVALID_PARAM = -3,
    TRANSPORT_NOT_CONNECTED = -4,
    TRANSPORT_ERROR_INVALID_PARAM = -3,  /* Alias */
    TRANSPORT_ERROR_INVALID_MESSAGE = -5
} TransportStatus;

/* Transport message types */
typedef enum {
    TRANSPORT_MSG_TYPE_REQUEST = 1,
    TRANSPORT_MSG_TYPE_RESPONSE = 2
} TransportMessageType;

/* Maximum fields per message */
#define TRANSPORT_MAX_FIELDS 16

/* Transport field structure */
typedef struct {
    uint16_t tag;
    size_t length;
    const uint8_t *value;
    /* Inline storage for small scalar values.  Callers frequently pass the
     * address of a local stack variable (status byte, conn_req, msg_len,
     * association IDs).  The response is serialized later, after the handler
     * has returned, so those locals are out of scope by then.  Copying small
     * values here keeps them valid for the lifetime of the message. */
    uint8_t inline_buf[8];
} TransportField;

/* Transport message structure */
typedef struct {
    uint8_t command_tag;
    uint8_t field_count;
    TransportField fields[TRANSPORT_MAX_FIELDS];
    uint8_t *data;
    size_t length;
    size_t capacity;
} TransportMessage;

/* Stub functions for transport message operations */
static inline TransportStatus transport_message_init(TransportMessage *msg, TransportMessageType type)
{
    if (!msg) return TRANSPORT_INVALID_PARAM;
    msg->command_tag = 0;
    msg->field_count = 0;
    msg->data = NULL;
    msg->length = 0;
    msg->capacity = 0;
    (void)type;  /* Unused */
    return TRANSPORT_OK;
}

static inline TransportStatus transport_message_set_command(TransportMessage *msg, uint8_t cmd_tag)
{
    if (!msg) return TRANSPORT_INVALID_PARAM;
    msg->command_tag = cmd_tag;
    return TRANSPORT_OK;
}

/* Stub function for adding fields to message */
static inline TransportStatus transport_message_add_field(TransportMessage *msg, uint16_t tag, const void *data, size_t length)
{
    if (!msg || msg->field_count >= TRANSPORT_MAX_FIELDS) {
        return TRANSPORT_ERROR;
    }
    TransportField *f = &msg->fields[msg->field_count];
    f->tag = tag;
    f->length = length;
    /* Small scalar values are usually passed as the address of a caller-local
     * variable that goes out of scope before serialization.  Copy them into
     * the field's own inline storage so they remain valid.  Large payloads are
     * passed via persistent global buffers and keep pointer semantics. */
    if ((data != NULL) && (length > 0U) && (length <= sizeof(f->inline_buf))) {
        memcpy(f->inline_buf, data, length);
        f->value = f->inline_buf;
    } else {
        f->value = (const uint8_t*)data;
    }
    msg->field_count++;
    return TRANSPORT_OK;
}

/* ============================================================================
 * Platform-agnostic logging for the KTA bridge layer.
 *
 * On ESP-IDF/ESP32 platforms (detected via ESP_IDF_VERSION) these forward
 * to the ESP-IDF esp_log driver.  On all other platforms (bare-metal, Linux,
 * Windows, SG41, Nordic) logging is silenced by default.
 *
 * To enable stdout logging on non-ESP32 targets, define KTA_BRIDGE_LOG_ENABLE
 * before including this header (e.g. add -DKTA_BRIDGE_LOG_ENABLE to CFLAGS).
 * ============================================================================ */
#if defined(ESP_IDF_VERSION)                               /* ESP-IDF present */
#  include "esp_log.h"
#  define KTA_LOG_I(tag, fmt, ...)      ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#  define KTA_LOG_W(tag, fmt, ...)      ESP_LOGW(tag, fmt, ##__VA_ARGS__)
#  define KTA_LOG_E(tag, fmt, ...)      ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#  define KTA_LOG_BUF_HEX(tag, buf, len) \
       ESP_LOG_BUFFER_HEX_LEVEL(tag, buf, len, ESP_LOG_INFO)
#elif defined(KTA_BRIDGE_LOG_ENABLE)                       /* Host logging     */
#  include <stdio.h>
#  define KTA_LOG_I(tag, fmt, ...)      printf("[I][%s] " fmt "\n", tag, ##__VA_ARGS__)
#  define KTA_LOG_W(tag, fmt, ...)      printf("[W][%s] " fmt "\n", tag, ##__VA_ARGS__)
#  define KTA_LOG_E(tag, fmt, ...)      printf("[E][%s] " fmt "\n", tag, ##__VA_ARGS__)
#  define KTA_LOG_BUF_HEX(tag, buf, len) ((void)0)
#else                                                       /* Logging disabled */
#  define KTA_LOG_I(tag, fmt, ...)      ((void)0)
#  define KTA_LOG_W(tag, fmt, ...)      ((void)0)
#  define KTA_LOG_E(tag, fmt, ...)      ((void)0)
#  define KTA_LOG_BUF_HEX(tag, buf, len) ((void)0)
#endif

/* ============================================================================
 * Bridge KTA Interface
 * ============================================================================ */

#ifdef __cplusplus
extern "C" {
#endif

/** Bridge command tags (maps to k_kta.h APIs) */
typedef enum {
    /* K_KTA.H APIs */
    BRIDGE_CMD_INITIALIZE              = 0xA0, /* ktaInitialize() */
    BRIDGE_CMD_STARTUP                 = 0xA1, /* ktaStartup(seed, profileUid, serialNum, version) */
    BRIDGE_CMD_SET_DEVICE_INFO         = 0xA2, /* ktaSetDeviceInformation(profileUid, serialNum) */
    BRIDGE_CMD_EXCHANGE_MESSAGE        = 0xA3, /* ktaExchangeMessage(ks2kta_msg, kta2ks_msg) */
    BRIDGE_CMD_KEYSTREAM_STATUS        = 0xA4, /* ktaKeyStreamStatus() */
    BRIDGE_CMD_GET_OBJECT_WITH_ASSOC   = 0xA5, /* ktaGetObjectWithAssociation(objId) */
    BRIDGE_CMD_GET_OBJECT              = 0xA6, /* ktaGetObject(objId) */
    BRIDGE_CMD_SIGN_HASH               = 0xA7, /* ktaSignHash(keyId, hash) */
    BRIDGE_CMD_REFURBISH               = 0xA8, /* ktaRefurbish() */
} BridgeCmd;

/** Bridge field tags for command parameters */
typedef enum {
    /* Generic */
    BRIDGE_FIELD_L1_SEG_SEED            = 0x0001, /* uint8_t[16] - L1 segmentation seed */
    BRIDGE_FIELD_CONTEXT_PROFILE_UID    = 0x0002, /* uint8_t[1..32] - Context profile UID */
    BRIDGE_FIELD_CONTEXT_SERIAL_NUM     = 0x0003, /* uint8_t[1..16] - Context serial number */
    BRIDGE_FIELD_CONTEXT_VERSION        = 0x0004, /* uint8_t[1..16] - Context version */
    
    BRIDGE_FIELD_DEVICE_PROFILE_UID     = 0x0005, /* uint8_t[1..32] - Device profile UID */
    BRIDGE_FIELD_DEVICE_SERIAL_NUM      = 0x0006, /* uint8_t[1..16] - Device serial number */
    
    BRIDGE_FIELD_KS_MSG_TO_PROCESS      = 0x0007, /* uint8_t[0..6144] - Message from keySTREAM */
    BRIDGE_FIELD_KTA_MSG_TO_SEND        = 0x0008, /* uint8_t[0..6144] - Message to keySTREAM (output) */
    
    BRIDGE_FIELD_OBJECT_ID              = 0x0009, /* uint32_t - Object identifier */
    BRIDGE_FIELD_ASSOCIATED_KEY_ID      = 0x000A, /* uint32_t - Associated key ID (output) */
    BRIDGE_FIELD_ASSOCIATED_OBJ_ID      = 0x000B, /* uint32_t - Associated object ID (output) */
    
    BRIDGE_FIELD_KEY_ID                 = 0x000C, /* uint32_t - Key identifier */
    BRIDGE_FIELD_HASH_DATA              = 0x000D, /* uint8_t[1..256] - Hash data */
    BRIDGE_FIELD_SIGNED_HASH            = 0x000E, /* uint8_t[1..512] - Signed hash (output) */
    
    BRIDGE_FIELD_OBJECT_DATA            = 0x000F, /* uint8_t[variable] - Object data (output) */
    
    /* Response/Status */
    BRIDGE_FIELD_STATUS                 = 0x0101, /* uint8_t - TKStatus return code */
    BRIDGE_FIELD_KS_CMD_STATUS          = 0x0102, /* uint8_t - TKktaKeyStreamStatus */
    BRIDGE_FIELD_CONN_REQUEST           = 0x0103, /* uint8_t - Connection request status (0=provisioned, 1=needs onboarding) */
    BRIDGE_FIELD_MSG_LEN                = 0x0104, /* uint16_t - Length of outgoing KTA message */
} BridgeField;

/**
 * @brief Maximum size for KTA protocol messages
 * 
 * This buffer holds messages exchanged between KTA and KeyStream server.
 * Default: 6144 bytes (6KB) - sufficient for typical KTA operations.
 * 
 * Minimum recommended: 2048 bytes
 * Maximum typical: 8192 bytes
 */
#define C_BRIDGE_KTA_MESSAGE_BUFFER_SIZE    (6144U)

/**
 * @brief Maximum size for object data
 * 
 * This buffer holds object data retrieved via ktaGetObject() or
 * ktaGetObjectWithAssociation() APIs.
 * Default: 6144 bytes (6KB)
 * 
 * Adjust based on expected object sizes in your application.
 */
#define C_BRIDGE_OBJECT_DATA_BUFFER_SIZE    (6144U)

/**
 * @brief Maximum size for signed hash data
 * 
 * This buffer holds signed hash output from ktaSignHash() API.
 * Default: 512 bytes
 * 
 * Typical signature sizes:
 * - RSA-2048: 256 bytes
 * - RSA-4096: 512 bytes
 * - ECDSA P-256: 64 bytes
 */
#define C_BRIDGE_SIGNED_HASH_BUFFER_SIZE    (512U)

/**
 * @brief Handle a transport KTA command and build a response.
 *
 * This function is transport-agnostic and works with any backend
 * (UART, USB, BLE, Zigbee, etc.).
 *
 * @param[in]  request Incoming transport message
 * @param[out] response Outgoing transport response message
 *
 * @return TRANSPORT_SUCCESS on success, error otherwise
 */
TransportStatus bridge_kta_handle_transport(const TransportMessage *request,
                                             TransportMessage *response);

#ifdef __cplusplus
}
#endif

#endif /* KTA_BRIDGE_H */

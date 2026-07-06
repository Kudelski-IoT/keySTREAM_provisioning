/**
 * @file backend_message.h
 * @brief Backend Message Protocol Layer
 * 
 * Backend-agnostic message protocol supporting UART/BLE/Zigbee/USB.
 * This is the SAME interface for both MCU and host sides.
 * 
 * Platform-independent message serialization/deserialization API using
 * TLV (Tag-Length-Value) format that works over any binary transport.
 */

#ifndef BACKEND_MESSAGE_H
#define BACKEND_MESSAGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Constants & Definitions
 * ============================================================================ */

/** Backend Message Communication Parameters */
#define BACKEND_MESSAGE_MAX_SIZE       6144
#define BACKEND_MESSAGE_BUFFER_SIZE    8192
#define BACKEND_MESSAGE_DEFAULT_TIMEOUT_MS     10000

/** Message Structure Parameters */
#define BACKEND_MESSAGE_MAX_FIELDS     5  /**< Max fields: handle_get_object_with_assoc uses 4 */

/** Message Type Definitions */
typedef enum {
    BACKEND_MSG_TYPE_COMMAND  = 0x01,
    BACKEND_MSG_TYPE_RESPONSE = 0x02,
} BackendMessageType;

/** Protocol Status Codes */
typedef enum {
    BACKEND_MESSAGE_SUCCESS                = 0x00,
    BACKEND_MESSAGE_ERROR_INVALID_PARAM    = 0x01,
    BACKEND_MESSAGE_ERROR_BUFFER_FULL      = 0x02,
    BACKEND_MESSAGE_ERROR_BUFFER_EMPTY     = 0x03,
    BACKEND_MESSAGE_ERROR_TIMEOUT          = 0x04,
    BACKEND_MESSAGE_ERROR_CRC_MISMATCH     = 0x05,
    BACKEND_MESSAGE_ERROR_INVALID_MESSAGE  = 0x06,
    BACKEND_MESSAGE_ERROR_SERIALIZATION    = 0x07,
    BACKEND_MESSAGE_ERROR_DESERIALIZATION  = 0x08,
    BACKEND_MESSAGE_ERROR_HAL_FAILURE      = 0x09,
    BACKEND_MESSAGE_ERROR_NOT_INITIALIZED  = 0x0A,
    BACKEND_MESSAGE_ERROR_UNKNOWN          = 0xFF,
} BackendMessageStatus;

/* ============================================================================
 * Data Structures (TLV Protocol)
 * ============================================================================ */

/**
 * @struct BackendMessageField
 * @brief Individual field within a message (TLV format)
 */
typedef struct {
    uint16_t tag;           /**< Field tag identifier */
    uint16_t length;        /**< Field length in bytes */
    uint8_t *value;         /**< Pointer to field value (variable length) */
} BackendMessageField;

/**
 * @struct BackendMessage
 * @brief Simple message structure for KTA API commands
 * 
 * Simplified design: One message = One command with multiple fields.
 * No crypto, no transaction tracking - just simple request/response.
 */
typedef struct {
    uint8_t message_type;               /**< Message type (COMMAND/RESPONSE) */
    uint8_t command_tag;                /**< Single command tag */
    uint8_t field_count;                /**< Number of fields */
    BackendMessageField fields[BACKEND_MESSAGE_MAX_FIELDS];  /**< Field array */
} BackendMessage;

/* ============================================================================
 * Protocol Layer - Public API
 * ============================================================================ */

/**
 * @brief Initialize backend message protocol layer
 * 
 * Must be called before any other protocol operations. Sets up internal state.
 * Does NOT initialize hardware (see backend SAL layer for that).
 *
 * @return BACKEND_MESSAGE_SUCCESS on success, error code otherwise
 */
BackendMessageStatus backend_message_init(void);

/**
 * @brief Deinitialize backend message protocol layer
 *
 * @return BACKEND_MESSAGE_SUCCESS on success, error code otherwise
 */
BackendMessageStatus backend_message_deinit(void);

/**
 * @brief Initialize message structure
 *
 * Prepares a BackendMessage for use by clearing all fields.
 *
 * @param[in,out] xpMsg    Pointer to message structure. Should not be NULL.
 * @param[in]     xMsgType Message type (COMMAND or RESPONSE)
 * @return BACKEND_MESSAGE_SUCCESS on success
 */
BackendMessageStatus backend_message_create(BackendMessage *xpMsg, uint8_t xMsgType);

/**
 * @brief Set command tag for message
 *
 * @param[in,out] xpMsg   Pointer to message structure. Should not be NULL.
 * @param[in]     xCmdTag Command tag identifier
 * @return BACKEND_MESSAGE_SUCCESS on success
 */
BackendMessageStatus backend_message_set_command(BackendMessage *xpMsg, uint8_t xCmdTag);

/**
 * @brief Add a field to the last command in message
 *
 * @param[in,out] xpMsg      Pointer to message structure. Should not be NULL.
 * @param[in]     xFieldTag  Field tag identifier
 * @param[in]     xpValue    Pointer to field data. Should not be NULL.
 * @param[in]     xLength    Length of field data
 * @return BACKEND_MESSAGE_SUCCESS on success
 */
BackendMessageStatus backend_message_add_field(BackendMessage *xpMsg,
                                            uint16_t xFieldTag,
                                            const uint8_t *xpValue,
                                            uint16_t xLength);

/**
 * @brief Serialize a message to binary format
 *
 * Converts a BackendMessage structure to binary format (TLV encoding)
 * suitable for transmission over any binary transport.
 *
 * @param[in]     xpMsg          Pointer to message structure. Should not be NULL.
 * @param[out]    xpOutput       Pointer to output buffer. Should not be NULL.
 * @param[in]     xOutputSize    Size of output buffer
 * @param[out]    xpOutputLength Pointer to store actual serialized length. Should not be NULL.
 * @return BACKEND_MESSAGE_SUCCESS on success
 */
BackendMessageStatus backend_message_serialize(const BackendMessage *xpMsg,
                                            uint8_t *xpOutput,
                                            size_t xOutputSize,
                                            size_t *xpOutputLength);

/**
 * @brief Deserialize binary data to message structure
 *
 * Converts binary TLV data received from backend into BackendMessage structure.
 *
 * @warning The deserialized BackendMessage stores `value` pointers that
 *          alias into @p xpData. The caller MUST keep @p xpData valid and
 *          unmodified for the entire lifetime of @p xpMsg, otherwise field
 *          accesses (e.g. via backend_message_get_field()) will return
 *          dangling pointers and produce undefined behavior. If you need
 *          the message to outlive the input buffer, copy the field values
 *          out before the buffer goes out of scope.
 *
 * @param[in]     xpData   Pointer to binary data. Should not be NULL.
 * @param[in]     xLength  Length of binary data
 * @param[out]    xpMsg    Pointer to output message structure. Should not be NULL.
 * @return BACKEND_MESSAGE_SUCCESS on success
 */
BackendMessageStatus backend_message_deserialize(const uint8_t *xpData,
                                              size_t xLength,
                                              BackendMessage *xpMsg);

/**
 * @brief Calculate CRC32 checksum for data
 *
 * @param[in] xpData  Pointer to data. Should not be NULL.
 * @param[in] xLength Length of data
 * @return CRC32 checksum value
 */
uint32_t backend_message_calculate_crc32(const uint8_t *xpData, size_t xLength);

/**
 * @brief Verify CRC32 checksum
 *
 * @param[in] xpData       Pointer to data. Should not be NULL.
 * @param[in] xLength      Length of data
 * @param[in] xExpectedCrc Expected CRC value
 * @return true if CRC matches, false otherwise
 */
bool backend_message_verify_crc32(const uint8_t *xpData, size_t xLength, uint32_t xExpectedCrc);

/**
 * @brief Get field value from message by tag (helper utility)
 *
 * @param[in]  xpMsg     Pointer to message structure. Should not be NULL.
 * @param[in]  xTag      Field tag to search for
 * @param[out] xpLength  Pointer to store field length. Should not be NULL.
 * @return Pointer to field value, or NULL if not found
 */
const uint8_t* backend_message_get_field(const BackendMessage *xpMsg, uint16_t xTag, size_t *xpLength);

/**
 * @brief Check if message structure is valid (helper utility)
 *
 * @param msg Pointer to message structure
 * @return true if valid, false otherwise
 */
bool backend_message_is_valid(const BackendMessage *msg);

#ifdef __cplusplus
}
#endif

#endif /* BACKEND_MESSAGE_H */

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
 * @file kta_async_client.h
 * @brief Asynchronous KTA Client Wrapper
 * 
 * ┌────────────────────────────────────────────────────────────────┐
 * │              INTERNAL IMPLEMENTATION HEADER                    │
 * │                                                                │
 * │ DO NOT INCLUDE THIS HEADER IN CUSTOMER APPLICATIONS            │
 * │                                                                │
 * │ Customers should only include: ktaFieldMgntHook.h              │
 * │                                                                │
 * │ This header is for internal library implementation only.       │
 * │ It provides the async/threading layer between the customer     │
 * │ API and the backend transport layer.                           │
 * └────────────────────────────────────────────────────────────────┘
 * 
 * MEMORY FOOTPRINT (optimized for low-end devices):
 *   - KtaAsyncClient struct: ~11KB RAM
 *   - Thread stack: 8-16KB (platform-dependent)
 *   - Total RAM usage: ~20-27KB per client instance
 * 
 * Provides async/callback-based interface for KTA API calls over any backend transport.
 * Handles threading, callbacks, and request/response logging.
 * 
 * Backend selection is compile-time via KTA_CLIENT_BACKEND macro:
 *   -DKTA_CLIENT_BACKEND=BACKEND_TYPE_UART
 *   -DKTA_CLIENT_BACKEND=BACKEND_TYPE_BLE
 *   -DKTA_CLIENT_BACKEND=BACKEND_TYPE_USB
 *   -DKTA_CLIENT_BACKEND=BACKEND_TYPE_ZIGBEE
 */

#ifndef KTA_ASYNC_CLIENT_H
#define KTA_ASYNC_CLIENT_H

#include "../../../backends/backend_interface.h"
#include "../../../backends/backend_message.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Compile-Time Backend Selection
 * ============================================================================ */

/** Default backend if not specified at compile time */
#ifndef KTA_CLIENT_BACKEND
#define KTA_CLIENT_BACKEND      BACKEND_TYPE_UART
#endif

/* ============================================================================
 * KTA API Types
 * ============================================================================ */

typedef enum {
    KTA_API_INITIALIZE = 0,
    KTA_API_STARTUP,
    KTA_API_SET_DEVICE_INFO,
    KTA_API_EXCHANGE_MESSAGE,
    KTA_API_KEYSTREAM_STATUS,
    KTA_API_REFURBISH,
} KtaApiType;

/* ============================================================================
 * KTA Request Structure (Optimized for low-end devices)
 * ============================================================================ */

typedef struct {
    KtaApiType api_type;
    uint32_t request_id;
    
    /* API-specific parameters (inline to avoid heap allocation) */
    union {
        struct {
            /* No parameters for Initialize */
            uint8_t reserved;
        } initialize;
        
        struct {
            uint8_t seed[16];
            uint8_t profile_uid[32];
            uint16_t profile_uid_len;  /* uint16_t sufficient */
            uint8_t serial_num[16];
            uint16_t serial_num_len;
            uint8_t version[16];
            uint16_t version_len;
        } startup;
        
        struct {
            uint8_t profile_uid[32];
            uint16_t profile_uid_len;
            uint8_t serial_num[16];
            uint16_t serial_num_len;
        } set_device_info;
        
        struct {
            uint8_t ks_msg[6144];  /* KTA max message size */
            uint16_t ks_msg_len;   /* uint16_t sufficient for 6KB max */
        } exchange_message;
        
        struct {
            /* No parameters for KeystreamStatus */
            uint8_t reserved;
        } keystream_status;
    } params;
} KtaRequest;  /* Total: ~6.2KB per request */

/* ============================================================================
 * KTA Response Structure (Optimized for low-end devices)
 * ============================================================================ */

typedef struct {
    uint32_t request_id;
    KtaApiType api_type;
    int32_t status_code;
    
    /* Response data (reduced from 8KB to 4KB for low-end devices) */
    uint8_t data[4096];
    uint16_t data_len;  /* uint16_t sufficient for 4KB max */
} KtaResponse;  /* Total: ~4KB per response */

/* ============================================================================
 * KTA Response Callback
 * ============================================================================ */

/**
 * @brief KTA API response callback
 * 
 * @param[in] xpRequest  Original request. Should not be NULL.
 * @param[in] xpResponse Response from MCU (NULL if error)
 * @param[in] xpError    Error message (NULL if success)
 * @param[in] xpUserData User-provided context
 */
typedef void (*KtaResponseCallback)(
    const KtaRequest *xpRequest,
    const KtaResponse *xpResponse,
    const char *xpError,
    void *xpUserData
);

/* ============================================================================
 * Async KTA Client Context (Optimized for low-end devices)
 * ============================================================================ */

typedef struct {
    /* Backend type (compile-time selection) */
    BackendType backend_type;
    bool is_connected;
    bool is_running;
    
    /* Platform-specific threading context */
    void *platform_thread_context;
    
    /* Logging (optional, can be disabled to save memory) */
    FILE *log_file;
    char log_filename[128];  /* Reduced from 256 */
    bool logging_enabled;
    
    /* Pending requests (reduced from 16 to 8 for low-end devices) */
    KtaRequest pending_requests[8];
    uint8_t pending_count;
    uint32_t next_request_id;
    
    /* Callbacks */
    KtaResponseCallback response_callback;
    void *user_data;
    
    /* Receive buffer for partial messages (reduced from 8KB to 4KB) */
    uint8_t rx_buffer[4096];
    uint16_t rx_buffer_len;  /* uint16_t sufficient for 4KB max */
} KtaAsyncClient;  /* Total: ~54KB (8 pending requests * ~6.2KB each + 4KB rx buffer) */

/* ============================================================================
 * Async KTA Client Functions
 * ============================================================================ */

/**
 * @brief Initialize async KTA client
 * 
 * Uses compile-time backend selection (KTA_CLIENT_BACKEND macro).
 * SAL handles its own configuration internally (baud rates, UUIDs, etc.).
 * 
 * @param[in,out] xpClient   Client context to initialize. Should not be NULL.
 * @param[in]     xLogToFile Enable automatic request/response logging
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus kta_async_client_init(
    KtaAsyncClient *xpClient,
    bool xLogToFile
);

/**
 * @brief Set response callback
 * 
 * @param[in,out] xpClient   KTA client context. Should not be NULL.
 * @param[in]     xCallback  Response callback function
 * @param[in]     xpUserData User context pointer
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus kta_async_client_set_callback(
    KtaAsyncClient *xpClient,
    KtaResponseCallback xCallback,
    void *xpUserData
);

/**
 * @brief Start async KTA client
 * 
 * Opens backend connection and starts async processing.
 * 
 * @param[in,out] xpClient KTA client context. Should not be NULL.
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus kta_async_client_start(KtaAsyncClient *xpClient);

/**
 * @brief Stop async KTA client
 * 
 * @param[in,out] xpClient KTA client context. Should not be NULL.
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus kta_async_client_stop(KtaAsyncClient *xpClient);

/**
 * @brief Send ktaInitialize() request
 * 
 * @param[in] xpClient KTA client context. Should not be NULL.
 * @return Request ID (> 0) on success, 0 on error
 */
uint32_t kta_async_initialize(KtaAsyncClient *xpClient);

/**
 * @brief Send ktaStartup() request
 * 
 * @param[in] xpClient       KTA client context. Should not be NULL.
 * @param[in] xpSeed         L1 segmentation seed (16 bytes). Should not be NULL.
 * @param[in] xpProfileUid   Context profile UID. Should not be NULL.
 * @param[in] xProfileUidLen Profile UID length (1-32 bytes)
 * @param[in] xpSerialNum    Context serial number. Should not be NULL.
 * @param[in] xSerialNumLen  Serial number length (1-16 bytes)
 * @param[in] xpVersion      Context version. Should not be NULL.
 * @param[in] xVersionLen    Version length (1-16 bytes)
 * @return Request ID (> 0) on success, 0 on error
 */
uint32_t kta_async_startup(
    KtaAsyncClient *xpClient,
    const uint8_t *xpSeed,
    const uint8_t *xpProfileUid, size_t xProfileUidLen,
    const uint8_t *xpSerialNum, size_t xSerialNumLen,
    const uint8_t *xpVersion, size_t xVersionLen
);

/**
 * @brief Send ktaSetDeviceInformation() request
 * 
 * @param[in] xpClient       KTA client context. Should not be NULL.
 * @param[in] xpProfileUid   Device profile UID. Should not be NULL.
 * @param[in] xProfileUidLen Profile UID length (1-32 bytes)
 * @param[in] xpSerialNum    Device serial number. Should not be NULL.
 * @param[in] xSerialNumLen  Serial number length (1-16 bytes)
 * @return Request ID (> 0) on success, 0 on error
 */
uint32_t kta_async_set_device_info(
    KtaAsyncClient *xpClient,
    const uint8_t *xpProfileUid, size_t xProfileUidLen,
    const uint8_t *xpSerialNum, size_t xSerialNumLen
);

/**
 * @brief Send ktaExchangeMessage() request
 * 
 * @param[in] xpClient  KTA client context. Should not be NULL.
 * @param[in] xpKsMsg   Message from KeyStream server. Should not be NULL.
 * @param[in] xKsMsgLen Message length (0-6144 bytes)
 * @return Request ID (> 0) on success, 0 on error
 */
uint32_t kta_async_exchange_message(
    KtaAsyncClient *xpClient,
    const uint8_t *xpKsMsg,
    size_t xKsMsgLen
);

/**
 * @brief Send ktaKeyStreamStatus() request
 * 
 * @param[in] xpClient KTA client context. Should not be NULL.
 * @return Request ID (> 0) on success, 0 on error
 */
uint32_t kta_async_keystream_status(KtaAsyncClient *xpClient);

/**
 * @brief Send REFURBISH command to MCU (resets lifecycle state and bridge flags).
 * @param[in] xpClient  Async client instance.
 * @return Non-zero request ID on success, 0 on failure.
 */
uint32_t kta_async_refurbish(KtaAsyncClient *xpClient);

/**
 * @brief Check if client is connected
 * 
 * @param[in] xpClient KTA client context. Should not be NULL.
 * @return true if connected, false otherwise
 */
bool kta_async_is_connected(const KtaAsyncClient *xpClient);

/**
 * @brief Get number of pending requests
 * 
 * @param[in] xpClient KTA client context. Should not be NULL.
 * @return Number of pending requests
 */
uint8_t kta_async_get_pending_count(const KtaAsyncClient *xpClient);

/**
 * @brief Deinitialize async KTA client
 * 
 * @param[in,out] xpClient KTA client context. Should not be NULL.
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus kta_async_client_deinit(KtaAsyncClient *xpClient);

#ifdef __cplusplus
}
#endif

#endif /* KTA_ASYNC_CLIENT_H */

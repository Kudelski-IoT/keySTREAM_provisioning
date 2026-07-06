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
********************************************************************************//**
 * @file kta_async_client.c
 * @brief Asynchronous KTA Client Implementation - Bare Metal Platform
 *
 * Bare-metal implementation: no RTOS, no dynamic memory, no file system.
 *
 * Because there is no background thread available on bare metal, the
 * "async" interface is fulfilled in a synchronous/blocking manner:
 *
 *   1. kta_async_client_start()  - opens the backend (no thread spawned).
 *   2. send_kta_request()        - serializes the request, sends it over the
 *                                  backend, then immediately enters a bounded
 *                                  receive loop.  When a complete TLV response
 *                                  arrives the registered callback is invoked
 *                                  inline before the function returns.
 *   3. kta_async_client_stop()   - closes the backend.
 *
 * Consequence for the caller (ktaFieldMgntHook.c):
 *   By the time send_kta_request() returns a non-zero request ID the callback
 *   has already been executed and g_waiting_for_response has been cleared.
 *   The wait_for_response() polling loop therefore exits on the very first
 *   iteration with the result already in place.
 *
 * Logging is omitted: bare-metal targets typically have no writable file
 * system.  Define KTA_ENABLE_LOGGING externally and provide a custom
 * log sink if needed.
 */

#include "../include/kta_async_client.h"
#include <string.h>

/* ============================================================================
 * Compile-time tunables
 * ============================================================================ */

/** Maximum number of receive attempts (each attempt calls backend_receive
 *  once with a short timeout).  Multiply by the backend timeout to get the
 *  overall per-request deadline, e.g. 300 × 100 ms = 30 s. */
#ifndef KTA_BAREMETAL_MAX_RECV_ATTEMPTS
#define KTA_BAREMETAL_MAX_RECV_ATTEMPTS  (300U)
#endif

/** Stack-allocated receive buffer size for the blocking receive loop. */
#define KTA_BAREMETAL_RECV_BUF_SIZE      (2048U)

/** Stack-allocated serialization buffer size. */
#define KTA_BAREMETAL_SEND_BUF_SIZE      (2048U)

/* ============================================================================
 * Bare-metal "thread context" (unused – present only to satisfy the generic
 * KtaAsyncClient struct layout expected by the shared header)
 * ============================================================================ */

typedef struct {
    volatile bool running; /* Not used for scheduling; tracks open state */
} BaremetalThreadContext;

/* Static allocation – avoids heap dependency */
static BaremetalThreadContext s_thread_ctx;

/* ============================================================================
 * Internal helpers
 * ============================================================================ */

static void process_received_data(KtaAsyncClient *xpClient,
                                   const uint8_t   *xpData,
                                   size_t           xLength)
{
    if ((NULL == xpClient) || (NULL == xpData) || (0U == xLength)) {
        return;
    }

    /* Append to receive buffer */
    if ((xpClient->rx_buffer_len + xLength) > sizeof(xpClient->rx_buffer)) {
        /* Overflow – discard accumulated data and start fresh */
        xpClient->rx_buffer_len = 0U;
        return;
    }

    (void)memcpy(xpClient->rx_buffer + xpClient->rx_buffer_len, xpData, xLength);
    xpClient->rx_buffer_len += xLength;

    /* Try to parse a complete TLV message */
    BackendMessage msg;
    (void)memset(&msg, 0, sizeof(msg));

    BackendMessageStatus parseStatus =
        backend_message_deserialize(xpClient->rx_buffer, xpClient->rx_buffer_len, &msg);

    if (BACKEND_MESSAGE_SUCCESS == parseStatus) {
        /* Build a KtaResponse from the raw TLV fields */
        KtaResponse response;
        (void)memset(&response, 0, sizeof(response));

        response.request_id  = (msg.field_count > 0U) ? msg.fields[0].tag : 0U;
        response.status_code = 0; /* Extracted from TLV fields by the caller */

        if ((msg.field_count > 0U) &&
            (msg.fields[0].length > 0U) &&
            (msg.fields[0].length <= sizeof(response.data))) {
            (void)memcpy(response.data, msg.fields[0].value, msg.fields[0].length);
            response.data_len = msg.fields[0].length;
        }

        /* Locate the matching pending request */
        KtaRequest *pMatchedReq = NULL;
        for (uint8_t i = 0U; i < xpClient->pending_count; i++) {
            if (xpClient->pending_requests[i].request_id == response.request_id) {
                pMatchedReq = &xpClient->pending_requests[i];
                break;
            }
        }

        /* Invoke the registered callback synchronously */
        if (NULL != xpClient->response_callback) {
            xpClient->response_callback(pMatchedReq, &response, NULL,
                                        xpClient->user_data);
        }

        /* Remove from pending queue (simplified: decrement count) */
        if (xpClient->pending_count > 0U) {
            xpClient->pending_count--;
        }

        /* Reset buffer for the next message */
        xpClient->rx_buffer_len = 0U;
    }
}

/**
 * @brief Serialize, send, and synchronously receive a KTA request.
 *
 * This function blocks until either a complete response is received and
 * dispatched through the callback, or the retry limit is exhausted.
 *
 * @param xpClient  Initialized client instance.
 * @param xpReq     Populated request (api_type must be set by the caller).
 *
 * @return  Non-zero request ID on success, 0 on failure.
 */
static uint32_t send_kta_request(KtaAsyncClient *xpClient, KtaRequest *xpReq)
{
    if ((NULL == xpClient) || (NULL == xpReq)) {
        return 0U;
    }

    if (!xpClient->is_running) {
        return 0U;
    }

    /* Assign a monotonically increasing request ID */
    xpReq->request_id = ++xpClient->next_request_id;

    /* Build a minimal TLV command message */
    BackendMessage msg;
    (void)memset(&msg, 0, sizeof(msg));
    (void)backend_message_create(&msg, BACKEND_MSG_TYPE_COMMAND);
    (void)backend_message_set_command(&msg, (uint8_t)xpReq->api_type);

    /* Serialize into a stack buffer (no heap) */
    uint8_t sendBuf[KTA_BAREMETAL_SEND_BUF_SIZE];
    size_t  serializedLen = 0U;

    BackendMessageStatus serStatus =
        backend_message_serialize(&msg, sendBuf, sizeof(sendBuf), &serializedLen);
    if (BACKEND_MESSAGE_SUCCESS != serStatus) {
        return 0U;
    }

    /* Transmit */
    BackendStatus sendStatus = backend_send(sendBuf, serializedLen);
    if (BACKEND_OK != sendStatus) {
        return 0U;
    }

    /* Track in the pending queue (bounded by the actual array size) */
    const uint8_t pendingCapacity =
        (uint8_t)(sizeof(xpClient->pending_requests) /
                  sizeof(xpClient->pending_requests[0]));
    if (xpClient->pending_count < pendingCapacity) {
        (void)memcpy(&xpClient->pending_requests[xpClient->pending_count],
                     xpReq, sizeof(KtaRequest));
        xpClient->pending_count++;
    } else {
        /* Queue full – cannot track this request */
        return 0U;
    }

    /* Snapshot the pending count so we can detect when process_received_data()
     * dispatches a response (it decrements pending_count on success). */
    const uint8_t pendingBefore = xpClient->pending_count;

    /* ---- Synchronous receive loop ---- 
     * Poll the backend until a complete response arrives or we time out.
     * The short per-call timeout is provided by the SAL (backend_set_timeout).
     * process_received_data() calls the callback inline, which will clear
     * g_waiting_for_response before we return to the caller. */
    (void)backend_set_timeout(100U); /* 100 ms per attempt */

    uint8_t recvBuf[KTA_BAREMETAL_RECV_BUF_SIZE];
    size_t  received = 0U;

    for (uint32_t attempt = 0U; attempt < KTA_BAREMETAL_MAX_RECV_ATTEMPTS; attempt++) {
        received = 0U;
        BackendStatus recvStatus =
            backend_receive(recvBuf, sizeof(recvBuf), &received);

        if ((BACKEND_OK == recvStatus) && (received > 0U)) {
            process_received_data(xpClient, recvBuf, received);

            /* A successful parse decrements pending_count: that is our
             * signal that the callback has run and the response is ready. */
            if (xpClient->pending_count < pendingBefore) {
                break; /* Response fully processed */
            }
        } else if (BACKEND_TIMEOUT == recvStatus) {
            /* Expected – keep waiting */
            continue;
        } else {
            /* Unexpected transport error; abort */
            break;
        }
    }

    return xpReq->request_id;
}

/* ============================================================================
 * Public API Implementation
 * ============================================================================ */

BackendStatus kta_async_client_init(KtaAsyncClient *xpClient, bool xLogToFile)
{
    if (NULL == xpClient) {
        return BACKEND_INVALID_PARAM;
    }

    (void)memset(xpClient, 0, sizeof(KtaAsyncClient));

    /* Logging is not supported on bare metal (no file system assumed) */
    (void)xLogToFile;
    xpClient->logging_enabled = false;
    xpClient->log_file        = NULL;

    xpClient->backend_type    = KTA_CLIENT_BACKEND;

    /* Initialize the transport backend */
    BackendStatus status = backend_init(xpClient->backend_type);
    if (BACKEND_OK != status) {
        return status;
    }

    xpClient->next_request_id = 0U;
    xpClient->pending_count   = 0U;
    xpClient->is_connected    = false;
    xpClient->is_running      = false;

    return BACKEND_OK;
}

BackendStatus kta_async_client_set_callback(KtaAsyncClient    *xpClient,
                                             KtaResponseCallback xCallback,
                                             void               *xpUserData)
{
    if (NULL == xpClient) {
        return BACKEND_INVALID_PARAM;
    }

    xpClient->response_callback = xCallback;
    xpClient->user_data         = xpUserData;

    return BACKEND_OK;
}

BackendStatus kta_async_client_start(KtaAsyncClient *xpClient)
{
    if (NULL == xpClient) {
        return BACKEND_INVALID_PARAM;
    }

    if (xpClient->is_running) {
        return BACKEND_OK; /* Already open */
    }

    /* Open the backend; SAL reads its own compile-time config */
    BackendStatus status = backend_open(NULL);
    if (BACKEND_OK != status) {
        return status;
    }

    /* Set up bare-metal "context" (no actual thread) */
    (void)memset(&s_thread_ctx, 0, sizeof(s_thread_ctx));
    s_thread_ctx.running             = true;
    xpClient->platform_thread_context = &s_thread_ctx;

    xpClient->is_connected = true;
    xpClient->is_running   = true;

    return BACKEND_OK;
}

BackendStatus kta_async_client_stop(KtaAsyncClient *xpClient)
{
    if (NULL == xpClient) {
        return BACKEND_INVALID_PARAM;
    }

    if (!xpClient->is_running) {
        return BACKEND_OK;
    }

    if (NULL != xpClient->platform_thread_context) {
        ((BaremetalThreadContext *)xpClient->platform_thread_context)->running = false;
        xpClient->platform_thread_context = NULL;
    }

    (void)backend_close();

    xpClient->is_running   = false;
    xpClient->is_connected = false;

    return BACKEND_OK;
}

/* ---- KTA API wrappers ---------------------------------------------------- */

uint32_t kta_async_initialize(KtaAsyncClient *xpClient)
{
    KtaRequest req;
    (void)memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_INITIALIZE;

    return send_kta_request(xpClient, &req);
}

uint32_t kta_async_startup(KtaAsyncClient  *xpClient,
                            const uint8_t   *xpSeed,
                            const uint8_t   *xpProfileUid,   size_t xProfileUidLen,
                            const uint8_t   *xpSerialNum,    size_t xSerialNumLen,
                            const uint8_t   *xpVersion,      size_t xVersionLen)
{
    if ((NULL == xpSeed) || (NULL == xpProfileUid) ||
        (NULL == xpSerialNum) || (NULL == xpVersion)) {
        return 0U;
    }

    KtaRequest req;
    (void)memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_STARTUP;

    (void)memcpy(req.params.startup.seed, xpSeed, 16U);
    (void)memcpy(req.params.startup.profile_uid, xpProfileUid, xProfileUidLen);
    req.params.startup.profile_uid_len = (uint16_t)xProfileUidLen;
    (void)memcpy(req.params.startup.serial_num, xpSerialNum, xSerialNumLen);
    req.params.startup.serial_num_len  = (uint16_t)xSerialNumLen;
    (void)memcpy(req.params.startup.version, xpVersion, xVersionLen);
    req.params.startup.version_len     = (uint16_t)xVersionLen;

    return send_kta_request(xpClient, &req);
}

uint32_t kta_async_set_device_info(KtaAsyncClient *xpClient,
                                    const uint8_t  *xpProfileUid, size_t xProfileUidLen,
                                    const uint8_t  *xpSerialNum,  size_t xSerialNumLen)
{
    if ((NULL == xpProfileUid) || (NULL == xpSerialNum)) {
        return 0U;
    }

    KtaRequest req;
    (void)memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_SET_DEVICE_INFO;

    (void)memcpy(req.params.set_device_info.profile_uid, xpProfileUid, xProfileUidLen);
    req.params.set_device_info.profile_uid_len = (uint16_t)xProfileUidLen;
    (void)memcpy(req.params.set_device_info.serial_num, xpSerialNum, xSerialNumLen);
    req.params.set_device_info.serial_num_len  = (uint16_t)xSerialNumLen;

    return send_kta_request(xpClient, &req);
}

uint32_t kta_async_exchange_message(KtaAsyncClient *xpClient,
                                     const uint8_t  *xpKsMsg,
                                     size_t          xKsMsgLen)
{
    if (NULL == xpClient) {
        return 0U;
    }

    if (xKsMsgLen > 6144U) {
        return 0U; /* Exceeds maximum message size */
    }

    KtaRequest req;
    (void)memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_EXCHANGE_MESSAGE;

    if ((NULL != xpKsMsg) && (xKsMsgLen > 0U)) {
        (void)memcpy(req.params.exchange_message.ks_msg, xpKsMsg, xKsMsgLen);
        req.params.exchange_message.ks_msg_len = (uint16_t)xKsMsgLen;
    } else {
        /* First exchange with an empty message */
        req.params.exchange_message.ks_msg_len = 0U;
    }

    return send_kta_request(xpClient, &req);
}

uint32_t kta_async_keystream_status(KtaAsyncClient *xpClient)
{
    KtaRequest req;
    (void)memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_KEYSTREAM_STATUS;

    return send_kta_request(xpClient, &req);
}

bool kta_async_is_connected(const KtaAsyncClient *xpClient)
{
    return (NULL != xpClient) ? xpClient->is_connected : false;
}

uint8_t kta_async_get_pending_count(const KtaAsyncClient *xpClient)
{
    return (NULL != xpClient) ? xpClient->pending_count : 0U;
}

BackendStatus kta_async_client_deinit(KtaAsyncClient *xpClient)
{
    if (NULL == xpClient) {
        return BACKEND_INVALID_PARAM;
    }

    if (xpClient->is_running) {
        (void)kta_async_client_stop(xpClient);
    }

    /* No log file to close on bare metal */

    BackendStatus status = backend_deinit();

    (void)memset(xpClient, 0, sizeof(KtaAsyncClient));

    return status;
}

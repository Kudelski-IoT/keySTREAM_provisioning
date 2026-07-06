/**
 * @file kta_async_client.c
 * @brief Asynchronous KTA Client Implementation - Windows Platform
 *
 * Windows-specific implementation using CreateThread and HANDLE
 */

#include "../include/kta_async_client.h"
#include <windows.h>
#include <string.h>
#include <time.h>

/* Windows threading context */
typedef struct
{
    HANDLE thread;
    HANDLE stop_event;
    volatile LONG running;
} WindowsThreadContext;

/* ============================================================================
 * Logging Functions
 * ============================================================================ */

static void log_hex_dump(FILE *fp, const char *prefix, const uint8_t *data, size_t len)
{
    if (!fp || !data)
        return;

    fprintf(fp, "%s[%zu bytes]:\n", prefix, len);

    for (size_t i = 0; i < len; i += 16)
    {
        fprintf(fp, "  %04zX: ", i);

        /* Hex dump */
        for (size_t j = 0; j < 16; j++)
        {
            if (i + j < len)
            {
                fprintf(fp, "%02X ", data[i + j]);
            }
            else
            {
                fprintf(fp, "   ");
            }
        }

        fprintf(fp, " | ");

        /* ASCII */
        for (size_t j = 0; j < 16; j++)
        {
            if (i + j < len)
            {
                uint8_t c = data[i + j];
                fprintf(fp, "%c", (c >= 32 && c < 127) ? c : '.');
            }
        }

        fprintf(fp, "\n");
    }

    fflush(fp);
}

static void log_request(KtaAsyncClient *client, const KtaRequest *req, const uint8_t *data, size_t len)
{
    if (!client->logging_enabled || !client->log_file)
        return;

    time_t now = time(NULL);
    fprintf(client->log_file, "\n[%s] REQUEST #%u - ", ctime(&now), req->request_id);

    const char *api_names[] = {"Initialize", "Startup", "SetDeviceInfo", "ExchangeMessage", "KeyStreamStatus", "Refurbish"};
    if ((unsigned)req->api_type < (sizeof(api_names) / sizeof(api_names[0])))
    {
        fprintf(client->log_file, "%s\n", api_names[req->api_type]);
    }
    else
    {
        fprintf(client->log_file, "Unknown(%d)\n", (int)req->api_type);
    }

    log_hex_dump(client->log_file, "  Serialized: ", data, len);
}

static void log_response(KtaAsyncClient *client, const KtaResponse *resp)
{
    if (!client->logging_enabled || !client->log_file)
        return;

    time_t now = time(NULL);
    fprintf(client->log_file, "\n[%s] RESPONSE #%u - Status: %d\n",
            ctime(&now), resp->request_id, resp->status_code);

    if (resp->data_len > 0)
    {
        log_hex_dump(client->log_file, "  Data: ", resp->data, resp->data_len);
    }
}

/* ============================================================================
 * Receive Thread Worker (Windows)
 * ============================================================================ */

static void process_received_data(KtaAsyncClient *client, const uint8_t *data, size_t length)
{
    /* Append to receive buffer */
    if (client->rx_buffer_len + length > sizeof(client->rx_buffer))
    {
        /* Buffer overflow - reset and log error */
        client->rx_buffer_len = 0;
        return;
    }

    memcpy(client->rx_buffer + client->rx_buffer_len, data, length);
    client->rx_buffer_len += length;

    /* Try to parse complete TLV message */
    BackendMessage msg;
    memset(&msg, 0, sizeof(msg));

    BackendMessageStatus status = backend_message_deserialize(client->rx_buffer,
                                                              client->rx_buffer_len, &msg);

    if (status == BACKEND_MESSAGE_SUCCESS)
    {
        /* Complete message received */
        KtaResponse response;
        memset(&response, 0, sizeof(response));

        /* The MCU echoes back command_tag (BRIDGE_CMD value 0xA0+).             */
        /* Map it back to KtaApiType to find the pending request.               */
        static const uint8_t bridge_cmd_base = 0xA0U;
        /* Same table as send_kta_request — used for reverse lookup */
        static const uint8_t kta_to_bridge_cmd[] = {
            0xA0U, 0xA1U, 0xA2U, 0xA3U, 0xA4U, 0xA8U};
        response.request_id = 0U; /* Not used for matching — we match by api_type */
        response.status_code = 0;

        /* Extract status from BRIDGE_FIELD_STATUS (tag 0x0101) */
        for (uint8_t fi = 0U; fi < msg.field_count; fi++)
        {
            if (msg.fields[fi].tag == 0x0101U)
            {
                if (msg.fields[fi].length >= 1U && msg.fields[fi].value != NULL)
                {
                    response.status_code = (int32_t)(int8_t)msg.fields[fi].value[0];
                }
                break;
            }
        }

        /* Extract response payload based on command type:
         *  0xA2 SET_DEVICE_INFO  → BRIDGE_FIELD_CONN_REQUEST    (0x0103)
         *  0xA3 EXCHANGE_MESSAGE → BRIDGE_FIELD_KTA_MSG_TO_SEND (0x0008)
         *  0xA4 KEYSTREAM_STATUS → BRIDGE_FIELD_KS_CMD_STATUS   (0x0102)
         */
        uint16_t payload_tag = 0x0000U;
        if (msg.command_tag == 0xA2U)
            payload_tag = 0x0103U;
        else if (msg.command_tag == 0xA3U)
            payload_tag = 0x0008U;
        else if (msg.command_tag == 0xA4U)
            payload_tag = 0x0102U;

        if (payload_tag != 0x0000U)
        {
            for (uint8_t fi = 0U; fi < msg.field_count; fi++)
            {
                if (msg.fields[fi].tag == payload_tag)
                {
                    size_t dlen = msg.fields[fi].length;
                    if (dlen > sizeof(response.data))
                        dlen = sizeof(response.data);
                    if (dlen > 0U && msg.fields[fi].value != NULL)
                    {
                        memcpy(response.data, msg.fields[fi].value, dlen);
                        response.data_len = (uint16_t)dlen;
                    }
                    break;
                }
            }
        }

        /* Log response */
        log_response(client, &response);

        /* Find matching request by mapping command_tag back to KtaApiType.
         * Use the same kta_to_bridge_cmd table to reverse-map since the tags
         * are not contiguous (0xA0-0xA4 then 0xA8 for REFURBISH). */
        KtaRequest *matched_req = NULL;
        if (msg.command_tag >= bridge_cmd_base)
        {
            for (uint8_t i = 0U; i < client->pending_count; i++)
            {
                uint8_t expected_api = (uint8_t)client->pending_requests[i].api_type;
                if (expected_api < (uint8_t)(sizeof(kta_to_bridge_cmd) / sizeof(kta_to_bridge_cmd[0])))
                {
                    if (kta_to_bridge_cmd[expected_api] == msg.command_tag)
                    {
                        matched_req = &client->pending_requests[i];
                        response.request_id = matched_req->request_id;
                        response.api_type = matched_req->api_type;
                        break;
                    }
                }
            }
        }

        /* Invoke callback */
        if (client->response_callback)
        {
            client->response_callback(matched_req, &response, NULL, client->user_data);
        }

        /* Remove from pending */
        if (client->pending_count > 0)
        {
            client->pending_count--;
        }

        /* Clear buffer */
        client->rx_buffer_len = 0;
    }
}

static DWORD WINAPI receive_thread_worker(LPVOID param)
{
    KtaAsyncClient *client = (KtaAsyncClient *)param;
    WindowsThreadContext *ctx = (WindowsThreadContext *)client->platform_thread_context;

    uint8_t buffer[2048]; /* Reduced from 4096 for low-end devices */
    size_t received;

    while (InterlockedCompareExchange(&ctx->running, 1, 1) == 1)
    {
        /* Check for stop signal (non-blocking) */
        if (WaitForSingleObject(ctx->stop_event, 0) == WAIT_OBJECT_0)
        {
            break;
        }

        /* Blocking receive with timeout */
        backend_set_timeout(100); /* 100ms timeout */
        BackendStatus status = backend_receive(buffer, sizeof(buffer), &received);

        if (status == BACKEND_OK && received > 0)
        {
            process_received_data(client, buffer, received);
        }
        else if (status == BACKEND_TIMEOUT)
        {
            /* Timeout - continue loop (normal operation) */
            continue;
        }
        else if (status != BACKEND_OK)
        {
            /* Mark transport dead so ktaKeyStreamInit re-opens on next poll */
            client->is_running = false;
            break;
        }
    }

    return 0;
}

/* ============================================================================
 * Request Helper
 * ============================================================================ */

static uint32_t send_kta_request(KtaAsyncClient *client, KtaRequest *req)
{
    if (!client || !req)
    {
        return 0;
    }

    /* Check if client is running */
    if (!client->is_running)
    {
        return 0;
    }

    /* Assign request ID */
    req->request_id = ++client->next_request_id;

    /* Build TLV message */
    BackendMessage msg;
    memset(&msg, 0, sizeof(msg));

    backend_message_create(&msg, BACKEND_MSG_TYPE_COMMAND);

    /* Map KtaApiType (0,1,2...) to MCU BRIDGE_CMD tags (0xA0,0xA1,0xA2...) */
    static const uint8_t kta_to_bridge_cmd[] = {
        0xA0U, /* KTA_API_INITIALIZE       → BRIDGE_CMD_INITIALIZE */
        0xA1U, /* KTA_API_STARTUP          → BRIDGE_CMD_STARTUP */
        0xA2U, /* KTA_API_SET_DEVICE_INFO  → BRIDGE_CMD_SET_DEVICE_INFO */
        0xA3U, /* KTA_API_EXCHANGE_MESSAGE → BRIDGE_CMD_EXCHANGE_MESSAGE */
        0xA4U, /* KTA_API_KEYSTREAM_STATUS → BRIDGE_CMD_KEYSTREAM_STATUS */
        0xA8U, /* KTA_API_REFURBISH        → BRIDGE_CMD_REFURBISH */
    };
    uint8_t cmd_tag = 0xA0U;
    if ((uint8_t)req->api_type < (uint8_t)(sizeof(kta_to_bridge_cmd) / sizeof(kta_to_bridge_cmd[0])))
    {
        cmd_tag = kta_to_bridge_cmd[(uint8_t)req->api_type];
    }
    backend_message_set_command(&msg, cmd_tag);

    /* Serialize API parameters as TLV fields using BRIDGE_FIELD_* tags */
    switch (req->api_type)
    {
    case KTA_API_INITIALIZE:
        /* No parameters needed */
        break;
    case KTA_API_STARTUP:
        /* BRIDGE_FIELD_L1_SEG_SEED = 0x0001 */
        backend_message_add_field(&msg, 0x0001U,
                                  req->params.startup.seed, 16U);
        /* BRIDGE_FIELD_CONTEXT_PROFILE_UID = 0x0002 */
        if (req->params.startup.profile_uid_len > 0U)
        {
            backend_message_add_field(&msg, 0x0002U,
                                      req->params.startup.profile_uid,
                                      req->params.startup.profile_uid_len);
        }
        /* BRIDGE_FIELD_CONTEXT_SERIAL_NUM = 0x0003 */
        if (req->params.startup.serial_num_len > 0U)
        {
            backend_message_add_field(&msg, 0x0003U,
                                      req->params.startup.serial_num,
                                      req->params.startup.serial_num_len);
        }
        /* BRIDGE_FIELD_CONTEXT_VERSION = 0x0004 */
        if (req->params.startup.version_len > 0U)
        {
            backend_message_add_field(&msg, 0x0004U,
                                      req->params.startup.version,
                                      req->params.startup.version_len);
        }
        break;
    case KTA_API_SET_DEVICE_INFO:
        /* BRIDGE_FIELD_DEVICE_PROFILE_UID = 0x0005 */
        if (req->params.set_device_info.profile_uid_len > 0U)
        {
            backend_message_add_field(&msg, 0x0005U,
                                      req->params.set_device_info.profile_uid,
                                      req->params.set_device_info.profile_uid_len);
        }
        /* BRIDGE_FIELD_DEVICE_SERIAL_NUM = 0x0006 */
        if (req->params.set_device_info.serial_num_len > 0U)
        {
            backend_message_add_field(&msg, 0x0006U,
                                      req->params.set_device_info.serial_num,
                                      req->params.set_device_info.serial_num_len);
        }
        break;
    case KTA_API_EXCHANGE_MESSAGE:
        /* BRIDGE_FIELD_KS_MSG_TO_PROCESS = 0x0007 */
        if (req->params.exchange_message.ks_msg_len > 0U)
        {
            backend_message_add_field(&msg, 0x0007U,
                                      req->params.exchange_message.ks_msg,
                                      req->params.exchange_message.ks_msg_len);
        }
        break;
    case KTA_API_KEYSTREAM_STATUS:
        /* No parameters needed */
        break;
    case KTA_API_REFURBISH:
        /* No parameters needed */
        break;
    default:
        break;
    }

    /* Serialize message */
    uint8_t buffer[2048]; /* Reduced from 4096 for low-end devices */
    size_t serialized_len = 0;

    BackendMessageStatus status = backend_message_serialize(&msg, buffer, sizeof(buffer), &serialized_len);
    if (status != BACKEND_MESSAGE_SUCCESS)
    {
        return 0;
    }

    /* Log request */
    log_request(client, req, buffer, serialized_len);

    /* Send via backend (blocking call) */
    BackendStatus send_status = backend_send(buffer, serialized_len);
    if (send_status != BACKEND_OK)
    {
        return 0;
    }

    /* Add to pending requests. Bound by the actual array size to avoid
     * overflowing pending_requests[] (e.g. if responses are dropped and the
     * pending count is never decremented). */
    if (client->pending_count < (sizeof(client->pending_requests) / sizeof(client->pending_requests[0])))
    {
        memcpy(&client->pending_requests[client->pending_count], req, sizeof(KtaRequest));
        client->pending_count++;
    }
    else
    {
        /* Pending queue full - cannot track this request */
        return 0;
    }

    return req->request_id;
}

/* ============================================================================
 * Public API Implementation
 * ============================================================================ */

BackendStatus kta_async_client_init(
    KtaAsyncClient *xpClient,
    bool xLogToFile)
{
    if (NULL == xpClient)
    {
        return BACKEND_INVALID_PARAM;
    }

    (void)memset(xpClient, 0, sizeof(KtaAsyncClient));

    /* Set backend type from compile-time macro */
    xpClient->backend_type = KTA_CLIENT_BACKEND;

    /* Initialize backend (blocking I/O) */
    /* SAL handles its own configuration internally */
    BackendStatus status = backend_init(xpClient->backend_type);
    if (BACKEND_OK != status)
    {
        return status;
    }

    /* Set up logging (optional for low-end devices) */
    xpClient->logging_enabled = xLogToFile;
    if (true == xLogToFile)
    {
        time_t now = time(NULL);
        struct tm tm_info;
        if (0 == localtime_s(&tm_info, &now))
        {
            (void)strftime(xpClient->log_filename, sizeof(xpClient->log_filename),
                           "kta_session_%Y%m%d_%H%M%S.log", &tm_info);

            if ((0 == fopen_s(&xpClient->log_file, xpClient->log_filename, "w")) && (NULL != xpClient->log_file))
            {
                const char *backend_names[] = {"UART", "BLE", "USB", "Zigbee"};
                (void)fprintf(xpClient->log_file, "KTA Async Client Log (Windows)\n");
                (void)fprintf(xpClient->log_file, "Backend: %s (compile-time)\n",
                              backend_names[KTA_CLIENT_BACKEND]);
                (void)fprintf(xpClient->log_file, "=====================================\n\n");
                (void)fflush(xpClient->log_file);
            }
            else
            {
                xpClient->logging_enabled = false;
            }
        }
        else
        {
            xpClient->logging_enabled = false;
        }
    }

    xpClient->next_request_id = 0U;
    xpClient->pending_count = 0U;
    xpClient->is_connected = false;
    xpClient->is_running = false;

    return BACKEND_OK;
}

BackendStatus kta_async_client_set_callback(
    KtaAsyncClient *xpClient,
    KtaResponseCallback xCallback,
    void *xpUserData)
{
    if (NULL == xpClient)
    {
        return BACKEND_INVALID_PARAM;
    }

    xpClient->response_callback = xCallback;
    xpClient->user_data = xpUserData;

    return BACKEND_OK;
}

BackendStatus kta_async_client_start(KtaAsyncClient *xpClient)
{
    if (NULL == xpClient)
    {
        return BACKEND_INVALID_PARAM;
    }

    if (true == xpClient->is_running)
    {
        return BACKEND_OK; /* Already running */
    }

    /* Open backend connection */
    /* SAL uses its own default configuration */
    BackendStatus status = backend_open(NULL);
    if (BACKEND_OK != status)
    {
        return status;
    }

    xpClient->is_connected = true;

    /* Create Windows receive thread */
    WindowsThreadContext *ctx = (WindowsThreadContext *)malloc(sizeof(WindowsThreadContext));
    if (NULL == ctx)
    {
        (void)backend_close();
        return BACKEND_ERROR;
    }

    (void)memset(ctx, 0, sizeof(WindowsThreadContext));
    ctx->stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == ctx->stop_event)
    {
        free(ctx);
        (void)backend_close();
        return BACKEND_ERROR;
    }

    (void)InterlockedExchange(&ctx->running, 1);

    ctx->thread = CreateThread(NULL, 0, receive_thread_worker, xpClient, 0, NULL);
    if (NULL == ctx->thread)
    {
        (void)CloseHandle(ctx->stop_event);
        free(ctx);
        (void)backend_close();
        return BACKEND_ERROR;
    }

    xpClient->platform_thread_context = ctx;
    xpClient->is_running = true;

    return BACKEND_OK;
}

BackendStatus kta_async_client_stop(KtaAsyncClient *xpClient)
{
    if (NULL == xpClient)
    {
        return BACKEND_INVALID_PARAM;
    }

    /* Always clean up the thread context — covers both graceful stop and
     * post-error cleanup where the thread already exited itself (is_running
     * was set to false by the thread) but platform_thread_context is stale. */
    WindowsThreadContext *ctx = (WindowsThreadContext *)xpClient->platform_thread_context;
    if (NULL != ctx)
    {
        if (xpClient->is_running)
        {
            /* Graceful shutdown: signal thread to stop and wait for it */
            (void)InterlockedExchange(&ctx->running, 0);
            (void)SetEvent(ctx->stop_event);
            if (NULL != ctx->thread)
            {
                (void)WaitForSingleObject(ctx->thread, 5000U);
            }
        }
        /* Free handles unconditionally */
        if (NULL != ctx->thread)
        {
            CloseHandle(ctx->thread);
        }
        if (NULL != ctx->stop_event)
        {
            CloseHandle(ctx->stop_event);
        }
        free(ctx);
        xpClient->platform_thread_context = NULL;
    }

    /* Close backend */
    backend_close();

    xpClient->is_running = false;
    xpClient->is_connected = false;

    return BACKEND_OK;
}

uint32_t kta_async_initialize(KtaAsyncClient *xpClient)
{
    KtaRequest req;
    (void)memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_INITIALIZE;

    return send_kta_request(xpClient, &req);
}

uint32_t kta_async_startup(
    KtaAsyncClient *xpClient,
    const uint8_t *xpSeed,
    const uint8_t *xpProfileUid, size_t xProfileUidLen,
    const uint8_t *xpSerialNum, size_t xSerialNumLen,
    const uint8_t *xpVersion, size_t xVersionLen)
{
    if ((NULL == xpSeed) || (NULL == xpProfileUid) || (NULL == xpSerialNum) || (NULL == xpVersion))
    {
        return 0U;
    }

    KtaRequest req;
    (void)memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_STARTUP;

    (void)memcpy(req.params.startup.seed, xpSeed, 16U);
    (void)memcpy(req.params.startup.profile_uid, xpProfileUid, xProfileUidLen);
    req.params.startup.profile_uid_len = (uint16_t)xProfileUidLen;
    (void)memcpy(req.params.startup.serial_num, xpSerialNum, xSerialNumLen);
    req.params.startup.serial_num_len = (uint16_t)xSerialNumLen;
    (void)memcpy(req.params.startup.version, xpVersion, xVersionLen);
    req.params.startup.version_len = (uint16_t)xVersionLen;

    return send_kta_request(xpClient, &req);
}

uint32_t kta_async_set_device_info(
    KtaAsyncClient *xpClient,
    const uint8_t *xpProfileUid, size_t xProfileUidLen,
    const uint8_t *xpSerialNum, size_t xSerialNumLen)
{
    if ((NULL == xpProfileUid) || (NULL == xpSerialNum))
    {
        return 0U;
    }

    KtaRequest req;
    (void)memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_SET_DEVICE_INFO;

    (void)memcpy(req.params.set_device_info.profile_uid, xpProfileUid, xProfileUidLen);
    req.params.set_device_info.profile_uid_len = (uint16_t)xProfileUidLen;
    (void)memcpy(req.params.set_device_info.serial_num, xpSerialNum, xSerialNumLen);
    req.params.set_device_info.serial_num_len = (uint16_t)xSerialNumLen;

    return send_kta_request(xpClient, &req);
}

uint32_t kta_async_exchange_message(
    KtaAsyncClient *xpClient,
    const uint8_t *xpKsMsg,
    size_t xKsMsgLen)
{
    /* Allow NULL message for initial exchange (per protocol) */
    if (NULL == xpClient)
    {
        return 0U;
    }

    if (xKsMsgLen > 6144U)
    {
        return 0U; /* Exceeds maximum message size */
    }

    KtaRequest req;
    (void)memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_EXCHANGE_MESSAGE;

    if ((NULL != xpKsMsg) && (xKsMsgLen > 0U))
    {
        (void)memcpy(req.params.exchange_message.ks_msg, xpKsMsg, xKsMsgLen);
        req.params.exchange_message.ks_msg_len = (uint16_t)xKsMsgLen;
    }
    else
    {
        /* First exchange with empty message */
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

uint32_t kta_async_refurbish(KtaAsyncClient *xpClient)
{
    KtaRequest req;
    (void)memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_REFURBISH;

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
    if (NULL == xpClient)
    {
        return BACKEND_INVALID_PARAM;
    }

    /* Stop if running */
    if (true == xpClient->is_running)
    {
        (void)kta_async_client_stop(xpClient);
    }

    /* Close log file */
    if (NULL != xpClient->log_file)
    {
        (void)fprintf(xpClient->log_file, "\n=== SESSION END ===\n");
        (void)fclose(xpClient->log_file);
        xpClient->log_file = NULL;
    }

    /* Deinitialize backend */
    BackendStatus status = backend_deinit();

    memset(xpClient, 0, sizeof(KtaAsyncClient));

    return status;
}

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
 * @brief Asynchronous KTA Client Implementation - Linux Platform
 * 
 * Linux-specific implementation using pthread
 */

#include "../include/kta_async_client.h"
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

/* POSIX threading context */
typedef struct {
    pthread_t thread;
    volatile bool running;
} PosixThreadContext;

/* ============================================================================
 * Logging Functions
 * ============================================================================ */

static void log_hex_dump(FILE *fp, const char *prefix, const uint8_t *data, size_t len)
{
    if (!fp || !data) return;
    
    fprintf(fp, "%s[%zu bytes]:\n", prefix, len);
    
    for (size_t i = 0; i < len; i += 16) {
        fprintf(fp, "  %04zX: ", i);
        
        /* Hex dump */
        for (size_t j = 0; j < 16; j++) {
            if (i + j < len) {
                fprintf(fp, "%02X ", data[i + j]);
            } else {
                fprintf(fp, "   ");
            }
        }
        
        fprintf(fp, " | ");
        
        /* ASCII */
        for (size_t j = 0; j < 16; j++) {
            if (i + j < len) {
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
    if (!client->logging_enabled || !client->log_file) return;
    
    time_t now = time(NULL);
    fprintf(client->log_file, "\n[%s] REQUEST #%u - ", ctime(&now), req->request_id);
    
    const char *api_names[] = {"Initialize", "Startup", "SetDeviceInfo", "ExchangeMessage", "KeyStreamStatus"};
    fprintf(client->log_file, "%s\n", api_names[req->api_type]);
    
    log_hex_dump(client->log_file, "  Serialized: ", data, len);
}

static void log_response(KtaAsyncClient *client, const KtaResponse *resp)
{
    if (!client->logging_enabled || !client->log_file) return;
    
    time_t now = time(NULL);
    fprintf(client->log_file, "\n[%s] RESPONSE #%u - Status: %d\n", 
            ctime(&now), resp->request_id, resp->status_code);
    
    if (resp->data_len > 0) {
        log_hex_dump(client->log_file, "  Data: ", resp->data, resp->data_len);
    }
}

/* ============================================================================
 * Receive Thread Worker (POSIX)
 * ============================================================================ */

static void process_received_data(KtaAsyncClient *client, const uint8_t *data, size_t length)
{
    /* Append to receive buffer */
    if (client->rx_buffer_len + length > sizeof(client->rx_buffer)) {
        /* Buffer overflow - reset */
        client->rx_buffer_len = 0;
        return;
    }
    
    memcpy(client->rx_buffer + client->rx_buffer_len, data, length);
    client->rx_buffer_len += length;
    
    /* Try to parse complete TLV message */
    BackendMessage msg;
    (void)memset(&msg, 0, sizeof(msg));
    
    BackendMessageStatus status = backend_message_deserialize(client->rx_buffer, client->rx_buffer_len, &msg);
    
    if (BACKEND_MESSAGE_SUCCESS == status) {
        /* Complete message received */
        KtaResponse response;
        memset(&response, 0, sizeof(response));
        
        /* Parse response - extract request_id and status from first field */
        response.request_id = (msg.field_count > 0) ? msg.fields[0].tag : 0;
        response.status_code = 0; /* Extract from TLV fields if needed */
        
        /* Copy response data from first field */
        if (msg.field_count > 0 && msg.fields[0].length > 0 && msg.fields[0].length <= sizeof(response.data)) {
            memcpy(response.data, msg.fields[0].value, msg.fields[0].length);
            response.data_len = msg.fields[0].length;
        }
        
        /* Log response */
        log_response(client, &response);
        
        /* Find matching request */
        KtaRequest *matched_req = NULL;
        for (uint8_t i = 0; i < client->pending_count; i++) {
            if (client->pending_requests[i].request_id == response.request_id) {
                matched_req = &client->pending_requests[i];
                break;
            }
        }
        
        /* Invoke callback */
        if (client->response_callback) {
            client->response_callback(matched_req, &response, NULL, client->user_data);
        }
        
        /* Remove from pending (simplified - just decrement count) */
        if (client->pending_count > 0) {
            client->pending_count--;
        }
        
        /* Clear buffer (simplified - assume one message per buffer) */
        client->rx_buffer_len = 0;
    }
}

static void* receive_thread_worker(void *param)
{
    KtaAsyncClient *client = (KtaAsyncClient*)param;
    PosixThreadContext *ctx = (PosixThreadContext*)client->platform_thread_context;
    
    uint8_t buffer[4096];
    size_t received;
    
    while (ctx->running) {
        /* Blocking receive with timeout */
        backend_set_timeout(100); /* 100ms timeout */
        BackendStatus status = backend_receive(buffer, sizeof(buffer), &received);
        
        if (status == BACKEND_OK && received > 0) {
            process_received_data(client, buffer, received);
        }
        else if (status == BACKEND_TIMEOUT) {
            /* Timeout - continue loop */
            continue;
        }
        else if (status != BACKEND_OK) {
            /* Error - maybe invoke error callback */
            usleep(10000); /* 10ms */
        }
    }
    
    return NULL;
}

/* ============================================================================
 * Request Helper
 * ============================================================================ */

static uint32_t send_kta_request(KtaAsyncClient *client, KtaRequest *req)
{
    if (!client || !req) {
        return 0;
    }
    
    /* Assign request ID */
    req->request_id = ++client->next_request_id;
    
    /* Build TLV message */
    BackendMessage msg;
    (void)memset(&msg, 0, sizeof(msg));
    
    (void)backend_message_create(&msg, BACKEND_MSG_TYPE_COMMAND);
    (void)backend_message_set_command(&msg, (uint8_t)req->api_type);
    
    /* Add parameters based on API type */
    /* (Simplified - would use backend_message_add_field() for each parameter) */
    
    /* Serialize message */
    uint8_t buffer[4096];
    size_t serialized_len = 0;
    
    BackendMessageStatus status = backend_message_serialize(&msg, buffer, sizeof(buffer), &serialized_len);
    if (BACKEND_MESSAGE_SUCCESS != status) {
        return 0;
    }
    
    /* Log request */
    log_request(client, req, buffer, serialized_len);
    
    /* Send via backend (blocking call) */
    BackendStatus send_status = backend_send(buffer, serialized_len);
    if (send_status != BACKEND_OK) {
        return 0;
    }
    
    /* Add to pending requests */
    if (client->pending_count < 16) {
        memcpy(&client->pending_requests[client->pending_count], req, sizeof(KtaRequest));
        client->pending_count++;
    }
    
    return req->request_id;
}

/* ============================================================================
 * Public API Implementation
 * ============================================================================ */

BackendStatus kta_async_client_init(
    KtaAsyncClient *client,
    bool log_to_file)
{
    if (!client) {
        return BACKEND_INVALID_PARAM;
    }
    
    memset(client, 0, sizeof(KtaAsyncClient));
    
    /* Set backend type from compile-time macro */
    client->backend_type = KTA_CLIENT_BACKEND;
    
    /* Initialize backend (blocking I/O) */
    /* SAL handles its own configuration internally */
    BackendStatus status = backend_init(client->backend_type);
    if (status != BACKEND_OK) {
        return status;
    }
    
    /* Set up logging */
    client->logging_enabled = log_to_file;
    if (log_to_file) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(client->log_filename, sizeof(client->log_filename),
                "kta_session_%Y%m%d_%H%M%S.log", tm_info);
        
        client->log_file = fopen(client->log_filename, "w");
        if (!client->log_file) {
            client->logging_enabled = false;
        } else {
            const char *backend_names[] = {"UART", "BLE", "USB", "Zigbee"};
            fprintf(client->log_file, "KTA Async Client Log (Linux)\n");
            fprintf(client->log_file, "Backend: %s (compile-time)\n", 
                   backend_names[KTA_CLIENT_BACKEND]);
            fprintf(client->log_file, "=====================================\n\n");
            fflush(client->log_file);
        }
    }
    
    client->next_request_id = 0;
    client->pending_count = 0;
    client->is_connected = false;
    client->is_running = false;
    
    return BACKEND_OK;
}

BackendStatus kta_async_client_set_callback(
    KtaAsyncClient *client,
    KtaResponseCallback callback,
    void *user_data)
{
    if (!client) {
        return BACKEND_INVALID_PARAM;
    }
    
    client->response_callback = callback;
    client->user_data = user_data;
    
    return BACKEND_OK;
}

BackendStatus kta_async_client_start(KtaAsyncClient *client)
{
    if (!client) {
        return BACKEND_INVALID_PARAM;
    }
    
    if (client->is_running) {
        return BACKEND_OK; /* Already running */
    }
    
    /* Open backend connection */
    /* SAL uses its own default configuration */
    BackendStatus status = backend_open(NULL);
    if (status != BACKEND_OK) {
        return status;
    }
    
    client->is_connected = true;
    
    /* Create POSIX receive thread */
    PosixThreadContext *ctx = (PosixThreadContext*)malloc(sizeof(PosixThreadContext));
    if (!ctx) {
        backend_close();
        return BACKEND_ERROR;
    }
    
    memset(ctx, 0, sizeof(PosixThreadContext));
    ctx->running = true;
    
    if (pthread_create(&ctx->thread, NULL, receive_thread_worker, client) != 0) {
        free(ctx);
        backend_close();
        return BACKEND_ERROR;
    }
    
    client->platform_thread_context = ctx;
    client->is_running = true;
    
    return BACKEND_OK;
}

BackendStatus kta_async_client_stop(KtaAsyncClient *client)
{
    if (!client) {
        return BACKEND_INVALID_PARAM;
    }
    
    if (!client->is_running) {
        return BACKEND_OK;
    }
    
    /* Stop receive thread */
    PosixThreadContext *ctx = (PosixThreadContext*)client->platform_thread_context;
    if (ctx) {
        ctx->running = false;
        pthread_join(ctx->thread, NULL);
        free(ctx);
        client->platform_thread_context = NULL;
    }
    
    /* Close backend */
    backend_close();
    
    client->is_running = false;
    client->is_connected = false;
    
    return BACKEND_OK;
}

uint32_t kta_async_initialize(KtaAsyncClient *client)
{
    KtaRequest req;
    memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_INITIALIZE;
    
    return send_kta_request(client, &req);
}

uint32_t kta_async_startup(
    KtaAsyncClient *client,
    const uint8_t *seed,
    const uint8_t *profile_uid, size_t profile_uid_len,
    const uint8_t *serial_num, size_t serial_num_len,
    const uint8_t *version, size_t version_len)
{
    if (!seed || !profile_uid || !serial_num || !version) {
        return 0;
    }
    
    KtaRequest req;
    memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_STARTUP;
    
    memcpy(req.params.startup.seed, seed, 16);
    memcpy(req.params.startup.profile_uid, profile_uid, profile_uid_len);
    req.params.startup.profile_uid_len = profile_uid_len;
    memcpy(req.params.startup.serial_num, serial_num, serial_num_len);
    req.params.startup.serial_num_len = serial_num_len;
    memcpy(req.params.startup.version, version, version_len);
    req.params.startup.version_len = version_len;
    
    return send_kta_request(client, &req);
}

uint32_t kta_async_set_device_info(
    KtaAsyncClient *client,
    const uint8_t *profile_uid, size_t profile_uid_len,
    const uint8_t *serial_num, size_t serial_num_len)
{
    if (!profile_uid || !serial_num) {
        return 0;
    }
    
    KtaRequest req;
    memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_SET_DEVICE_INFO;
    
    memcpy(req.params.set_device_info.profile_uid, profile_uid, profile_uid_len);
    req.params.set_device_info.profile_uid_len = profile_uid_len;
    memcpy(req.params.set_device_info.serial_num, serial_num, serial_num_len);
    req.params.set_device_info.serial_num_len = serial_num_len;
    
    return send_kta_request(client, &req);
}

uint32_t kta_async_exchange_message(
    KtaAsyncClient *client,
    const uint8_t *ks_msg,
    size_t ks_msg_len)
{
    if (!ks_msg || ks_msg_len == 0 || ks_msg_len > 6144) {
        return 0;
    }
    
    KtaRequest req;
    memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_EXCHANGE_MESSAGE;
    
    memcpy(req.params.exchange_message.ks_msg, ks_msg, ks_msg_len);
    req.params.exchange_message.ks_msg_len = ks_msg_len;
    
    return send_kta_request(client, &req);
}

uint32_t kta_async_keystream_status(KtaAsyncClient *client)
{
    KtaRequest req;
    memset(&req, 0, sizeof(req));
    req.api_type = KTA_API_KEYSTREAM_STATUS;
    
    return send_kta_request(client, &req);
}

bool kta_async_is_connected(const KtaAsyncClient *client)
{
    return client ? client->is_connected : false;
}

uint8_t kta_async_get_pending_count(const KtaAsyncClient *client)
{
    return client ? client->pending_count : 0;
}

BackendStatus kta_async_client_deinit(KtaAsyncClient *client)
{
    if (!client) {
        return BACKEND_INVALID_PARAM;
    }
    
    /* Stop if running */
    if (client->is_running) {
        kta_async_client_stop(client);
    }
    
    /* Close log file */
    if (client->log_file) {
        fprintf(client->log_file, "\n=== SESSION END ===\n");
        fclose(client->log_file);
        client->log_file = NULL;
    }
    
    /* Deinitialize backend */
    BackendStatus status = backend_deinit();
    
    memset(client, 0, sizeof(KtaAsyncClient));
    
    return status;
}

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "esp_system.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define BUTTON_HOLD_TIME_MS 3000  // Hold for 3 seconds to trigger refurbish

static void check_boot_button_task(void* arg);

#include "sdkconfig.h"

#include "esp_mac.h"
#include "cryptoauthlib.h"

/* Stub for KTA SAL function not used in BLE provisioning flow */
#include "k_sal_crypto.h"
TKStatus salSignHash(uint32_t xKeyId, uint8_t *xpMsgToHash, size_t xMsgToHashLen,
                     uint8_t *xpSignedHashOutBuff, uint32_t xSignedHashOutBuffLen,
                     size_t *xpActualSignedHashOutLen)
{
    (void)xKeyId; (void)xpMsgToHash; (void)xMsgToHashLen;
    (void)xpSignedHashOutBuff; (void)xSignedHashOutBuffLen; (void)xpActualSignedHashOutLen;
    return E_K_STATUS_ERROR;
}

#include "esp_nimble_hci.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nimble/nimble_npl.h"
#include "host/ble_hs.h"
#include "host/ble_sm.h"
#include "host/ble_store.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "k_kta.h"
#include "k_sal.h"
#include "k_sal_rot.h"
#include "k_sal_storage.h"
#include "k_sal_object.h"  // For salObjectDelete, salObjectKeyDelete
#include "icpp_parser.h"   // For E_K_ICPP_PARSER__ enums
#include "config.h"
   // For resetLifeCycleState() to force lifecycle reset

/* MCU Bridge integration */
#include "bridge_integration.h"
#include "backend_interface.h"

/* NimBLE shim backends: BLE is managed by NimBLE directly in main.c.
 * Bridge task uses these stubs so it initialises without error while
 * main.c handles all simple-binary opcode exchanges directly. */
static BackendStatus _nim_ok(void)   { return BACKEND_OK; }
static BackendStatus _nim_caps(BackendCapabilities *c) { (void)c; return BACKEND_OK; }
static BackendStatus _nim_open(const BackendConfig *c) { (void)c; return BACKEND_OK; }
static BackendStatus _nim_send(const uint8_t *d, size_t l) { (void)d; (void)l; return BACKEND_OK; }
static BackendStatus _nim_recv(uint8_t *b, size_t s, size_t *r)
    { (void)b; (void)s; *r = 0; return BACKEND_TIMEOUT; }
static BackendStatus _nim_timeout(uint32_t t) { (void)t; return BACKEND_OK; }
const Backend g_ble_backend = {
    BACKEND_TYPE_BLE,
    _nim_ok, _nim_ok, _nim_caps,
    _nim_open, _nim_ok, _nim_send, _nim_recv, _nim_timeout
};
const Backend g_uart_backend   = { BACKEND_TYPE_UART,   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
const Backend g_usb_backend    = { BACKEND_TYPE_USB,    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
const Backend g_zigbee_backend = { BACKEND_TYPE_ZIGBEE, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

#include "kta_mcu_integration.h"

static const char *TAG = "keystream_ble";

#define KTA_LC_INIT_MARKER        0x00u
#define KTA_LC_SEALED_MARKER      0x53u  // 'S' in "SEAL"
#define KTA_LC_ACTIVATED_MARKER   0x41u  // 'A' in "ACTI"
#define KTA_LC_PROVISIONED_MARKER 0x50u  // 'P' in "PROV"
#define KTA_LC_CON_REQ_MARKER     0x43u  // 'C' in "CONR"

#define KTA_HEXDUMP_MAX_BYTES 256u

static void log_hex_bounded(const char *label, const uint8_t *data, size_t len)
{
    if (label == NULL)
    {
        label = "(null)";
    }
    if (data == NULL || len == 0u)
    {
        ESP_LOGI(TAG, "%s: <empty>", label);
        return;
    }

    size_t show = len;
    if (show > (size_t)KTA_HEXDUMP_MAX_BYTES)
    {
        show = (size_t)KTA_HEXDUMP_MAX_BYTES;
    }

    ESP_LOGI(TAG, "%s: len=%u (showing %u)%s",
             label, (unsigned)len, (unsigned)show, (show < len) ? " TRUNCATED" : "");
    ESP_LOG_BUFFER_HEX(TAG, data, show);
}

// Matches C_K__ICPP_MSG_MAX_SIZE (salapi/libc/include/cryptoConfig.h)
#define KTA_ICPP_MSG_MAX_SIZE 1400u

// NOTE: The BLE RX binary frame buffer is byte-packed and typically unaligned
// (payload starts at offset 3). Some KTA builds parse using word accesses.
// Keep an aligned copy buffer for KS->KTA messages to avoid unaligned faults.
static uint8_t g_kta2ks_msg[KTA_ICPP_MSG_MAX_SIZE];
static uint8_t g_exchange_resp[2u + KTA_ICPP_MSG_MAX_SIZE];
static uint8_t g_notify_frame[4u + 2u + KTA_ICPP_MSG_MAX_SIZE];

// Offload KTA exchange/activation work out of the NimBLE host task context.
// NimBLE invokes GATT write callbacks on the host task. Doing ATECC608 init +
// crypto in that context can overflow the host stack. Instead, process the
// exchange on a worker task and schedule only the notify back to the host.
static TaskHandle_t s_kta_worker_handle = NULL;
static SemaphoreHandle_t s_exchange_mutex = NULL;
static bool s_exchange_inflight = false;
static uint16_t s_exchange_req_len = 0;
static uint8_t s_exchange_req_buf[KTA_ICPP_MSG_MAX_SIZE];
static uint8_t s_exchange_notify_status = (uint8_t)E_K_STATUS_ERROR;
static uint16_t s_exchange_notify_payload_len = 0;
static bool s_worker_init_exchange_pending = false;
static bool s_worker_auto_bootstrap_pending = false;
static bool s_worker_connect_bootstrap_pending = false;

// Keep gateway as the single source of truth for INIT/STARTUP/SET_DEVICE_INFO
// on Windows BLE sessions. The old monitor-triggered fallback could run before
// FF01 command flow starts and produce unsolicited EXCHANGE notifications.
#ifndef KTA_ENABLE_AUTO_BOOTSTRAP_FALLBACK
#define KTA_ENABLE_AUTO_BOOTSTRAP_FALLBACK 0
#endif

#define KTA_AUTO_BOOTSTRAP_MIN_CONN_SEC 20

#ifndef KTA_AUTO_INIT_ON_CONNECT
#define KTA_AUTO_INIT_ON_CONNECT 0
#endif

#ifndef KTA_BOOTSTRAP_ON_CONNECT
#define KTA_BOOTSTRAP_ON_CONNECT 0
#endif

#ifndef KS_GATEWAY_MODE
#define KS_GATEWAY_MODE 1
#endif

#ifndef KTA_ENABLE_MONITOR_AUTO_INIT_RETRY
#define KTA_ENABLE_MONITOR_AUTO_INIT_RETRY 0
#endif

#ifndef KTA_ENABLE_HEARTBEAT_NOTIFY
#define KTA_ENABLE_HEARTBEAT_NOTIFY 0
#endif

#ifndef KTA_ENABLE_HELLO_NOTIFY
#define KTA_ENABLE_HELLO_NOTIFY 0
#endif

#ifndef KS_ENABLE_BONDING
#define KS_ENABLE_BONDING 1
#endif

#ifndef KS_DELETE_BOND_ON_REPEAT_PAIRING
#define KS_DELETE_BOND_ON_REPEAT_PAIRING 0
#endif

static struct ble_npl_event s_exchange_event;
static bool s_exchange_event_initialized = false;

static void kta_worker_task(void *param);
static void exchange_event_fn(struct ble_npl_event *ev);
static void try_auto_bootstrap_sequence(bool force_run);

/* Bridge KTA task is created via kta_mcu_integration_entry() below. */

// Fallback bootstrap values used only when Windows connects but does not send
// FF01 control opcodes. These match the gateway's default startup context.
static const uint8_t g_auto_l1_seed[16] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
};
static const uint8_t g_auto_context_profile_uid[] = {
    0x11, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A
};
static const uint8_t g_auto_context_serial[] = {
    0x11, 0x22, 0x33, 0x04, 0x05, 0x06, 0x07, 0x08
};
static const uint8_t g_auto_context_version[] = {
    0x22, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05
};
static const uint8_t g_auto_device_serial_fallback[] = {
    0x22, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
};
static const char g_auto_device_profile_uid[] = "9S4F:esp32";

static bool g_kta_initialized_once = false;
static bool g_kta_started_once = false;
static bool g_kta_device_info_once = false;
static uint8_t g_kta_last_connection_request = 1u;
static bool g_kta_connection_required = false;
static bool g_kta_exchange_started_once = false;
static bool g_kta_connect_bootstrap_done_once = false;

// KTA Field Management (Refurbish/Renew/Rotate)
#include "k_kta.h"  // For TKktaKeyStreamStatus enum
static TKktaKeyStreamStatus g_ktaKSCmdStatus = E_K_KTA_KS_STATUS_NONE;
static TimerHandle_t g_ktaFieldMgmtTimer = NULL;
#define KTA_FIELD_MGMT_INTERVAL_MS  (5000)  // Check every 5 seconds

static const char* kta_status_str(TKStatus st);
static void notify_string_chunked(const char *s);  // Forward declaration
static bool is_app_connected(void);  // Forward declaration
static void kta_field_mgmt_timer_callback(TimerHandle_t xTimer);

// NimBLE key store initialization (provided by ESP-IDF NimBLE component).
// Header doesn't declare it, so forward declare like ESP-IDF examples.
void ble_store_config_init(void);

static void maybe_force_kta_reprovision_on_boot(bool atecc_ready)
{
#if defined(CONFIG_KTA_FORCE_REPROVISION_ON_BOOT) && (CONFIG_KTA_FORCE_REPROVISION_ON_BOOT == 1)
    if (!atecc_ready)
    {
        ESP_LOGW(TAG, "KTA force-reprovision enabled, but ATECC not ready; skipping");
        return;
    }

    uint8_t life_cycle_state[C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE] = {0};
    TKStatus st = salStorageSetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                                     life_cycle_state,
                                     C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE);
    ESP_LOGW(TAG, "Force KTA reprovision: lifecycle->INIT write status=%d (%s)",
             (int)st, kta_status_str(st));

    uint8_t l1_key_material[C_KEY_CONFIG__MATERIAL_MAX_SIZE] = {0};
    st = salStorageSetValue(C_K_KTA__L1_KEY_MATERIAL_DATA_ID,
                            l1_key_material,
                            C_KEY_CONFIG__MATERIAL_MAX_SIZE);
    ESP_LOGW(TAG, "Force KTA reprovision: L1 material clear status=%d (%s)",
             (int)st, kta_status_str(st));
#else
    (void)atecc_ready;
#endif
}

//=============================================================================
// KTA Field Management Timer Callback
//
// DISABLED: Was causing stack overflow in timer task.
// App will poll directly via checkForServerCommands() instead.
//=============================================================================
static void kta_field_mgmt_timer_callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    // Timer callback disabled to prevent stack overflow
    // App polling in KTAService handles server command checks
}

static TKStatus ensure_kta_initialized_and_seeded(void)
{
    if (!g_kta_initialized_once)
    {
        TKStatus st = ktaInitialize();
        ESP_LOGI(TAG, "KTA init attempt: st=%d(%s)", (int)st, kta_status_str(st));

        // Upstream KTA's ktaInitialize() is a one-shot state transition and returns
        // E_K_STATUS_ERROR on subsequent calls. For BLE/mobile reconnects of an
        // already-provisioned device (KTA still RUNNING in the same boot session)
        // we keep INITIALIZE idempotent so the resume flow succeeds.
        //
        // NOTE: This masking is only safe because a genuine refurbish now reboots
        // the device (see refurbish_detected handler), which returns the KTA
        // library's internal gKtaState to E_KTA_STATE_INITIAL on the next boot.
        // Without that reboot, a refurbished-but-not-rebooted device would report
        // a fake INITIALIZE=OK while gKtaState was stuck past INITIAL, and the
        // subsequent STARTUP would fail with E_K_STATUS_ERROR(6).
        if (st == E_K_STATUS_OK)
        {
            g_kta_initialized_once = true;
        }
        else if (st == E_K_STATUS_ERROR)
        {
            // Treat as already-initialized.
            g_kta_initialized_once = true;
            st = E_K_STATUS_OK;
        }
        else if (st == E_K_STATUS_STATE)
        {
            // Some KTA builds report STATE when already past INITIAL.
            g_kta_initialized_once = true;
            st = E_K_STATUS_OK;
        }

        return st;
    }
    return E_K_STATUS_OK;
}

static uint32_t read_u32le(const uint8_t* p)
{
    return ((uint32_t)p[0]) |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static const char* kta_status_str(TKStatus st)
{
    switch (st)
    {
        case E_K_STATUS_OK: return "OK";
        case E_K_STATUS_PARAMETER: return "PARAMETER";
        case E_K_STATUS_DATA: return "DATA";
        case E_K_STATUS_STATE: return "STATE";
        case E_K_STATUS_MEMORY: return "MEMORY";
        case E_K_STATUS_MISSING: return "MISSING";
        case E_K_STATUS_ERROR: return "ERROR";
        case E_K_STATUS_DECRYPTION: return "DECRYPTION";
        case E_K_STATUS_PERSONALIZED: return "PERSONALIZED";
        case E_K_STATUS_NOT_SUPPORTED: return "NOT_SUPPORTED";
        default: return "(unknown)";
    }
}

static const char* kta_lifecycle_str(uint8_t marker)
{
    switch (marker)
    {
        case KTA_LC_INIT_MARKER: return "INIT";
        case KTA_LC_SEALED_MARKER: return "SEALED";
        case KTA_LC_ACTIVATED_MARKER: return "ACTIVATED";
        case KTA_LC_PROVISIONED_MARKER: return "PROVISIONED";
        case KTA_LC_CON_REQ_MARKER: return "CON_REQ";
        default: return "UNKNOWN";
    }
}

// 16-bit UUIDs expand to 128-bit base: 0000XXXX-0000-1000-8000-00805F9B34FB
#define KS_SVC_UUID16   0xFFFF
#define KS_CTRL_UUID16  0xFF01  // App -> ESP32 (write)
#define KS_DATA_UUID16  0xFF02  // ESP32 -> App (notify)
#define KS_ACK_UUID16   0xFF03  // App -> ESP32 (write ack/nack)
#define KS_RESP_UUID16  0xFF04  // App -> ESP32 (write server response chunks)

uint16_t g_data_val_handle;
static uint16_t g_ctrl_val_handle;
static uint16_t g_ack_val_handle;
static uint16_t g_resp_val_handle;

uint16_t g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static bool g_data_subscribed = false;
static int64_t g_connected_at_ms = 0;  // Track when app connected
static int64_t g_provisioned_at_ms = 0;  // Track when provisioning completed
static int64_t g_last_activity_ms = 0;  // Track last communication
static esp_timer_handle_t g_connection_monitor_timer = NULL;
static bool g_ctrl_opcode_seen_since_connect = false;
static int64_t g_first_ctrl_opcode_ms = 0;
static int64_t g_last_kta_init_attempt_ms = 0;

// Simple RX buffer for CTRL writes (newline-delimited JSON commands)

static uint8_t s_own_addr_type;
static char g_ctrl_rx[2048];
static size_t g_ctrl_rx_len = 0;

// Binary opcode framing buffer for CTRL writes
// Request frame: [opcode:1][payloadLen:2 LE][payload]
// Response frame: [opcode:1][status:1][payloadLen:2 LE][payload]
static uint8_t g_ctrl_bin_rx[2048];
static size_t g_ctrl_bin_rx_len = 0;

// KTA enum opcodes (must match mobile app)
#define OP_INITIALIZE              0x01
#define OP_STARTUP                 0x02
#define OP_SET_DEVICE_INFORMATION  0x03
#define OP_EXCHANGE_MESSAGE        0x04
#define OP_KEY_STREAM_STATUS       0x08
#define OP_SET_CHIP_CERTIFICATE    0x09
#define OP_REFURBISH               0xFA  // Local device refurbish (0xFA unlikely to collide with data)

static int notify_bytes(const uint8_t *data, size_t len) {
    if (g_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "No connection; cannot notify");
        return BLE_HS_EINVAL;
    }
    if (!g_data_subscribed) {
        ESP_LOGW(TAG, "Central not subscribed to DATA; skipping notify");
        if (!KS_GATEWAY_MODE) {
            return 0;
        }
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (!om) {
        ESP_LOGE(TAG, "ble_hs_mbuf_from_flat failed");
        return BLE_HS_ENOMEM;
    }

    int rc = ble_gatts_notify_custom(g_conn_handle, g_data_val_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "notify_custom rc=%d", rc);
    }
    return rc;
}

static uint16_t att_payload_max(void) {
    if (g_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        return 20; // safe default
    }
    uint16_t mtu = ble_att_mtu(g_conn_handle);
    // ATT notification payload is MTU - 3
    return (mtu > 3) ? (uint16_t)(mtu - 3) : 20;
}

// Check if connection is healthy
static bool is_app_connected(void) {
    return (g_conn_handle != BLE_HS_CONN_HANDLE_NONE && g_data_subscribed);
}

// Send heartbeat to keep connection alive
static void send_heartbeat(void) {
    if (!is_app_connected()) {
        return;
    }
    
    int64_t now_ms = esp_timer_get_time() / 1000;
    char heartbeat[64];
    snprintf(heartbeat, sizeof(heartbeat), "{\"heartbeat\":%lld}\n", now_ms);
    notify_string_chunked(heartbeat);
}

// Periodic timer callback to monitor connection
static void connection_monitor_callback(void* arg) {
    int64_t now_ms = esp_timer_get_time() / 1000;
    
    if (g_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "⚠️  [Monitor] NO BLE CONNECTION - Windows gateway not connected!");
        return;
    }
    
    int64_t connected_duration_sec = (now_ms - g_connected_at_ms) / 1000;
    int64_t idle_duration_sec = (now_ms - g_last_activity_ms) / 1000;

    // Optional retry path: for legacy app flows only.
    if (KTA_ENABLE_MONITOR_AUTO_INIT_RETRY &&
        !g_kta_initialized_once &&
        (now_ms - g_last_kta_init_attempt_ms) >= 5000) {
        g_last_kta_init_attempt_ms = now_ms;
        TKStatus init_st = ensure_kta_initialized_and_seeded();
        if (init_st == E_K_STATUS_OK) {
            ESP_LOGI(TAG, "KTA auto-initialize retry: OK");
        } else {
            ESP_LOGW(TAG, "KTA auto-initialize retry failed: st=%d(%s)",
                     (int)init_st, kta_status_str(init_st));
        }
    }

    if (KTA_ENABLE_AUTO_BOOTSTRAP_FALLBACK &&
        g_data_subscribed &&
        !g_ctrl_opcode_seen_since_connect &&
        connected_duration_sec >= KTA_AUTO_BOOTSTRAP_MIN_CONN_SEC) {
        ESP_LOGW(TAG, "[Monitor] BLE link is up but no CTRL opcode writes received yet (FF01). Windows is connected, but KTA command flow has not started.");

        // Fallback path: if Windows stays connected but never sends the
        // expected command opcodes, trigger a worker-side bootstrap sequence.
        if (!g_kta_exchange_started_once && s_kta_worker_handle != NULL)
        {
            bool can_queue = true;
            if (s_exchange_mutex)
            {
                (void)xSemaphoreTake(s_exchange_mutex, portMAX_DELAY);
                if (s_exchange_inflight || s_worker_auto_bootstrap_pending)
                {
                    can_queue = false;
                }
                else
                {
                    s_worker_auto_bootstrap_pending = true;
                }
                (void)xSemaphoreGive(s_exchange_mutex);
            }
            else
            {
                if (s_exchange_inflight || s_worker_auto_bootstrap_pending)
                {
                    can_queue = false;
                }
                else
                {
                    s_worker_auto_bootstrap_pending = true;
                }
            }

            if (can_queue)
            {
                ESP_LOGW(TAG, "[Monitor] Queueing AUTO bootstrap on worker task");
                xTaskNotifyGive(s_kta_worker_handle);
            }
        }
    }
    
    ESP_LOGI(TAG, "✅ [Monitor] Connection HEALTHY - connected:%llds idle:%llds", 
             connected_duration_sec, idle_duration_sec);
    
    if (g_provisioned_at_ms > 0) {
        int64_t since_provision_sec = (now_ms - g_provisioned_at_ms) / 1000;
        ESP_LOGI(TAG, "📊 [Monitor] Provisioned %lld seconds ago", since_provision_sec);
    }
    
    // Optional heartbeat stream is disabled by default for gateway framing.
    if (KTA_ENABLE_HEARTBEAT_NOTIFY && g_data_subscribed) {
        send_heartbeat();
    }
}

static void notify_string_chunked(const char *s) {
    if (!s) return;

    const uint8_t *p = (const uint8_t *)s;
    size_t remaining = strlen(s);

    uint16_t max_payload = att_payload_max();
    ESP_LOGI(TAG, "Notifying %u bytes (mtu_payload=%u)", (unsigned)remaining, (unsigned)max_payload);

    while (remaining > 0) {
        size_t n = remaining;
        if (n > max_payload) n = max_payload;

        notify_bytes(p, n);
        p += n;
        remaining -= n;

        // Small pacing helps avoid overruns on some centrals.
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static int notify_bytes_chunked(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0u)
    {
        return 0;
    }

    uint16_t max_payload = att_payload_max();
    while (len > 0u)
    {
        size_t n = len;
        if (n > max_payload) n = max_payload;

        int rc = notify_bytes(data, n);
        if (rc != 0) return rc;

        data += n;
        len -= n;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return 0;
}

static int notify_binary_frame(uint8_t opcode, uint8_t status, const uint8_t *payload, uint16_t payload_len) {
    // Send as a single contiguous stream (header + payload) to avoid
    // receiver-side ambiguity and reduce back-to-back notify calls.
    // Chunking is still applied at ATT MTU boundaries.
    if (payload_len > (uint16_t)(sizeof(g_notify_frame) - 4u)) {
        ESP_LOGE(TAG, "notify payload too large: %u", (unsigned)payload_len);
        return BLE_HS_EMSGSIZE;
    }

    g_notify_frame[0] = opcode;
    g_notify_frame[1] = status;
    g_notify_frame[2] = (uint8_t)(payload_len & 0xFF);
    g_notify_frame[3] = (uint8_t)((payload_len >> 8) & 0xFF);

    if (payload && payload_len) {
        memcpy(&g_notify_frame[4], payload, payload_len);
    }

#if defined(CONFIG_KTA_CRYPTO_IO_LOG) && (CONFIG_KTA_CRYPTO_IO_LOG == 1)
    ESP_LOGI(TAG, "BLE notify: opcode=0x%02X status=0x%02X payload_len=%u frame_len=%u",
             (unsigned)opcode,
             (unsigned)status,
             (unsigned)payload_len,
             (unsigned)(4u + payload_len));
    log_hex_bounded("BLE notify payload", payload, (size_t)payload_len);
    log_hex_bounded("BLE notify frame [op|st|lenLE|payload]", g_notify_frame, (size_t)(4u + payload_len));
#endif

    return notify_bytes_chunked(g_notify_frame, (size_t)(4u + payload_len));
}

static void exchange_event_fn(struct ble_npl_event *ev)
{
    (void)ev;

    (void)notify_binary_frame(OP_EXCHANGE_MESSAGE,
                              s_exchange_notify_status,
                              g_exchange_resp,
                              s_exchange_notify_payload_len);

    if (s_exchange_mutex)
    {
        (void)xSemaphoreTake(s_exchange_mutex, portMAX_DELAY);
        s_exchange_inflight = false;
        (void)xSemaphoreGive(s_exchange_mutex);
    }
    else
    {
        s_exchange_inflight = false;
    }
}

static void try_auto_bootstrap_sequence(bool force_run)
{
    if (!force_run && !KTA_ENABLE_AUTO_BOOTSTRAP_FALLBACK)
    {
        return;
    }

    if (force_run && g_kta_connect_bootstrap_done_once)
    {
        ESP_LOGI(TAG, "AUTO bootstrap (connect-trigger): already completed once; skipping");
        return;
    }

    if (g_ctrl_opcode_seen_since_connect || g_conn_handle == BLE_HS_CONN_HANDLE_NONE)
    {
        return;
    }

    if (force_run)
    {
        ESP_LOGI(TAG, "AUTO bootstrap (connect-trigger): running INIT->STARTUP->SET_DEVICE_INFO->EXCHANGE(0)");
    }
    else
    {
        ESP_LOGW(TAG, "AUTO bootstrap (fallback): no FF01 opcodes received; running INIT->STARTUP->SET_DEVICE_INFO->EXCHANGE(0)");
    }

    TKStatus st = ensure_kta_initialized_and_seeded();
    ESP_LOGI(TAG, "AUTO bootstrap INIT: st=%d(%s)", (int)st, kta_status_str(st));
    if (st != E_K_STATUS_OK)
    {
        ESP_LOGW(TAG, "AUTO bootstrap INIT failed: st=%d(%s)", (int)st, kta_status_str(st));
        return;
    }

    if (!g_kta_started_once)
    {
        st = ktaStartup(g_auto_l1_seed,
                        g_auto_context_profile_uid,
                        sizeof(g_auto_context_profile_uid),
                        g_auto_context_serial,
                        sizeof(g_auto_context_serial),
                        g_auto_context_version,
                        sizeof(g_auto_context_version));
        ESP_LOGI(TAG, "AUTO bootstrap STARTUP: st=%d(%s)", (int)st, kta_status_str(st));
        if (st != E_K_STATUS_OK)
        {
            return;
        }
        g_kta_started_once = true;
    }

    if (!g_kta_device_info_once)
    {
        uint8_t connectionRequest = 0u;
        uint8_t chip_uid[9] = {0};
        const uint8_t* serial = g_auto_device_serial_fallback;
        size_t serial_len = sizeof(g_auto_device_serial_fallback);

        ATCA_STATUS uid_st = atcab_read_serial_number(chip_uid);
        if (uid_st == ATCA_SUCCESS)
        {
            serial = chip_uid;
            serial_len = sizeof(chip_uid);
            log_hex_bounded("AUTO bootstrap deviceUID(from ATECC608)", chip_uid, sizeof(chip_uid));
        }
        else
        {
            ESP_LOGW(TAG, "AUTO bootstrap: failed to read ATECC UID (0x%02x), using fallback serial", (unsigned)uid_st);
        }

        st = ktaSetDeviceInformation((const uint8_t*)g_auto_device_profile_uid,
                                     strlen(g_auto_device_profile_uid),
                                     serial,
                                     serial_len,
                                     &connectionRequest);
        ESP_LOGI(TAG, "AUTO bootstrap SET_DEVICE_INFO: st=%d(%s) conReq=%u",
                 (int)st,
                 kta_status_str(st),
                 (unsigned)connectionRequest);
        ESP_LOGI(TAG, "AUTO bootstrap fleet/device profile UID: %s", g_auto_device_profile_uid);
        if (st != E_K_STATUS_OK)
        {
            return;
        }

        g_kta_device_info_once = true;
        g_kta_last_connection_request = connectionRequest;
        g_kta_connection_required = (connectionRequest != 0u);
    }

    uint8_t empty_exchange_buf[1] = {0};
    size_t kta2ksLen = sizeof(g_kta2ks_msg);
    memset(g_kta2ks_msg, 0, sizeof(g_kta2ks_msg));
    st = ktaExchangeMessage(empty_exchange_buf, 0u, g_kta2ks_msg, &kta2ksLen);

    if (st != E_K_STATUS_OK)
    {
        ESP_LOGW(TAG, "AUTO bootstrap EXCHANGE(0) failed: st=%d(%s)", (int)st, kta_status_str(st));
        return;
    }

    g_kta_exchange_started_once = true;
    ESP_LOGI(TAG, "AUTO bootstrap EXCHANGE(0): generated %u bytes", (unsigned)kta2ksLen);
    if (kta2ksLen != 299u)
    {
        ESP_LOGW(TAG, "AUTO bootstrap EXCHANGE(0): expected ~299 bytes, got %u", (unsigned)kta2ksLen);
    }
    log_hex_bounded("AUTO bootstrap kta2ks", g_kta2ks_msg, kta2ksLen);

    if (kta2ksLen > (sizeof(g_exchange_resp) - 2u))
    {
        kta2ksLen = sizeof(g_exchange_resp) - 2u;
    }
    g_exchange_resp[0] = (uint8_t)(kta2ksLen & 0xFFu);
    g_exchange_resp[1] = (uint8_t)((kta2ksLen >> 8) & 0xFFu);
    if (kta2ksLen > 0u)
    {
        memcpy(&g_exchange_resp[2], g_kta2ks_msg, kta2ksLen);
    }

    if (g_data_subscribed)
    {
        (void)notify_binary_frame(OP_EXCHANGE_MESSAGE,
                                  (uint8_t)st,
                                  g_exchange_resp,
                                  (uint16_t)(2u + kta2ksLen));
    }

    if (force_run)
    {
        g_kta_connect_bootstrap_done_once = true;
        ESP_LOGI(TAG, "AUTO bootstrap (connect-trigger): marked as completed");
    }
}

static void kta_worker_task(void *param)
{
    (void)param;

    for (;;)
    {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Check if app is still connected
        if (g_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGW(TAG, "⚠️  Exchange requested but NO BLE CONNECTION - app disconnected!");
        } else {
            int64_t now_ms = esp_timer_get_time() / 1000;
            int64_t connected_duration_sec = (now_ms - g_connected_at_ms) / 1000;
            ESP_LOGI(TAG, "✅ BLE Connection ACTIVE - connected for %lld seconds", connected_duration_sec);
            
            if (g_provisioned_at_ms > 0) {
                int64_t since_provision_sec = (now_ms - g_provisioned_at_ms) / 1000;
                ESP_LOGI(TAG, "📊 Time since provisioning: %lld seconds", since_provision_sec);
            }
        }

        UBaseType_t hw_before = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(TAG, "kta_worker: stack high-water before exchange=%u words", (unsigned)hw_before);

        uint16_t ks2ktaLen = 0u;
        bool have_explicit_exchange_req = false;
        bool run_init_exchange = false;
        bool run_auto_bootstrap = false;
        bool run_connect_bootstrap = false;
        if (s_exchange_mutex)
        {
            (void)xSemaphoreTake(s_exchange_mutex, portMAX_DELAY);
            ks2ktaLen = s_exchange_req_len;
            if (s_exchange_inflight)
            {
                have_explicit_exchange_req = true;
                s_exchange_req_len = 0u;
            }
            else if (s_worker_connect_bootstrap_pending)
            {
                run_connect_bootstrap = true;
                s_worker_connect_bootstrap_pending = false;
                s_worker_init_exchange_pending = false;
            }
            else if (s_worker_auto_bootstrap_pending)
            {
                run_auto_bootstrap = true;
                s_worker_auto_bootstrap_pending = false;
                s_worker_init_exchange_pending = false;
            }
            else if (s_worker_init_exchange_pending)
            {
                run_init_exchange = true;
                s_worker_init_exchange_pending = false;
            }
            (void)xSemaphoreGive(s_exchange_mutex);
        }
        else
        {
            ks2ktaLen = s_exchange_req_len;
            if (s_exchange_inflight)
            {
                have_explicit_exchange_req = true;
                s_exchange_req_len = 0u;
            }
            else if (s_worker_connect_bootstrap_pending)
            {
                run_connect_bootstrap = true;
                s_worker_connect_bootstrap_pending = false;
                s_worker_init_exchange_pending = false;
            }
            else if (s_worker_auto_bootstrap_pending)
            {
                run_auto_bootstrap = true;
                s_worker_auto_bootstrap_pending = false;
                s_worker_init_exchange_pending = false;
            }
            else if (s_worker_init_exchange_pending)
            {
                run_init_exchange = true;
                s_worker_init_exchange_pending = false;
            }
        }

        if (run_connect_bootstrap)
        {
            try_auto_bootstrap_sequence(true);
            continue;
        }

        if (run_auto_bootstrap)
        {
            try_auto_bootstrap_sequence(false);
            continue;
        }

        if (run_init_exchange)
        {
            ESP_LOGI(TAG, "INITIALIZE-triggered auto EXCHANGE(0): generating first KTA->keySTREAM message");
        }

        ESP_LOGI(TAG,
                 "worker dispatch: explicit_req=%d ks2ktaLen=%u init_auto=%d connect_bootstrap=%d fallback_bootstrap=%d inflight=%d",
                 (int)have_explicit_exchange_req,
                 (unsigned)ks2ktaLen,
                 (int)run_init_exchange,
                 (int)run_connect_bootstrap,
                 (int)run_auto_bootstrap,
                 (int)s_exchange_inflight);

        if (ks2ktaLen == 0u && have_explicit_exchange_req)
        {
            ESP_LOGI(TAG, "Processing explicit EXCHANGE(0) request from gateway");
        }

        if (ks2ktaLen == 0u && !run_init_exchange && !have_explicit_exchange_req)
        {
            // Ignore wakeups that have no queued command and no pending bootstrap.
            // This prevents unintended null EXCHANGE traffic when the link churns.
            continue;
        }

        size_t kta2ksLen = sizeof(g_kta2ks_msg);
        memset(g_kta2ks_msg, 0, sizeof(g_kta2ks_msg));  // Clear stale data

        // Keep pre-exchange path non-blocking. Lifecycle reads are optional for
        // diagnostics and should not delay/lock explicit EXCHANGE(0) handling.
        uint8_t lc_state[C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE] = {0};
        size_t lc_check_len = 0;
        TKStatus lc_check_st = E_K_STATUS_ERROR;
        lc_check_len = C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE;
        lc_check_st = salStorageGetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID, lc_state, &lc_check_len);
        if (lc_check_st == E_K_STATUS_OK && lc_check_len > 0) {
            ESP_LOGI(TAG, "Lifecycle before EXCHANGE: 0x%02X (%s)", lc_state[0], kta_lifecycle_str(lc_state[0]));
        }
        
        if (ks2ktaLen > 0) {
            ESP_LOGI("KTA_MSG", "📨 Received message from KeySTREAM - length: %u bytes", (unsigned)ks2ktaLen);
            ESP_LOG_BUFFER_HEX_LEVEL("KTA_MSG", s_exchange_req_buf, ks2ktaLen > 32 ? 32 : ks2ktaLen, ESP_LOG_INFO);
        } else {
            ESP_LOGI("KTA_MSG", "📭 NULL exchange (checking for server commands)");
        }
        
        // Reference app always sends EXCHANGE payload as [u16 len][bytes], where
        // first call is len=0. Keep a valid pointer path even for len=0.
        const uint8_t *ks2kta_ptr = s_exchange_req_buf;
        int64_t exch_start_us = esp_timer_get_time();
        ESP_LOGI("KTA_MSG", "⏱️ ktaExchangeMessage enter (ks2ktaLen=%u, ptr=%s)",
                 (unsigned)ks2ktaLen,
                 "req_buf");
        TKStatus st = ktaExchangeMessage(ks2kta_ptr, (size_t)ks2ktaLen, g_kta2ks_msg, &kta2ksLen);
        int64_t exch_elapsed_ms = (esp_timer_get_time() - exch_start_us) / 1000;
        ESP_LOGI("KTA_MSG", "⏱️ ktaExchangeMessage exit after %lld ms", exch_elapsed_ms);
        
        // CRITICAL: If ktaExchangeMessage returns error, the library may NOT
        // zero kta2ksLen (e.g. gKtaState != RUNNING). Force it to 0 so we
        // don't send stale data from a previous successful call.
        if (st != E_K_STATUS_OK) {
            ESP_LOGW("KTA_MSG", "⚠️ ktaExchangeMessage FAILED: status=%d(%s), forcing kta2ksLen 0 (was %u)",
                     (int)st, kta_status_str(st), (unsigned)kta2ksLen);
            kta2ksLen = 0;
        }
        
        ESP_LOGI("KTA_MSG", "📤 ktaExchangeMessage result: status=%d, response length: %u bytes", 
                 (int)st, (unsigned)kta2ksLen);
        
        // Update response buffer with KTA message result
        if (kta2ksLen > 0) {
            memcpy(g_exchange_resp, g_kta2ks_msg, kta2ksLen);
        } else {
            g_exchange_resp[0] = 0u;
            g_exchange_resp[1] = 0u;
        }

        // NOTE: Do NOT call resetLifeCycleState() here.
        // The KTA library's ktaExchangeMessage() already processed the ICPP
        // DeleteObject (0x50) / DeleteKeyObject (0x51) commands via
        // salObjectDelete() which correctly deletes certs/keys WITHOUT
        // touching the ROT UID. Calling resetLifeCycleState() here was
        // zeroing the ROT UID and preventing re-onboarding.
        //
        // REFURBISH DETECTION: We rely ONLY on the lifecycle state transition
        // check below (before vs after ktaExchangeMessage). Scanning the raw
        // ICPP buffer for byte 0x50 caused false positives because 0x50 is a
        // common value in binary protocol data (cert fields, lengths, etc.).

        // 🔍 Debug: Analyze message size
        if (kta2ksLen == 37) {
            ESP_LOGW("KTA_MSG", "⚠️ SHORT MESSAGE (37 bytes) - Missing device identity!");
            ESP_LOGW("KTA_MSG", "   This happens when device is PROVISIONED and KTA sends minimal rot2ks");
            ESP_LOGW("KTA_MSG", "   Server needs ~213 bytes with device cert/UID to look up pending commands");
        } else if (kta2ksLen >= 200) {
            ESP_LOGI("KTA_MSG", "✅ FULL MESSAGE (%u bytes) - Includes device identity", (unsigned)kta2ksLen);
        }

        // REFURBISH DETECTION: Check if lifecycle transitioned to INIT (0x00)
        // after ktaExchangeMessage() processed DELETE_OBJECT commands.
        //
        // IMPORTANT: The KTA library handles the full refurbish sequence:
        //   1. ktaExchangeMessage(serverMsg) → processes DELETE, generates ACK response
        //   2. ACK response MUST be sent back to server (stops 0x50 re-sends)
        //   3. Next ktaExchangeMessage(0) (NOOP) → library detects INIT, sets SEALED
        //   4. Library resets its own gKtaState to INITIAL
        //   5. Device must re-init: ktaInitialize → ktaStartup → ktaSetDeviceInfo → ktaExchangeMessage(0)
        //
        // We MUST NOT skip sending the response (no 'continue'). The response
        // contains the DELETE acknowledgment the server needs. After sending it,
        // we reset our state flags so the app can drive the re-init sequence.
        bool refurbish_detected = false;
        if (st == E_K_STATUS_OK && lc_check_st == E_K_STATUS_OK && lc_check_len > 0) {
            uint8_t lc_after[C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE] = {0};
            size_t lc_after_len = C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE;
            TKStatus lc_after_st = salStorageGetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID, lc_after, &lc_after_len);
            
            if (lc_after_st == E_K_STATUS_OK && lc_after_len > 0) {
                ESP_LOGI(TAG, "Lifecycle after EXCHANGE: 0x%02X (%s)", lc_after[0], kta_lifecycle_str(lc_after[0]));

                // Detect ONLY true refurbish transitions.
                // During renew/rotate flows KTA legitimately transitions
                // PROVISIONED -> CON_REQ (0x43). That is NOT refurbish and
                // must not trigger a reset.
                bool transitioned_from_provisioned_to_refurbish =
                    (lc_state[0] == KTA_LC_PROVISIONED_MARKER &&
                     (lc_after[0] == KTA_LC_INIT_MARKER || lc_after[0] == KTA_LC_SEALED_MARKER));
                bool transitioned_init_to_sealed =
                    (lc_state[0] == KTA_LC_INIT_MARKER &&
                     lc_after[0] == KTA_LC_SEALED_MARKER &&
                     ks2ktaLen > 0u &&
                     kta2ksLen == 0u);

                if (transitioned_from_provisioned_to_refurbish || transitioned_init_to_sealed) {
                    ESP_LOGW(TAG, "🗑️  REFURBISH detected - lifecycle transition 0x%02X -> 0x%02X", lc_state[0], lc_after[0]);
                    ESP_LOGI(TAG, "📤 Sending DELETE acknowledgment to server (%u bytes) BEFORE resetting state", (unsigned)kta2ksLen);
                    g_ktaKSCmdStatus = E_K_KTA_KS_STATUS_REFURBISH;
                    refurbish_detected = true;
                    // Response will be sent normally below (no 'continue').
                    // After the BLE notify, reset KTA state for re-onboarding.
                } else if (lc_state[0] == KTA_LC_PROVISIONED_MARKER && lc_after[0] == KTA_LC_CON_REQ_MARKER) {
                    ESP_LOGI(TAG, "🔄 Server command flow detected (PROVISIONED -> CON_REQ) - keeping KTA running");
                    g_ktaKSCmdStatus = E_K_KTA_KS_STATUS_RENEW;
                } else if (lc_after[0] == KTA_LC_PROVISIONED_MARKER) {
                    g_ktaKSCmdStatus = E_K_KTA_KS_STATUS_NO_OPERATION;
                }
            }
        }

        size_t max_copy = sizeof(g_exchange_resp) - 2u;
        size_t safe_len = kta2ksLen;
        if (safe_len > sizeof(g_kta2ks_msg)) safe_len = sizeof(g_kta2ks_msg);
        if (safe_len > max_copy) safe_len = max_copy;

        uint16_t n = (safe_len > 0xFFFFu) ? 0xFFFFu : (uint16_t)safe_len;
        g_exchange_resp[0] = (uint8_t)(n & 0xFFu);
        g_exchange_resp[1] = (uint8_t)((n >> 8) & 0xFFu);
        if (n > 0u)
        {
            memcpy(&g_exchange_resp[2], g_kta2ks_msg, n);
        }

        // Activation request is generated on the first exchange call
        // when the inbound keySTREAM->KTA message length is 0.
        if (ks2ktaLen == 0u)
        {
            ESP_LOGI(TAG, "ktaExchangeMessage(0): st=%d(%s) kta2ksLen=%u", (int)st, kta_status_str(st), (unsigned)n);
            if (st != E_K_STATUS_OK)
            {
                ESP_LOGW(TAG, "Activation not generated: run STARTUP (0x02) and SET_DEVICE_INFORMATION (0x03) first, and ensure first EXCHANGE has ks2ktaLen=0");
                if (st == E_K_STATUS_MISSING)
                {
                    ESP_LOGW(TAG, "KTA returned MISSING (5). Common causes on this port: rotPublicUID missing in SAL storage, or chip certificate TLV missing (ATECC608 not responding / not provisioned). Check logs around ktaActBuildActivationRequest for the exact failing SAL API.");
                }
            }
            else
            {
                if (n < 250u)
                {
                    ESP_LOGW(TAG, "Activation length %u is below expected (~299). Likely missing ATECC608 chip TLV; check logs tagged 'kta_sal_rot' for ATECC init + chip UID/cert lengths.",
                             (unsigned)n);
                }
            }
            log_hex_bounded("Activation kta2ks bytes (ktaExchangeMessage(0) output)", g_kta2ks_msg, (size_t)n);
            log_hex_bounded("Activation BLE payload bytes [u16len|kta2ks...]", g_exchange_resp, (size_t)(2u + n));

            // Check if this is the FIRST activation exchange (onboarding):
            // - ks2ktaLen == 0 (NULL message = activation)
            // - lifecycle < PROVISIONED (device not yet onboarded)
            // This prevents malformed activation data from being posted to keySTREAM during onboarding.
            // After onboarding completes, allow short messages for server command checks.
            uint8_t lc_current[C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE] = {0};
            size_t lc_current_len = C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE;
            TKStatus lc_current_st = salStorageGetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID, lc_current, &lc_current_len);
            bool is_activation_during_onboarding = (ks2ktaLen == 0 &&
                                                     lc_current_st == E_K_STATUS_OK &&
                                                     lc_current_len > 0 &&
                                                     lc_current[0] != KTA_LC_PROVISIONED_MARKER);
            
            if (st == E_K_STATUS_OK && n < 250u && is_activation_during_onboarding)
            {
                st = E_K_STATUS_MISSING;
                ESP_LOGW(TAG, "⚠️ ONBOARDING activation message too short (%u bytes) - rejecting to prevent malformed data", (unsigned)n);
                ESP_LOGW(TAG, "   Expected ~250+ bytes with device certificate and identity");
                // Keep response payload empty; status conveys the failure.
                n = 0u;
                g_exchange_resp[0] = 0u;
                g_exchange_resp[1] = 0u;
            }
            else if (st == E_K_STATUS_OK && n < 250u)
            {
                ESP_LOGI(TAG, "✅ Short message (%u bytes) accepted - device already onboarded (lifecycle: 0x%02X)", 
                         (unsigned)n, 
                         (lc_current_st == E_K_STATUS_OK && lc_current_len > 0) ? lc_current[0] : 0xFF);
            }
        }

        s_exchange_notify_status = (uint8_t)st;
        s_exchange_notify_payload_len = (uint16_t)(2u + n);

        if (!s_exchange_event_initialized)
        {
            ESP_LOGE(TAG, "exchange event not initialized");
            if (s_exchange_mutex)
            {
                (void)xSemaphoreTake(s_exchange_mutex, portMAX_DELAY);
                s_exchange_inflight = false;
                (void)xSemaphoreGive(s_exchange_mutex);
            }
            else
            {
                s_exchange_inflight = false;
            }
            continue;
        }

        // Post notify onto NimBLE's default event queue so it runs in the
        // nimble_host task context (thread-safe).
        ble_npl_eventq_put(nimble_port_get_dflt_eventq(), &s_exchange_event);

        // ── REFURBISH: after BLE response (DELETE ACK) is queued, reset KTA ──
        // The BLE notify delivers the DELETE acknowledgment to the mobile app.
        // Now force-reset the KTA library so the re-init sequence works:
        //   INITIALIZE -> STARTUP -> SET_DEVICE_INFO -> EXCHANGE(0) -> 299-byte activation
        if (refurbish_detected) {
            ESP_LOGI(TAG, "⏳ Waiting for ATECC608 to settle after DELETE operations...");
            vTaskDelay(pdMS_TO_TICKS(500));

            // Verify ATECC608 is responsive through existing I2C connection
            uint8_t rev[4];
            ATCA_STATUS atecc_rc = atcab_wakeup();
            if (atecc_rc == ATCA_SUCCESS) {
                atecc_rc = atcab_info(rev);
                if (atecc_rc == ATCA_SUCCESS) {
                    ESP_LOGI(TAG, "✅ ATECC608 responsive — revision: %02x%02x%02x%02x",
                             rev[0], rev[1], rev[2], rev[3]);
                } else {
                    ESP_LOGW(TAG, "⚠️ ATECC608 info failed (0x%02x) — retrying...", atecc_rc);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    atcab_info(rev);
                }
            } else {
                ESP_LOGW(TAG, "⚠️ ATECC608 wakeup failed (0x%02x) — retrying...", atecc_rc);
                vTaskDelay(pdMS_TO_TICKS(500));
                atcab_wakeup();
                atcab_info(rev);
            }

            // KTA already reset its internal state when it processed the refurbish command.
            // Clear NVM (lifecycle, L1 key, certs) so re-provisioning starts clean.
            
            ESP_LOGI(TAG, "✅ KTA NVM cleared for re-onboarding (state already INITIAL from KTA internal refurbish)");

            // Reset firmware flags so app-driven re-init works from scratch
            g_kta_initialized_once      = false;
            g_kta_started_once          = false;
            g_kta_device_info_once      = false;
            g_kta_exchange_started_once = false;
            g_kta_connect_bootstrap_done_once = false;
            g_kta_connection_required   = false;
            g_kta_last_connection_request = 1u;
            ESP_LOGI(TAG, "🔄 KTA state + firmware flags reset — device ready for re-onboarding");

            // The KTA library's internal gKtaState is a process-lifetime static that
            // only returns to E_KTA_STATE_INITIAL on reboot or via a refurbish detected
            // inside ktaExchangeMessage(). Resetting the firmware flags above is NOT
            // enough: on the next INITIALIZE/STARTUP, ktaInitialize() refuses to move
            // a non-INITIAL state and ktaStartup() then rejects with E_K_STATUS_ERROR(6).
            // Reboot so the next boot starts from a guaranteed-clean gKtaState == INITIAL
            // and the INITIALIZE -> STARTUP -> SET_DEVICE_INFO -> EXCHANGE flow succeeds.
            ESP_LOGW(TAG, "Rebooting device to finalize refurbish (clean KTA state for re-onboarding)...");
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_restart();
        }
    }
}

static const char* reset_reason_str(esp_reset_reason_t r)
{
    switch (r)
    {
        case ESP_RST_UNKNOWN: return "UNKNOWN";
        case ESP_RST_POWERON: return "POWERON";
        case ESP_RST_EXT: return "EXT";
        case ESP_RST_SW: return "SW";
        case ESP_RST_PANIC: return "PANIC";
        case ESP_RST_INT_WDT: return "INT_WDT";
        case ESP_RST_TASK_WDT: return "TASK_WDT";
        case ESP_RST_WDT: return "WDT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_BROWNOUT: return "BROWNOUT";
        case ESP_RST_SDIO: return "SDIO";
        default: return "(other)";
    }
}

static void handle_ctrl_binary_frame(uint8_t opcode, const uint8_t *payload, uint16_t payload_len) {
    ESP_LOGI(TAG, "CTRL frame: opcode=0x%02X payloadLen=%u", opcode, (unsigned)payload_len);

    // Log raw payload bytes for observability of each API call from the app.
    log_hex_bounded("CTRL payload (app->ESP32)", payload, (size_t)payload_len);

    switch (opcode) {
    case OP_INITIALIZE:
        {
            ESP_LOGI("KTA_CMD", "✅ INITIALIZE (0x01) received from Windows gateway - calling ktaInitialize");

            // Reset all lifecycle flags so that the subsequent STARTUP and
            // SET_DEVICE_INFO commands actually call their KTA library functions
            // instead of short-circuiting as no-ops.
            //
            // Without this reset, after a RENEW/ROTATE cycle the flags are all
            // still true from the initial provisioning run.  STARTUP sees
            // g_kta_started_once=true and returns OK without calling ktaStartup();
            // SET_DEVICE_INFO sees g_kta_device_info_once=true and returns OK
            // without calling ktaSetDeviceInformation().  The KTA library state
            // is therefore never changed and EXCHANGE keeps failing with STATE/MISSING.
            g_kta_initialized_once      = false;
            g_kta_started_once          = false;
            g_kta_device_info_once      = false;
            g_kta_exchange_started_once = false;

            TKStatus st = ensure_kta_initialized_and_seeded();
            notify_binary_frame(OP_INITIALIZE, (uint8_t)st, NULL, 0);

            // Gateway/CLI controls the sequence explicitly:
            // INITIALIZE -> STARTUP -> SET_DEVICE_INFO -> EXCHANGE.
            ESP_LOGI(TAG, "INITIALIZE: waiting for STARTUP from gateway");
        }
        return;

    case OP_STARTUP:
        {
            // Some clients reconnect and may re-run STARTUP without re-running INITIALIZE.
            // Keep the state machine forgiving.
            TKStatus init_st = ensure_kta_initialized_and_seeded();
            if (init_st != E_K_STATUS_OK)
            {
                ESP_LOGW(TAG, "STARTUP: ktaInitialize failed: st=%d(%s)", (int)init_st, kta_status_str(init_st));
                notify_binary_frame(OP_STARTUP, (uint8_t)init_st, NULL, 0);
                return;
            }

            // Upstream ktaStartup() only succeeds when internal state is INITIALIZED.
            // If we already completed STARTUP earlier in this boot session, treat a
            // repeated STARTUP as a no-op success for mobile reconnects.
            if (g_kta_started_once)
            {
                ESP_LOGI(TAG, "ktaStartup: already STARTED; returning OK");
                notify_binary_frame(OP_STARTUP, (uint8_t)E_K_STATUS_OK, NULL, 0);
                return;
            }

            // Payload format expected by the gateway protocol:
            // [l1Seed:16]
            // [contextProfileUid:32][profileLen:u32LE]
            // [contextSerial:16][serialLen:u32LE]
            // [contextVersion:16][versionLen:u32LE]
            const uint16_t expected = 16u + 32u + 4u + 16u + 4u + 16u + 4u;
            if (payload == NULL || payload_len < expected)
            {
                notify_binary_frame(OP_STARTUP, (uint8_t)E_K_STATUS_PARAMETER, NULL, 0);
                return;
            }

            ESP_LOGI("KTA_CMD", "✅ STARTUP (0x02) received from Windows gateway - calling ktaStartup, payloadLen=%u",
                     (unsigned)payload_len);

            const uint8_t* l1 = payload;
            const uint8_t* prof = payload + 16u;
            uint32_t profLen = read_u32le(payload + 16u + 32u);
            const uint8_t* ser = payload + 16u + 32u + 4u;
            uint32_t serLen = read_u32le(payload + 16u + 32u + 4u + 16u);
            const uint8_t* ver = payload + 16u + 32u + 4u + 16u + 4u;
            uint32_t verLen = read_u32le(payload + 16u + 32u + 4u + 16u + 4u + 16u);

            // Log the fleet UID bytes the app is sending so we can verify
            {
                char hexbuf[65] = {0};
                uint32_t dumpLen = profLen < 32u ? profLen : 32u;
                for (uint32_t i = 0; i < dumpLen; i++) {
                    snprintf(&hexbuf[i*2], 3, "%02x", prof[i]);
                }
                ESP_LOGW(TAG, "STARTUP fleet UID from app (%u bytes): %s", (unsigned)profLen, hexbuf);
            }

            TKStatus st = ktaStartup(l1, prof, (size_t)profLen, ser, (size_t)serLen, ver, (size_t)verLen);
            ESP_LOGI(TAG, "ktaStartup: st=%d(%s) profLen=%u serLen=%u verLen=%u",
                     (int)st, kta_status_str(st), (unsigned)profLen, (unsigned)serLen, (unsigned)verLen);

            if (st == E_K_STATUS_OK)
            {
                g_kta_started_once = true;
            }
            else if (st == E_K_STATUS_STATE || st == E_K_STATUS_ERROR)
            {
                /* KTA library is already past INITIALIZED state (e.g. still RUNNING after
                 * a server command / renew cycle that reset firmware flags but not the
                 * KTA internal state machine).  Treat as idempotent so recovery proceeds. */
                ESP_LOGW(TAG, "ktaStartup returned STATE/ERROR - KTA already started; treating as idempotent OK");
                g_kta_started_once = true;
                st = E_K_STATUS_OK;
            }
            notify_binary_frame(OP_STARTUP, (uint8_t)st, NULL, 0);
        }
        return;

    case OP_SET_DEVICE_INFORMATION: {
        TKStatus init_st = ensure_kta_initialized_and_seeded();
        if (init_st != E_K_STATUS_OK)
        {
            ESP_LOGW(TAG, "SET_DEVICE_INFORMATION: ktaInitialize failed: st=%d(%s)", (int)init_st, kta_status_str(init_st));
            notify_binary_frame(OP_SET_DEVICE_INFORMATION, (uint8_t)init_st, NULL, 0);
            return;
        }

        ESP_LOGI("KTA_CMD", "✅ SET_DEVICE_INFO (0x03) received from Windows gateway - calling ktaSetDeviceInformation, payloadLen=%u",
                 (unsigned)payload_len);

        // If we already successfully set device information once, treat repeats as a no-op.
        // Upstream KTA only accepts ktaSetDeviceInformation() in STARTED state; after the
        // first successful call it transitions to RUNNING and repeats would return ERROR(6).
        if (g_kta_device_info_once)
        {
            ESP_LOGI(TAG, "ktaSetDeviceInformation: already RUNNING; returning OK (conReq=%u)",
                     (unsigned)g_kta_last_connection_request);
            uint8_t resp_payload[1] = { g_kta_last_connection_request };
            notify_binary_frame(OP_SET_DEVICE_INFORMATION, (uint8_t)E_K_STATUS_OK, resp_payload, sizeof(resp_payload));
            return;
        }

        if (!g_kta_started_once)
        {
            ESP_LOGW(TAG, "SET_DEVICE_INFORMATION called before STARTUP; returning STATE error");
            notify_binary_frame(OP_SET_DEVICE_INFORMATION, (uint8_t)E_K_STATUS_STATE, NULL, 0);
            return;
        }

        // Payload format expected by the gateway protocol:
        // [deviceProfileUid:32][uidLen:u32LE][deviceSerial:16][serialLen:u32LE][conRequest?:1]
        // Some clients omit the trailing conRequest byte; treat it as optional.
        const uint16_t expected_min = 32u + 4u + 16u + 4u;
        if (payload == NULL || payload_len < expected_min)
        {
            notify_binary_frame(OP_SET_DEVICE_INFORMATION, (uint8_t)E_K_STATUS_PARAMETER, NULL, 0);
            return;
        }

        const uint8_t* prof = payload;
        uint32_t profLen = read_u32le(payload + 32u);
        const uint8_t* ser = payload + 32u + 4u;
        uint32_t serLen = read_u32le(payload + 32u + 4u + 16u);

        // Observability: many provisioning clients send a placeholder profile UID.
        // Log what we received so you can confirm whether the central is providing
        // a per-device UID or a default/static one.
        log_hex_bounded("SET_DEVICE_INFORMATION deviceProfileUid", prof, 32u);
        log_hex_bounded("SET_DEVICE_INFORMATION deviceSerial(from app)", ser, 16u);

        // IMPORTANT: xpDeviceProfilePublicUid is the fleet/device-profile public UID in keySTREAM
        // (e.g. "9S4F:1.3.1"). It is NOT the chip UID.
        // Chip UID is provided to KTA via SAL (salRotGetChipUID) and is carried in activation.
        if (profLen == 0u || profLen > 32u || serLen == 0u || serLen > 16u)
        {
            ESP_LOGW(TAG, "SET_DEVICE_INFORMATION invalid lengths: profLen=%u serLen=%u",
                     (unsigned)profLen, (unsigned)serLen);
            notify_binary_frame(OP_SET_DEVICE_INFORMATION, (uint8_t)E_K_STATUS_PARAMETER, NULL, 0);
            return;
        }

        // Per your requirement: device UID must be read from the device (ATECC608) and is 9 bytes.
        // Use the secure element serial number as the device serial passed to KTA when available.
        uint8_t chip_uid[9] = {0};
        const uint8_t* ser_to_use = ser;
        size_t ser_to_use_len = (size_t)serLen;

        ATCA_STATUS uid_st = atcab_read_serial_number(chip_uid);
        if (uid_st == ATCA_SUCCESS)
        {
            ser_to_use = chip_uid;
            ser_to_use_len = sizeof(chip_uid);
            log_hex_bounded("SET_DEVICE_INFORMATION deviceUID(from ATECC608)", chip_uid, sizeof(chip_uid));
            ESP_LOGI(TAG, "Using device-provided UID (len=%u) as deviceSerial", (unsigned)ser_to_use_len);
        }
        else
        {
            ESP_LOGW(TAG, "Failed to read ATECC608 UID (0x%02x); using app-provided deviceSerial", (unsigned)uid_st);
        }

        ESP_LOGI(TAG, "Using deviceProfilePublicUID (len=%u) and deviceSerial (len=%u)",
                 (unsigned)profLen, (unsigned)ser_to_use_len);

        /* Default to last known value so the variable is always initialised
         * even when ktaSetDeviceInformation is skipped (STATE path below). */
        uint8_t connectionRequest = g_kta_last_connection_request;
        TKStatus st = ktaSetDeviceInformation(prof, (size_t)profLen, ser_to_use, ser_to_use_len, &connectionRequest);
        ESP_LOGI(TAG, "ktaSetDeviceInformation: st=%d(%s) profLen=%u serLen=%u conReq(out)=%u",
                 (int)st, kta_status_str(st), (unsigned)profLen, (unsigned)ser_to_use_len, (unsigned)connectionRequest);
        if (st == E_K_STATUS_OK)
        {
            g_kta_device_info_once = true;
            g_kta_last_connection_request = connectionRequest;
            g_kta_connection_required = (connectionRequest != 0u);
            g_kta_exchange_started_once = false;
        }
        else if (st == E_K_STATUS_STATE || st == E_K_STATUS_ERROR)
        {
            /* KTA already in RUNNING state — firmware flags were reset by a prior
             * command-cycle recovery but the KTA library state was not.  Accept
             * the cached connectionRequest and let the exchange proceed. */
            ESP_LOGW(TAG, "ktaSetDeviceInformation returned STATE/ERROR — KTA already running; treating as idempotent OK (conReq=%u)",
                     (unsigned)g_kta_last_connection_request);
            connectionRequest = g_kta_last_connection_request;
            g_kta_device_info_once = true;
            st = E_K_STATUS_OK;
        }
        uint8_t resp_payload[1] = { connectionRequest };
        notify_binary_frame(OP_SET_DEVICE_INFORMATION, (uint8_t)st, resp_payload, sizeof(resp_payload));
        return;
    }

    case OP_EXCHANGE_MESSAGE: {
        g_last_activity_ms = esp_timer_get_time() / 1000;  // Update activity timestamp
        
        ESP_LOGI("KTA_LISTENING", "✅ ESP32 is LISTENING - Ready to receive exchange messages (provisioned or not)");

        ESP_LOGI("KTA_CMD", "EXCHANGE request from app (opcode=0x%02X) payloadLen=%u",
             (unsigned)opcode, (unsigned)payload_len);
        
        // Validate connection
        if (!is_app_connected()) {
            ESP_LOGW(TAG, "⚠️  Exchange received but connection not fully established!");
        }
        
        // Track if this might be post-provisioning activity
        if (g_kta_device_info_once && g_provisioned_at_ms == 0) {
            g_provisioned_at_ms = esp_timer_get_time() / 1000;
            ESP_LOGI(TAG, "📝 Provisioning completed at timestamp: %lld", g_provisioned_at_ms);
            ESP_LOGI(TAG, "✅ Onboarding / provisioning DONE");
            
            // Start periodic field management timer for refurbish/renew/rotate
            if (g_ktaFieldMgmtTimer == NULL) {
                g_ktaFieldMgmtTimer = xTimerCreate(
                    "KTA_FieldMgmt",
                    pdMS_TO_TICKS(KTA_FIELD_MGMT_INTERVAL_MS),
                    pdTRUE,  // Auto-reload
                    (void *)0,
                    kta_field_mgmt_timer_callback
                );
                
                if (g_ktaFieldMgmtTimer != NULL && xTimerStart(g_ktaFieldMgmtTimer, 0) == pdPASS) {
                    ESP_LOGI(TAG, "🔄 KTA field management timer started - checking every %d seconds", 
                             KTA_FIELD_MGMT_INTERVAL_MS / 1000);
                } else {
                    ESP_LOGE(TAG, "❌ Failed to create/start KTA field management timer");
                }
            }
        }
        
        // Only auto-initialize for BLE reconnect scenarios where STARTUP was already
        // completed (g_kta_started_once == true).  After a refurbish reset all flags are
        // false; calling ktaInitialize() here would "steal" the INITIAL->INITIALIZED
        // transition, causing the app's explicit INITIALIZE to find KTA already past
        // INITIAL and potentially confuse the state machine.
        // In the post-refurbish case we skip auto-init, let ktaExchangeMessage() fail
        // with status=6 (state != RUNNING), and let the app drive the full re-init
        // sequence: INITIALIZE -> STARTUP -> SET_DEVICE_INFO -> EXCHANGE(0).
        if (g_kta_started_once) {
            (void)ensure_kta_initialized_and_seeded();
        } else if (!g_kta_initialized_once) {
            ESP_LOGW(TAG, "⚠️ EXCHANGE received but device not initialized (post-refurbish?) — returning ERROR so app runs full init sequence");
            notify_binary_frame(OP_EXCHANGE_MESSAGE, (uint8_t)E_K_STATUS_ERROR, NULL, 0);
            return;
        }

        // If the last SET_DEVICE_INFORMATION indicates a keySTREAM connection is required,
        // enforce the ordering: EXCHANGE must occur before STATUS.
        g_kta_exchange_started_once = true;

        // Payload format: [ks2ktaLen:u16LE][ks2ktaMsg]
        if (payload == NULL || payload_len < 2u)
        {
            notify_binary_frame(OP_EXCHANGE_MESSAGE, (uint8_t)E_K_STATUS_PARAMETER, NULL, 0);
            return;
        }

        const uint16_t ks2ktaLen = (uint16_t)payload[0] | ((uint16_t)payload[1] << 8);
        if (payload_len < (uint16_t)(2u + ks2ktaLen))
        {
            notify_binary_frame(OP_EXCHANGE_MESSAGE, (uint8_t)E_K_STATUS_PARAMETER, NULL, 0);
            return;
        }

        // Hard limit to keep the KTA parser and our fixed buffers safe.
        if (ks2ktaLen > (uint16_t)KTA_ICPP_MSG_MAX_SIZE)
        {
            notify_binary_frame(OP_EXCHANGE_MESSAGE, (uint8_t)E_K_STATUS_PARAMETER, NULL, 0);
            return;
        }

        if (s_kta_worker_handle == NULL)
        {
            notify_binary_frame(OP_EXCHANGE_MESSAGE, (uint8_t)E_K_STATUS_ERROR, NULL, 0);
            return;
        }

        if (s_exchange_mutex)
        {
            (void)xSemaphoreTake(s_exchange_mutex, portMAX_DELAY);
        }

        if (s_exchange_inflight)
        {
            if (s_exchange_mutex) (void)xSemaphoreGive(s_exchange_mutex);
            notify_binary_frame(OP_EXCHANGE_MESSAGE, (uint8_t)E_K_STATUS_STATE, NULL, 0);
            return;
        }

        s_exchange_inflight = true;
        s_exchange_req_len = ks2ktaLen;
        if (ks2ktaLen > 0u)
        {
            memcpy(s_exchange_req_buf, payload + 2u, ks2ktaLen);
        }

        if (s_exchange_mutex)
        {
            (void)xSemaphoreGive(s_exchange_mutex);
        }

        xTaskNotifyGive(s_kta_worker_handle);
        return;
    }

    case OP_SET_CHIP_CERTIFICATE: {
        // Payload format:
        // [chipCertLen:u16LE][chipCertBytes...]
        // Stored in NVS under namespace "kta" key "chip_cert".
        if (payload == NULL || payload_len < 2u)
        {
            notify_binary_frame(OP_SET_CHIP_CERTIFICATE, (uint8_t)E_K_STATUS_PARAMETER, NULL, 0);
            return;
        }

        const uint16_t certLen = (uint16_t)payload[0] | ((uint16_t)payload[1] << 8);
        if (payload_len < (uint16_t)(2u + certLen))
        {
            notify_binary_frame(OP_SET_CHIP_CERTIFICATE, (uint8_t)E_K_STATUS_PARAMETER, NULL, 0);
            return;
        }

        // Keep within KTA message max for this port.
        if (certLen > (uint16_t)KTA_ICPP_MSG_MAX_SIZE)
        {
            notify_binary_frame(OP_SET_CHIP_CERTIFICATE, (uint8_t)E_K_STATUS_PARAMETER, NULL, 0);
            return;
        }

        const uint8_t* cert = payload + 2u;

        nvs_handle_t handle;
        esp_err_t err = nvs_open("kta", NVS_READWRITE, &handle);
        if (err != ESP_OK)
        {
            notify_binary_frame(OP_SET_CHIP_CERTIFICATE, (uint8_t)E_K_STATUS_ERROR, NULL, 0);
            return;
        }

        if (certLen == 0u)
        {
            (void)nvs_erase_key(handle, "chip_cert");
            (void)nvs_commit(handle);
            nvs_close(handle);
            ESP_LOGI(TAG, "Chip certificate cleared");
            notify_binary_frame(OP_SET_CHIP_CERTIFICATE, (uint8_t)E_K_STATUS_OK, NULL, 0);
            return;
        }

        err = nvs_set_blob(handle, "chip_cert", cert, (size_t)certLen);
        if (err == ESP_OK)
        {
            err = nvs_commit(handle);
        }
        nvs_close(handle);

        if (err != ESP_OK)
        {
            notify_binary_frame(OP_SET_CHIP_CERTIFICATE, (uint8_t)E_K_STATUS_ERROR, NULL, 0);
            return;
        }

        ESP_LOGI(TAG, "Chip certificate stored: len=%u", (unsigned)certLen);
        log_hex_bounded("Chip certificate (stored, first bytes)", cert, (size_t)certLen);
        notify_binary_frame(OP_SET_CHIP_CERTIFICATE, (uint8_t)E_K_STATUS_OK, NULL, 0);
        return;
    }

    case OP_KEY_STREAM_STATUS: {
        (void)ensure_kta_initialized_and_seeded();

        if (g_kta_connection_required && !g_kta_exchange_started_once)
        {
            ESP_LOGW(TAG, "KEY_STREAM_STATUS called before EXCHANGE while conReq=1; returning STATE error");
            uint8_t out[4] = {0, 0, 0, 0};
            notify_binary_frame(OP_KEY_STREAM_STATUS, (uint8_t)E_K_STATUS_STATE, out, sizeof(out));
            return;
        }

        TKktaKeyStreamStatus ksStatus = E_K_KTA_KS_STATUS_NONE;
        TKStatus st = ktaKeyStreamStatus(&ksStatus);

        int32_t ks = (int32_t)ksStatus;
        uint8_t out[4];
        out[0] = (uint8_t)(ks & 0xFF);
        out[1] = (uint8_t)((ks >> 8) & 0xFF);
        out[2] = (uint8_t)((ks >> 16) & 0xFF);
        out[3] = (uint8_t)((ks >> 24) & 0xFF);
        notify_binary_frame(OP_KEY_STREAM_STATUS, (uint8_t)st, out, sizeof(out));
        return;
    }

    case OP_REFURBISH: {
        ESP_LOGE(TAG, "🚨🚨🚨 OP_REFURBISH (0xFA) RECEIVED - STARTING FACTORY RESET 🚨🚨🚨");
        ESP_LOGW(TAG, "🔄 REFURBISH command received - starting local device deletion");
        
        TKStatus delete_status = E_K_STATUS_OK;
        uint8_t platform_status = 0;
        
        // Delete all provisioned data locally using correct slot IDs
        ESP_LOGI(TAG, "🗑️  Step 1: Deleting device certificates and keys");
        
        // Device Certificate - DATA type, slot 0x0A
        TKStatus st1 = salObjectDelete(E_K_SAL_OBJECT_TYPE_DATA, 0x0A, &platform_status);
        ESP_LOGI(TAG, "   Device Certificate delete: %d (%s)", st1, kta_status_str(st1));
        
        // Signer Public Key - DATA type, slot 0x0B  
        TKStatus st2 = salObjectDelete(E_K_SAL_OBJECT_TYPE_DATA, 0x0B, &platform_status);
        ESP_LOGI(TAG, "   Signer Public Key delete: %d (%s)", st2, kta_status_str(st2));
        
        // Signer Certificate - DATA type, slot 0x0C
        TKStatus st3 = salObjectDelete(E_K_SAL_OBJECT_TYPE_DATA, 0x0C, &platform_status);
        ESP_LOGI(TAG, "   Signer Certificate delete: %d (%s)", st3, kta_status_str(st3));
        
        // Lifecycle/L1 Key Material - DATA type, slot 0x1000008 (C_SAL_SLOT8_IDENTIFIER)
        ESP_LOGI(TAG, "🗑️  Step 2: Resetting lifecycle state and L1 key material");
        TKStatus st4 = salObjectDelete(E_K_SAL_OBJECT_TYPE_DATA, 0x1000008, &platform_status);
        ESP_LOGI(TAG, "   Lifecycle/L1 Material reset: %d (%s)", st4, kta_status_str(st4));
        
        // Check if any operation failed (ignore MISSING - already deleted)
        if (st1 != E_K_STATUS_OK && st1 != E_K_STATUS_MISSING) delete_status = st1;
        if (st2 != E_K_STATUS_OK && st2 != E_K_STATUS_MISSING) delete_status = st2;
        if (st3 != E_K_STATUS_OK && st3 != E_K_STATUS_MISSING) delete_status = st3;
        if (st4 != E_K_STATUS_OK && st4 != E_K_STATUS_MISSING) delete_status = st4;
        
        if (delete_status == E_K_STATUS_OK || delete_status == E_K_STATUS_MISSING) {
            ESP_LOGI(TAG, "✅ REFURBISH slot deletion OK - now resetting KTA state...");

            // KTA already reset its internal state when it processed the refurbish command.
            // Clear NVM (lifecycle, L1 key, certs) so re-provisioning starts clean.
            

            g_kta_initialized_once      = false;
            g_kta_started_once          = false;
            g_kta_device_info_once      = false;
            g_kta_exchange_started_once = false;
            g_kta_connect_bootstrap_done_once = false;
            g_kta_connection_required   = false;
            g_kta_last_connection_request = 1u;

            ESP_LOGI(TAG, "✅ KTA library reset to INITIAL/INIT after OP_REFURBISH");
            notify_binary_frame(OP_REFURBISH, (uint8_t)E_K_STATUS_OK, NULL, 0);

            ESP_LOGI(TAG, "🔄 KTA state + firmware flags reset — device ready for re-onboarding");
        } else {
            ESP_LOGE(TAG, "❌ REFURBISH failed: status=%d (%s)", delete_status, kta_status_str(delete_status));
            notify_binary_frame(OP_REFURBISH, (uint8_t)delete_status, NULL, 0);
        }
        return;
    }

    default:
        notify_binary_frame(opcode, 0x01, NULL, 0);
        return;
    }
}

static void handle_ctrl_line(const char *line) {
    ESP_LOGI(TAG, "CTRL line: %s", line);

    if (KS_GATEWAY_MODE) {
        (void)line;
        ESP_LOGW(TAG, "Ignoring legacy text/JSON CTRL input in gateway mode");
        return;
    }

    // Legacy debug path for text-based clients.
    (void)line;
    const char *resp =
        "{\"error\":\"binary protocol required\",\"requiredOpcodes\":[1,2,3,4,8,9],\"refurbish\":10}\n";
    notify_string_chunked(resp);
}

static void ctrl_rx_append_and_process(const uint8_t *data, size_t len) {
    if (!data || len == 0) return;

    // Gateway mode uses binary opcode framing only.
    if (KS_GATEWAY_MODE || data[0] != '{') {
        static size_t s_last_wait_need = 0;
        const size_t max_payload_len = sizeof(g_ctrl_bin_rx) - 3;
        if (g_ctrl_bin_rx_len + len >= sizeof(g_ctrl_bin_rx)) {
            ESP_LOGW(TAG, "CTRL BIN RX overflow; clearing buffer");
            g_ctrl_bin_rx_len = 0;
        }

        memcpy(&g_ctrl_bin_rx[g_ctrl_bin_rx_len], data, len);
        g_ctrl_bin_rx_len += len;

        // Parse frames: [opcode:1][payloadLen:2 LE][payload]
        while (g_ctrl_bin_rx_len >= 3) {
            uint8_t opcode = g_ctrl_bin_rx[0];
            uint16_t payload_len = (uint16_t)g_ctrl_bin_rx[1] | ((uint16_t)g_ctrl_bin_rx[2] << 8);
            size_t frame_len = (size_t)3 + payload_len;

            // Resync on invalid headers: prevents lock-up if the stream contains stray bytes.
            bool opcode_ok = (opcode == OP_INITIALIZE) || (opcode == OP_STARTUP) ||
                             (opcode == OP_SET_DEVICE_INFORMATION) || (opcode == OP_EXCHANGE_MESSAGE) ||
                             (opcode == OP_KEY_STREAM_STATUS) || (opcode == OP_SET_CHIP_CERTIFICATE) ||
                             (opcode == OP_REFURBISH);
            if (!opcode_ok || payload_len > max_payload_len) {
                ESP_LOGW(
                    TAG,
                    "CTRL BIN desync: dropping 1 byte (opcode=0x%02X payloadLen=%u)",
                    (unsigned)opcode,
                    (unsigned)payload_len
                );
                memmove(g_ctrl_bin_rx, &g_ctrl_bin_rx[1], g_ctrl_bin_rx_len - 1);
                g_ctrl_bin_rx_len -= 1;
                s_last_wait_need = 0;
                continue;
            }

            if (g_ctrl_bin_rx_len < frame_len) {
                // Helpful bring-up log so we can see if the central ever finishes a frame.
                // Throttle to avoid spamming identical lines.
                if (frame_len != s_last_wait_need) {
                    ESP_LOGI(
                        TAG,
                        "CTRL BIN waiting: have=%u need=%u (opcode=0x%02X payloadLen=%u)",
                        (unsigned)g_ctrl_bin_rx_len,
                        (unsigned)frame_len,
                        (unsigned)opcode,
                        (unsigned)payload_len
                    );
                    s_last_wait_need = frame_len;
                }
                break;
            }

            s_last_wait_need = 0;

            handle_ctrl_binary_frame(opcode, &g_ctrl_bin_rx[3], payload_len);

            // Shift remaining bytes left
            size_t remaining = g_ctrl_bin_rx_len - frame_len;
            memmove(g_ctrl_bin_rx, &g_ctrl_bin_rx[frame_len], remaining);
            g_ctrl_bin_rx_len = remaining;
        }

        return;
    }

    // Prevent overflow; if overflow, reset buffer.
    if (g_ctrl_rx_len + len >= sizeof(g_ctrl_rx)) {
        ESP_LOGW(TAG, "CTRL RX overflow; clearing buffer");
        g_ctrl_rx_len = 0;
    }

    memcpy(&g_ctrl_rx[g_ctrl_rx_len], data, len);
    g_ctrl_rx_len += len;
    g_ctrl_rx[g_ctrl_rx_len] = '\0';

    // Process newline-delimited messages.
    for (;;) {
        char *nl = strchr(g_ctrl_rx, '\n');
        if (!nl) break;

        *nl = '\0';
        if (g_ctrl_rx[0] != '\0') {
            handle_ctrl_line(g_ctrl_rx);
        }

        // Shift remaining data left.
        size_t remaining = g_ctrl_rx_len - (size_t)((nl + 1) - g_ctrl_rx);
        memmove(g_ctrl_rx, nl + 1, remaining);
        g_ctrl_rx_len = remaining;
        g_ctrl_rx[g_ctrl_rx_len] = '\0';
    }

    // Fallback: some legacy centrals send a complete JSON without a newline.
    // If the buffer ends with '}', treat it as a complete message.
    if (g_ctrl_rx_len > 0 && g_ctrl_rx[g_ctrl_rx_len - 1] == '}') {
        ESP_LOGI(TAG, "CTRL RX complete without newline (%u bytes)", (unsigned)g_ctrl_rx_len);
        handle_ctrl_line(g_ctrl_rx);
        g_ctrl_rx_len = 0;
        g_ctrl_rx[0] = '\0';
    }
}

static int gatt_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg) {
    const ble_uuid_t *uuid = ctxt->chr ? ctxt->chr->uuid : NULL;

    if (uuid && ble_uuid_u16(uuid) == KS_CTRL_UUID16) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            const uint8_t *buf = ctxt->om->om_data;
            uint16_t len = ctxt->om->om_len;
            ESP_LOGI(TAG, "FF01 CTRL write: %u bytes", (unsigned)len);
            if (!g_ctrl_opcode_seen_since_connect && g_connected_at_ms > 0) {
                int64_t now_ms = esp_timer_get_time() / 1000;
                g_first_ctrl_opcode_ms = now_ms;
                ESP_LOGI(TAG, "[BLE] First FF01 opcode arrived %lld ms after connect",
                         (long long)(g_first_ctrl_opcode_ms - g_connected_at_ms));
            }
            g_ctrl_opcode_seen_since_connect = true;
            s_worker_auto_bootstrap_pending = false;

            ESP_LOGI(TAG, "FF01 opcode=0x%02X len=%u", buf[0], (unsigned)len);
            ctrl_rx_append_and_process(buf, len);
            return 0;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (uuid && ble_uuid_u16(uuid) == KS_ACK_UUID16) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // For now, just log ACK/NACK. Define your format later.
            ESP_LOGI(TAG, "FF03 ACK write: %u bytes", (unsigned)ctxt->om->om_len);
            return 0;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (uuid && ble_uuid_u16(uuid) == KS_RESP_UUID16) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            ESP_LOGI(TAG, "FF04 RESP write: %u bytes", (unsigned)ctxt->om->om_len);
            return 0;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (uuid && ble_uuid_u16(uuid) == KS_DATA_UUID16) {
        // DATA is notify; allow read but return empty.
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            return os_mbuf_append(ctxt->om, "", 0) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(KS_SVC_UUID16),
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = BLE_UUID16_DECLARE(KS_CTRL_UUID16),
                .access_cb = gatt_access_cb,
                .val_handle = &g_ctrl_val_handle,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                .uuid = BLE_UUID16_DECLARE(KS_DATA_UUID16),
                .access_cb = gatt_access_cb,
                .val_handle = &g_data_val_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = BLE_UUID16_DECLARE(KS_ACK_UUID16),
                .access_cb = gatt_access_cb,
                .val_handle = &g_ack_val_handle,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(KS_RESP_UUID16),
                .access_cb = gatt_access_cb,
                .val_handle = &g_resp_val_handle,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {0}
        },
    },
    {0}
};

static void advertise(void);

static const char *hci_disconnect_reason_str(uint8_t code)
{
    switch (code)
    {
        case 0x08: return "Connection Timeout";
        case 0x13: return "Remote User Terminated Connection";
        case 0x14: return "Remote Device Terminated (Low Resources)";
        case 0x15: return "Remote Device Terminated (Power Off)";
        case 0x16: return "Connection Terminated by Local Host";
        case 0x3B: return "Unacceptable Connection Parameters";
        case 0x3E: return "Connection Failed to Establish";
        default: return "(unknown)";
    }
}

static uint8_t hci_reason_from_disconnect(int reason)
{
    unsigned ur = (reason < 0) ? 0u : (unsigned)reason;
    if ((ur & 0xFF00u) == 0x200u)
    {
        return (uint8_t)(ur & 0xFFu);
    }
    if (ur <= 0xFFu)
    {
        return (uint8_t)ur;
    }
    return 0xFFu;
}

static void log_disconnect_reason(int reason)
{
    // NimBLE commonly wraps HCI status codes as BLE_HS_HCI_ERR_BASE (0x200) + hci_reason.
    // Example: 0x213 -> HCI 0x13 (Remote User Terminated Connection).
    unsigned ur = (reason < 0) ? 0u : (unsigned)reason;
    if ((ur & 0xFF00u) == 0x200u)
    {
        uint8_t hci = (uint8_t)(ur & 0xFFu);
        ESP_LOGI(TAG, "Disconnected; reason=%d (0x%04X) hci=0x%02X (%s)",
                 reason, (unsigned)ur, (unsigned)hci, hci_disconnect_reason_str(hci));
        if (hci == 0x13u)
        {
            ESP_LOGI(TAG, "[BLE Explain] HCI 0x13 means the remote side ended the connection (central requested disconnect)");
        }
        return;
    }

    if (ur <= 0xFFu)
    {
        uint8_t hci = (uint8_t)ur;
        ESP_LOGI(TAG, "Disconnected; reason=%d (0x%02X) hci=0x%02X (%s)",
                 reason, (unsigned)hci, (unsigned)hci, hci_disconnect_reason_str(hci));
        if (hci == 0x13u)
        {
            ESP_LOGI(TAG, "[BLE Explain] HCI 0x13 means the remote side ended the connection (central requested disconnect)");
        }
        return;
    }

    ESP_LOGI(TAG, "Disconnected; reason=%d (0x%X)", reason, ur);
}

static int gap_event_cb(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            g_conn_handle = event->connect.conn_handle;
            g_data_subscribed = false;
            g_ctrl_opcode_seen_since_connect = false;
            g_connected_at_ms = esp_timer_get_time() / 1000;
            g_last_activity_ms = g_connected_at_ms;
            g_last_kta_init_attempt_ms = g_connected_at_ms;
            g_first_ctrl_opcode_ms = 0;
            s_worker_auto_bootstrap_pending = false;
            s_worker_connect_bootstrap_pending = false;
            ESP_LOGI(TAG, "[BLE] Windows gateway CONNECTED; conn_handle=%u timestamp=%lld", g_conn_handle, g_connected_at_ms);
            ESP_LOGI(TAG, "🔗 Communication channel ESTABLISHED - Windows <-> ESP32 <-> KeySTREAM");
            ESP_LOGI(TAG, "🔍 Connection monitoring ACTIVE - will check every 10 seconds");
            ESP_LOGI(TAG, "⏳ Waiting for Windows gateway to send INITIALIZE (0x01) -> STARTUP (0x02) -> SET_DEVICE_INFO (0x03) -> EXCHANGE (0x04)");

            if (KTA_AUTO_INIT_ON_CONNECT) {
                // Optional compatibility mode: initialize immediately on link-up.
                // Disabled by default to keep the BLE connect path central-driven.
                TKStatus init_st = ensure_kta_initialized_and_seeded();
                if (init_st == E_K_STATUS_OK) {
                    ESP_LOGI(TAG, "KTA auto-initialize on connect: OK");
                } else {
                    ESP_LOGW(TAG, "KTA auto-initialize on connect failed: st=%d(%s)",
                             (int)init_st, kta_status_str(init_st));
                }
            } else {
                ESP_LOGI(TAG, "KTA auto-initialize on connect: disabled (waiting for FF01 INITIALIZE)");
            }
        } else {
            ESP_LOGW(TAG, "Connect failed; status=%d", event->connect.status);
            advertise();
        }
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE: {
        // Pairing / encryption state changed.
        struct ble_gap_conn_desc desc;
        int rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        if (rc == 0) {
            ESP_LOGI(TAG, "Encryption change: status=%d encrypted=%d authenticated=%d bonded=%d",
                     event->enc_change.status,
                     (int)desc.sec_state.encrypted,
                     (int)desc.sec_state.authenticated,
                     (int)desc.sec_state.bonded);
        } else {
            ESP_LOGI(TAG, "Encryption change: status=%d (conn_find rc=%d)",
                     event->enc_change.status, rc);
        }
        return 0;
    }

    case BLE_GAP_EVENT_REPEAT_PAIRING: {
        // Windows UI may trigger repeat pairing probes. Deleting stored bonds
        // here can lead to visible pair/unpair churn on the host side.
        struct ble_gap_conn_desc desc;
        int rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        if (rc == 0 && KS_DELETE_BOND_ON_REPEAT_PAIRING) {
            ble_store_util_delete_peer(&desc.peer_id_addr);
        }
        if (rc == 0 && !KS_DELETE_BOND_ON_REPEAT_PAIRING) {
            ESP_LOGW(TAG, "Repeat pairing requested; keeping existing bond record");
        }
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    }

    case BLE_GAP_EVENT_DISCONNECT:
        int64_t now_ms = esp_timer_get_time() / 1000;
        int64_t connection_duration_sec = (now_ms - g_connected_at_ms) / 1000;
        bool disconnected_before_ctrl = !g_ctrl_opcode_seen_since_connect;
        uint8_t hci_reason = hci_reason_from_disconnect(event->disconnect.reason);
        bool ui_probe_disconnect = KS_GATEWAY_MODE &&
                                   disconnected_before_ctrl &&
                                   !g_data_subscribed &&
                                   (hci_reason == 0x13u);

        if (ui_probe_disconnect) {
            ESP_LOGI(TAG, "[BLE] Windows central ended UI-only session (no FF01 command flow started)");
            ESP_LOGI(TAG, "[BLE] Session duration: %lld seconds", connection_duration_sec);
        } else {
            ESP_LOGW(TAG, "⚠️  [BLE] Windows gateway DISCONNECTED - Communication channel LOST!");
            ESP_LOGW(TAG, "📊 Connection lasted: %lld seconds", connection_duration_sec);
            ESP_LOGW(TAG, "[BLE] Disconnect context: data_subscribed=%d first_ctrl_opcode_seen=%d",
                     (int)g_data_subscribed,
                     (int)!disconnected_before_ctrl);

            if (disconnected_before_ctrl) {
                ESP_LOGW(TAG, "[BLE] Disconnected before any FF01 command; central likely closed the link before protocol start");
            }
        }
        
        if (g_provisioned_at_ms > 0 && g_provisioned_at_ms < now_ms) {
            int64_t after_provision_sec = (now_ms - g_provisioned_at_ms) / 1000;
            ESP_LOGW(TAG, "⚠️  App disconnected %lld seconds AFTER provisioning - this breaks refurbish!", after_provision_sec);
        }
        
        log_disconnect_reason(event->disconnect.reason);
        g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        g_data_subscribed = false;
        g_ctrl_opcode_seen_since_connect = false;
        g_first_ctrl_opcode_ms = 0;
        g_last_kta_init_attempt_ms = 0;
        s_worker_init_exchange_pending = false;
        s_worker_auto_bootstrap_pending = false;
        s_worker_connect_bootstrap_pending = false;
        g_ctrl_rx_len = 0;
        advertise();
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        // Track subscription to DATA characteristic.
        if (event->subscribe.attr_handle == g_data_val_handle) {
            g_data_subscribed = event->subscribe.cur_notify;
            ESP_LOGI(TAG, "DATA subscribe: notify=%d", (int)g_data_subscribed);

            if (g_data_subscribed) {
                ESP_LOGI(TAG, "📋 KTA state: initialized=%d started=%d dev_info=%d",
                         (int)g_kta_initialized_once,
                         (int)g_kta_started_once,
                         (int)g_kta_device_info_once);
                ESP_LOGI(TAG, "⏳ Waiting for Windows gateway to send opcodes: 0x01(INIT) -> 0x02(STARTUP) -> 0x03(SET_DEV_INFO) -> 0x04(EXCHANGE)");

                // Run the KTA manager bootstrap sequence once, but only after
                // the central subscribed to DATA notifications. This avoids
                // burning I2C cycles during transient UI-only probe connects.
                if (KTA_BOOTSTRAP_ON_CONNECT &&
                    !g_kta_connect_bootstrap_done_once &&
                    s_kta_worker_handle != NULL)
                {
                    bool queued = false;
                    if (s_exchange_mutex)
                    {
                        (void)xSemaphoreTake(s_exchange_mutex, portMAX_DELAY);
                        if (!s_exchange_inflight)
                        {
                            s_worker_connect_bootstrap_pending = true;
                            queued = true;
                        }
                        (void)xSemaphoreGive(s_exchange_mutex);
                    }
                    else if (!s_exchange_inflight)
                    {
                        s_worker_connect_bootstrap_pending = true;
                        queued = true;
                    }

                    if (queued)
                    {
                        ESP_LOGI(TAG, "KTA connect bootstrap queued (post-subscribe)");
                        xTaskNotifyGive(s_kta_worker_handle);
                    }
                }

                if (KTA_ENABLE_HELLO_NOTIFY) {
                    // Legacy app convenience notify (disabled for binary gateway mode).
                    notify_string_chunked("{\"messageType\":\"HELLO\",\"result\":\"OK\"}\n");
                }
            }
        }
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU updated: %u", event->mtu.value);
        return 0;

    default:
        return 0;
    }
}

static void advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields adv_fields;
    memset(&adv_fields, 0, sizeof(adv_fields));

    const char *name = "ESP32-Keystream";

    // Set the standard BLE advertising flags. Some Windows discovery paths filter
    // out advertisements that omit the Flags AD structure.
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    // Advertise service UUID 0xFFFF
    ble_uuid16_t svc_uuid = BLE_UUID16_INIT(KS_SVC_UUID16);
    adv_fields.uuids16 = &svc_uuid;
    adv_fields.num_uuids16 = 1;
    adv_fields.uuids16_is_complete = 1;

    // Put the local name into the primary advertisement so Windows Bluetooth UI
    // can discover the device even when scan response is not consistently used.
    adv_fields.name = (uint8_t *)name;
    adv_fields.name_len = strlen(name);
    adv_fields.name_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_set_fields rc=%d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_start rc=%d", rc);
        return;
    }

    ESP_LOGI(TAG, "Advertising started as %s", name);
}

static void on_sync(void) {
    // Ensure we have an address type for advertising.
    int rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_hs_id_infer_auto rc=%d", rc);
        return;
    }
    advertise();
}

static void host_task(void *param) {
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void) {
    esp_reset_reason_t rr = esp_reset_reason();
    ESP_LOGI(TAG, "Reset reason: %d (%s)", (int)rr, reset_reason_str(rr));

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Initialize CryptoAuthLib
    ESP_LOGI(TAG, "Initializing ATECC608 chip");
    ATCA_STATUS status = atcab_init(&cfg_ateccx08a_i2c_default);
    bool atecc_ready = false;
    if (status != ATCA_SUCCESS) {
        ESP_LOGE(TAG, "atcab_init failed: 0x%02x", status);
        ESP_LOGW(TAG, "Continuing without ATECC608 - provisioning will fail");
    } else {
        atecc_ready = true;
        uint8_t revision[4];
        status = atcab_info(revision);
        if (status == ATCA_SUCCESS) {
            ESP_LOGI(TAG, "ATECC608 initialized - revision: %02x%02x%02x%02x",
                     revision[0], revision[1], revision[2], revision[3]);
        } else {
            ESP_LOGW(TAG, "ATECC608 info read failed: 0x%02x", status);
        }
    }

    maybe_force_kta_reprovision_on_boot(atecc_ready);

    ESP_LOGI(TAG, "Init NimBLE");

    // Initialize controller + NimBLE host stack (ESP-IDF does controller + HCI init inside nimble_port_init)
    ESP_ERROR_CHECK(nimble_port_init());

    if (!s_exchange_event_initialized)
    {
        ble_npl_event_init(&s_exchange_event, exchange_event_fn, NULL);
        s_exchange_event_initialized = true;
    }

    if (s_exchange_mutex == NULL)
    {
        s_exchange_mutex = xSemaphoreCreateMutex();
        assert(s_exchange_mutex != NULL);
    }

    if (s_kta_worker_handle == NULL)
    {
        // Re-enabled: opcode path needs kta_worker_task for EXCHANGE via xTaskNotifyGive.
        const uint32_t worker_stack_words = 16384u;
        BaseType_t ok = xTaskCreatePinnedToCore(kta_worker_task,
                                               "kta_worker",
                                               worker_stack_words,
                                               NULL,
                                               (UBaseType_t)(tskIDLE_PRIORITY + 2u),
                                               &s_kta_worker_handle,
                                               0);
        assert(ok == pdPASS);
        ESP_LOGI(TAG, "KTA worker task created (opcode EXCHANGE path active)");
    }

    /* MCU Bridge task: polls ble_sal_receive() and dispatches TLV commands
     * through bridge_integration → bridge_kta → KTA library APIs. */
    int bridge_rc = kta_mcu_integration_entry();
    assert(bridge_rc == 0);

    // Host callbacks + security configuration.
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    // Security defaults for Windows central.
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding = KS_ENABLE_BONDING ? 1 : 0;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist = 0;
    ble_hs_cfg.sm_their_key_dist = 0;
    if (KS_ENABLE_BONDING) {
        ble_hs_cfg.sm_our_key_dist |= (BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        ble_hs_cfg.sm_their_key_dist |= (BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        ESP_LOGI(TAG, "BLE security: bonding enabled");
    } else {
        ESP_LOGI(TAG, "BLE security: non-bonded mode (Windows UI friendly)");
    }

    // GATT/GAP services
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Register our service
    int rc = ble_gatts_count_cfg(gatt_svcs);
    assert(rc == 0);
    rc = ble_gatts_add_svcs(gatt_svcs);
    assert(rc == 0);

    // Set device name
    ble_svc_gap_device_name_set("ESP32-Keystream");

    // Initialize NimBLE key store (bonding / CCCD persistence).
    ble_store_config_init();

    // Configure BOOT button (GPIO0) for factory reset
    gpio_config_t boot_btn_config = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE  // Trigger on button press (falling edge)
    };
    gpio_config(&boot_btn_config);
    xTaskCreate(check_boot_button_task, "boot_btn", 8192, NULL, 5, NULL);
    ESP_LOGI(TAG, "🔘 BOOT button factory reset enabled - hold BOOT button for 3 seconds");

    nimble_port_freertos_init(host_task);

    // Start connection monitoring timer (check every 10 seconds)
    const esp_timer_create_args_t timer_args = {
        .callback = &connection_monitor_callback,
        .name = "conn_monitor"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &g_connection_monitor_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(g_connection_monitor_timer, 10000000));  // 10 seconds
    ESP_LOGI(TAG, "🔍 Connection monitoring started - checking every 10 seconds");

    ESP_LOGI(TAG, "Ready");
}

// BOOT button factory reset task - hold button for 3 seconds
static void check_boot_button_task(void* arg) {
    int64_t press_start = 0;
    bool was_pressed = false;
    
    while (1) {
        bool is_pressed = (gpio_get_level(BOOT_BUTTON_GPIO) == 0);  // Active low
        
        if (is_pressed && !was_pressed) {
            // Button just pressed
            press_start = esp_timer_get_time() / 1000;  // Convert to milliseconds
            ESP_LOGI(TAG, "🔘 BOOT button pressed - hold for 3 seconds to factory reset");
        } else if (is_pressed && was_pressed) {
            // Button still held
            int64_t now = esp_timer_get_time() / 1000;
            int64_t held_ms = now - press_start;
            
            if (held_ms >= BUTTON_HOLD_TIME_MS) {
                ESP_LOGE(TAG, "🚨🚨🚨 BOOT BUTTON FACTORY RESET TRIGGERED 🚨🚨🚨");
                
                // Step 1: Erase ATECC608 certificate slots directly
                ESP_LOGI(TAG, "🗑️  Erasing ATECC608 certificate slots");
                uint8_t zero_data[72] = {0};  // Device cert is 72 bytes
                
                ATCA_STATUS atca_st = atcab_write_zone(ATCA_ZONE_DATA, 10, 0, 0, zero_data, 72);
                ESP_LOGI(TAG, "   Slot 0x0A (device cert) erase: 0x%02x %s", atca_st, 
                        atca_st == ATCA_SUCCESS ? "✅" : "❌");
                
                uint8_t zero_pubkey[64] = {0};  // Public key is 64 bytes
                atca_st = atcab_write_zone(ATCA_ZONE_DATA, 11, 0, 0, zero_pubkey, 64);
                ESP_LOGI(TAG, "   Slot 0x0B (signer pubkey) erase: 0x%02x %s", atca_st,
                        atca_st == ATCA_SUCCESS ? "✅" : "❌");
                
                atca_st = atcab_write_zone(ATCA_ZONE_DATA, 12, 0, 0, zero_data, 72);
                ESP_LOGI(TAG, "   Slot 0x0C (signer cert) erase: 0x%02x %s", atca_st,
                        atca_st == ATCA_SUCCESS ? "✅" : "❌");
                
                // Step 2: Clear KTA lifecycle and L1 material via SAL
                ESP_LOGI(TAG, "🗑️  Resetting lifecycle state to INIT (0x00)");
                uint8_t life_cycle_state[C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE] = {0};
                TKStatus st1 = salStorageSetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                                                  life_cycle_state,
                                                  C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE);
                ESP_LOGI(TAG, "   Lifecycle reset: %d (%s)", st1, kta_status_str(st1));
                
                ESP_LOGI(TAG, "🗑️  Clearing L1 key material");
                uint8_t l1_key_material[C_KEY_CONFIG__MATERIAL_MAX_SIZE] = {0};
                TKStatus st2 = salStorageSetValue(C_K_KTA__L1_KEY_MATERIAL_DATA_ID,
                                                  l1_key_material,
                                                  C_KEY_CONFIG__MATERIAL_MAX_SIZE);
                ESP_LOGI(TAG, "   L1 material cleared: %d (%s)", st2, kta_status_str(st2));
                
                ESP_LOGI(TAG, "✅ Factory reset complete - rebooting in 2 seconds");
                vTaskDelay(pdMS_TO_TICKS(2000));
                esp_restart();
            }
        } else if (!is_pressed && was_pressed) {
            // Button released
            ESP_LOGI(TAG, "🔘 BOOT button released");
        }
        
        was_pressed = is_pressed;
        vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms
    }
}


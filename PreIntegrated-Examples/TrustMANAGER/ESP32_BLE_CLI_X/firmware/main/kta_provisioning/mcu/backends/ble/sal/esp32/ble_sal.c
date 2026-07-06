/**
 * @file ble_sal.c
 * @brief BLE SAL Implementation for ESP32 â€” Shared NimBLE Mode
 *
 * This implementation does NOT own the BLE stack. The main application
 * (main.c) initialises NimBLE and the GATT service. This SAL hooks into
 * the existing NimBLE context by:
 *   - Receiving FF01 write data via ble_sal_enqueue_rx() (called from GATT callback)
 *   - Sending FF02 notifications via ble_gatts_notify_custom() using shared handles
 *
 * Shared state imported from main.c:
 *   extern uint16_t g_conn_handle;      â€” active connection handle
 *   extern uint16_t g_data_val_handle;  â€” FF02 characteristic value handle
 */

#include "../../ble_sal.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "host/ble_hs.h"
#include "esp_log.h"

static const char *TAG = "BLE_SAL";

/* ============================================================================
 * Shared handles from main.c (NimBLE owns these)
 * ============================================================================ */

extern uint16_t g_conn_handle;
extern uint16_t g_data_val_handle;

/* ============================================================================
 * Internal State
 * ============================================================================ */

#define BLE_SAL_RX_QUEUE_DEPTH  8
#define BLE_SAL_RX_ITEM_SIZE    512

typedef struct {
    uint8_t  data[BLE_SAL_RX_ITEM_SIZE];
    uint16_t length;
} BleSalRxItem;

static QueueHandle_t  s_rx_queue    = NULL;
static bool           s_initialised = false;
static BleCallbacks   s_callbacks   = {0};

/* ============================================================================
 * Public API called by main.c GATT write callback
 * ============================================================================ */

/**
 * @brief Enqueue raw bytes received on FF01 so bridge_integration can poll them.
 *
 * Called from the NimBLE host task context (gatt_access_cb).
 * Must be non-blocking.
 */
void ble_sal_enqueue_rx(const uint8_t *data, size_t length)
{
    if (!s_rx_queue || !data || length == 0) {
        return;
    }

    BleSalRxItem item;
    item.length = (length > sizeof(item.data)) ? sizeof(item.data) : (uint16_t)length;
    memcpy(item.data, data, item.length);

    /* Best-effort: drop if queue full (should not happen at BLE rates) */
    if (xQueueSend(s_rx_queue, &item, 0) != pdTRUE) {
        ESP_LOGW(TAG, "RX queue full â€” dropping %u bytes", (unsigned)item.length);
    }
}

/* ============================================================================
 * BLE SAL Interface Implementation
 * ============================================================================ */

BleSalStatus ble_sal_init(const BleConfig *config, const BleCallbacks *callbacks)
{
    (void)config;  /* BLE stack owned by main.c â€” config ignored */

    if (s_initialised) {
        return BLE_SAL_OK;
    }

    if (callbacks) {
        s_callbacks = *callbacks;
    }

    s_rx_queue = xQueueCreate(BLE_SAL_RX_QUEUE_DEPTH, sizeof(BleSalRxItem));
    if (!s_rx_queue) {
        ESP_LOGE(TAG, "Failed to create RX queue");
        return BLE_SAL_ERROR;
    }

    s_initialised = true;
    ESP_LOGI(TAG, "Initialised (shared NimBLE mode)");
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_deinit(void)
{
    if (!s_initialised) {
        return BLE_SAL_NOT_INITIALIZED;
    }

    if (s_rx_queue) {
        vQueueDelete(s_rx_queue);
        s_rx_queue = NULL;
    }

    s_initialised = false;
    ESP_LOGI(TAG, "Deinitialised");
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_start_advertising(void)
{
    /* Advertising is managed by main.c */
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_stop_advertising(void)
{
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_connect(const uint8_t addr[6], uint32_t timeout_ms)
{
    (void)addr;
    (void)timeout_ms;
    /* Connection management is owned by main.c */
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_disconnect(void)
{
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_send(const uint8_t *data, size_t length)
{
    if (!s_initialised) {
        return BLE_SAL_NOT_INITIALIZED;
    }
    if (!data || length == 0) {
        return BLE_SAL_INVALID_PARAM;
    }
    if (g_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        return BLE_SAL_NOT_CONNECTED;
    }

    /* Send as chunked BLE notifications respecting ATT MTU */
    uint16_t mtu = ble_att_mtu(g_conn_handle);
    uint16_t max_payload = (mtu > 3) ? (mtu - 3) : 20;
    const uint8_t *ptr = data;
    size_t remaining = length;

    while (remaining > 0) {
        size_t chunk = (remaining > max_payload) ? max_payload : remaining;

        struct os_mbuf *om = ble_hs_mbuf_from_flat(ptr, chunk);
        if (!om) {
            ESP_LOGE(TAG, "mbuf alloc failed");
            return BLE_SAL_ERROR;
        }

        int rc = ble_gatts_notify_custom(g_conn_handle, g_data_val_handle, om);
        if (rc != 0) {
            ESP_LOGE(TAG, "notify_custom failed: %d", rc);
            return BLE_SAL_ERROR;
        }

        ptr       += chunk;
        remaining -= chunk;

        if (remaining > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));  /* pace chunks like main.c */
        }
    }

    return BLE_SAL_OK;
}

BleSalStatus ble_sal_receive(uint8_t *buffer, size_t buffer_size,
                             size_t *bytes_read, uint32_t timeout_ms)
{
    if (!s_initialised) {
        return BLE_SAL_NOT_INITIALIZED;
    }
    if (!buffer || !bytes_read) {
        return BLE_SAL_INVALID_PARAM;
    }

    *bytes_read = 0;

    BleSalRxItem item;
    TickType_t ticks = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);

    if (xQueueReceive(s_rx_queue, &item, ticks) != pdTRUE) {
        return BLE_SAL_TIMEOUT;
    }

    size_t copy_len = (item.length > buffer_size) ? buffer_size : item.length;
    memcpy(buffer, item.data, copy_len);
    *bytes_read = copy_len;

    return BLE_SAL_OK;
}

BleSalStatus ble_sal_get_state(BleState *state)
{
    if (!state) {
        return BLE_SAL_INVALID_PARAM;
    }

    if (g_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        *state = BLE_STATE_CONNECTED;
    } else {
        *state = BLE_STATE_DISCONNECTED;
    }

    return BLE_SAL_OK;
}

BleSalStatus ble_sal_get_mtu(uint16_t *mtu)
{
    if (!mtu) {
        return BLE_SAL_INVALID_PARAM;
    }

    if (g_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        *mtu = ble_att_mtu(g_conn_handle);
    } else {
        *mtu = 23;  /* default */
    }

    return BLE_SAL_OK;
}

BleSalStatus ble_sal_set_tx_power(int8_t power_dbm)
{
    (void)power_dbm;
    /* TX power managed by main.c */
    return BLE_SAL_OK;
}


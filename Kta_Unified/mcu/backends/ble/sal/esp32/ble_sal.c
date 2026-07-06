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
 * @file ble_sal_esp32.c
 * @brief BLE SAL Implementation for ESP32 (ESP-IDF)
 * 
 * ESP32-specific implementation of the BLE SAL interface.
 * Uses ESP-IDF Bluedroid/NimBLE stack APIs.
 */

#include "../../ble_sal.h"
#include "ble_config.h"
#include <string.h>

/* ESP32 BLE includes */

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "BLE_SAL";

/* ============================================================================
 * BLE Service/Characteristic UUIDs from Config
 * ============================================================================ */

/* Service UUID: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E (Nordic UART Service compatible) */
static const uint8_t SERVICE_UUID[16] = BLE_SERVICE_UUID_BYTES;

/* TX Characteristic UUID: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E */
static const uint8_t TX_CHAR_UUID[16] = BLE_TX_CHAR_UUID_BYTES;

/* RX Characteristic UUID: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E */
static const uint8_t RX_CHAR_UUID[16] = BLE_RX_CHAR_UUID_BYTES;

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool g_ble_initialized = false;
static BleState g_ble_state = BLE_STATE_IDLE;
static uint16_t g_conn_id = 0;
static uint16_t g_mtu = BLE_DEFAULT_MTU;  /* Default BLE MTU from config */
static BleCallbacks g_callbacks = {0};

static esp_gatt_if_t g_gatts_if = ESP_GATT_IF_NONE;
static uint16_t g_service_handle = 0;
static uint16_t g_tx_char_handle = 0;
static uint16_t g_rx_char_handle = 0;

/* Receive queue for data */
static QueueHandle_t g_rx_queue = NULL;

typedef struct {
    uint8_t data[BLE_RX_DATA_BUFFER_SIZE];
    size_t length;
} BleRxData;

/* ============================================================================
 * GAP Event Handler
 * ============================================================================ */

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "Advertising data set complete");
            esp_ble_gap_start_advertising(&param->adv_data_cmpl.adv_data_raw);
            break;
            
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "Advertising started");
                g_ble_state = BLE_STATE_ADVERTISING;
            } else {
                ESP_LOGE(TAG, "Advertising start failed: %d", param->adv_start_cmpl.status);
            }
            break;
            
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            ESP_LOGI(TAG, "Advertising stopped");
            if (g_ble_state == BLE_STATE_ADVERTISING) {
                g_ble_state = BLE_STATE_IDLE;
            }
            break;
            
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(TAG, "Connection params updated: interval=%d, latency=%d, timeout=%d",
                     param->update_conn_params.interval,
                     param->update_conn_params.latency,
                     param->update_conn_params.timeout);
            break;
            
        default:
            break;
    }
}

/* ============================================================================
 * GATT Server Event Handler
 * ============================================================================ */

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:
            ESP_LOGI(TAG, "GATT server registered: app_id=%d", param->reg.app_id);
            g_gatts_if = gatts_if;
            
            /* Create GATT service */
            esp_ble_gatts_create_service(gatts_if, (esp_gatt_srvc_id_t *)SERVICE_UUID, 10);
            break;
            
        case ESP_GATTS_CREATE_EVT:
            ESP_LOGI(TAG, "Service created: handle=%d", param->create.service_handle);
            g_service_handle = param->create.service_handle;
            esp_ble_gatts_start_service(g_service_handle);
            
            /* Add TX characteristic (notify) */
            esp_ble_gatts_add_char(g_service_handle, (esp_bt_uuid_t *)TX_CHAR_UUID,
                                   ESP_GATT_PERM_READ,
                                   ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                                   NULL, NULL);
            break;
            
        case ESP_GATTS_ADD_CHAR_EVT:
            ESP_LOGI(TAG, "Characteristic added: handle=%d", param->add_char.attr_handle);
            if (!g_tx_char_handle) {
                g_tx_char_handle = param->add_char.attr_handle;
                /* Add RX characteristic (write) */
                esp_ble_gatts_add_char(g_service_handle, (esp_bt_uuid_t *)RX_CHAR_UUID,
                                       ESP_GATT_PERM_WRITE,
                                       ESP_GATT_CHAR_PROP_BIT_WRITE,
                                       NULL, NULL);
            } else {
                g_rx_char_handle = param->add_char.attr_handle;
            }
            break;
            
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "BLE connected: conn_id=%d", param->connect.conn_id);
            g_conn_id = param->connect.conn_id;
            g_ble_state = BLE_STATE_CONNECTED;
            
            if (g_callbacks.on_connected) {
                g_callbacks.on_connected();
            }
            break;
            
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "BLE disconnected");
            g_ble_state = BLE_STATE_DISCONNECTED;
            
            if (g_callbacks.on_disconnected) {
                g_callbacks.on_disconnected();
            }
            
            /* Restart advertising */
            ble_sal_start_advertising();
            break;
            
        case ESP_GATTS_WRITE_EVT:
            if (param->write.handle == g_rx_char_handle) {
                ESP_LOGI(TAG, "Data received: %d bytes", param->write.len);
                
                /* Queue received data */
                if (g_rx_queue) {
                    BleRxData rx_data;
                    rx_data.length = (param->write.len > sizeof(rx_data.data)) ? 
                                     sizeof(rx_data.data) : param->write.len;
                    memcpy(rx_data.data, param->write.value, rx_data.length);
                    xQueueSend(g_rx_queue, &rx_data, 0);
                }
                
                if (g_callbacks.on_data_received) {
                    g_callbacks.on_data_received(param->write.value, param->write.len);
                }
            }
            break;
            
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(TAG, "MTU exchanged: %d", param->mtu.mtu);
            g_mtu = param->mtu.mtu;
            break;
            
        default:
            break;
    }
}

/* ============================================================================
 * BLE SAL Implementation for ESP32
 * ============================================================================ */

BleSalStatus ble_sal_init(const BleConfig *config, const BleCallbacks *callbacks)
{
    if (!config) {
        return BLE_SAL_INVALID_PARAM;
    }
    
    if (g_ble_initialized) {
        ESP_LOGW(TAG, "BLE already initialized");
        return BLE_SAL_OK;
    }
    
    /* Save callbacks */
    if (callbacks) {
        g_callbacks = *callbacks;
    }
    
    /* Create RX queue */
    g_rx_queue = xQueueCreate(BLE_RX_QUEUE_SIZE, sizeof(BleRxData));
    if (!g_rx_queue) {
        ESP_LOGE(TAG, "Failed to create RX queue");
        return BLE_SAL_ERROR;
    }
    
    /* Initialize Bluetooth controller */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth controller init failed: %s", esp_err_to_name(ret));
        return BLE_SAL_ERROR;
    }
    
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return BLE_SAL_ERROR;
    }
    
    /* Initialize Bluedroid stack */
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return BLE_SAL_ERROR;
    }
    
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return BLE_SAL_ERROR;
    }
    
    /* Register callbacks */
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_register_callback(gatts_event_handler);
    
    /* Register GATT server app */
    esp_ble_gatts_app_register(0);
    
    g_ble_initialized = true;
    g_ble_state = BLE_STATE_IDLE;
    
    ESP_LOGI(TAG, "BLE initialized: %s", config->device_name);
    
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_deinit(void)
{
    if (!g_ble_initialized) {
        return BLE_SAL_NOT_INITIALIZED;
    }
    
    if (g_rx_queue) {
        vQueueDelete(g_rx_queue);
        g_rx_queue = NULL;
    }
    
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    
    g_ble_initialized = false;
    g_ble_state = BLE_STATE_IDLE;
    
    ESP_LOGI(TAG, "BLE deinitialized");
    
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_start_advertising(void)
{
    if (!g_ble_initialized) {
        return BLE_SAL_NOT_INITIALIZED;
    }
    
    /* Set advertising parameters */
    esp_ble_adv_params_t adv_params = {
        .adv_int_min = 0x20,
        .adv_int_max = 0x40,
        .adv_type = ADV_TYPE_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };
    
    esp_ble_gap_start_advertising(&adv_params);
    
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_stop_advertising(void)
{
    if (!g_ble_initialized) {
        return BLE_SAL_NOT_INITIALIZED;
    }
    
    esp_ble_gap_stop_advertising();
    
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_connect(const uint8_t addr[6], uint32_t timeout_ms)
{
    /* Central role connection - not implemented in this peripheral example */
    return BLE_SAL_ERROR;
}

BleSalStatus ble_sal_disconnect(void)
{
    if (!g_ble_initialized) {
        return BLE_SAL_NOT_INITIALIZED;
    }
    
    if (g_ble_state != BLE_STATE_CONNECTED) {
        return BLE_SAL_NOT_CONNECTED;
    }
    
    esp_ble_gatts_close(g_gatts_if, g_conn_id);
    
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_send(const uint8_t *data, size_t length)
{
    if (!g_ble_initialized) {
        return BLE_SAL_NOT_INITIALIZED;
    }
    
    if (g_ble_state != BLE_STATE_CONNECTED) {
        return BLE_SAL_NOT_CONNECTED;
    }
    
    if (!data || length == 0) {
        return BLE_SAL_INVALID_PARAM;
    }
    
    if (length > (g_mtu - 3)) {
        ESP_LOGW(TAG, "Data length %d exceeds MTU %d", length, g_mtu);
        return BLE_SAL_MTU_EXCEEDED;
    }
    
    /* Send notification */
    esp_err_t ret = esp_ble_gatts_send_indicate(g_gatts_if, g_conn_id, g_tx_char_handle,
                                                 length, (uint8_t *)data, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send notification: %s", esp_err_to_name(ret));
        return BLE_SAL_ERROR;
    }
    
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_receive(uint8_t *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms)
{
    if (!g_ble_initialized) {
        return BLE_SAL_NOT_INITIALIZED;
    }
    
    if (!buffer || !bytes_read || buffer_size == 0) {
        return BLE_SAL_INVALID_PARAM;
    }
    
    if (!g_rx_queue) {
        return BLE_SAL_ERROR;
    }
    
    BleRxData rx_data;
    TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
    
    if (xQueueReceive(g_rx_queue, &rx_data, ticks) == pdTRUE) {
        size_t copy_len = (rx_data.length > buffer_size) ? buffer_size : rx_data.length;
        memcpy(buffer, rx_data.data, copy_len);
        *bytes_read = copy_len;
        return BLE_SAL_OK;
    }
    
    *bytes_read = 0;
    return BLE_SAL_TIMEOUT;
}

BleSalStatus ble_sal_get_state(BleState *state)
{
    if (!state) {
        return BLE_SAL_INVALID_PARAM;
    }
    
    *state = g_ble_state;
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_get_mtu(uint16_t *mtu)
{
    if (!mtu) {
        return BLE_SAL_INVALID_PARAM;
    }
    
    *mtu = g_mtu;
    return BLE_SAL_OK;
}

BleSalStatus ble_sal_set_tx_power(int8_t power_dbm)
{
    if (!g_ble_initialized) {
        return BLE_SAL_NOT_INITIALIZED;
    }
    
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, power_dbm);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN, power_dbm);
    
    ESP_LOGI(TAG, "TX power set to %d dBm", power_dbm);
    
    return BLE_SAL_OK;
}
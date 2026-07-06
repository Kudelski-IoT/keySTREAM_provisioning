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
 * @file uart_sal_esp32.c
 * @brief UART SAL Implementation for ESP32 (ESP-IDF)
 * 
 * ESP32-specific implementation of the UART SAL interface.
 * Uses ESP-IDF UART driver APIs.
 */

#include "../../uart_sal.h"
#include "uart_config.h"
#include <string.h>

/* ESP32 UART driver includes */
#ifdef ESP_PLATFORM
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "UART_SAL";

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool g_uart_initialized = false;
static uart_port_t g_uart_port = UART_NUM_0;
static uint32_t g_rx_timeout_ms = 1000;

/* ============================================================================
 * UART SAL Implementation for ESP32
 * ============================================================================ */

UartSalStatus uart_sal_init(const UartConfig *config)
{
    if (!config) {
        return UART_SAL_INVALID_PARAM;
    }
    
    if (g_uart_initialized) {
        ESP_LOGW(TAG, "UART already initialized");
        return UART_SAL_OK;
    }
    
    /* Map generic UART port to ESP32 UART port */
    switch (config->port_num) {
        case 0: g_uart_port = UART_NUM_0; break;
        case 1: g_uart_port = UART_NUM_1; break;
        case 2: g_uart_port = UART_NUM_2; break;
        default:
            ESP_LOGE(TAG, "Invalid UART port: %d", config->port_num);
            return UART_SAL_INVALID_PARAM;
    }
    
    /* Configure ESP32 UART parameters */
    uart_config_t uart_config = {
        .baud_rate = config->baud_rate,
        .data_bits = (config->data_bits == UART_DATA_BITS_8) ? UART_DATA_8_BITS : UART_DATA_7_BITS,
        .parity = (config->parity == UART_PARITY_NONE) ? UART_PARITY_DISABLE :
                  (config->parity == UART_PARITY_EVEN) ? UART_PARITY_EVEN : UART_PARITY_ODD,
        .stop_bits = (config->stop_bits == UART_STOP_BITS_1) ? UART_STOP_BITS_1 : UART_STOP_BITS_2,
        .flow_ctrl = config->flow_control ? UART_HW_FLOWCTRL_CTS_RTS : UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = UART_RX_FLOW_CTRL_THRESH,
        .source_clk = UART_SCLK_APB,
    };
    
    /* Set UART parameters */
    esp_err_t err = uart_param_config(g_uart_port, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(err));
        return UART_SAL_ERROR;
    }
    
    /* Set UART pins from config */
#if UART_USE_DEFAULT_PINS
    /* Use board default pins (no change) */
    uart_set_pin(g_uart_port, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
#else
    /* Use configured pins based on UART port */
    if (g_uart_port == UART_NUM_0) {
        /* UART0: Use configured pins or defaults */
        uart_set_pin(g_uart_port, UART_TX_PIN, UART_RX_PIN, 
                     config->flow_control ? UART_RTS_PIN : UART_PIN_NO_CHANGE,
                     config->flow_control ? UART_CTS_PIN : UART_PIN_NO_CHANGE);
    } else if (g_uart_port == UART_NUM_1) {
        /* UART1: Use alternate pins from config */
        uart_set_pin(g_uart_port, UART1_TX_PIN, UART1_RX_PIN, 
                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    } else if (g_uart_port == UART_NUM_2) {
        /* UART2: Use alternate pins from config */
        uart_set_pin(g_uart_port, UART2_TX_PIN, UART2_RX_PIN, 
                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
#endif
    
    /* Install UART driver with RX/TX buffers */
    err = uart_driver_install(g_uart_port, 
                              config->rx_buffer_size ? config->rx_buffer_size : 2048,
                              config->tx_buffer_size ? config->tx_buffer_size : 2048,
                              0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(err));
        return UART_SAL_ERROR;
    }
    
    g_uart_initialized = true;
    ESP_LOGI(TAG, "UART%d initialized: %d baud, %d%c%d", 
             g_uart_port, config->baud_rate, 
             config->data_bits,
             (config->parity == UART_PARITY_NONE) ? 'N' : 
             (config->parity == UART_PARITY_EVEN) ? 'E' : 'O',
             config->stop_bits == UART_STOP_BITS_1 ? 1 : 2);
    
    return UART_SAL_OK;
}

UartSalStatus uart_sal_deinit(void)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }
    
    esp_err_t err = uart_driver_delete(g_uart_port);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to delete UART driver: %s", esp_err_to_name(err));
        return UART_SAL_ERROR;
    }
    
    g_uart_initialized = false;
    ESP_LOGI(TAG, "UART%d deinitialized", g_uart_port);
    
    return UART_SAL_OK;
}

UartSalStatus uart_sal_write(const uint8_t *data, size_t length, uint32_t timeout_ms)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }
    
    if (!data || length == 0) {
        return UART_SAL_INVALID_PARAM;
    }
    
    /* ESP-IDF uart_write_bytes uses ticks for timeout */
    TickType_t ticks = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);
    
    int written = uart_write_bytes(g_uart_port, data, length);
    if (written < 0) {
        ESP_LOGE(TAG, "UART write failed");
        return UART_SAL_ERROR;
    }
    
    if ((size_t)written != length) {
        ESP_LOGW(TAG, "UART write incomplete: %d/%d bytes", written, length);
        return UART_SAL_TIMEOUT;
    }
    
    return UART_SAL_OK;
}

UartSalStatus uart_sal_read(uint8_t *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }
    
    if (!buffer || !bytes_read || buffer_size == 0) {
        return UART_SAL_INVALID_PARAM;
    }
    
    /* ESP-IDF uart_read_bytes uses ticks for timeout */
    TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
    
    int length = uart_read_bytes(g_uart_port, buffer, buffer_size, ticks);
    if (length < 0) {
        ESP_LOGE(TAG, "UART read failed");
        *bytes_read = 0;
        return UART_SAL_ERROR;
    }
    
    *bytes_read = (size_t)length;
    
    if (length == 0 && timeout_ms > 0) {
        return UART_SAL_TIMEOUT;
    }
    
    return UART_SAL_OK;
}

UartSalStatus uart_sal_available(size_t *available)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }
    
    if (!available) {
        return UART_SAL_INVALID_PARAM;
    }
    
    size_t bytes_available = 0;
    esp_err_t err = uart_get_buffered_data_len(g_uart_port, &bytes_available);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get buffered data length: %s", esp_err_to_name(err));
        return UART_SAL_ERROR;
    }
    
    *available = bytes_available;
    return UART_SAL_OK;
}

UartSalStatus uart_sal_flush_tx(uint32_t timeout_ms)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }
    
    TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
    esp_err_t err = uart_wait_tx_done(g_uart_port, ticks);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART TX flush timeout");
        return UART_SAL_TIMEOUT;
    }
    
    return UART_SAL_OK;
}

UartSalStatus uart_sal_flush_rx(void)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }
    
    esp_err_t err = uart_flush_input(g_uart_port);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to flush RX buffer: %s", esp_err_to_name(err));
        return UART_SAL_ERROR;
    }
    
    return UART_SAL_OK;
}

UartSalStatus uart_sal_set_timeout(uint32_t timeout_ms)
{
    g_rx_timeout_ms = timeout_ms;
    return UART_SAL_OK;
}

#else
/* Non-ESP32 platform - provide stub implementation */
UartSalStatus uart_sal_init(const UartConfig *config) { return UART_SAL_ERROR; }
UartSalStatus uart_sal_deinit(void) { return UART_SAL_ERROR; }
UartSalStatus uart_sal_write(const uint8_t *data, size_t length, uint32_t timeout_ms) { return UART_SAL_ERROR; }
UartSalStatus uart_sal_read(uint8_t *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms) { return UART_SAL_ERROR; }
UartSalStatus uart_sal_available(size_t *available) { return UART_SAL_ERROR; }
UartSalStatus uart_sal_flush_tx(uint32_t timeout_ms) { return UART_SAL_ERROR; }
UartSalStatus uart_sal_flush_rx(void) { return UART_SAL_ERROR; }
UartSalStatus uart_sal_set_timeout(uint32_t timeout_ms) { return UART_SAL_ERROR; }
#endif /* ESP_PLATFORM */

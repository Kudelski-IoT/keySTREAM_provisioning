/**
 * @file transport_uart_mcu.c
 * @brief UART Transport Backend for MCU (FreeRTOS/Bare-metal)
 * 
 * MCU-specific UART implementation using platform UART drivers.
 * Adapts to FreeRTOS, Zephyr, bare-metal, or other RTOS.
 */

#include "../backend_interface.h"
#include "uart_sal.h"      /* SAL interface */
#include <string.h>

/* Platform-specific config is included via Makefile:
 * -Ibackends/uart/sal/<platform>
 * This makes uart_config.h visible with platform-specific settings.
 */

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool g_backend_initialized = false;
static bool g_backend_connected = false;
static uint32_t g_backend_timeout_ms = 5000;

/* ============================================================================
 * UART Backend Implementation (MCU)
 * ============================================================================ */

/* Forward declarations */
static BackendStatus uart_backend_close(void);

static BackendStatus uart_backend_init(void)
{
    /* Note: UART configuration is read from uart_config.h at compile-time.
     * The Makefile includes the platform-specific config directory.
     * SAL layer will read config defines directly from uart_config.h.
     */
    
    UartSalStatus status = uart_sal_init(NULL);  /* NULL = use config from uart_config.h */
    if (status != UART_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    g_backend_initialized = true;
    return BACKEND_OK;
}

static BackendStatus uart_backend_deinit(void)
{
    if (g_backend_connected) {
        uart_backend_close();
    }
    
    uart_sal_deinit();
    g_backend_initialized = false;
    return BACKEND_OK;
}

static BackendStatus uart_backend_get_capabilities(BackendCapabilities *caps)
{
    if (!caps) {
        return BACKEND_INVALID_PARAM;
    }
    
    caps->max_packet_size = 1024;       /* MCU buffer size (configurable) */
    caps->max_message_size = 1024;
    caps->supports_fragmentation = false;
    caps->requires_connection = true;
    caps->is_reliable = true;
    caps->is_bidirectional = true;
    
    return BACKEND_OK;
}

static BackendStatus uart_backend_open(const BackendConfig *config)
{
    if (!config || config->type != BACKEND_TYPE_UART) {
        return BACKEND_INVALID_PARAM;
    }
    
    /* Config is already read from uart_config.h by SAL layer at init.
     * No need to pass config here - SAL uses compile-time defines.
     */
    
    g_backend_connected = true;
    return BACKEND_OK;
}

static BackendStatus uart_backend_close(void)
{
    if (!g_backend_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    /* SAL close is handled in deinit */
    g_backend_connected = false;
    return BACKEND_OK;
}

static BackendStatus uart_backend_send(const uint8_t *data, size_t length)
{
    if (!g_backend_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    if (!data || length == 0) {
        return BACKEND_INVALID_PARAM;
    }
    
    UartSalStatus status = uart_sal_write(data, length, 1000);  /* 1 second timeout */
    if (status != UART_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus uart_backend_receive(uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!g_backend_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    if (!buffer || !received_length) {
        return BACKEND_INVALID_PARAM;
    }
    
    UartSalStatus status = uart_sal_read(buffer, buffer_size, received_length, g_backend_timeout_ms);
    
    if (status == UART_SAL_TIMEOUT) {
        return BACKEND_TIMEOUT;
    } else if (status != UART_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus uart_backend_set_timeout(uint32_t timeout_ms)
{
    g_backend_timeout_ms = timeout_ms;
    return BACKEND_OK;
}

/* ============================================================================
 * Backend Registration
 * ============================================================================ */

const Backend g_uart_backend = {
    .type = BACKEND_TYPE_UART,
    .init = uart_backend_init,
    .deinit = uart_backend_deinit,
    .get_capabilities = uart_backend_get_capabilities,
    .open = uart_backend_open,
    .close = uart_backend_close,
    .send = uart_backend_send,
    .receive = uart_backend_receive,
    .set_timeout = uart_backend_set_timeout
};

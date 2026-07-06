/**
 * @file backend_uart_gateway.c
 * @brief UART Backend Implementation for Gateway
 * 
 * Generic UART backend that works across all gateway platforms.
 * Calls platform-specific SAL for actual hardware operations.
 */

#include "backend_interface.h"
#include "uart_sal.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool g_uart_initialized = false;
static bool g_uart_open = false;
static uint32_t g_timeout_ms = 5000;

/* ============================================================================
 * Backend Implementation Functions
 * ============================================================================ */

static BackendStatus uart_backend_init(void)
{
    if (g_uart_initialized) {
        return BACKEND_OK;
    }
    
    UartSalStatus status = uart_sal_init();
    if (status != UART_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    g_uart_initialized = true;
    return BACKEND_OK;
}

static BackendStatus uart_backend_deinit(void)
{
    if (!g_uart_initialized) {
        return BACKEND_OK;
    }
    
    if (g_uart_open) {
        uart_sal_close();
        g_uart_open = false;
    }
    
    uart_sal_deinit();
    g_uart_initialized = false;
    return BACKEND_OK;
}

static BackendStatus uart_backend_get_capabilities(BackendCapabilities *caps)
{
    if (!caps) {
        return BACKEND_INVALID_PARAM;
    }
    
    caps->max_packet_size = 4096;
    caps->max_message_size = 4096;
    caps->supports_fragmentation = false;
    caps->requires_connection = true;
    caps->is_reliable = true;
    caps->is_bidirectional = true;
    
    return BACKEND_OK;
}

static BackendStatus uart_backend_open(const BackendConfig *config)
{
    if (!g_uart_initialized) {
        return BACKEND_ERROR;
    }
    
    if (g_uart_open) {
        uart_sal_close();
    }
    
    /* If config is NULL, uart_sal_open will use defaults from uart_config.h */
    UartSalConfig sal_config;
    if (config && config->type == BACKEND_TYPE_UART) {
        /* Use provided config */
        strncpy(sal_config.port_name, config->config.uart.port_name, sizeof(sal_config.port_name) - 1);
        sal_config.port_name[sizeof(sal_config.port_name) - 1] = '\0';
        sal_config.baud_rate = config->config.uart.baud_rate;
        sal_config.data_bits = config->config.uart.data_bits;
        sal_config.stop_bits = config->config.uart.stop_bits;
        sal_config.parity = config->config.uart.parity;
        sal_config.flow_control = config->config.uart.flow_control;
        
        UartSalStatus status = uart_sal_open(&sal_config);
        if (status != UART_SAL_OK) {
            return BACKEND_ERROR;
        }
    } else {
        /* Use SAL defaults - pass NULL */
        UartSalStatus status = uart_sal_open(NULL);
        if (status != UART_SAL_OK) {
            return BACKEND_ERROR;
        }
    }
    
    uart_sal_set_timeout(g_timeout_ms);
    g_uart_open = true;
    return BACKEND_OK;
}

static BackendStatus uart_backend_close(void)
{
    if (!g_uart_open) {
        return BACKEND_OK;
    }
    
    uart_sal_close();
    g_uart_open = false;
    return BACKEND_OK;
}

static BackendStatus uart_backend_send(const uint8_t *data, size_t length)
{
    if (!data || length == 0) {
        return BACKEND_INVALID_PARAM;
    }
    
    if (!g_uart_open) {
        return BACKEND_NOT_CONNECTED;
    }
    
    size_t written = 0;
    UartSalStatus status = uart_sal_write(data, length, &written);
    
    if (status != UART_SAL_OK) {
        return (status == UART_SAL_TIMEOUT) ? BACKEND_TIMEOUT : BACKEND_ERROR;
    }
    
    if (written != length) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus uart_backend_receive(uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!buffer || buffer_size == 0 || !received_length) {
        return BACKEND_INVALID_PARAM;
    }
    
    if (!g_uart_open) {
        return BACKEND_NOT_CONNECTED;
    }
    
    UartSalStatus status = uart_sal_read(buffer, buffer_size, received_length);
    
    if (status == UART_SAL_TIMEOUT) {
        return BACKEND_TIMEOUT;
    }
    
    if (status != UART_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus uart_backend_set_timeout(uint32_t timeout_ms)
{
    g_timeout_ms = timeout_ms;
    
    if (g_uart_open) {
        UartSalStatus status = uart_sal_set_timeout(timeout_ms);
        if (status != UART_SAL_OK) {
            return BACKEND_ERROR;
        }
    }
    
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
    .set_timeout = uart_backend_set_timeout,
};

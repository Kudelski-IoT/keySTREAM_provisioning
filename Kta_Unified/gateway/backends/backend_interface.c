/**
 * @file backend_interface.c
 * @brief Backend Interface Implementation for Gateway
 * 
 * Provides a unified interface to select and dispatch to different transport backends.
 * Gateway application uses this to communicate with MCU through UART/BLE/USB/Zigbee.
 */

#include "backend_interface.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Internal State
 * ============================================================================ */

static const Backend *g_current_backend = NULL;
static BackendType g_current_type = BACKEND_TYPE_UART;

/* ============================================================================
 * Backend Selection
 * ============================================================================ */

static const Backend* select_backend(BackendType type)
{
    switch (type) {
        case BACKEND_TYPE_UART:
            return &g_uart_backend;
        case BACKEND_TYPE_BLE:
        case BACKEND_TYPE_USB:
        case BACKEND_TYPE_ZIGBEE:
            /* These backends are not implemented for Windows gateway */
            return NULL;
        default:
            return NULL;
    }
}

/* ============================================================================
 * Backend Interface Functions
 * ============================================================================ */

BackendStatus backend_init(BackendType type)
{
    g_current_backend = select_backend(type);
    if (!g_current_backend) {
        return BACKEND_NOT_SUPPORTED;
    }
    
    g_current_type = type;
    return g_current_backend->init();
}

BackendStatus backend_deinit(void)
{
    if (!g_current_backend) {
        return BACKEND_ERROR;
    }
    
    BackendStatus status = g_current_backend->deinit();
    g_current_backend = NULL;
    return status;
}

BackendStatus backend_get_capabilities(BackendCapabilities *caps)
{
    if (!g_current_backend || !caps) {
        return BACKEND_INVALID_PARAM;
    }
    
    return g_current_backend->get_capabilities(caps);
}

BackendStatus backend_open(const BackendConfig *config)
{
    if (!g_current_backend) {
        return BACKEND_INVALID_PARAM;
    }
    
    /* Config can be NULL - SAL will use its default configuration */
    return g_current_backend->open(config);
}

BackendStatus backend_close(void)
{
    if (!g_current_backend) {
        return BACKEND_ERROR;
    }
    
    return g_current_backend->close();
}

BackendStatus backend_send(const uint8_t *data, size_t length)
{
    if (!g_current_backend || !data || length == 0) {
        return BACKEND_INVALID_PARAM;
    }
    
    return g_current_backend->send(data, length);
}

BackendStatus backend_receive(uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!g_current_backend || !buffer || buffer_size == 0 || !received_length) {
        return BACKEND_INVALID_PARAM;
    }
    
    return g_current_backend->receive(buffer, buffer_size, received_length);
}

BackendStatus backend_set_timeout(uint32_t timeout_ms)
{
    if (!g_current_backend) {
        return BACKEND_ERROR;
    }
    
    return g_current_backend->set_timeout(timeout_ms);
}

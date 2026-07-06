/**
 * @file backend_interface.c
 * @brief Backend Interface Implementation for MCU
 * 
 * Provides a unified interface to select and dispatch to different transport backends.
 * This layer acts as a dispatcher/router to the appropriate backend (UART/BLE/USB/Zigbee).
 */

#include "backend_interface.h"
#include <string.h>

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
            return NULL;  /* Not implemented */
        case BACKEND_TYPE_USB:
            return NULL;  /* Not implemented */
        case BACKEND_TYPE_ZIGBEE:
            return NULL;  /* Not implemented */
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
    if (!g_current_backend || !g_current_backend->init) {
        g_current_backend = NULL;
        return BACKEND_NOT_SUPPORTED;
    }

    g_current_type = type;
    BackendStatus rc = g_current_backend->init();
    if (rc != BACKEND_OK) {
        /* Init failed – do not leave a dangling reference */
        g_current_backend = NULL;
    }
    return rc;
}

BackendStatus backend_deinit(void)
{
    if (!g_current_backend) {
        return BACKEND_ERROR;
    }
    BackendStatus status = g_current_backend->deinit
        ? g_current_backend->deinit()
        : BACKEND_OK;
    g_current_backend = NULL;
    return status;
}

BackendStatus backend_get_capabilities(BackendCapabilities *caps)
{
    if (!g_current_backend || !g_current_backend->get_capabilities) {
        return BACKEND_ERROR;
    }

    return g_current_backend->get_capabilities(caps);
}

BackendStatus backend_open(const BackendConfig *config)
{
    if (!g_current_backend || !g_current_backend->open) {
        return BACKEND_ERROR;
    }

    if (!config || config->type != g_current_type) {
        return BACKEND_INVALID_PARAM;
    }

    return g_current_backend->open(config);
}

BackendStatus backend_close(void)
{
    if (!g_current_backend || !g_current_backend->close) {
        return BACKEND_ERROR;
    }

    return g_current_backend->close();
}

BackendStatus backend_send(const uint8_t *data, size_t length)
{
    if (!g_current_backend || !g_current_backend->send) {
        return BACKEND_ERROR;
    }

    return g_current_backend->send(data, length);
}

BackendStatus backend_receive(uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!g_current_backend || !g_current_backend->receive) {
        return BACKEND_ERROR;
    }

    if (!buffer || !received_length || buffer_size == 0) {
        return BACKEND_INVALID_PARAM;
    }

    return g_current_backend->receive(buffer, buffer_size, received_length);
}

BackendStatus backend_set_timeout(uint32_t timeout_ms)
{
    if (!g_current_backend || !g_current_backend->set_timeout) {
        return BACKEND_ERROR;
    }

    return g_current_backend->set_timeout(timeout_ms);
}

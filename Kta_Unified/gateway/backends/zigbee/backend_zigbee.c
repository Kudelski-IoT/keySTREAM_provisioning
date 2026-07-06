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
 * @file backend_zigbee_gateway.c
 * @brief Zigbee Backend Implementation for Gateway
 * 
 * Generic Zigbee backend that works across all gateway platforms.
 * Calls platform-specific SAL for actual Zigbee operations.
 */

#include "backend_interface.h"
#include "zigbee_sal.h"
#include <string.h>

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool g_zigbee_initialized = false;
static bool g_zigbee_connected = false;
static uint32_t g_timeout_ms = 5000;

/* ============================================================================
 * Backend Implementation Functions
 * ============================================================================ */

static BackendStatus zigbee_backend_init(void)
{
    if (g_zigbee_initialized) {
        return BACKEND_OK;
    }
    
    ZigbeeSalStatus status = zigbee_sal_init();
    if (status != ZIGBEE_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    g_zigbee_initialized = true;
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_deinit(void)
{
    if (!g_zigbee_initialized) {
        return BACKEND_OK;
    }
    
    if (g_zigbee_connected) {
        zigbee_sal_disconnect();
        g_zigbee_connected = false;
    }
    
    zigbee_sal_deinit();
    g_zigbee_initialized = false;
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_get_capabilities(BackendCapabilities *caps)
{
    if (!caps) {
        return BACKEND_INVALID_PARAM;
    }
    
    caps->max_packet_size = 127;
    caps->max_message_size = 4096;
    caps->supports_fragmentation = true;
    caps->requires_connection = true;
    caps->is_reliable = false;
    caps->is_bidirectional = true;
    
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_open(const BackendConfig *config)
{
    if (!config || config->type != BACKEND_TYPE_ZIGBEE) {
        return BACKEND_INVALID_PARAM;
    }
    
    if (!g_zigbee_initialized) {
        return BACKEND_ERROR;
    }
    
    if (g_zigbee_connected) {
        zigbee_sal_disconnect();
    }
    
    ZigbeeSalConfig sal_config;
    sal_config.pan_id = config->config.zigbee.pan_id;
    sal_config.channel = config->config.zigbee.channel;
    sal_config.tx_power = config->config.zigbee.tx_power;
    strncpy(sal_config.device_address, config->config.zigbee.device_address, sizeof(sal_config.device_address) - 1);
    sal_config.device_address[sizeof(sal_config.device_address) - 1] = '\0';
    
    ZigbeeSalStatus status = zigbee_sal_connect(&sal_config);
    if (status != ZIGBEE_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    zigbee_sal_set_timeout(g_timeout_ms);
    g_zigbee_connected = true;
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_close(void)
{
    if (!g_zigbee_connected) {
        return BACKEND_OK;
    }
    
    zigbee_sal_disconnect();
    g_zigbee_connected = false;
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_send(const uint8_t *data, size_t length)
{
    if (!data || length == 0) {
        return BACKEND_INVALID_PARAM;
    }
    
    if (!g_zigbee_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    size_t written = 0;
    ZigbeeSalStatus status = zigbee_sal_write(data, length, &written);
    
    if (status != ZIGBEE_SAL_OK) {
        return (status == ZIGBEE_SAL_TIMEOUT) ? BACKEND_TIMEOUT : BACKEND_ERROR;
    }
    
    if (written != length) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_receive(uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!buffer || buffer_size == 0 || !received_length) {
        return BACKEND_INVALID_PARAM;
    }
    
    if (!g_zigbee_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    ZigbeeSalStatus status = zigbee_sal_read(buffer, buffer_size, received_length);
    
    if (status == ZIGBEE_SAL_TIMEOUT) {
        return BACKEND_TIMEOUT;
    }
    
    if (status != ZIGBEE_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_set_timeout(uint32_t timeout_ms)
{
    g_timeout_ms = timeout_ms;
    
    if (g_zigbee_connected) {
        ZigbeeSalStatus status = zigbee_sal_set_timeout(timeout_ms);
        if (status != ZIGBEE_SAL_OK) {
            return BACKEND_ERROR;
        }
    }
    
    return BACKEND_OK;
}

/* ============================================================================
 * Backend Registration
 * ============================================================================ */

const Backend g_zigbee_backend = {
    .type = BACKEND_TYPE_ZIGBEE,
    .init = zigbee_backend_init,
    .deinit = zigbee_backend_deinit,
    .get_capabilities = zigbee_backend_get_capabilities,
    .open = zigbee_backend_open,
    .close = zigbee_backend_close,
    .send = zigbee_backend_send,
    .receive = zigbee_backend_receive,
    .set_timeout = zigbee_backend_set_timeout,
};

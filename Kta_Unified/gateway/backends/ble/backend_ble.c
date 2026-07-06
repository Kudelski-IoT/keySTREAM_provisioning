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
 * @file backend_ble_gateway.c
 * @brief BLE Backend Implementation for Gateway
 * 
 * Generic BLE backend that works across all gateway platforms.
 * Calls platform-specific SAL for actual BLE operations.
 */

#include "backend_interface.h"
#include "ble_sal.h"
#include <string.h>

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool g_ble_initialized = false;
static bool g_ble_connected = false;
static uint32_t g_timeout_ms = 5000;
static uint16_t g_mtu = 23;

/* ============================================================================
 * Backend Implementation Functions
 * ============================================================================ */

static BackendStatus ble_backend_init(void)
{
    if (g_ble_initialized) {
        return BACKEND_OK;
    }
    
    BleSalStatus status = ble_sal_init();
    if (status != BLE_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    g_ble_initialized = true;
    return BACKEND_OK;
}

static BackendStatus ble_backend_deinit(void)
{
    if (!g_ble_initialized) {
        return BACKEND_OK;
    }
    
    if (g_ble_connected) {
        ble_sal_disconnect();
        g_ble_connected = false;
    }
    
    ble_sal_deinit();
    g_ble_initialized = false;
    return BACKEND_OK;
}

static BackendStatus ble_backend_get_capabilities(BackendCapabilities *caps)
{
    if (!caps) {
        return BACKEND_INVALID_PARAM;
    }
    
    caps->max_packet_size = g_mtu - 3;
    caps->max_message_size = 4096;
    caps->supports_fragmentation = true;
    caps->requires_connection = true;
    caps->is_reliable = true;
    caps->is_bidirectional = true;
    
    return BACKEND_OK;
}

static BackendStatus ble_backend_open(const BackendConfig *config)
{
    if (!config || config->type != BACKEND_TYPE_BLE) {
        return BACKEND_INVALID_PARAM;
    }
    
    if (!g_ble_initialized) {
        return BACKEND_ERROR;
    }
    
    if (g_ble_connected) {
        ble_sal_disconnect();
    }
    
    BleSalConfig sal_config;
    strncpy(sal_config.device_address, config->config.ble.device_address, sizeof(sal_config.device_address) - 1);
    sal_config.device_address[sizeof(sal_config.device_address) - 1] = '\0';
    sal_config.mtu = config->config.ble.mtu;
    sal_config.use_bonding = config->config.ble.use_bonding;
    sal_config.scan_timeout_ms = config->config.ble.scan_timeout_ms;
    
    BleSalStatus status = ble_sal_connect(&sal_config);
    if (status != BLE_SAL_OK) {
        return (status == BLE_SAL_NOT_FOUND) ? BACKEND_NOT_CONNECTED : BACKEND_ERROR;
    }
    
    ble_sal_get_mtu(&g_mtu);
    ble_sal_set_timeout(g_timeout_ms);
    g_ble_connected = true;
    return BACKEND_OK;
}

static BackendStatus ble_backend_close(void)
{
    if (!g_ble_connected) {
        return BACKEND_OK;
    }
    
    ble_sal_disconnect();
    g_ble_connected = false;
    return BACKEND_OK;
}

static BackendStatus ble_backend_send(const uint8_t *data, size_t length)
{
    if (!data || length == 0) {
        return BACKEND_INVALID_PARAM;
    }
    
    if (!g_ble_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    size_t written = 0;
    BleSalStatus status = ble_sal_write(data, length, &written);
    
    if (status != BLE_SAL_OK) {
        return (status == BLE_SAL_TIMEOUT) ? BACKEND_TIMEOUT : BACKEND_ERROR;
    }
    
    if (written != length) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus ble_backend_receive(uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!buffer || buffer_size == 0 || !received_length) {
        return BACKEND_INVALID_PARAM;
    }
    
    if (!g_ble_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    BleSalStatus status = ble_sal_read(buffer, buffer_size, received_length);
    
    if (status == BLE_SAL_TIMEOUT) {
        return BACKEND_TIMEOUT;
    }
    
    if (status != BLE_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus ble_backend_set_timeout(uint32_t timeout_ms)
{
    g_timeout_ms = timeout_ms;
    
    if (g_ble_connected) {
        BleSalStatus status = ble_sal_set_timeout(timeout_ms);
        if (status != BLE_SAL_OK) {
            return BACKEND_ERROR;
        }
    }
    
    return BACKEND_OK;
}

/* ============================================================================
 * Backend Registration
 * ============================================================================ */

const Backend g_ble_backend = {
    .type = BACKEND_TYPE_BLE,
    .init = ble_backend_init,
    .deinit = ble_backend_deinit,
    .get_capabilities = ble_backend_get_capabilities,
    .open = ble_backend_open,
    .close = ble_backend_close,
    .send = ble_backend_send,
    .receive = ble_backend_receive,
    .set_timeout = ble_backend_set_timeout,
};

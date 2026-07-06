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
 * @file transport_ble_mcu.c
 * @brief BLE Transport Backend for MCU
 * 
 * MCU-specific BLE implementation for Nordic nRF, ESP32, etc.
 * Handles BLE GATT characteristics and fragmentation.
 */

#include "backend_interface.h"
#include "ble/ble_sal.h"       /* SAL interface */
#include <string.h>

/* Platform-specific config is included via Makefile:
 * -Ibackends/ble/sal/<platform>
 * This makes ble_config.h visible with platform-specific settings.
 */

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool g_ble_initialized = false;
static bool g_ble_connected = false;
static uint16_t g_ble_mtu = 23;  /* Default BLE MTU */

/* Forward declarations */
static BackendStatus ble_backend_close(void);

/* ============================================================================
 * BLE Backend Implementation (MCU)
 * ============================================================================ */

static BackendStatus ble_backend_init(void)
{
    /* Config is read from ble_config.h at compile-time */
    BleSalStatus status = ble_sal_init(NULL, NULL);  /* NULL config + NULL callbacks = use defaults */
    if (status != BLE_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    g_ble_initialized = true;
    return BACKEND_OK;
}

static BackendStatus ble_backend_deinit(void)
{
    if (g_ble_connected) {
        ble_backend_close();
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
    
    caps->max_packet_size = g_ble_mtu - 3;  /* MTU minus ATT header */
    caps->max_message_size = 2048;           /* With fragmentation */
    caps->supports_fragmentation = true;
    caps->requires_connection = true;
    caps->is_reliable = true;                /* BLE GATT is reliable */
    caps->is_bidirectional = true;
    
    return BACKEND_OK;
}

static BackendStatus ble_backend_open(const BackendConfig *config)
{
    if (!config || config->type != BACKEND_TYPE_BLE) {
        return BACKEND_INVALID_PARAM;
    }
    
    /* Platform-specific BLE peripheral setup */
    /* Example ESP32:
     * - Configure GAP parameters (device name, advertising)
     * - Setup GATT service with TX/RX characteristics
     * - Start advertising
     */
    
    g_ble_connected = true;
    return BACKEND_OK;
}

static BackendStatus ble_backend_close(void)
{
    if (!g_ble_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    /* Platform-specific BLE disconnect */
    g_ble_connected = false;
    
    return BACKEND_OK;
}

static BackendStatus ble_backend_send(const uint8_t *data, size_t length)
{
    if (!g_ble_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    if (!data || length == 0) {
        return BACKEND_INVALID_PARAM;
    }
    
    /* SAL handles fragmentation based on MTU */
    BleSalStatus status = ble_sal_send(data, length);
    if (status != BLE_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus ble_backend_receive(uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!g_ble_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    if (!buffer || !received_length) {
        return BACKEND_INVALID_PARAM;
    }
    
    /* BLE is typically event-driven, but provide timeout-based receive */
    BleSalStatus status = ble_sal_receive(buffer, buffer_size, received_length, 100);  /* 100ms timeout */
    
    if (status == BLE_SAL_TIMEOUT) {
        return BACKEND_TIMEOUT;
    } else if (status != BLE_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus ble_backend_set_timeout(uint32_t timeout_ms)
{
    /* BLE is event-driven, timeout applies to receive operations */
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
    .set_timeout = ble_backend_set_timeout
};

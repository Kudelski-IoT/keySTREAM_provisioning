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
 * @file transport_zigbee_mcu.c
 * @brief Zigbee Transport Backend for MCU
 * 
 * MCU-specific Zigbee implementation for wireless mesh networking.
 */

#include "backend_interface.h"
#include "zigbee/zigbee_sal.h" /* SAL interface */
#include <string.h>

/* Platform-specific config is included via Makefile:
 * -Ibackends/zigbee/sal/<platform>
 * This makes zigbee_config.h visible with platform-specific settings.
 */

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool g_zigbee_initialized = false;
static bool g_zigbee_connected = false;

/* ============================================================================
 * Zigbee Backend Implementation (MCU)
 * ============================================================================ */

static BackendStatus zigbee_backend_init(void)
{
    /* Config is read from zigbee_config.h at compile-time */
    ZigbeeSalStatus status = zigbee_sal_init(NULL, NULL);  /* NULL config + NULL callbacks */
    if (status != ZIGBEE_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    g_zigbee_initialized = true;
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_deinit(void)
{
    if (g_zigbee_connected) {
        zigbee_backend_close();
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
    
    caps->max_packet_size = 127;        /* Zigbee max payload */
    caps->max_message_size = 1024;      /* With fragmentation */
    caps->supports_fragmentation = true;
    caps->requires_connection = false;  /* Mesh network */
    caps->is_reliable = true;           /* With ACKs */
    caps->is_bidirectional = true;
    
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_open(const BackendConfig *config)
{
    if (!config || config->type != BACKEND_TYPE_ZIGBEE) {
        return BACKEND_INVALID_PARAM;
    }
    
    /* Platform-specific Zigbee network join */
    /* Example:
     * - Set PAN ID, channel
     * - Join network or form coordinator
     * - Wait for network up event
     */
    
    g_zigbee_connected = true;
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_close(void)
{
    if (!g_zigbee_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    /* Platform-specific Zigbee leave network */
    g_zigbee_connected = false;
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_send(const uint8_t *data, size_t length)
{
    if (!g_zigbee_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    if (!data || length == 0) {
        return BACKEND_INVALID_PARAM;
    }
    
    ZigbeeSalStatus status = zigbee_sal_send(data, length);
    if (status != ZIGBEE_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_receive(uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!g_zigbee_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    if (!buffer || !received_length) {
        return BACKEND_INVALID_PARAM;
    }
    
    /* Zigbee is typically event-driven, but provide timeout-based receive */
    ZigbeeSalStatus status = zigbee_sal_receive(buffer, buffer_size, received_length, 100);  /* 100ms timeout */
    
    if (status == ZIGBEE_SAL_TIMEOUT) {
        return BACKEND_TIMEOUT;
    } else if (status != ZIGBEE_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus zigbee_backend_set_timeout(uint32_t timeout_ms)
{
    /* Zigbee timeout configuration */
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
    .set_timeout = zigbee_backend_set_timeout
};

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
 * @file transport_usb_mcu.c
 * @brief USB Transport Backend for MCU (USB Device)
 * 
 * MCU-specific USB device implementation.
 * Uses CDC (Communications Device Class) for serial emulation.
 */

#include "backend_interface.h"
#include "usb/usb_sal.h"       /* SAL interface */
#include <string.h>

/* Platform-specific config is included via Makefile:
 * -Ibackends/usb/sal/<platform>
 * This makes usb_config.h visible with platform-specific settings.
 */

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool g_usb_initialized = false;
static bool g_usb_connected = false;

/* ============================================================================
 * USB Backend Implementation (MCU)
 * ============================================================================ */

static BackendStatus usb_backend_init(void)
{
    /* Config is read from usb_config.h at compile-time */
    UsbSalStatus status = usb_sal_init(NULL);  /* NULL = use defaults from usb_config.h */
    if (status != USB_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    g_usb_initialized = true;
    return BACKEND_OK;
}

static BackendStatus usb_backend_deinit(void)
{
    if (g_usb_connected) {
        usb_backend_close();
    }
    
    usb_sal_deinit();
    g_usb_initialized = false;
    return BACKEND_OK;
}

static BackendStatus usb_backend_get_capabilities(BackendCapabilities *caps)
{
    if (!caps) {
        return BACKEND_INVALID_PARAM;
    }
    
    caps->max_packet_size = 64;         /* USB Full Speed bulk endpoint */
    caps->max_message_size = 2048;      /* With fragmentation */
    caps->supports_fragmentation = true;
    caps->requires_connection = true;
    caps->is_reliable = true;
    caps->is_bidirectional = true;
    
    return BACKEND_OK;
}

static BackendStatus usb_backend_open(const BackendConfig *config)
{
    if (!config || config->type != BACKEND_TYPE_USB) {
        return BACKEND_INVALID_PARAM;
    }
    
    /* Platform-specific USB CDC setup */
    /* Example ESP32 (TinyUSB):
     * - Configure CDC descriptor
     * - Mount USB device
     * - Wait for host enumeration
     */
    
    g_usb_connected = true;
    return BACKEND_OK;
}

static BackendStatus usb_backend_close(void)
{
    if (!g_usb_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    /* Platform-specific USB disconnect */
    g_usb_connected = false;
    return BACKEND_OK;
}

static BackendStatus usb_backend_send(const uint8_t *data, size_t length)
{
    if (!g_usb_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    if (!data || length == 0) {
        return BACKEND_INVALID_PARAM;
    }
    
    UsbSalStatus status = usb_sal_write(data, length, 1000);  /* 1 second timeout */
    if (status != USB_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus usb_backend_receive(uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!g_usb_connected) {
        return BACKEND_NOT_CONNECTED;
    }
    
    if (!buffer || !received_length) {
        return BACKEND_INVALID_PARAM;
    }
    
    UsbSalStatus status = usb_sal_read(buffer, buffer_size, received_length, 100);  /* 100ms timeout */
    
    if (status == USB_SAL_TIMEOUT) {
        return BACKEND_TIMEOUT;
    } else if (status != USB_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus usb_backend_set_timeout(uint32_t timeout_ms)
{
    /* USB CDC timeout configuration */
    return BACKEND_OK;
}

/* ============================================================================
 * Backend Registration
 * ============================================================================ */

const Backend g_usb_backend = {
    .type = BACKEND_TYPE_USB,
    .init = usb_backend_init,
    .deinit = usb_backend_deinit,
    .get_capabilities = usb_backend_get_capabilities,
    .open = usb_backend_open,
    .close = usb_backend_close,
    .send = usb_backend_send,
    .receive = usb_backend_receive,
    .set_timeout = usb_backend_set_timeout
};

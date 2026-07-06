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
 * @file backend_usb_gateway.c
 * @brief USB Backend Implementation for Gateway
 * 
 * Generic USB backend that works across all gateway platforms.
 * Calls platform-specific SAL for actual USB operations.
 */

#include "backend_interface.h"
#include "usb_sal.h"
#include <string.h>

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool g_usb_initialized = false;
static bool g_usb_open = false;
static uint32_t g_timeout_ms = 5000;

/* ============================================================================
 * Backend Implementation Functions
 * ============================================================================ */

static BackendStatus usb_backend_init(void)
{
    if (g_usb_initialized) {
        return BACKEND_OK;
    }
    
    UsbSalStatus status = usb_sal_init();
    if (status != USB_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    g_usb_initialized = true;
    return BACKEND_OK;
}

static BackendStatus usb_backend_deinit(void)
{
    if (!g_usb_initialized) {
        return BACKEND_OK;
    }
    
    if (g_usb_open) {
        usb_sal_close();
        g_usb_open = false;
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
    
    caps->max_packet_size = 64;
    caps->max_message_size = 4096;
    caps->supports_fragmentation = false;
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
    
    if (!g_usb_initialized) {
        return BACKEND_ERROR;
    }
    
    if (g_usb_open) {
        usb_sal_close();
    }
    
    UsbSalConfig sal_config;
    sal_config.vid = config->config.usb.vid;
    sal_config.pid = config->config.usb.pid;
    strncpy(sal_config.serial_number, config->config.usb.serial_number, sizeof(sal_config.serial_number) - 1);
    sal_config.serial_number[sizeof(sal_config.serial_number) - 1] = '\0';
    sal_config.interface_num = config->config.usb.interface_num;
    
    UsbSalStatus status = usb_sal_open(&sal_config);
    if (status != USB_SAL_OK) {
        return (status == USB_SAL_NOT_FOUND) ? BACKEND_NOT_CONNECTED : BACKEND_ERROR;
    }
    
    usb_sal_set_timeout(g_timeout_ms);
    g_usb_open = true;
    return BACKEND_OK;
}

static BackendStatus usb_backend_close(void)
{
    if (!g_usb_open) {
        return BACKEND_OK;
    }
    
    usb_sal_close();
    g_usb_open = false;
    return BACKEND_OK;
}

static BackendStatus usb_backend_send(const uint8_t *data, size_t length)
{
    if (!data || length == 0) {
        return BACKEND_INVALID_PARAM;
    }
    
    if (!g_usb_open) {
        return BACKEND_NOT_CONNECTED;
    }
    
    size_t written = 0;
    UsbSalStatus status = usb_sal_write(data, length, &written);
    
    if (status != USB_SAL_OK) {
        return (status == USB_SAL_TIMEOUT) ? BACKEND_TIMEOUT : BACKEND_ERROR;
    }
    
    if (written != length) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus usb_backend_receive(uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!buffer || buffer_size == 0 || !received_length) {
        return BACKEND_INVALID_PARAM;
    }
    
    if (!g_usb_open) {
        return BACKEND_NOT_CONNECTED;
    }
    
    UsbSalStatus status = usb_sal_read(buffer, buffer_size, received_length);
    
    if (status == USB_SAL_TIMEOUT) {
        return BACKEND_TIMEOUT;
    }
    
    if (status != USB_SAL_OK) {
        return BACKEND_ERROR;
    }
    
    return BACKEND_OK;
}

static BackendStatus usb_backend_set_timeout(uint32_t timeout_ms)
{
    g_timeout_ms = timeout_ms;
    
    if (g_usb_open) {
        UsbSalStatus status = usb_sal_set_timeout(timeout_ms);
        if (status != USB_SAL_OK) {
            return BACKEND_ERROR;
        }
    }
    
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
    .set_timeout = usb_backend_set_timeout,
};

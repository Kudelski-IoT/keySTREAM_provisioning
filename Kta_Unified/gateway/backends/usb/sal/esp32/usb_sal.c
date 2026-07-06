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
 * @file usb_sal_esp32.c
 * @brief USB SAL Implementation for ESP32 (Gateway)
 * 
 * ESP32 USB implementation using TinyUSB stack.
 */

#include "../../usb_sal.h"
#include <string.h>

#ifdef ESP_PLATFORM

#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "esp_log.h"

static const char *TAG = "USB_SAL_ESP32";

static bool g_initialized = false;
static bool g_connected = false;

UsbSalStatus usb_sal_init(void)
{
    if (g_initialized) {
        return USB_SAL_OK;
    }
    
    /* Configure TinyUSB */
    tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false,
    };
    
    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TinyUSB init failed: %s", esp_err_to_name(ret));
        return USB_SAL_ERROR;
    }
    
    g_initialized = true;
    g_connected = false;
    
    ESP_LOGI(TAG, "USB initialized");
    return USB_SAL_OK;
}

UsbSalStatus usb_sal_deinit(void)
{
    if (!g_initialized) {
        return USB_SAL_OK;
    }
    
    /* TinyUSB doesn't provide explicit deinit */
    g_initialized = false;
    
    ESP_LOGI(TAG, "USB deinitialized");
    return USB_SAL_OK;
}

UsbSalStatus usb_sal_connect(uint16_t vendor_id, uint16_t product_id)
{
    if (!g_initialized) {
        return USB_SAL_ERROR;
    }
    
    /* TODO: Implement USB device enumeration and connection */
    (void)vendor_id;
    (void)product_id;
    
    ESP_LOGI(TAG, "Connecting to USB device VID:0x%04X PID:0x%04X", vendor_id, product_id);
    
    return USB_SAL_ERROR; /* Stub */
}

UsbSalStatus usb_sal_disconnect(void)
{
    if (!g_initialized) {
        return USB_SAL_ERROR;
    }
    
    g_connected = false;
    ESP_LOGI(TAG, "USB disconnected");
    return USB_SAL_OK;
}

UsbSalStatus usb_sal_write(const uint8_t *data, size_t length)
{
    if (!g_initialized || !g_connected || data == NULL || length == 0) {
        return USB_SAL_INVALID_PARAM;
    }
    
    /* TODO: Implement USB write */
    ESP_LOGI(TAG, "USB write: %zu bytes", length);
    
    return USB_SAL_ERROR; /* Stub */
}

UsbSalStatus usb_sal_read(uint8_t *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms)
{
    if (!g_initialized || !g_connected || buffer == NULL || bytes_read == NULL) {
        return USB_SAL_INVALID_PARAM;
    }
    
    (void)buffer_size;
    (void)timeout_ms;
    
    *bytes_read = 0;
    return USB_SAL_ERROR; /* Stub */
}

UsbSalStatus usb_sal_is_connected(bool *connected)
{
    if (connected == NULL) {
        return USB_SAL_INVALID_PARAM;
    }
    
    *connected = g_connected;
    return USB_SAL_OK;
}

#endif /* ESP_PLATFORM */

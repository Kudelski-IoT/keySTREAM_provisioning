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
 * @file usb_sal_linux.c
 * @brief USB SAL Implementation for Linux (Gateway)
 * 
 * Linux USB implementation using libusb.
 */

#include "../../usb_sal.h"
#include <stdio.h>
#include <string.h>

#ifdef __linux__

/* TODO: Include libusb when implementing */
/* #include <libusb-1.0/libusb.h> */

static bool g_initialized = false;
static bool g_connected = false;
/* static libusb_device_handle *g_dev_handle = NULL; */

UsbSalStatus usb_sal_init(void)
{
    if (g_initialized) {
        return USB_SAL_OK;
    }
    
    /* TODO: Initialize libusb */
    /* int ret = libusb_init(NULL); */
    /* if (ret < 0) return USB_SAL_ERROR; */
    
    g_initialized = true;
    g_connected = false;
    
    printf("USB SAL: Initialized (Linux libusb)\n");
    return USB_SAL_OK;
}

UsbSalStatus usb_sal_deinit(void)
{
    if (!g_initialized) {
        return USB_SAL_OK;
    }
    
    if (g_connected) {
        usb_sal_disconnect();
    }
    
    /* libusb_exit(NULL); */
    g_initialized = false;
    
    printf("USB SAL: Deinitialized\n");
    return USB_SAL_OK;
}

UsbSalStatus usb_sal_connect(uint16_t vendor_id, uint16_t product_id)
{
    if (!g_initialized) {
        return USB_SAL_ERROR;
    }
    
    if (g_connected) {
        fprintf(stderr, "USB SAL: Already connected\n");
        return USB_SAL_OK;
    }
    
    /* TODO: Open USB device by VID/PID */
    /* g_dev_handle = libusb_open_device_with_vid_pid(NULL, vendor_id, product_id); */
    
    printf("USB SAL: Connecting to VID:0x%04X PID:0x%04X\n", vendor_id, product_id);
    
    return USB_SAL_ERROR; /* Stub */
}

UsbSalStatus usb_sal_disconnect(void)
{
    if (!g_initialized) {
        return USB_SAL_ERROR;
    }
    
    if (!g_connected) {
        return USB_SAL_OK;
    }
    
    /* if (g_dev_handle) { */
    /*     libusb_close(g_dev_handle); */
    /*     g_dev_handle = NULL; */
    /* } */
    
    g_connected = false;
    printf("USB SAL: Disconnected\n");
    return USB_SAL_OK;
}

UsbSalStatus usb_sal_write(const uint8_t *data, size_t length)
{
    if (!g_initialized || !g_connected || data == NULL || length == 0) {
        return USB_SAL_INVALID_PARAM;
    }
    
    /* TODO: Implement USB bulk write */
    printf("USB SAL: Write %zu bytes\n", length);
    
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

#endif /* __linux__ */

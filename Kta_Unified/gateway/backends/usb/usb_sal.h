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
 * @file usb_sal.h
 * @brief USB Software Abstraction Layer Interface
 * 
 * Platform-independent USB CDC interface. Each platform provides its own implementation.
 */

#ifndef USB_SAL_H
#define USB_SAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * USB Protocol Constants (USB-IF Standard - Do Not Modify)
 * ============================================================================ */

/** USB Class Codes (from USB-IF) */
#define USB_CLASS_CDC               0x02  /* Communications Device Class */
#define USB_CLASS_DATA              0x0A  /* CDC-Data */

/** USB CDC Subclass Codes */
#define USB_CDC_SUBCLASS_ACM        0x02  /* Abstract Control Model */

/** USB CDC Protocol Codes */
#define USB_CDC_PROTOCOL_NONE       0x00  /* No class specific protocol */
#define USB_CDC_PROTOCOL_AT         0x01  /* AT Commands (v.25ter) */

/** USB Endpoint Direction Masks */
#define USB_ENDPOINT_DIR_MASK       0x80
#define USB_ENDPOINT_IN             0x80  /* Device to Host */
#define USB_ENDPOINT_OUT            0x00  /* Host to Device */

/** USB Endpoint Number Mask */
#define USB_ENDPOINT_NUM_MASK       0x0F

/* ============================================================================
 * Common USB Configuration (Protocol-Level, Not Platform-Specific)
 * ============================================================================ */

/** Standard max packet size for Full-Speed USB */
#ifndef USB_MAX_PACKET_SIZE
#define USB_MAX_PACKET_SIZE         64
#endif

/** Receive buffer size - common across platforms */
#ifndef USB_RX_BUFFER_SIZE
#define USB_RX_BUFFER_SIZE          2048
#endif

/** Transmit buffer size - common across platforms */
#ifndef USB_TX_BUFFER_SIZE
#define USB_TX_BUFFER_SIZE          2048
#endif

/** Number of async transfer buffers - common setting */
#ifndef USB_NUM_TRANSFER_BUFFERS
#define USB_NUM_TRANSFER_BUFFERS    4
#endif

/** Connection timeout - standard across platforms (ms) */
#ifndef USB_CONNECTION_TIMEOUT_MS
#define USB_CONNECTION_TIMEOUT_MS   10000
#endif

/** Transfer timeout - standard across platforms (ms) */
#ifndef USB_TRANSFER_TIMEOUT_MS
#define USB_TRANSFER_TIMEOUT_MS     5000
#endif

/** Read timeout - standard across platforms (ms) */
#ifndef USB_READ_TIMEOUT_MS
#define USB_READ_TIMEOUT_MS         1000
#endif

/** Write timeout - standard across platforms (ms) */
#ifndef USB_WRITE_TIMEOUT_MS
#define USB_WRITE_TIMEOUT_MS        1000
#endif

/** Enable debug logging - common across platforms */
#ifndef USB_DEBUG_ENABLED
#define USB_DEBUG_ENABLED           true
#endif

/* USB SAL Status Codes */
typedef enum {
    USB_SAL_OK = 0,
    USB_SAL_ERROR = -1,
    USB_SAL_TIMEOUT = -2,
    USB_SAL_NOT_OPEN = -3,
    USB_SAL_INVALID_PARAM = -4,
    USB_SAL_NOT_FOUND = -5,
} UsbSalStatus;

/* USB Configuration */
typedef struct {
    uint16_t vid;
    uint16_t pid;
    char serial_number[64];
    uint8_t interface_num;
} UsbSalConfig;

/* Platform-specific USB SAL functions */
UsbSalStatus usb_sal_init(void);
UsbSalStatus usb_sal_deinit(void);
UsbSalStatus usb_sal_open(const UsbSalConfig *config);
UsbSalStatus usb_sal_close(void);
UsbSalStatus usb_sal_write(const uint8_t *data, size_t length, size_t *written);
UsbSalStatus usb_sal_read(uint8_t *buffer, size_t buffer_size, size_t *read_count);
UsbSalStatus usb_sal_set_timeout(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* USB_SAL_H */

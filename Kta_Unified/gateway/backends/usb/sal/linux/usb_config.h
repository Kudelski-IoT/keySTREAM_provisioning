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
 * @file usb_config.h
 * @brief USB Configuration for Linux (Gateway)
 * 
 * Linux libusb-specific configuration for gateway application.
 */

#ifndef USB_CONFIG_LINUX_GATEWAY_H
#define USB_CONFIG_LINUX_GATEWAY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../usb_sal.h"  /* For USB protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * USB Device Selection
 * ============================================================================ */

/* NOTE: Common USB settings (max packet size, buffer sizes, timeouts, debug enable)
 *       are defined in usb_sal.h. Override them here only if platform needs different values. */

/** Target USB VID (Vendor ID of MCU device) */
#ifndef USB_TARGET_VID
#define USB_TARGET_VID              0x303A      /* Espressif */
#endif

/** Target USB PID (Product ID of MCU device) */
#ifndef USB_TARGET_PID
#define USB_TARGET_PID              0x1001
#endif

/** USB device serial number (empty = any) */
#ifndef USB_DEVICE_SERIAL
#define USB_DEVICE_SERIAL           ""
#endif

/* ============================================================================
 * USB CDC/ACM Configuration
 * ============================================================================ */

/** CDC interface number */
#ifndef USB_CDC_INTERFACE
#define USB_CDC_INTERFACE           0
#endif

/** Bulk IN endpoint */
#ifndef USB_BULK_IN_EP
#define USB_BULK_IN_EP              0x81
#endif

/** Bulk OUT endpoint */
#ifndef USB_BULK_OUT_EP
#define USB_BULK_OUT_EP             0x01
#endif

/* NOTE: Common USB setting (USB_MAX_PACKET_SIZE) is defined in usb_sal.h */

/* ============================================================================
 * libusb Configuration
 * ============================================================================ */

/** libusb debug level (0=none, 4=verbose) */
#ifndef USB_LIBUSB_DEBUG_LEVEL
#define USB_LIBUSB_DEBUG_LEVEL      2
#endif

/** Claim interface with kernel driver detach */
#ifndef USB_DETACH_KERNEL_DRIVER
#define USB_DETACH_KERNEL_DRIVER    true
#endif

/** Reattach kernel driver on close */
#ifndef USB_REATTACH_KERNEL_DRIVER
#define USB_REATTACH_KERNEL_DRIVER  true
#endif

/* ============================================================================
 * Buffer Configuration
 * ============================================================================ */

/* NOTE: Common USB buffer settings (USB_RX_BUFFER_SIZE, USB_TX_BUFFER_SIZE,
 *       USB_NUM_TRANSFER_BUFFERS) are defined in usb_sal.h */

/* ============================================================================
 * Timeout Configuration
 * ============================================================================ */

/* NOTE: Common USB timeout settings are defined in usb_sal.h */

/* ============================================================================
 * Device Discovery
 * ============================================================================ */

/** Auto-discover USB devices on init */
#ifndef USB_AUTO_DISCOVER
#define USB_AUTO_DISCOVER           true
#endif

/** Re-enumerate on connection error */
#ifndef USB_AUTO_REENUMERATE
#define USB_AUTO_REENUMERATE        true
#endif

/* ============================================================================
 * Debug Configuration
 * ============================================================================ */

/* NOTE: Common debug setting (USB_DEBUG_ENABLED) is defined in usb_sal.h */

/** Log USB transfers */
#ifndef USB_LOG_TRANSFERS
#define USB_LOG_TRANSFERS           false
#endif

/* ============================================================================
 * Gateway Application Configuration (USB-specific)
 * ============================================================================ */

/** Auto-connect on gateway startup */
#ifndef USB_AUTO_CONNECT
#define USB_AUTO_CONNECT            true
#endif

/** Number of connection retry attempts */
#ifndef USB_CONNECT_RETRIES
#define USB_CONNECT_RETRIES         10
#endif

/** Delay between connection retries (ms) */
#ifndef USB_CONNECT_RETRY_DELAY_MS
#define USB_CONNECT_RETRY_DELAY_MS  1000
#endif

/** Request queue size (USB bulk transfers are fast) */
#ifndef USB_QUEUE_SIZE
#define USB_QUEUE_SIZE              16
#endif

/** Maximum message size for reassembly */
#ifndef USB_MAX_MESSAGE_SIZE
#define USB_MAX_MESSAGE_SIZE        65536
#endif

/** Fragment size for large messages */
#ifndef USB_FRAGMENT_SIZE
#define USB_FRAGMENT_SIZE           512
#endif

/** Reassembly timeout (ms) */
#ifndef USB_REASSEMBLY_TIMEOUT_MS
#define USB_REASSEMBLY_TIMEOUT_MS   2000
#endif

/** Process KTA requests sequentially (recommended) */
#ifndef USB_SEQUENTIAL_PROCESSING
#define USB_SEQUENTIAL_PROCESSING   true
#endif

#ifdef __cplusplus
}
#endif

#endif /* USB_CONFIG_LINUX_GATEWAY_H */

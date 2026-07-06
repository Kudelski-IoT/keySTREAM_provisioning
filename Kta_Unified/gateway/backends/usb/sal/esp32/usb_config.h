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
 * @brief USB Configuration for ESP32 (Gateway)
 * 
 * ESP32-specific USB configuration for gateway application.
 */

#ifndef USB_CONFIG_ESP32_GATEWAY_H
#define USB_CONFIG_ESP32_GATEWAY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../usb_sal.h"  /* For USB protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * USB Host Configuration (Gateway acts as USB Host)
 * ============================================================================ */

/** Target USB VID (Vendor ID of MCU device to connect) */
#ifndef USB_TARGET_VID
#define USB_TARGET_VID              0x303A      /* Espressif VID */
#endif

/** Target USB PID (Product ID of MCU device) */
#ifndef USB_TARGET_PID
#define USB_TARGET_PID              0x1001
#endif

/** Max packet size */
#ifndef USB_MAX_PACKET_SIZE
#define USB_MAX_PACKET_SIZE         64
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

/* ============================================================================
 * Buffer Configuration
 * ============================================================================ */

/** Receive buffer size (bytes) */
#ifndef USB_RX_BUFFER_SIZE
#define USB_RX_BUFFER_SIZE          2048
#endif

/** Transmit buffer size (bytes) */
#ifndef USB_TX_BUFFER_SIZE
#define USB_TX_BUFFER_SIZE          2048
#endif

/** Number of USB transfer buffers */
#ifndef USB_NUM_ISOC_BUFFERS
#define USB_NUM_ISOC_BUFFERS        4
#endif

/* ============================================================================
 * Timeout Configuration
 * ============================================================================ */

/** Connection timeout (ms) */
#ifndef USB_CONNECTION_TIMEOUT_MS
#define USB_CONNECTION_TIMEOUT_MS   10000
#endif

/** Transfer timeout (ms) */
#ifndef USB_TRANSFER_TIMEOUT_MS
#define USB_TRANSFER_TIMEOUT_MS     1000
#endif

/** Read timeout (ms) */
#ifndef USB_READ_TIMEOUT_MS
#define USB_READ_TIMEOUT_MS         100
#endif

/* ============================================================================
 * Power Management
 * ============================================================================ */

/** USB port current limit (mA) */
#ifndef USB_PORT_CURRENT_LIMIT_MA
#define USB_PORT_CURRENT_LIMIT_MA   500
#endif

/** Enable USB suspend/resume */
#ifndef USB_SUSPEND_ENABLED
#define USB_SUSPEND_ENABLED         false
#endif

/* ============================================================================
 * Debug Configuration
 * ============================================================================ */

/** Enable USB debug logging */
#ifndef USB_DEBUG_ENABLED
#define USB_DEBUG_ENABLED           true
#endif

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

#endif /* USB_CONFIG_ESP32_GATEWAY_H */

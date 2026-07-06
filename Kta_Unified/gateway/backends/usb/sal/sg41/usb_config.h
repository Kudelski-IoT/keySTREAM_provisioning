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
 * @brief USB Configuration for Microchip SG41 (SAML11)
 * 
 * SG41-specific USB CDC configuration.
 */

#ifndef USB_CONFIG_SG41_H
#define USB_CONFIG_SG41_H

#include <stdint.h>
#include <stdbool.h>
#include "../../usb_sal.h"  /* For USB protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * USB Device Descriptor (SG41)
 * ============================================================================ */

/** USB Vendor ID */
#ifndef USB_VENDOR_ID
#define USB_VENDOR_ID               0x04D8      /* Microchip VID */
#endif

/** USB Product ID */
#ifndef USB_PRODUCT_ID
#define USB_PRODUCT_ID              0x0001
#endif

/** Device release number (BCD format) */
#ifndef USB_DEVICE_RELEASE
#define USB_DEVICE_RELEASE          0x0100      /* 1.0.0 */
#endif

/* ============================================================================
 * USB String Descriptors
 * ============================================================================ */

/** Manufacturer string */
#ifndef USB_MANUFACTURER_STRING
#define USB_MANUFACTURER_STRING     "Microchip"
#endif

/** Product string */
#ifndef USB_PRODUCT_STRING
#define USB_PRODUCT_STRING          "SG41 KTA Device"
#endif

/** Serial number string */
#ifndef USB_SERIAL_NUMBER
#define USB_SERIAL_NUMBER           "SG41-001"
#endif

/* ============================================================================
 * USB CDC Configuration (SG41 - Full-Speed only)
 * ============================================================================ */

/** Max packet size (64 for Full-Speed) */
#ifndef USB_MAX_PACKET_SIZE
#define USB_MAX_PACKET_SIZE         64
#endif

/** USB speed (0=Full-Speed, SG41 doesn't support High-Speed) */
#ifndef USB_SPEED_HIGH
#define USB_SPEED_HIGH              0
#endif

/* ============================================================================
 * Buffer Configuration (SG41 has limited 16KB RAM)
 * ============================================================================ */

/** Receive buffer size (bytes) - keep small */
#ifndef USB_RX_BUFFER_SIZE
#define USB_RX_BUFFER_SIZE          512
#endif

/** Transmit buffer size (bytes) */
#ifndef USB_TX_BUFFER_SIZE
#define USB_TX_BUFFER_SIZE          512
#endif

/* ============================================================================
 * Timeout Configuration
 * ============================================================================ */

/** Default read timeout (milliseconds) */
#ifndef USB_READ_TIMEOUT_MS
#define USB_READ_TIMEOUT_MS         5000
#endif

/** Default write timeout (milliseconds) */
#ifndef USB_WRITE_TIMEOUT_MS
#define USB_WRITE_TIMEOUT_MS        5000
#endif

/** USB enumeration timeout (milliseconds) */
#ifndef USB_ENUM_TIMEOUT_MS
#define USB_ENUM_TIMEOUT_MS         5000
#endif

#ifdef __cplusplus
}
#endif

#endif /* USB_CONFIG_SG41_H */

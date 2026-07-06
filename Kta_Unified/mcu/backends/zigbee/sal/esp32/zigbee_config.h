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
 * @file zigbee_config.h
 * @brief Zigbee Transport Configuration for MCU
 * 
 * Platform-specific Zigbee configuration settings.
 * Modify these values to match your network setup.
 */

#ifndef ZIGBEE_CONFIG_H
#define ZIGBEE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "../../zigbee_sal.h"  /* For Zigbee protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Zigbee Network Configuration
 * ============================================================================ */

/** PAN ID (Personal Area Network ID) */
#ifndef ZIGBEE_PAN_ID
#define ZIGBEE_PAN_ID               0x1234
#endif

/** Short address (0x0000=Coordinator, 0xFFFF=Not joined) */
#ifndef ZIGBEE_SHORT_ADDRESS
#define ZIGBEE_SHORT_ADDRESS        0x0000
#endif

/** Zigbee channel (11-26 for 2.4 GHz) */
#ifndef ZIGBEE_CHANNEL
#define ZIGBEE_CHANNEL              15
#endif

/** Extended address (IEEE 802.15.4 64-bit address) */
#ifndef ZIGBEE_EXTENDED_ADDRESS
#define ZIGBEE_EXTENDED_ADDRESS     {0x00, 0x12, 0x4B, 0x00, 0x01, 0x02, 0x03, 0x04}
#endif

/* ============================================================================
 * Device Type
 * ============================================================================ */

/** Device type (use ZIGBEE_DEVICE_COORDINATOR, ZIGBEE_DEVICE_ROUTER, or ZIGBEE_DEVICE_END_DEVICE) */
#ifndef ZIGBEE_DEVICE_TYPE
#define ZIGBEE_DEVICE_TYPE          ZIGBEE_DEVICE_ROUTER
#endif

/* ============================================================================
 * Security Configuration
 * ============================================================================ */

/** Enable security */
#ifndef ZIGBEE_SECURITY_ENABLED
#define ZIGBEE_SECURITY_ENABLED     true
#endif

/** Network key (128-bit) */
#ifndef ZIGBEE_NETWORK_KEY
#define ZIGBEE_NETWORK_KEY          {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, \
                                     0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10}
#endif

/* ============================================================================
 * Power Configuration
 * ============================================================================ */

/** TX power (dBm) - range depends on platform */
#ifndef ZIGBEE_TX_POWER_DBM
#define ZIGBEE_TX_POWER_DBM         0
#endif

/** Sleep mode for end devices */
#ifndef ZIGBEE_SLEEP_ENABLED
#define ZIGBEE_SLEEP_ENABLED        false
#endif

/* ============================================================================
 * Buffer Configuration
 * ============================================================================ */

/** Receive buffer size (bytes) */
#ifndef ZIGBEE_RX_BUFFER_SIZE
#define ZIGBEE_RX_BUFFER_SIZE       1024
#endif

/** Transmit buffer size (bytes) */
#ifndef ZIGBEE_TX_BUFFER_SIZE
#define ZIGBEE_TX_BUFFER_SIZE       1024
#endif

/** Maximum payload size (802.15.4 limit minus headers) */
#ifndef ZIGBEE_MAX_PAYLOAD_SIZE
#define ZIGBEE_MAX_PAYLOAD_SIZE     100
#endif

/* ============================================================================
 * Timeout Configuration
 * ============================================================================ */

/** Network join timeout (milliseconds) */
#ifndef ZIGBEE_JOIN_TIMEOUT_MS
#define ZIGBEE_JOIN_TIMEOUT_MS      30000
#endif

/** Default read timeout (milliseconds) */
#ifndef ZIGBEE_READ_TIMEOUT_MS
#define ZIGBEE_READ_TIMEOUT_MS      5000
#endif

/** Default write timeout (milliseconds) */
#ifndef ZIGBEE_WRITE_TIMEOUT_MS
#define ZIGBEE_WRITE_TIMEOUT_MS     5000
#endif

/* ============================================================================
 * Platform-Specific Settings
 * ============================================================================ */

/* ESP32 with ESP-ZIGBEE SDK */
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2)
    #ifndef ZIGBEE_TX_POWER_DBM
    #define ZIGBEE_TX_POWER_DBM     5           /* +5 dBm */
    #endif
#endif

/* Silicon Labs EFR32 */
#if defined(EFR32MG12) || defined(EFR32MG13)
    #ifndef ZIGBEE_TX_POWER_DBM
    #define ZIGBEE_TX_POWER_DBM     10          /* +10 dBm */
    #endif
#endif

/* NXP JN51xx */
#if defined(JN5169) || defined(JN5179)
    #ifndef ZIGBEE_TX_POWER_DBM
    #define ZIGBEE_TX_POWER_DBM     0
    #endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZIGBEE_CONFIG_H */

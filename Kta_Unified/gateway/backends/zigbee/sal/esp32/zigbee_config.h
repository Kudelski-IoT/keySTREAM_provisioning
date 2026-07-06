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
 * @brief Zigbee Configuration for ESP32 (Gateway)
 * 
 * ESP32-specific Zigbee configuration for gateway application.
 */

#ifndef ZIGBEE_CONFIG_ESP32_GATEWAY_H
#define ZIGBEE_CONFIG_ESP32_GATEWAY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../zigbee_sal.h"  /* For Zigbee protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Zigbee Gateway Configuration
 * ============================================================================ */

/** Device type (use ZIGBEE_DEVICE_COORDINATOR, ZIGBEE_DEVICE_ROUTER, or ZIGBEE_DEVICE_END_DEVICE) */
#ifndef ZIGBEE_DEVICE_TYPE
#define ZIGBEE_DEVICE_TYPE          ZIGBEE_DEVICE_COORDINATOR
#endif

/** PAN ID (Personal Area Network ID) */
#ifndef ZIGBEE_PAN_ID
#define ZIGBEE_PAN_ID               0x1234
#endif

/* NOTE: Common Zigbee settings (ZIGBEE_CHANNEL, ZIGBEE_PERMIT_JOINING,
 *       ZIGBEE_PERMIT_JOIN_DURATION, ZIGBEE_SECURITY_ENABLED, ZIGBEE_TX_POWER,
 *       buffer sizes, timeouts, debug) are defined in zigbee_sal.h.\n *       Override below only if needed. */

/** Extended address (IEEE 802.15.4 64-bit address) */
#ifndef ZIGBEE_EXTENDED_ADDRESS
#define ZIGBEE_EXTENDED_ADDRESS     {0x00, 0x12, 0x4B, 0x00, 0xAA, 0xBB, 0xCC, 0xDD}
#endif

/* ============================================================================
 * Network Configuration
 * ============================================================================ */

/* NOTE: Common network settings are defined in zigbee_sal.h */

/** Maximum number of children (end devices) */
#ifndef ZIGBEE_MAX_CHILDREN
#define ZIGBEE_MAX_CHILDREN         20
#endif

/** Maximum number of routers */
#ifndef ZIGBEE_MAX_ROUTERS
#define ZIGBEE_MAX_ROUTERS          10
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
#define ZIGBEE_NETWORK_KEY          {0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F, \
                                     0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E}
#endif

/** Trust center address (coordinator address) */
#ifndef ZIGBEE_TRUST_CENTER_ADDRESS
#define ZIGBEE_TRUST_CENTER_ADDRESS 0x0000
#endif

/* ============================================================================
 * Power Configuration
 * ============================================================================ */

/** TX power (dBm) */
#ifndef ZIGBEE_TX_POWER_DBM
#define ZIGBEE_TX_POWER_DBM         15
#endif

/* ============================================================================
 * Endpoint Configuration
 * ============================================================================ */

/** Gateway endpoint ID */
#ifndef ZIGBEE_GATEWAY_ENDPOINT
#define ZIGBEE_GATEWAY_ENDPOINT     1
#endif

/** Profile ID */
#ifndef ZIGBEE_PROFILE_ID
#define ZIGBEE_PROFILE_ID           0x0104      /* Home Automation */
#endif

/** Device ID */
#ifndef ZIGBEE_DEVICE_ID
#define ZIGBEE_DEVICE_ID            0x0005      /* Configuration Tool */
#endif

/* ============================================================================
 * Cluster Configuration (For KTA communication)
 * ============================================================================ */

/** Custom KTA cluster ID */
#ifndef ZIGBEE_KTA_CLUSTER_ID
#define ZIGBEE_KTA_CLUSTER_ID       0xFC00      /* Manufacturer specific */
#endif

/* ============================================================================
 * Buffer Configuration
 * ============================================================================ */

/** Receive buffer size (bytes) */
#ifndef ZIGBEE_RX_BUFFER_SIZE
#define ZIGBEE_RX_BUFFER_SIZE       512
#endif

/** Transmit buffer size (bytes) */
#ifndef ZIGBEE_TX_BUFFER_SIZE
#define ZIGBEE_TX_BUFFER_SIZE       512
#endif

/** Network event queue size */
#ifndef ZIGBEE_EVENT_QUEUE_SIZE
#define ZIGBEE_EVENT_QUEUE_SIZE     10
#endif

/* ============================================================================
 * Timeout Configuration
 * ============================================================================ */

/** Connection timeout (ms) */
#ifndef ZIGBEE_CONNECTION_TIMEOUT_MS
#define ZIGBEE_CONNECTION_TIMEOUT_MS    30000
#endif

/** Message timeout (ms) */
#ifndef ZIGBEE_MESSAGE_TIMEOUT_MS
#define ZIGBEE_MESSAGE_TIMEOUT_MS   5000
#endif

/* ============================================================================
 * Debug Configuration
 * ============================================================================ */

/** Enable Zigbee debug logging */
#ifndef ZIGBEE_DEBUG_ENABLED
#define ZIGBEE_DEBUG_ENABLED        true
#endif

/** Log network events */
#ifndef ZIGBEE_LOG_NETWORK_EVENTS
#define ZIGBEE_LOG_NETWORK_EVENTS   true
#endif

/* ============================================================================
 * Gateway Application Configuration (Zigbee-specific)
 * ============================================================================ */

/** Auto-connect on gateway startup */
#ifndef ZIGBEE_AUTO_CONNECT
#define ZIGBEE_AUTO_CONNECT         true
#endif

/** Number of connection retry attempts */
#ifndef ZIGBEE_CONNECT_RETRIES
#define ZIGBEE_CONNECT_RETRIES      5
#endif

/** Delay between connection retries (ms) - Zigbee network formation is slow */
#ifndef ZIGBEE_CONNECT_RETRY_DELAY_MS
#define ZIGBEE_CONNECT_RETRY_DELAY_MS   5000
#endif

/** Request queue size - Zigbee is slower, smaller queue */
#ifndef ZIGBEE_QUEUE_SIZE
#define ZIGBEE_QUEUE_SIZE           8
#endif

/** Maximum message size for reassembly */
#ifndef ZIGBEE_MAX_MESSAGE_SIZE
#define ZIGBEE_MAX_MESSAGE_SIZE     4096
#endif

/** Fragment size - Zigbee MSDU limit */
#ifndef ZIGBEE_FRAGMENT_SIZE
#define ZIGBEE_FRAGMENT_SIZE        80
#endif

/** Reassembly timeout (ms) - Zigbee mesh needs longer timeout */
#ifndef ZIGBEE_REASSEMBLY_TIMEOUT_MS
#define ZIGBEE_REASSEMBLY_TIMEOUT_MS    10000
#endif

/** Process KTA requests sequentially (recommended for mesh) */
#ifndef ZIGBEE_SEQUENTIAL_PROCESSING
#define ZIGBEE_SEQUENTIAL_PROCESSING    true
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZIGBEE_CONFIG_ESP32_GATEWAY_H */

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
 * @brief Zigbee Configuration for Windows (Gateway)
 * 
 * Windows-specific Zigbee configuration for gateway application using USB coordinator.
 */

#ifndef ZIGBEE_CONFIG_WINDOWS_GATEWAY_H
#define ZIGBEE_CONFIG_WINDOWS_GATEWAY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../zigbee_sal.h"  /* For Zigbee protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Zigbee Coordinator Configuration
 * ============================================================================ */

/* NOTE: Common Zigbee settings (channel, TX power, security, timeouts, buffer sizes,
 *       permit joining, debug enable) are defined in zigbee_sal.h.
 *       Override them here only if platform needs different values. */

/** COM port for Zigbee coordinator (e.g., "COM3") */
#ifndef ZIGBEE_COM_PORT
#define ZIGBEE_COM_PORT             L"COM3"
#endif

/** Serial baud rate */
#ifndef ZIGBEE_SERIAL_BAUD
#define ZIGBEE_SERIAL_BAUD          115200
#endif

/** Device type (use ZIGBEE_DEVICE_COORDINATOR, ZIGBEE_DEVICE_ROUTER, or ZIGBEE_DEVICE_END_DEVICE) */
#ifndef ZIGBEE_DEVICE_TYPE
#define ZIGBEE_DEVICE_TYPE          ZIGBEE_DEVICE_COORDINATOR
#endif

/** PAN ID */
#ifndef ZIGBEE_PAN_ID
#define ZIGBEE_PAN_ID               0x1234
#endif

/* NOTE: Common Zigbee settings (ZIGBEE_CHANNEL, ZIGBEE_PERMIT_JOINING,
 *       ZIGBEE_PERMIT_JOIN_DURATION, ZIGBEE_SECURITY_ENABLED) are
 *       defined in zigbee_sal.h. Override below only if needed. */

/** Extended address */
#ifndef ZIGBEE_EXTENDED_ADDRESS
#define ZIGBEE_EXTENDED_ADDRESS     0x00124B00AABBCCDDULL
#endif

/* NOTE: Common Zigbee settings (ZIGBEE_CHANNEL, ZIGBEE_PERMIT_JOINING,
 *       ZIGBEE_PERMIT_JOIN_DURATION, ZIGBEE_SECURITY_ENABLED) are
 *       defined in zigbee_sal.h. Override below only if needed. */

/* ============================================================================
 * Network Configuration
 * ============================================================================ */

/** Maximum number of children */
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

/* NOTE: Common security setting (ZIGBEE_SECURITY_ENABLED) is defined in zigbee_sal.h */

/** Network key (128-bit hex string) */
#ifndef ZIGBEE_NETWORK_KEY_HEX
#define ZIGBEE_NETWORK_KEY_HEX      "01030507090B0D0F00020406080A0C0E"
#endif

/* ============================================================================
 * Gateway Configuration
 * ============================================================================ */

/** Gateway endpoint ID */
#ifndef ZIGBEE_GATEWAY_ENDPOINT
#define ZIGBEE_GATEWAY_ENDPOINT     1
#endif

/** Profile ID */
#ifndef ZIGBEE_PROFILE_ID
#define ZIGBEE_PROFILE_ID           0x0104
#endif

/** Device ID */
#ifndef ZIGBEE_DEVICE_ID
#define ZIGBEE_DEVICE_ID            0x0005
#endif

/** Custom KTA cluster ID */
#ifndef ZIGBEE_KTA_CLUSTER_ID
#define ZIGBEE_KTA_CLUSTER_ID       0xFC00
#endif

/* ============================================================================
 * Buffer Configuration
 * ============================================================================ */

/* NOTE: Common buffer settings (ZIGBEE_RX_BUFFER_SIZE, ZIGBEE_TX_BUFFER_SIZE)
 *       are defined in zigbee_sal.h */

/** Event queue size */
#ifndef ZIGBEE_EVENT_QUEUE_SIZE
#define ZIGBEE_EVENT_QUEUE_SIZE     10
#endif

/* ============================================================================
 * Serial Port Configuration (Windows)
 * ============================================================================ */

/** Use overlapped I/O (async) */
#ifndef ZIGBEE_USE_OVERLAPPED_IO
#define ZIGBEE_USE_OVERLAPPED_IO    true
#endif

/** RTS/CTS hardware flow control */
#ifndef ZIGBEE_HARDWARE_FLOW_CONTROL
#define ZIGBEE_HARDWARE_FLOW_CONTROL    false
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

/** Serial read timeout (ms) */
#ifndef ZIGBEE_SERIAL_TIMEOUT_MS
#define ZIGBEE_SERIAL_TIMEOUT_MS    100
#endif

/* ============================================================================
 * Debug Configuration
 * ============================================================================ */

/* NOTE: Common debug setting (ZIGBEE_DEBUG_ENABLED) is defined in zigbee_sal.h */

/** Log network events */
#ifndef ZIGBEE_LOG_NETWORK_EVENTS
#define ZIGBEE_LOG_NETWORK_EVENTS   true
#endif

/** Log to Windows Event Log */
#ifndef ZIGBEE_USE_EVENT_LOG
#define ZIGBEE_USE_EVENT_LOG        false
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

#endif /* ZIGBEE_CONFIG_WINDOWS_GATEWAY_H */

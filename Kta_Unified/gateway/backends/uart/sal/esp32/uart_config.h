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
 * @file uart_config.h
 * @brief UART Configuration for ESP32 (Gateway)
 * 
 * ESP32-specific UART configuration for gateway application.
 * Modify these values to match your hardware setup.
 */

#ifndef UART_CONFIG_ESP32_GATEWAY_H
#define UART_CONFIG_ESP32_GATEWAY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../uart_sal.h"  /* For UART protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * UART Hardware Configuration
 * ============================================================================ */

/* NOTE: Common UART settings (baud rate, data/stop/parity bits, flow control,
 *       buffer sizes, timeouts, debug enable) are defined in uart_sal.h.
 *       Override them here only if platform needs different values. */

/** UART peripheral number (0, 1, 2, etc.) */
#ifndef UART_PORT_NUM
#define UART_PORT_NUM               0
#endif

/* NOTE: Common UART settings (UART_BAUD_RATE, UART_DATA_BITS, UART_PARITY,
 *       UART_STOP_BITS, UART_FLOW_CONTROL, UART_RX/TX_BUFFER_SIZE, timeouts)
 *       are defined in uart_sal.h. Override them below only if needed. */

/* ============================================================================
 * Pin Configuration (Platform-Specific)
 * ============================================================================ */

/** TX Pin (GPIO number) */
#ifndef UART_TX_PIN
#define UART_TX_PIN                 10
#endif

/** RX Pin (GPIO number) */
#ifndef UART_RX_PIN
#define UART_RX_PIN                 11
#endif

/** RTS Pin (if flow control enabled) */
#ifndef UART_RTS_PIN
#define UART_RTS_PIN                12
#endif

/** CTS Pin (if flow control enabled) */
#ifndef UART_CTS_PIN
#define UART_CTS_PIN                13
#endif

/* ============================================================================
 * ESP32-Specific UART Configuration
 * ============================================================================ */

/** RX flow control threshold (ESP32 UART driver) */
#ifndef UART_RX_FLOW_CTRL_THRESH
#define UART_RX_FLOW_CTRL_THRESH    122
#endif

/** Use default pins (no change) - set to true to use board defaults */
#ifndef UART_USE_DEFAULT_PINS
#define UART_USE_DEFAULT_PINS       false
#endif

/** Alternate pin definitions for UART1 (if not using defaults) */
#ifndef UART1_TX_PIN
#define UART1_TX_PIN                10
#endif

#ifndef UART1_RX_PIN
#define UART1_RX_PIN                9
#endif

/** Alternate pin definitions for UART2 (if not using defaults) */
#ifndef UART2_TX_PIN
#define UART2_TX_PIN                17
#endif

#ifndef UART2_RX_PIN
#define UART2_RX_PIN                16
#endif

/* ============================================================================
 * Buffer Configuration
 * ============================================================================ */

/** Receive buffer size (bytes) */
#ifndef UART_RX_BUFFER_SIZE
#define UART_RX_BUFFER_SIZE         1024
#endif

/** Transmit buffer size (bytes) */
#ifndef UART_TX_BUFFER_SIZE
#define UART_TX_BUFFER_SIZE         1024
#endif

/* ============================================================================
 * Timeout Configuration
 * ============================================================================ */

/** Default read timeout (milliseconds) */
#ifndef UART_READ_TIMEOUT_MS
#define UART_READ_TIMEOUT_MS        5000
#endif

/** Default write timeout (milliseconds) */
#ifndef UART_WRITE_TIMEOUT_MS
#define UART_WRITE_TIMEOUT_MS       5000
#endif

/* ============================================================================
 * Gateway Application Configuration (UART-specific)
 * ============================================================================ */

/** Auto-connect on gateway startup */
#ifndef UART_AUTO_CONNECT
#define UART_AUTO_CONNECT           true
#endif

/** Number of connection retry attempts */
#ifndef UART_CONNECT_RETRIES
#define UART_CONNECT_RETRIES        30
#endif

/** Delay between connection retries (ms) - UART is fast */
#ifndef UART_CONNECT_RETRY_DELAY_MS
#define UART_CONNECT_RETRY_DELAY_MS 1000
#endif

/** Request queue size (UART can handle many requests) */
#ifndef UART_QUEUE_SIZE
#define UART_QUEUE_SIZE             16
#endif

/** Maximum message size for reassembly */
#ifndef UART_MAX_MESSAGE_SIZE
#define UART_MAX_MESSAGE_SIZE       8192
#endif

/** Fragment size for large messages - UART can handle larger chunks */
#ifndef UART_FRAGMENT_SIZE
#define UART_FRAGMENT_SIZE          256
#endif

/** Reassembly timeout (ms) - UART is fast, shorter timeout */
#ifndef UART_REASSEMBLY_TIMEOUT_MS
#define UART_REASSEMBLY_TIMEOUT_MS  2000
#endif

/** Process KTA requests sequentially (recommended) */
#ifndef UART_SEQUENTIAL_PROCESSING
#define UART_SEQUENTIAL_PROCESSING  true
#endif

#ifdef __cplusplus
}
#endif

#endif /* UART_CONFIG_ESP32_GATEWAY_H */

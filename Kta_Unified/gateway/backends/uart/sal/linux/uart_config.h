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
 * @brief UART Configuration for Linux (Gateway)
 * 
 * Linux-specific UART configuration for gateway application.
 */

#ifndef UART_CONFIG_LINUX_GATEWAY_H
#define UART_CONFIG_LINUX_GATEWAY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../uart_sal.h"  /* For UART protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * UART Device Configuration (Linux)
 * ============================================================================ */

/* NOTE: Common UART settings (baud rate, data/stop/parity bits, flow control,
 *       buffer sizes, timeouts, debug enable) are defined in uart_sal.h.
 *       Override them here only if platform needs different values. */

/** Default serial port device */
#ifndef UART_DEVICE_PATH
#define UART_DEVICE_PATH            "/dev/ttyUSB0"
#endif

/** Alternative serial ports (for auto-detection) */
#ifndef UART_ALT_DEVICE_PATHS
#define UART_ALT_DEVICE_PATHS       {"/dev/ttyACM0", "/dev/ttyS0", "/dev/ttyUSB1"}
#endif

/* NOTE: Common UART settings (UART_BAUD_RATE, UART_DATA_BITS, UART_PARITY,
 *       UART_STOP_BITS, UART_FLOW_CONTROL, UART_RX/TX_BUFFER_SIZE, timeouts)
 *       are defined in uart_sal.h. Override them below only if needed. */

/* ============================================================================
 * Timeout Configuration
 * ============================================================================ */

/* NOTE: Common timeout settings are defined in uart_sal.h */

/* ============================================================================
 * Linux Terminal Settings
 * ============================================================================ */

/** Enable canonical mode (line buffering) */
#ifndef UART_CANONICAL_MODE
#define UART_CANONICAL_MODE         false
#endif

/** Enable echo */
#ifndef UART_ECHO_ENABLED
#define UART_ECHO_ENABLED           false
#endif

/** Minimum characters for read */
#ifndef UART_VMIN
#define UART_VMIN                   0
#endif

/** Read timeout (deciseconds, 0 = poll) */
#ifndef UART_VTIME
#define UART_VTIME                  1
#endif

/* ============================================================================
 * Auto-Detection Configuration
 * ============================================================================ */

/** Enable auto-detection of serial ports */
#ifndef UART_AUTO_DETECT
#define UART_AUTO_DETECT            true
#endif

/** Auto-detect timeout per port (ms) */
#ifndef UART_AUTO_DETECT_TIMEOUT_MS
#define UART_AUTO_DETECT_TIMEOUT_MS 500
#endif

/* ============================================================================
 * Debug Configuration
 * ============================================================================ */

/** Enable UART debug logging */
#ifndef UART_DEBUG_ENABLED
#define UART_DEBUG_ENABLED          true
#endif

/** Log raw data transfers */
#ifndef UART_LOG_RAW_DATA
#define UART_LOG_RAW_DATA           false
#endif

/** Use syslog for logging */
#ifndef UART_USE_SYSLOG
#define UART_USE_SYSLOG             false
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

#endif /* UART_CONFIG_LINUX_GATEWAY_H */

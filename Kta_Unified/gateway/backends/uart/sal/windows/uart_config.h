/**
 * @file uart_config.h
 * @brief UART Configuration for Windows (Gateway)
 * 
 * Windows-specific UART configuration for gateway application.
 */

#ifndef UART_CONFIG_WINDOWS_GATEWAY_H
#define UART_CONFIG_WINDOWS_GATEWAY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../uart_sal.h"  /* For UART protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * UART Device Configuration (Windows)
 * ============================================================================ */

/* NOTE: Common UART settings (baud rate, data/stop/parity bits, flow control,
 *       buffer sizes, timeouts, debug enable) are defined in uart_sal.h.
 *       Override them here only if platform needs different values. */

/** Default COM port */
#ifndef UART_COM_PORT
#define UART_COM_PORT               L"COM3"
#endif

/** COM port name (for display/logging) */
#ifndef UART_COM_PORT_STR
#define UART_COM_PORT_STR           "COM3"
#endif

/* NOTE: Common UART settings (UART_BAUD_RATE, UART_DATA_BITS, UART_PARITY,
 *       UART_STOP_BITS, UART_FLOW_CONTROL, UART_RX/TX_BUFFER_SIZE, timeouts)
 *       are defined in uart_sal.h. Override them below only if needed. */

/* ============================================================================
 * Buffer Configuration
 * ============================================================================ */

/** Windows driver buffer size */
#ifndef UART_DRIVER_BUFFER_SIZE
#define UART_DRIVER_BUFFER_SIZE     4096
#endif

/* ============================================================================
 * Timeout Configuration
 * ============================================================================ */

/* NOTE: Common timeout settings (UART_READ_TIMEOUT_MS, UART_WRITE_TIMEOUT_MS,
 *       UART_CONNECTION_TIMEOUT_MS) are defined in uart_sal.h */

/** ReadIntervalTimeout (0=disabled) */
#ifndef UART_READ_INTERVAL_TIMEOUT
#define UART_READ_INTERVAL_TIMEOUT  MAXDWORD    /* Immediate return */
#endif

/** ReadTotalTimeoutMultiplier */
#ifndef UART_READ_TIMEOUT_MULTIPLIER
#define UART_READ_TIMEOUT_MULTIPLIER    0
#endif

/** ReadTotalTimeoutConstant */
#ifndef UART_READ_TIMEOUT_CONSTANT
#define UART_READ_TIMEOUT_CONSTANT  100
#endif

/* ============================================================================
 * Windows COM Port Settings
 * ============================================================================ */

/** Use overlapped I/O (async) */
#ifndef UART_USE_OVERLAPPED_IO
#define UART_USE_OVERLAPPED_IO      true
#endif

/** DTR (Data Terminal Ready) control */
#ifndef UART_DTR_CONTROL
#define UART_DTR_CONTROL            1           /* DTR_CONTROL_ENABLE */
#endif

/** RTS (Request To Send) control */
#ifndef UART_RTS_CONTROL
#define UART_RTS_CONTROL            1           /* RTS_CONTROL_ENABLE */
#endif

/** Enable binary mode */
#ifndef UART_BINARY_MODE
#define UART_BINARY_MODE            true
#endif

/** Discard null characters */
#ifndef UART_DISCARD_NULL
#define UART_DISCARD_NULL           false
#endif

/* ============================================================================
 * Auto-Detection Configuration
 * ============================================================================ */

/** Enable auto-detection of COM ports */
#ifndef UART_AUTO_DETECT
#define UART_AUTO_DETECT            true
#endif

/** Scan COM ports from COM1 to COM_MAX */
#ifndef UART_COM_PORT_MAX
#define UART_COM_PORT_MAX           20
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

/** Log to Windows Event Log */
#ifndef UART_USE_EVENT_LOG
#define UART_USE_EVENT_LOG          false
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

#endif /* UART_CONFIG_WINDOWS_GATEWAY_H */

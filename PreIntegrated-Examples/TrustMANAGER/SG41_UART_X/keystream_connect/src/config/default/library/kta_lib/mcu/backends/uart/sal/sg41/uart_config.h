/**
 * @file uart_config.h
 * @brief UART Configuration for Microchip SG41 (SAML11)
 * 
 * SG41-specific UART configuration for SERCOM peripheral.
 */

#ifndef UART_CONFIG_SG41_H
#define UART_CONFIG_SG41_H

#include <stdint.h>
#include <stdbool.h>
#include "../../uart_sal.h"  /* For UART protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * UART Hardware Configuration (SG41 SERCOM)
 * ============================================================================ */

/** SERCOM instance used as UART (0-2 on SAML11) */
#ifndef UART_SERCOM_INSTANCE
#define UART_SERCOM_INSTANCE        1           /* SERCOM1 */
#endif

/** Baud rate */
#ifndef UART_BAUD_RATE
#define UART_BAUD_RATE              115200
#endif

/** Data bits (use UART_DATA_BITS_7 or UART_DATA_BITS_8) */
#ifndef UART_DATA_BITS
#define UART_DATA_BITS              UART_DATA_BITS_8
#endif

/** Parity (use UART_PARITY_NONE, UART_PARITY_ODD, or UART_PARITY_EVEN) */
#ifndef UART_PARITY
#define UART_PARITY                 UART_PARITY_NONE
#endif

/** Stop bits (use UART_STOP_BITS_1 or UART_STOP_BITS_2) */
#ifndef UART_STOP_BITS
#define UART_STOP_BITS              UART_STOP_BITS_1
#endif

/** Hardware flow control */
#ifndef UART_FLOW_CONTROL
#define UART_FLOW_CONTROL           false
#endif

/* ============================================================================
 * Pin Configuration (SAML11 PAxx pins)
 * ============================================================================ */

/** TX Pin (PAD configuration) */
#ifndef UART_TX_PAD
#define UART_TX_PAD                 0           /* PAD[0] */
#endif

#ifndef UART_TX_PIN
#define UART_TX_PIN                 4           /* PA04 */
#endif

/** RX Pin (PAD configuration) */
#ifndef UART_RX_PAD
#define UART_RX_PAD                 1           /* PAD[1] */
#endif

#ifndef UART_RX_PIN
#define UART_RX_PIN                 5           /* PA05 */
#endif

/* ============================================================================
 * Buffer Configuration (SG41 has limited RAM: 16KB)
 * ============================================================================ */

/** Receive buffer size (bytes) - keep small for SG41 */
#ifndef UART_RX_BUFFER_SIZE
#define UART_RX_BUFFER_SIZE         512
#endif

/** Transmit buffer size (bytes) */
#ifndef UART_TX_BUFFER_SIZE
#define UART_TX_BUFFER_SIZE         512
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
 * DMA Configuration (Optional)
 * ============================================================================ */

/** Enable DMA for TX (if available) */
#ifndef UART_USE_DMA_TX
#define UART_USE_DMA_TX             false
#endif

/** Enable DMA for RX (if available) */
#ifndef UART_USE_DMA_RX
#define UART_USE_DMA_RX             false
#endif

#ifdef __cplusplus
}
#endif

#endif /* UART_CONFIG_SG41_H */

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
 * @brief UART Transport Configuration for MCU
 * 
 * Platform-specific UART configuration settings.
 * Modify these values to match your hardware setup.
 */

#ifndef UART_CONFIG_H
#define UART_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "../../uart_sal.h"  /* For UART protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * UART Hardware Configuration
 * ============================================================================ */

/** UART peripheral number (0, 1, 2, etc.) */
#ifndef UART_PORT_NUM
#define UART_PORT_NUM               0
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

/** Hardware flow control (RTS/CTS) */
#ifndef UART_FLOW_CONTROL
#define UART_FLOW_CONTROL           false
#endif

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
 * Platform-Specific Examples
 * ============================================================================ */

/* ESP32 Example */
#if defined(CONFIG_IDF_TARGET_ESP32)
    #ifndef UART_PORT_NUM
    #define UART_PORT_NUM           1           /* UART1 */
    #endif
    #ifndef UART_TX_PIN
    #define UART_TX_PIN             17          /* GPIO17 */
    #endif
    #ifndef UART_RX_PIN
    #define UART_RX_PIN             16          /* GPIO16 */
    #endif
#endif

/* Microchip SG41 Example */
#if defined(__SAML11E16A__) || defined(__SAML11E15A__)
    #ifndef UART_PORT_NUM
    #define UART_PORT_NUM           1           /* SERCOM1 as UART */
    #endif
    #ifndef UART_TX_PIN
    #define UART_TX_PIN             4           /* PA04 */
    #endif
    #ifndef UART_RX_PIN
    #define UART_RX_PIN             5           /* PA05 */
    #endif
#endif

/* Microchip D21 Example */
#if defined(__SAMD21G18A__)
    #ifndef UART_PORT_NUM
    #define UART_PORT_NUM           0           /* SERCOM0 */
    #endif
    #ifndef UART_TX_PIN
    #define UART_TX_PIN             10          /* PA10 */
    #endif
    #ifndef UART_RX_PIN
    #define UART_RX_PIN             11          /* PA11 */
    #endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* UART_CONFIG_H */

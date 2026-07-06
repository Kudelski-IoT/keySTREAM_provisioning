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
 * @file ble_config.h
 * @brief BLE Configuration for Nordic nRF52
 * 
 * Nordic nRF52-specific BLE configuration using SoftDevice.
 */

#ifndef BLE_CONFIG_NORDIC_H
#define BLE_CONFIG_NORDIC_H

#include <stdint.h>
#include <stdbool.h>#include "../../ble_sal.h"  /* For BLE protocol constants */
#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * BLE Device Configuration (Nordic nRF52)
 * ============================================================================ */

/** Device name (advertised) */
#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME             "nRF52_KTA"
#endif

/** Maximum device name length */
#ifndef BLE_DEVICE_NAME_MAX_LEN
#define BLE_DEVICE_NAME_MAX_LEN     31
#endif

/* ============================================================================
 * BLE Connection Parameters
 * ============================================================================ */

/** Requested MTU size (nRF52 supports up to 247) */
#ifndef BLE_MTU_SIZE
#define BLE_MTU_SIZE                247
#endif

/** Use bonding/pairing */
#ifndef BLE_USE_BONDING
#define BLE_USE_BONDING             true
#endif

/** TX Power (dBm) - nRF52: -40, -20, -16, -12, -8, -4, 0, +3, +4 */
#ifndef BLE_TX_POWER_DBM
#define BLE_TX_POWER_DBM            4           /* +4 dBm */
#endif

/* ============================================================================
 * Advertising Configuration
 * ============================================================================ */

/* NOTE: BLE Service UUIDs are defined in ble_sal.h as standard protocol constants:
 *   - BLE_NORDIC_UART_SERVICE_UUID
 *   - BLE_NORDIC_UART_TX_CHAR_UUID  
 *   - BLE_NORDIC_UART_RX_CHAR_UUID
 * Platform-specific implementations convert these to required byte array format.
 */

/** Advertising interval (units of 0.625 ms) */
#ifndef BLE_ADV_INTERVAL_MIN
#define BLE_ADV_INTERVAL_MIN        0x0020      /* 20 ms */
#endif

#ifndef BLE_ADV_INTERVAL_MAX
#define BLE_ADV_INTERVAL_MAX        0x0040      /* 40 ms */
#endif

/** Advertising timeout (seconds, 0 = no timeout) */
#ifndef BLE_ADV_TIMEOUT_SEC
#define BLE_ADV_TIMEOUT_SEC         0
#endif

/* ============================================================================
 * Connection Parameters (nRF52 specific)
 * ============================================================================ */

/** Minimum connection interval (units of 1.25 ms) */
#ifndef BLE_CONN_INTERVAL_MIN
#define BLE_CONN_INTERVAL_MIN       6           /* 7.5 ms */
#endif

/** Maximum connection interval (units of 1.25 ms) */
#ifndef BLE_CONN_INTERVAL_MAX
#define BLE_CONN_INTERVAL_MAX       16          /* 20 ms */
#endif

/** Default MTU size before negotiation */
#ifndef BLE_DEFAULT_MTU
#define BLE_DEFAULT_MTU             23
#endif

/** Connection handle invalid value */
#ifndef BLE_CONN_HANDLE_INVALID
#define BLE_CONN_HANDLE_INVALID     0xFFFF
#endif

/** Slave latency (number of connection events) */
#ifndef BLE_SLAVE_LATENCY
#define BLE_SLAVE_LATENCY           0
#endif

/** Connection supervision timeout (units of 10 ms) */
#ifndef BLE_CONN_SUP_TIMEOUT
#define BLE_CONN_SUP_TIMEOUT        400         /* 4000 ms */
#endif

/* ============================================================================
 * Buffer Configuration (nRF52 has 64KB RAM)
 * ============================================================================ */

/** Receive buffer size (bytes) */
#ifndef BLE_RX_BUFFER_SIZE
#define BLE_RX_BUFFER_SIZE          2048
#endif

/** Transmit buffer size (bytes) */
#ifndef BLE_TX_BUFFER_SIZE
#define BLE_TX_BUFFER_SIZE          2048
#endif

/** Maximum packet size for fragmentation */
#ifndef BLE_MAX_PACKET_SIZE
#define BLE_MAX_PACKET_SIZE         247
#endif

/* ============================================================================
 * Timeout Configuration
 * ============================================================================ */

/** Connection timeout (milliseconds) */
#ifndef BLE_CONNECT_TIMEOUT_MS
#define BLE_CONNECT_TIMEOUT_MS      10000
#endif

/** Default read timeout (milliseconds) */
#ifndef BLE_READ_TIMEOUT_MS
#define BLE_READ_TIMEOUT_MS         5000
#endif

/** Default write timeout (milliseconds) */
#ifndef BLE_WRITE_TIMEOUT_MS
#define BLE_WRITE_TIMEOUT_MS        5000
#endif

/* ============================================================================
 * SoftDevice Configuration
 * ============================================================================ */

/** SoftDevice to use (S132, S140) */
#ifndef BLE_SOFTDEVICE
#define BLE_SOFTDEVICE              "S140"      /* nRF52840 */
#endif

/** Number of vendor-specific UUIDs */
#ifndef BLE_VENDOR_UUID_COUNT
#define BLE_VENDOR_UUID_COUNT       1
#endif

/** Maximum number of connections (peripheral + central) */
#ifndef BLE_MAX_CONNECTIONS
#define BLE_MAX_CONNECTIONS         1
#endif

#ifdef __cplusplus
}
#endif

#endif /* BLE_CONFIG_NORDIC_H */

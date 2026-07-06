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
 * @brief BLE Configuration for Nordic nRF52/nRF53 (Gateway)
 * 
 * Nordic-specific BLE configuration for gateway application.
 */

#ifndef BLE_CONFIG_NORDIC_GATEWAY_H
#define BLE_CONFIG_NORDIC_GATEWAY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../ble_sal.h"  /* For BLE protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * BLE Device Configuration (Nordic Gateway)
 * ============================================================================ */

/** Gateway device name */
#ifndef BLE_GATEWAY_NAME
#define BLE_GATEWAY_NAME            "Nordic_Gateway"
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

/** Connection timeout (ms) */
#ifndef BLE_CONNECTION_TIMEOUT_MS
#define BLE_CONNECTION_TIMEOUT_MS   10000
#endif

/* ============================================================================
 * BLE Scanning Parameters (Gateway as Central)
 * ============================================================================ */

/** Scan interval (units of 0.625ms) */
#ifndef BLE_SCAN_INTERVAL
#define BLE_SCAN_INTERVAL           0x00A0      /* 100ms */
#endif

/** Scan window (units of 0.625ms) */
#ifndef BLE_SCAN_WINDOW
#define BLE_SCAN_WINDOW             0x0050      /* 50ms */
#endif

/* NOTE: Common scanning parameters (BLE_SCAN_DURATION, BLE_SCAN_ACTIVE)
 *       are defined in ble_sal.h */

/* ============================================================================
 * BLE GAP Configuration
 * ============================================================================ */

/* NOTE: BLE Service UUIDs are defined in ble_sal.h as standard protocol constants:
 *   - BLE_NORDIC_UART_SERVICE_UUID
 *   - BLE_NORDIC_UART_TX_CHAR_UUID  
 *   - BLE_NORDIC_UART_RX_CHAR_UUID
 * Platform-specific implementations convert these to required byte array format.
 */

/** Minimum connection interval (units of 1.25ms) */
#ifndef BLE_MIN_CONN_INTERVAL
#define BLE_MIN_CONN_INTERVAL       MSEC_TO_UNITS(7.5, UNIT_1_25_MS)
#endif

/** Maximum connection interval (units of 1.25ms) */
#ifndef BLE_MAX_CONN_INTERVAL
#define BLE_MAX_CONN_INTERVAL       MSEC_TO_UNITS(30, UNIT_1_25_MS)
#endif

/** Slave latency */
#ifndef BLE_SLAVE_LATENCY
#define BLE_SLAVE_LATENCY           0
#endif

/** Connection supervision timeout (units of 10ms) */
#ifndef BLE_CONN_SUP_TIMEOUT
#define BLE_CONN_SUP_TIMEOUT        MSEC_TO_UNITS(4000, UNIT_10_MS)
#endif

/* ============================================================================
 * BLE Security
 * ============================================================================ */

/* NOTE: Common security setting (BLE_USE_BONDING) is defined in ble_sal.h */

/** Security mode (1=open, 2=encrypted) */
#ifndef BLE_SECURITY_MODE
#define BLE_SECURITY_MODE           1
#endif

/** Security level (1=no auth, 2=auth, 3=LESC) */
#ifndef BLE_SECURITY_LEVEL
#define BLE_SECURITY_LEVEL          1
#endif

/* ============================================================================
 * BLE GATT Client Configuration
 * ============================================================================ */

/* NOTE: Common GATT settings (BLE_ENABLE_NOTIFICATIONS, BLE_GATT_TIMEOUT_MS)
 *       are defined in ble_sal.h */

/* ============================================================================
 * BLE Power Management
 * ============================================================================ */

/** TX Power (dBm) - nRF52: -40, -20, -16, -12, -8, -4, 0, +3, +4 */
#ifndef BLE_TX_POWER_DBM
#define BLE_TX_POWER_DBM            4           /* +4 dBm */
#endif

/* ============================================================================
 * BLE Buffer Configuration
 * ============================================================================ */

/** Maximum simultaneous connections */
#ifndef BLE_MAX_CONNECTIONS
#define BLE_MAX_CONNECTIONS         4
#endif

/** RX Queue size */
#ifndef BLE_RX_QUEUE_SIZE
#define BLE_RX_QUEUE_SIZE           10
#endif

/** RX Data buffer size */
#ifndef BLE_RX_DATA_BUFFER_SIZE
#define BLE_RX_DATA_BUFFER_SIZE     512
#endif

/** Write buffer size */
#ifndef BLE_WRITE_BUFFER_SIZE
#define BLE_WRITE_BUFFER_SIZE       512
#endif

/** Read buffer size */
#ifndef BLE_READ_BUFFER_SIZE
#define BLE_READ_BUFFER_SIZE        512
#endif

/* ============================================================================
 * BLE Debug Configuration
 * ============================================================================ */

/* NOTE: Common debug setting (BLE_DEBUG_ENABLED) is defined in ble_sal.h */

/* ============================================================================
 * Gateway Application Configuration (BLE-specific)
 * ============================================================================ */

/** Auto-connect on gateway startup */
#ifndef BLE_AUTO_CONNECT
#define BLE_AUTO_CONNECT            true
#endif

/** Number of connection retry attempts - BLE needs more retries */
#ifndef BLE_CONNECT_RETRIES
#define BLE_CONNECT_RETRIES         50
#endif

/** Delay between connection retries (ms) - BLE needs longer delays */
#ifndef BLE_CONNECT_RETRY_DELAY_MS
#define BLE_CONNECT_RETRY_DELAY_MS  2000
#endif

/** Request queue size - BLE is slower, smaller queue */
#ifndef BLE_QUEUE_SIZE
#define BLE_QUEUE_SIZE              8
#endif

/** Maximum message size for reassembly */
#ifndef BLE_MAX_MESSAGE_SIZE
#define BLE_MAX_MESSAGE_SIZE        8192
#endif

/** Fragment size for large messages - BLE has low MTU */
#ifndef BLE_FRAGMENT_SIZE
#define BLE_FRAGMENT_SIZE           40
#endif

/** Reassembly timeout (ms) - BLE is slower, needs longer timeout */
#ifndef BLE_REASSEMBLY_TIMEOUT_MS
#define BLE_REASSEMBLY_TIMEOUT_MS   5000
#endif

/** Process KTA requests sequentially (required for BLE) */
#ifndef BLE_SEQUENTIAL_PROCESSING
#define BLE_SEQUENTIAL_PROCESSING   true
#endif

#ifdef __cplusplus
}
#endif

#endif /* BLE_CONFIG_NORDIC_GATEWAY_H */

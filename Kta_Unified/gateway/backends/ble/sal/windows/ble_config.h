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
 * @brief BLE Configuration for Windows (Gateway)
 * 
 * Windows-specific BLE configuration for gateway application.
 */

#ifndef BLE_CONFIG_WINDOWS_GATEWAY_H
#define BLE_CONFIG_WINDOWS_GATEWAY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../ble_sal.h"  /* For BLE protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * BLE Device Configuration (Windows Gateway)
 * ============================================================================ */

/** Gateway device name */
#ifndef BLE_GATEWAY_NAME
#define BLE_GATEWAY_NAME            "Windows_Gateway"
#endif

/* ============================================================================
 * BLE Connection Parameters
 * ============================================================================ */

/** Requested MTU size (Windows 10+ supports large MTU) */
#ifndef BLE_MTU_SIZE
#define BLE_MTU_SIZE                244
#endif

/** Connection timeout (ms) */
#ifndef BLE_CONNECTION_TIMEOUT_MS
#define BLE_CONNECTION_TIMEOUT_MS   10000
#endif

/** Auto-reconnect on disconnection */
#ifndef BLE_AUTO_RECONNECT
#define BLE_AUTO_RECONNECT          false
#endif

/* ============================================================================
 * BLE Scanning Parameters (Gateway as Central)
 * ============================================================================ */

/* NOTE: Common scanning parameters (BLE_SCAN_DURATION, BLE_SCAN_ACTIVE,
 *       BLE_SCAN_FILTER_DUPLICATES) are defined in ble_sal.h */

/* ============================================================================
 * BLE Security
 * ============================================================================ */

/* NOTE: BLE Service UUIDs are defined in ble_sal.h as standard protocol constants:
 *   - BLE_NORDIC_UART_SERVICE_UUID
 *   - BLE_NORDIC_UART_TX_CHAR_UUID  
 *   - BLE_NORDIC_UART_RX_CHAR_UUID
 * WinRT implementation converts these to Windows GUID format internally.
 */

/* NOTE: Common security setting (BLE_USE_BONDING) is defined in ble_sal.h */

/** Require encryption */
#ifndef BLE_REQUIRE_ENCRYPTION
#define BLE_REQUIRE_ENCRYPTION      false
#endif

/* ============================================================================
 * BLE GATT Client Configuration
 * ============================================================================ */

/* NOTE: Common GATT settings (BLE_ENABLE_NOTIFICATIONS, BLE_GATT_TIMEOUT_MS)
 *       are defined in ble_sal.h */

/** Maximum write size per operation (bytes) */
#ifndef BLE_MAX_WRITE_SIZE
#define BLE_MAX_WRITE_SIZE          512
#endif

/** Use WriteWithoutResponse when possible */
#ifndef BLE_USE_WRITE_WITHOUT_RESPONSE
#define BLE_USE_WRITE_WITHOUT_RESPONSE  true
#endif

/* ============================================================================
 * Windows Runtime Configuration
 * ============================================================================ */

/** Use Windows Runtime (WinRT) C++/WinRT bindings */
#ifndef BLE_USE_WINRT
#define BLE_USE_WINRT               true
#endif

/** Async operation timeout (ms) */
#ifndef BLE_ASYNC_TIMEOUT_MS
#define BLE_ASYNC_TIMEOUT_MS        10000
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

/** Log to Windows Event Log */
#ifndef BLE_USE_EVENT_LOG
#define BLE_USE_EVENT_LOG           false
#endif

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

#endif /* BLE_CONFIG_WINDOWS_GATEWAY_H */

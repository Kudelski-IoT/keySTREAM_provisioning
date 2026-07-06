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
 * @brief BLE Transport Configuration for MCU
 * 
 * Platform-specific BLE configuration settings.
 * Modify these values to match your requirements.
 */

#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "../../ble_sal.h"  /* For BLE protocol constants */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * BLE Device Configuration
 * ============================================================================ */

/** Device name (advertised) */
#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME             "esp32_KTA"
#endif

/** Maximum device name length */
#ifndef BLE_DEVICE_NAME_MAX_LEN
#define BLE_DEVICE_NAME_MAX_LEN     31
#endif

/* ============================================================================
 * BLE Connection Parameters
 * ============================================================================ */

/** Requested MTU size (minimum 23, maximum depends on platform) */
#ifndef BLE_MTU_SIZE
#define BLE_MTU_SIZE                244
#endif

/** Default MTU size before negotiation */
#ifndef BLE_DEFAULT_MTU
#define BLE_DEFAULT_MTU             23
#endif

/** RX Queue size (number of messages) */
#ifndef BLE_RX_QUEUE_SIZE
#define BLE_RX_QUEUE_SIZE           10
#endif

/** RX Data buffer size per message (bytes) */
#ifndef BLE_RX_DATA_BUFFER_SIZE
#define BLE_RX_DATA_BUFFER_SIZE     512
#endif

/** Use bonding/pairing */
#ifndef BLE_USE_BONDING
#define BLE_USE_BONDING             true
#endif

/** TX Power (dBm) - platform-specific range */
#ifndef BLE_TX_POWER_DBM
#define BLE_TX_POWER_DBM            0
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
 * Buffer Configuration
 * ============================================================================ */

/** Receive buffer size (bytes) */
#ifndef BLE_RX_BUFFER_SIZE
#define BLE_RX_BUFFER_SIZE          1024
#endif

/** Transmit buffer size (bytes) */
#ifndef BLE_TX_BUFFER_SIZE
#define BLE_TX_BUFFER_SIZE          1024
#endif

/** Maximum packet size for fragmentation */
#ifndef BLE_MAX_PACKET_SIZE
#define BLE_MAX_PACKET_SIZE         244
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
 * Platform-Specific Settings
 * ============================================================================ */

/* ESP32 Configuration */
#if defined(CONFIG_IDF_TARGET_ESP32)
    #ifndef BLE_MTU_SIZE
    #define BLE_MTU_SIZE            185         /* ESP32 maximum */
    #endif
    #ifndef BLE_TX_POWER_DBM
    #define BLE_TX_POWER_DBM        3           /* +3 dBm */
    #endif
#endif

/* Nordic nRF52 Configuration */
#if defined(NRF52)
    #ifndef BLE_MTU_SIZE
    #define BLE_MTU_SIZE            247         /* nRF52 maximum */
    #endif
    #ifndef BLE_TX_POWER_DBM
    #define BLE_TX_POWER_DBM        4           /* +4 dBm */
    #endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* BLE_CONFIG_H */

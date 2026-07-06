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
 * @file ble_sal.h
 * @brief BLE Software Abstraction Layer Interface
 * 
 * Platform-independent BLE interface. Each platform (Windows/Linux/Mac/ESP32/Nordic)
 * provides its own implementation.
 */

#ifndef BLE_SAL_H
#define BLE_SAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * BLE Protocol Constants (Standard - Do Not Modify)
 * ============================================================================ */

/** Nordic UART Service UUID (NUS) - Industry Standard */
#define BLE_NORDIC_UART_SERVICE_UUID    "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"

/** Nordic UART TX Characteristic UUID (Gateway/Central writes to device) */
#define BLE_NORDIC_UART_TX_CHAR_UUID    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

/** Nordic UART RX Characteristic UUID (Gateway/Central reads from device) */
#define BLE_NORDIC_UART_RX_CHAR_UUID    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

/** BLE 4.0 Specification Minimum MTU (ATT_MTU) */
#define BLE_MIN_MTU_SIZE                23

/** BLE Specification Default MTU (before negotiation) */
#define BLE_DEFAULT_MTU                 23

/* ============================================================================
 * Common BLE Configuration (Protocol-Level, Not Platform-Specific)
 * ============================================================================ */

/** Enable GATT notifications (standard BLE behavior) */
#ifndef BLE_ENABLE_NOTIFICATIONS
#define BLE_ENABLE_NOTIFICATIONS    true
#endif

/** GATT operation timeout - standard across all platforms (ms) */
#ifndef BLE_GATT_TIMEOUT_MS
#define BLE_GATT_TIMEOUT_MS         5000
#endif

/** Use bonding/pairing - common security setting */
#ifndef BLE_USE_BONDING
#define BLE_USE_BONDING             false
#endif

/** Scan duration - common scanning behavior (seconds) */
#ifndef BLE_SCAN_DURATION
#define BLE_SCAN_DURATION           10
#endif

/** Use active scanning - common BLE scanning mode */
#ifndef BLE_SCAN_ACTIVE
#define BLE_SCAN_ACTIVE             true
#endif

/** Filter duplicates during scan - common BLE behavior */
#ifndef BLE_SCAN_FILTER_DUPLICATES
#define BLE_SCAN_FILTER_DUPLICATES  true
#endif

/** Enable BLE debug logging - common across platforms */
#ifndef BLE_DEBUG_ENABLED
#define BLE_DEBUG_ENABLED           true
#endif

/* BLE SAL Status Codes */
typedef enum {
    BLE_SAL_OK = 0,
    BLE_SAL_ERROR = -1,
    BLE_SAL_TIMEOUT = -2,
    BLE_SAL_NOT_CONNECTED = -3,
    BLE_SAL_INVALID_PARAM = -4,
    BLE_SAL_NOT_FOUND = -5,
} BleSalStatus;

/* BLE Configuration */
typedef struct {
    char device_address[18];       /* BLE MAC address or device name */
    uint16_t mtu;
    bool use_bonding;
    uint32_t scan_timeout_ms;
} BleSalConfig;

/* Platform-specific BLE SAL functions */
BleSalStatus ble_sal_init(void);
BleSalStatus ble_sal_deinit(void);
BleSalStatus ble_sal_scan(const BleSalConfig *config);
BleSalStatus ble_sal_connect(const BleSalConfig *config);
BleSalStatus ble_sal_disconnect(void);
BleSalStatus ble_sal_write(const uint8_t *data, size_t length, size_t *written);
BleSalStatus ble_sal_read(uint8_t *buffer, size_t buffer_size, size_t *read_count);
BleSalStatus ble_sal_set_timeout(uint32_t timeout_ms);
BleSalStatus ble_sal_get_mtu(uint16_t *mtu);

#ifdef __cplusplus
}
#endif

#endif /* BLE_SAL_H */

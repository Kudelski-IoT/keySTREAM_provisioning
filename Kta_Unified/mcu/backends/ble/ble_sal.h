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
 * Provides a standardized interface for BLE operations across different platforms.
 * Each platform (ESP32, Nordic nRF, Microchip, STM32, etc.) implements this interface.
 * 
 * This interface is platform-independent and remains the same regardless of
 * the underlying BLE stack (ESP-IDF, Nordic SoftDevice, Bluez, etc.).
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

/** Nordic UART TX Characteristic UUID (MCU receives data on this characteristic) */
#define BLE_NORDIC_UART_TX_CHAR_UUID    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

/** Nordic UART RX Characteristic UUID (MCU sends data on this characteristic) */
#define BLE_NORDIC_UART_RX_CHAR_UUID    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

/** BLE 4.0 Specification Minimum MTU (ATT_MTU) */
#define BLE_MIN_MTU_SIZE                23

/* ============================================================================
 * BLE Configuration
 * ============================================================================ */

#define BLE_SAL_MAX_DEVICE_NAME 32
#define BLE_SAL_MAX_SERVICE_UUID 16
#define BLE_SAL_MAX_CHAR_UUID 16

typedef enum {
    BLE_ROLE_PERIPHERAL = 0,  /* BLE Peripheral (GATT Server) */
    BLE_ROLE_CENTRAL = 1      /* BLE Central (GATT Client) */
} BleRole;

typedef struct {
    char device_name[BLE_SAL_MAX_DEVICE_NAME];  /* BLE device name for advertising */
    BleRole role;                                /* Peripheral or Central */
    uint16_t mtu;                                /* Preferred MTU (23-512) */
    bool use_bonding;                            /* Enable pairing/bonding */
    uint16_t conn_interval_min;                  /* Min connection interval (units of 1.25ms) */
    uint16_t conn_interval_max;                  /* Max connection interval */
} BleConfig;

/* ============================================================================
 * BLE Status Codes
 * ============================================================================ */

typedef enum {
    BLE_SAL_OK = 0,
    BLE_SAL_ERROR = -1,
    BLE_SAL_TIMEOUT = -2,
    BLE_SAL_INVALID_PARAM = -3,
    BLE_SAL_NOT_INITIALIZED = -4,
    BLE_SAL_NOT_CONNECTED = -5,
    BLE_SAL_BUFFER_FULL = -6,
    BLE_SAL_MTU_EXCEEDED = -7
} BleSalStatus;

/* ============================================================================
 * BLE Connection State
 * ============================================================================ */

typedef enum {
    BLE_STATE_IDLE = 0,
    BLE_STATE_ADVERTISING = 1,
    BLE_STATE_CONNECTED = 2,
    BLE_STATE_DISCONNECTED = 3
} BleState;

/* ============================================================================
 * BLE Callbacks (Event-driven)
 * ============================================================================ */

typedef void (*BleConnectedCallback)(void);
typedef void (*BleDisconnectedCallback)(void);
typedef void (*BleDataReceivedCallback)(const uint8_t *data, size_t length);

typedef struct {
    BleConnectedCallback on_connected;
    BleDisconnectedCallback on_disconnected;
    BleDataReceivedCallback on_data_received;
} BleCallbacks;

/* ============================================================================
 * BLE SAL Interface Functions
 * ============================================================================ */

/**
 * @brief Initialize BLE stack and register callbacks
 * 
 * @param config BLE configuration parameters
 * @param callbacks Event callbacks (can be NULL)
 * @return BLE_SAL_OK on success, error code otherwise
 */
BleSalStatus ble_sal_init(const BleConfig *config, const BleCallbacks *callbacks);

/**
 * @brief Deinitialize BLE stack and release resources
 * 
 * @return BLE_SAL_OK on success, error code otherwise
 */
BleSalStatus ble_sal_deinit(void);

/**
 * @brief Start BLE advertising (for Peripheral role)
 * 
 * @return BLE_SAL_OK on success, error code otherwise
 */
BleSalStatus ble_sal_start_advertising(void);

/**
 * @brief Stop BLE advertising
 * 
 * @return BLE_SAL_OK on success, error code otherwise
 */
BleSalStatus ble_sal_stop_advertising(void);

/**
 * @brief Connect to a BLE peripheral (for Central role)
 * 
 * @param addr BLE device address (6 bytes)
 * @param timeout_ms Connection timeout in milliseconds
 * @return BLE_SAL_OK on success, error code otherwise
 */
BleSalStatus ble_sal_connect(const uint8_t addr[6], uint32_t timeout_ms);

/**
 * @brief Disconnect from BLE peer
 * 
 * @return BLE_SAL_OK on success, error code otherwise
 */
BleSalStatus ble_sal_disconnect(void);

/**
 * @brief Send data over BLE (notify/indicate)
 * 
 * @param data Pointer to data buffer
 * @param length Number of bytes to send
 * @return BLE_SAL_OK on success, error code otherwise
 */
BleSalStatus ble_sal_send(const uint8_t *data, size_t length);

/**
 * @brief Receive data from BLE (typically called from callback context)
 * 
 * @param buffer Pointer to receive buffer
 * @param buffer_size Size of receive buffer
 * @param bytes_read Pointer to store number of bytes actually read
 * @param timeout_ms Timeout in milliseconds
 * @return BLE_SAL_OK on success, error code otherwise
 */
BleSalStatus ble_sal_receive(uint8_t *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms);

/**
 * @brief Get current BLE connection state
 * 
 * @param state Pointer to store current state
 * @return BLE_SAL_OK on success, error code otherwise
 */
BleSalStatus ble_sal_get_state(BleState *state);

/**
 * @brief Get negotiated MTU size
 * 
 * @param mtu Pointer to store MTU value
 * @return BLE_SAL_OK on success, error code otherwise
 */
BleSalStatus ble_sal_get_mtu(uint16_t *mtu);

/**
 * @brief Set BLE transmission power
 * 
 * @param power_dbm Power in dBm (-12 to +9 typical range)
 * @return BLE_SAL_OK on success, error code otherwise
 */
BleSalStatus ble_sal_set_tx_power(int8_t power_dbm);

#ifdef __cplusplus
}
#endif

#endif /* BLE_SAL_H */

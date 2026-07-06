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
 * @file zigbee_sal.h
 * @brief Zigbee Software Abstraction Layer Interface
 * 
 * Platform-independent Zigbee interface for MCU backends.
 * Provides abstraction over different Zigbee stacks:
 * - ESP32-C6/H2: ESP-Zigbee SDK
 * - Silicon Labs: EmberZNet (EFR32)
 * - NXP: JN51xx Zigbee stack
 * - TI: Z-Stack (CC2652)
 */

#ifndef ZIGBEE_SAL_H
#define ZIGBEE_SAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Zigbee Protocol Constants (IEEE 802.15.4 / Zigbee Alliance - Do Not Modify)
 * ============================================================================ */

/** IEEE 802.15.4 Channel Range (2.4 GHz band) */
#define ZIGBEE_CHANNEL_MIN              11
#define ZIGBEE_CHANNEL_MAX              26

/** Standard Zigbee Profiles */
#define ZIGBEE_PROFILE_HOME_AUTOMATION  0x0104
#define ZIGBEE_PROFILE_LIGHT_LINK       0xC05E

/** IEEE 802.15.4 Address Lengths */
#define ZIGBEE_SHORT_ADDR_LEN           2   /* 16-bit short address */
#define ZIGBEE_EXT_ADDR_LEN             8   /* 64-bit extended (IEEE) address */

/* ============================================================================
 * Zigbee SAL Status Codes
 * ============================================================================ */

typedef enum {
    ZIGBEE_SAL_OK = 0,
    ZIGBEE_SAL_ERROR = -1,
    ZIGBEE_SAL_TIMEOUT = -2,
    ZIGBEE_SAL_INVALID_PARAM = -3,
    ZIGBEE_SAL_NOT_INITIALIZED = -4,
    ZIGBEE_SAL_NOT_CONNECTED = -5,
    ZIGBEE_SAL_BUFFER_FULL = -6,
    ZIGBEE_SAL_NOT_JOINED = -7,
    ZIGBEE_SAL_JOIN_FAILED = -8
} ZigbeeSalStatus;

/* ============================================================================
 * Zigbee Device Types
 * ============================================================================ */

typedef enum {
    ZIGBEE_DEVICE_COORDINATOR = 0,  /**< Network coordinator (forms network) */
    ZIGBEE_DEVICE_ROUTER = 1,       /**< Router (routes packets, can have children) */
    ZIGBEE_DEVICE_END_DEVICE = 2    /**< End device (leaf node, low power) */
} ZigbeeDeviceType;

/* ============================================================================
 * Zigbee State
 * ============================================================================ */

typedef enum {
    ZIGBEE_STATE_IDLE = 0,          /**< Not initialized or left network */
    ZIGBEE_STATE_JOINING,           /**< Attempting to join network */
    ZIGBEE_STATE_JOINED,            /**< Successfully joined network */
    ZIGBEE_STATE_CONNECTED,         /**< Connected and ready for data */
    ZIGBEE_STATE_ERROR              /**< Error state */
} ZigbeeState;

/* ============================================================================
 * Zigbee Configuration
 * ============================================================================ */

/**
 * @brief Zigbee network configuration
 */
typedef struct {
    ZigbeeDeviceType device_type;   /**< Device type (coordinator/router/end device) */
    uint16_t pan_id;                /**< PAN ID (0xFFFF = join any) */
    uint8_t channel;                /**< Channel (11-26 for 2.4 GHz) */
    uint8_t extended_address[8];    /**< IEEE 802.15.4 extended address (64-bit) */
    uint8_t network_key[16];        /**< Network encryption key (128-bit) */
    bool security_enabled;          /**< Enable network security */
    int8_t tx_power_dbm;            /**< TX power in dBm */
} ZigbeeConfig;

/**
 * @brief Zigbee callbacks
 */
typedef struct {
    void (*on_network_joined)(uint16_t pan_id, uint16_t short_addr);  /**< Network joined */
    void (*on_network_left)(void);                                     /**< Network left */
    void (*on_data_received)(uint16_t src_addr, const uint8_t *data, size_t length);  /**< Data received */
    void (*on_error)(ZigbeeSalStatus error);                           /**< Error occurred */
} ZigbeeCallbacks;

/* ============================================================================
 * Zigbee SAL Interface Functions
 * ============================================================================ */

/**
 * @brief Initialize Zigbee stack
 * 
 * @param[in] config Zigbee configuration
 * @param[in] callbacks Event callbacks (optional, can be NULL)
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_init(const ZigbeeConfig *config, const ZigbeeCallbacks *callbacks);

/**
 * @brief Deinitialize Zigbee stack
 * 
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_deinit(void);

/**
 * @brief Start Zigbee stack
 * 
 * Starts the Zigbee stack. For coordinators, forms a network.
 * For routers/end devices, initiates network scan.
 * 
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_start(void);

/**
 * @brief Join Zigbee network
 * 
 * Attempts to join a Zigbee network (for routers/end devices).
 * 
 * @param[in] timeout_ms Timeout in milliseconds
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_join_network(uint32_t timeout_ms);

/**
 * @brief Leave Zigbee network
 * 
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_leave_network(void);

/**
 * @brief Send data to a Zigbee device
 * 
 * @param[in] dest_addr Destination short address (0x0000 = coordinator, 0xFFFF = broadcast)
 * @param[in] data Data buffer to send
 * @param[in] length Number of bytes to send
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_send(uint16_t dest_addr, const uint8_t *data, size_t length);

/**
 * @brief Receive data from Zigbee
 * 
 * @param[out] buffer Buffer to store received data
 * @param[in] buffer_size Size of buffer
 * @param[out] bytes_received Number of bytes actually received
 * @param[in] timeout_ms Timeout in milliseconds
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_receive(uint8_t *buffer, size_t buffer_size, size_t *bytes_received, uint32_t timeout_ms);

/**
 * @brief Get device short address
 * 
 * @param[out] addr Short address (16-bit)
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_get_short_address(uint16_t *addr);

/**
 * @brief Get PAN ID
 * 
 * @param[out] pan_id PAN ID (16-bit)
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_get_pan_id(uint16_t *pan_id);

/**
 * @brief Check if joined to network
 * 
 * @param[out] joined True if joined to network
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_is_joined(bool *joined);

/**
 * @brief Get current Zigbee state
 * 
 * @param[out] state Current Zigbee state
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_get_state(ZigbeeState *state);

/**
 * @brief Set TX power
 * 
 * @param[in] power_dbm TX power in dBm (range depends on platform)
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_set_tx_power(int8_t power_dbm);

/**
 * @brief Get channel
 * 
 * @param[out] channel Current channel (11-26)
 * @return ZIGBEE_SAL_OK on success, error code otherwise
 */
ZigbeeSalStatus zigbee_sal_get_channel(uint8_t *channel);

#ifdef __cplusplus
}
#endif

#endif /* ZIGBEE_SAL_H */

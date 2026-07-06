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
 * Platform-independent Zigbee interface. Each platform provides its own implementation.
 */

#ifndef ZIGBEE_SAL_H
#define ZIGBEE_SAL_H

#include <stdint.h>
#include <stddef.h>   /* size_t */
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Zigbee Protocol Constants (IEEE 802.15.4 / Zigbee Alliance - Do Not Modify)
 * ============================================================================ */

/** Zigbee Device Types (per Zigbee specification) */
#define ZIGBEE_DEVICE_TYPE_COORDINATOR  0  /* Network Coordinator */
#define ZIGBEE_DEVICE_TYPE_ROUTER       1  /* Router */
#define ZIGBEE_DEVICE_TYPE_END_DEVICE   2  /* End Device */

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
 * Common Zigbee Configuration (Protocol-Level, Not Platform-Specific)
 * ============================================================================ */

/** Default Zigbee channel - common starting point */
#ifndef ZIGBEE_CHANNEL_DEFAULT
#define ZIGBEE_CHANNEL_DEFAULT      15
#endif

/** Default TX power (dBm) - balanced power/range */
#ifndef ZIGBEE_TX_POWER
#define ZIGBEE_TX_POWER             3
#endif

/** Security enabled by default - recommended practice */
#ifndef ZIGBEE_SECURITY_ENABLED
#define ZIGBEE_SECURITY_ENABLED     true
#endif

/** Enable permit joining - common for coordinator/router */
#ifndef ZIGBEE_PERMIT_JOINING
#define ZIGBEE_PERMIT_JOINING       true
#endif

/** Permit join duration - standard timeout (seconds) */
#ifndef ZIGBEE_PERMIT_JOIN_DURATION
#define ZIGBEE_PERMIT_JOIN_DURATION 180
#endif

/** Connection timeout - standard across platforms (ms) */
#ifndef ZIGBEE_CONNECTION_TIMEOUT_MS
#define ZIGBEE_CONNECTION_TIMEOUT_MS    10000
#endif

/** Read timeout - standard across platforms (ms) */
#ifndef ZIGBEE_READ_TIMEOUT_MS
#define ZIGBEE_READ_TIMEOUT_MS      1000
#endif

/** Write timeout - standard across platforms (ms) */
#ifndef ZIGBEE_WRITE_TIMEOUT_MS
#define ZIGBEE_WRITE_TIMEOUT_MS     1000
#endif

/** Receive buffer size - common across platforms */
#ifndef ZIGBEE_RX_BUFFER_SIZE
#define ZIGBEE_RX_BUFFER_SIZE       2048
#endif

/** Transmit buffer size - common across platforms */
#ifndef ZIGBEE_TX_BUFFER_SIZE
#define ZIGBEE_TX_BUFFER_SIZE       2048
#endif

/** Enable debug logging - common across platforms */
#ifndef ZIGBEE_DEBUG_ENABLED
#define ZIGBEE_DEBUG_ENABLED        true
#endif

/* Zigbee SAL Status Codes */
typedef enum {
    ZIGBEE_SAL_OK = 0,
    ZIGBEE_SAL_ERROR = -1,
    ZIGBEE_SAL_TIMEOUT = -2,
    ZIGBEE_SAL_NOT_CONNECTED = -3,
    ZIGBEE_SAL_INVALID_PARAM = -4,
} ZigbeeSalStatus;

/* Zigbee Configuration */
typedef struct {
    uint16_t pan_id;
    uint16_t channel;
    uint8_t tx_power;
    char device_address[24];
} ZigbeeSalConfig;

/* Platform-specific Zigbee SAL functions */
ZigbeeSalStatus zigbee_sal_init(void);
ZigbeeSalStatus zigbee_sal_deinit(void);
ZigbeeSalStatus zigbee_sal_connect(const ZigbeeSalConfig *config);
ZigbeeSalStatus zigbee_sal_disconnect(void);
ZigbeeSalStatus zigbee_sal_write(const uint8_t *data, size_t length, size_t *written);
ZigbeeSalStatus zigbee_sal_read(uint8_t *buffer, size_t buffer_size, size_t *read_count);
ZigbeeSalStatus zigbee_sal_set_timeout(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* ZIGBEE_SAL_H */

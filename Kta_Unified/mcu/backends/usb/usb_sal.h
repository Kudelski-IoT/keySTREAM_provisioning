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
 * @file usb_sal.h
 * @brief USB Software Abstraction Layer Interface
 * 
 * Platform-independent USB CDC interface for MCU backends.
 * Provides abstraction over different USB stacks:
 * - ESP32-S2/S3: TinyUSB
 * - Microchip: Harmony3 USB Device Stack
 * - STM32: STM32 HAL USB Device
 * - Nordic: nRF5 SDK USB (nRF52840)
 */

#ifndef USB_SAL_H
#define USB_SAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * USB Protocol Constants (USB-IF Standard - Do Not Modify)
 * ============================================================================ */

/** USB Class Codes (from USB-IF) */
#define USB_CLASS_CDC               0x02  /* Communications Device Class */
#define USB_CLASS_DATA              0x0A  /* CDC-Data */

/** USB CDC Subclass Codes */
#define USB_CDC_SUBCLASS_ACM        0x02  /* Abstract Control Model */

/** USB CDC Protocol Codes */
#define USB_CDC_PROTOCOL_NONE       0x00  /* No class specific protocol */
#define USB_CDC_PROTOCOL_AT         0x01  /* AT Commands (v.25ter) */

/** USB Endpoint Direction Masks */
#define USB_ENDPOINT_DIR_MASK       0x80
#define USB_ENDPOINT_IN             0x80  /* Device to Host */
#define USB_ENDPOINT_OUT            0x00  /* Host to Device */

/** USB Endpoint Number Mask */
#define USB_ENDPOINT_NUM_MASK       0x0F

/* ============================================================================
 * USB SAL Status Codes
 * ============================================================================ */

typedef enum {
    USB_SAL_OK = 0,
    USB_SAL_ERROR = -1,
    USB_SAL_TIMEOUT = -2,
    USB_SAL_INVALID_PARAM = -3,
    USB_SAL_NOT_INITIALIZED = -4,
    USB_SAL_NOT_CONNECTED = -5,
    USB_SAL_BUFFER_FULL = -6,
    USB_SAL_BUFFER_EMPTY = -7
} UsbSalStatus;

/* ============================================================================
 * USB Configuration
 * ============================================================================ */

/**
 * @brief USB device configuration
 */
typedef struct {
    uint16_t vendor_id;             /**< USB Vendor ID */
    uint16_t product_id;            /**< USB Product ID */
    const char *manufacturer;       /**< Manufacturer string */
    const char *product;            /**< Product string */
    const char *serial_number;      /**< Serial number string */
    uint16_t max_packet_size;       /**< Max packet size (64 or 512) */
    bool high_speed;                /**< High-Speed USB support */
} UsbConfig;

/* ============================================================================
 * USB SAL Interface Functions
 * ============================================================================ */

/**
 * @brief Initialize USB CDC interface
 * 
 * Initializes the USB device stack and CDC class driver.
 * 
 * @param[in] config USB configuration parameters
 * @return USB_SAL_OK on success, error code otherwise
 */
UsbSalStatus usb_sal_init(const UsbConfig *config);

/**
 * @brief Deinitialize USB interface
 * 
 * @return USB_SAL_OK on success, error code otherwise
 */
UsbSalStatus usb_sal_deinit(void);

/**
 * @brief Write data to USB CDC
 * 
 * @param[in] data Data buffer to send
 * @param[in] length Number of bytes to send
 * @param[in] timeout_ms Timeout in milliseconds
 * @return USB_SAL_OK on success, error code otherwise
 */
UsbSalStatus usb_sal_write(const uint8_t *data, size_t length, uint32_t timeout_ms);

/**
 * @brief Read data from USB CDC
 * 
 * @param[out] buffer Buffer to store received data
 * @param[in] buffer_size Size of buffer
 * @param[out] bytes_read Number of bytes actually read
 * @param[in] timeout_ms Timeout in milliseconds
 * @return USB_SAL_OK on success, error code otherwise
 */
UsbSalStatus usb_sal_read(uint8_t *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms);

/**
 * @brief Check if USB is connected
 * 
 * Checks if USB is enumerated and host is connected (DTR asserted).
 * 
 * @param[out] connected Connection status
 * @return USB_SAL_OK on success, error code otherwise
 */
UsbSalStatus usb_sal_is_connected(bool *connected);

/**
 * @brief Check number of bytes available to read
 * 
 * @param[out] available Number of bytes in RX buffer
 * @return USB_SAL_OK on success, error code otherwise
 */
UsbSalStatus usb_sal_available(size_t *available);

/**
 * @brief Flush TX buffer
 * 
 * Waits for all pending TX data to be transmitted.
 * 
 * @return USB_SAL_OK on success, error code otherwise
 */
UsbSalStatus usb_sal_flush_tx(void);

/**
 * @brief Flush RX buffer
 * 
 * Discards all data in the RX buffer.
 * 
 * @return USB_SAL_OK on success, error code otherwise
 */
UsbSalStatus usb_sal_flush_rx(void);

/**
 * @brief Set timeout for read/write operations
 * 
 * @param[in] timeout_ms Timeout in milliseconds
 * @return USB_SAL_OK on success, error code otherwise
 */
UsbSalStatus usb_sal_set_timeout(uint32_t timeout_ms);

/**
 * @brief Get USB connection state
 * 
 * Returns detailed USB state (configured, suspended, etc.)
 * 
 * @param[out] state USB device state
 * @return USB_SAL_OK on success, error code otherwise
 */
UsbSalStatus usb_sal_get_state(uint8_t *state);

/* ============================================================================
 * USB State Values
 * ============================================================================ */

#define USB_STATE_DETACHED      0   /**< USB cable not connected */
#define USB_STATE_ATTACHED      1   /**< USB cable connected, not enumerated */
#define USB_STATE_POWERED       2   /**< USB powered */
#define USB_STATE_DEFAULT       3   /**< Default state after reset */
#define USB_STATE_ADDRESSED     4   /**< Address assigned by host */
#define USB_STATE_CONFIGURED    5   /**< Configured and ready */
#define USB_STATE_SUSPENDED     6   /**< Suspended (low power) */

#ifdef __cplusplus
}
#endif

#endif /* USB_SAL_H */

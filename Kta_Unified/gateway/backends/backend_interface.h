/**
 * @file backend_interface.h
 * @brief Backend Interface Layer for Gateway
 * 
 * Provides unified interface to transport backends (UART, BLE, USB, Zigbee).
 * Gateway application uses these backends to communicate with MCU.
 * 
 * Architecture (Gateway Side):
 *   HTTP/TLS Client → Bridge Integration → Backend Interface → Backends → SAL (Windows/Linux/Mac/ESP32/etc.)
 *                                                                            ↓
 *                                                                          MCU Device
 */

#ifndef BACKEND_INTERFACE_H
#define BACKEND_INTERFACE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Backend Types
 * ============================================================================ */

typedef enum {
    BACKEND_TYPE_UART    = 0,
    BACKEND_TYPE_USB     = 1,
    BACKEND_TYPE_BLE     = 2,
    BACKEND_TYPE_ZIGBEE  = 3,
} BackendType;

/* ============================================================================
 * Backend Status Codes
 * ============================================================================ */

typedef enum {
    BACKEND_OK = 0,
    BACKEND_ERROR = -1,
    BACKEND_TIMEOUT = -2,
    BACKEND_NOT_CONNECTED = -3,
    BACKEND_INVALID_PARAM = -4,
    BACKEND_NOT_SUPPORTED = -5,
} BackendStatus;

/* ============================================================================
 * Configuration Structures
 * ============================================================================ */

typedef struct {
    char port_name[256];       /* COM port (Windows: "COM3", Linux: "/dev/ttyUSB0", Mac: "/dev/cu.usbserial") */
    uint32_t baud_rate;        /* Baud rate (115200, 921600, etc.) */
    uint8_t data_bits;         /* 7 or 8 */
    uint8_t stop_bits;         /* 1 or 2 */
    uint8_t parity;            /* 0=None, 1=Odd, 2=Even */
    bool flow_control;         /* RTS/CTS */
} BackendUartConfig;

typedef struct {
    char device_address[18];   /* BLE MAC address "XX:XX:XX:XX:XX:XX" or device name */
    uint16_t mtu;              /* Maximum Transmission Unit */
    bool use_bonding;          /* Pairing/bonding */
    uint32_t scan_timeout_ms;  /* Scan timeout for device discovery */
} BackendBleConfig;

typedef struct {
    uint16_t vid;              /* Vendor ID */
    uint16_t pid;              /* Product ID */
    char serial_number[64];    /* USB serial number (optional) */
    uint8_t interface_num;     /* USB interface number */
} BackendUsbConfig;

typedef struct {
    uint16_t pan_id;           /* PAN ID */
    uint16_t channel;          /* Zigbee channel (11-26) */
    uint8_t tx_power;          /* Transmission power */
    char device_address[24];   /* Zigbee device address */
} BackendZigbeeConfig;

typedef struct {
    BackendType type;
    union {
        BackendUartConfig uart;
        BackendBleConfig ble;
        BackendUsbConfig usb;
        BackendZigbeeConfig zigbee;
    } config;
} BackendConfig;

/* ============================================================================
 * Capabilities
 * ============================================================================ */

typedef struct {
    uint16_t max_packet_size;         /* Maximum single packet size */
    uint16_t max_message_size;        /* Maximum message size (with fragmentation) */
    bool supports_fragmentation;      /* Can handle message fragmentation */
    bool requires_connection;         /* Connection-oriented vs connectionless */
    bool is_reliable;                 /* Guaranteed delivery */
    bool is_bidirectional;            /* Supports both TX and RX */
} BackendCapabilities;

/* ============================================================================
 * Backend Interface Structure
 * ============================================================================ */

typedef struct {
    BackendType type;
    
    BackendStatus (*init)(void);
    BackendStatus (*deinit)(void);
    BackendStatus (*get_capabilities)(BackendCapabilities *caps);
    BackendStatus (*open)(const BackendConfig *config);
    BackendStatus (*close)(void);
    BackendStatus (*send)(const uint8_t *data, size_t length);
    BackendStatus (*receive)(uint8_t *buffer, size_t buffer_size, size_t *received_length);
    BackendStatus (*set_timeout)(uint32_t timeout_ms);
} Backend;

/* ============================================================================
 * Backend Registration
 * ============================================================================ */

extern const Backend g_uart_backend;
extern const Backend g_ble_backend;
extern const Backend g_usb_backend;
extern const Backend g_zigbee_backend;

/* ============================================================================
 * Backend Interface Functions
 * ============================================================================ */

/**
 * @brief Initialize backend interface with specified transport type
 * 
 * @param type Backend type (UART/BLE/USB/Zigbee)
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus backend_init(BackendType type);

/**
 * @brief Deinitialize backend interface
 * 
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus backend_deinit(void);

/**
 * @brief Get capabilities of current backend
 * 
 * @param caps Pointer to capabilities structure to fill
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus backend_get_capabilities(BackendCapabilities *caps);

/**
 * @brief Open/configure backend connection
 * 
 * @param config Backend configuration
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus backend_open(const BackendConfig *config);

/**
 * @brief Close backend connection
 * 
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus backend_close(void);

/**
 * @brief Send data through backend to MCU
 * 
 * @param data Data buffer to send
 * @param length Number of bytes to send
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus backend_send(const uint8_t *data, size_t length);

/**
 * @brief Receive data from backend (from MCU)
 * 
 * @param buffer Buffer to store received data
 * @param buffer_size Size of receive buffer
 * @param received_length Pointer to store actual bytes received
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus backend_receive(uint8_t *buffer, size_t buffer_size, size_t *received_length);

/**
 * @brief Set receive timeout
 * 
 * @param timeout_ms Timeout in milliseconds
 * @return BACKEND_OK on success, error code otherwise
 */
BackendStatus backend_set_timeout(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* BACKEND_INTERFACE_H */

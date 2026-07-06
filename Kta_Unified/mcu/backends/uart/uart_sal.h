/**
 * @file uart_sal.h
 * @brief UART Software Abstraction Layer Interface
 * 
 * Provides a standardized interface for UART operations across different platforms.
 * Each platform (ESP32, Microchip, STM32, etc.) implements this interface.
 * 
 * This interface is platform-independent and remains the same regardless of
 * the underlying hardware or RTOS.
 */

#ifndef UART_SAL_H
#define UART_SAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * UART Configuration (RS-232/RS-485 Standard)
 * ============================================================================ */

typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN = 1,
    UART_PARITY_ODD  = 2
} UartParity;

typedef enum {
    UART_STOP_BITS_1 = 0,
    UART_STOP_BITS_2 = 1
} UartStopBits;

typedef enum {
    UART_DATA_BITS_7 = 7,
    UART_DATA_BITS_8 = 8
} UartDataBits;

typedef struct {
    uint8_t port_num;           /* UART port number (0, 1, 2...) */
    uint32_t baud_rate;         /* Baud rate (9600, 115200, 921600, etc.) */
    UartDataBits data_bits;     /* Number of data bits */
    UartStopBits stop_bits;     /* Number of stop bits */
    UartParity parity;          /* Parity mode */
    bool flow_control;          /* Enable RTS/CTS flow control */
    uint16_t rx_buffer_size;    /* Receive buffer size in bytes */
    uint16_t tx_buffer_size;    /* Transmit buffer size in bytes */
} UartConfig;

/* ============================================================================
 * UART Status Codes
 * ============================================================================ */

typedef enum {
    UART_SAL_OK = 0,
    UART_SAL_ERROR = -1,
    UART_SAL_TIMEOUT = -2,
    UART_SAL_INVALID_PARAM = -3,
    UART_SAL_NOT_INITIALIZED = -4,
    UART_SAL_BUFFER_FULL = -5,
    UART_SAL_BUFFER_EMPTY = -6
} UartSalStatus;

/* ============================================================================
 * UART SAL Interface Functions
 * ============================================================================ */

/**
 * @brief Initialize the UART peripheral and allocate resources
 * 
 * @param config UART configuration parameters
 * @return UART_SAL_OK on success, error code otherwise
 */
UartSalStatus uart_sal_init(const UartConfig *config);

/**
 * @brief Deinitialize UART and release resources
 * 
 * @return UART_SAL_OK on success, error code otherwise
 */
UartSalStatus uart_sal_deinit(void);

/**
 * @brief Write data to UART (blocking or timeout-based)
 * 
 * @param data Pointer to data buffer
 * @param length Number of bytes to write
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking)
 * @return UART_SAL_OK on success, error code otherwise
 */
UartSalStatus uart_sal_write(const uint8_t *data, size_t length, uint32_t timeout_ms);

/**
 * @brief Read data from UART (blocking or timeout-based)
 * 
 * @param buffer Pointer to receive buffer
 * @param buffer_size Size of receive buffer
 * @param bytes_read Pointer to store number of bytes actually read
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking)
 * @return UART_SAL_OK on success, error code otherwise
 */
UartSalStatus uart_sal_read(uint8_t *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms);

/**
 * @brief Check how many bytes are available in the receive buffer
 * 
 * @param available Pointer to store number of available bytes
 * @return UART_SAL_OK on success, error code otherwise
 */
UartSalStatus uart_sal_available(size_t *available);

/**
 * @brief Flush the transmit buffer (wait for all data to be sent)
 * 
 * @param timeout_ms Timeout in milliseconds
 * @return UART_SAL_OK on success, error code otherwise
 */
UartSalStatus uart_sal_flush_tx(uint32_t timeout_ms);

/**
 * @brief Clear the receive buffer (discard all received data)
 * 
 * @return UART_SAL_OK on success, error code otherwise
 */
UartSalStatus uart_sal_flush_rx(void);

/**
 * @brief Set receive timeout for blocking operations
 * 
 * @param timeout_ms Timeout in milliseconds
 * @return UART_SAL_OK on success, error code otherwise
 */
UartSalStatus uart_sal_set_timeout(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* UART_SAL_H */

/**
 * @file uart_sal.h
 * @brief UART Software Abstraction Layer Interface
 * 
 * Platform-independent UART interface. Each platform (Windows/Linux/Mac)
 * provides its own implementation.
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
 * UART Protocol Constants (RS-232/RS-485 Standard - Do Not Modify)
 * ============================================================================ */

/** Parity Options */
#define UART_PARITY_NONE    0  /* No parity bit */
#define UART_PARITY_ODD     1  /* Odd parity */
#define UART_PARITY_EVEN    2  /* Even parity */

/** Data Bits Options */
#define UART_DATA_BITS_5    5
#define UART_DATA_BITS_6    6
#define UART_DATA_BITS_7    7
#define UART_DATA_BITS_8    8

/** Stop Bits Options */
#define UART_STOP_BITS_1    1  /* 1 stop bit */
#define UART_STOP_BITS_2    2  /* 2 stop bits */

/* ============================================================================
 * Common UART Configuration (Protocol-Level, Not Platform-Specific)
 * ============================================================================ */

/** Standard baud rate - common default across all platforms */
#ifndef UART_BAUD_RATE
#define UART_BAUD_RATE              115200
#endif

/** Standard data bits - most common setting */
#ifndef UART_DATA_BITS_DEFAULT
#define UART_DATA_BITS_DEFAULT      UART_DATA_BITS_8
#endif

/** Standard parity - most common setting */
#ifndef UART_PARITY_DEFAULT
#define UART_PARITY_DEFAULT         UART_PARITY_NONE
#endif

/** Standard stop bits - most common setting */
#ifndef UART_STOP_BITS_DEFAULT
#define UART_STOP_BITS_DEFAULT      UART_STOP_BITS_1
#endif

/** Hardware flow control - common default */
#ifndef UART_FLOW_CONTROL_DEFAULT
#define UART_FLOW_CONTROL_DEFAULT   false
#endif

/** Receive buffer size - common across platforms */
#ifndef UART_RX_BUFFER_SIZE
#define UART_RX_BUFFER_SIZE         2048
#endif

/** Transmit buffer size - common across platforms */
#ifndef UART_TX_BUFFER_SIZE
#define UART_TX_BUFFER_SIZE         2048
#endif

/** Read timeout - standard across platforms (ms) */
#ifndef UART_READ_TIMEOUT_MS
#define UART_READ_TIMEOUT_MS        100
#endif

/** Write timeout - standard across platforms (ms) */
#ifndef UART_WRITE_TIMEOUT_MS
#define UART_WRITE_TIMEOUT_MS       1000
#endif

/** Connection timeout - standard across platforms (ms) */
#ifndef UART_CONNECTION_TIMEOUT_MS
#define UART_CONNECTION_TIMEOUT_MS  5000
#endif

/** Enable debug logging - common across platforms */
#ifndef UART_DEBUG_ENABLED
#define UART_DEBUG_ENABLED          true
#endif

/* UART SAL Status Codes */
typedef enum {
    UART_SAL_OK = 0,
    UART_SAL_ERROR = -1,
    UART_SAL_TIMEOUT = -2,
    UART_SAL_NOT_OPEN = -3,
    UART_SAL_INVALID_PARAM = -4,
} UartSalStatus;

/* UART Configuration */
typedef struct {
    char port_name[256];
    uint32_t baud_rate;
    uint8_t data_bits;
    uint8_t stop_bits;
    uint8_t parity;
    bool flow_control;
} UartSalConfig;

/* Platform-specific UART SAL functions */
UartSalStatus uart_sal_init(void);
UartSalStatus uart_sal_deinit(void);
UartSalStatus uart_sal_open(const UartSalConfig *config);
UartSalStatus uart_sal_close(void);
UartSalStatus uart_sal_write(const uint8_t *data, size_t length, size_t *written);
UartSalStatus uart_sal_read(uint8_t *buffer, size_t buffer_size, size_t *read_count);
UartSalStatus uart_sal_set_timeout(uint32_t timeout_ms);
UartSalStatus uart_sal_flush(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_SAL_H */

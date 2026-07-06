/**
 * @file uart_sal.c
 * @brief UART SAL implementation for PIC32CX SG41 (Harmony3 SERCOM6 USART)
 *
 * This implementation is intentionally simple and polling-based.
 * - Non-blocking reads: return UART_SAL_TIMEOUT when no data available.
 * - Writes: attempt a single non-blocking write; return UART_SAL_TIMEOUT if busy.
 */

#include "../../uart_sal.h"
#include "uart_config.h"

#include <string.h>

/* Harmony project headers (SERCOM6 USART is enabled in this project) */
#include "definitions.h"

/* ============================================================================
 * Internal State
 * ============================================================================ */

static bool g_uart_initialized = false;
static uint32_t g_uart_timeout_ms = UART_READ_TIMEOUT_MS;

/* ============================================================================
 * Private Helper Functions
 * ============================================================================ */

static bool uart_hw_rx_ready(void)
{
    return SERCOM6_USART_ReceiverIsReady();
}

static bool uart_hw_tx_try_write(const uint8_t *data, size_t length)
{
    return SERCOM6_USART_Write((void *)data, length);
}

/* ============================================================================
 * UART SAL Interface Implementation (STUB)
 * ============================================================================ */

UartSalStatus uart_sal_init(const UartConfig *config)
{
    UartConfig local_cfg;

    if (config == NULL) {
        memset(&local_cfg, 0, sizeof(local_cfg));
        local_cfg.port_num = 0;
        local_cfg.baud_rate = UART_BAUD_RATE;
        local_cfg.data_bits = (UartDataBits)UART_DATA_BITS;
        local_cfg.stop_bits = (UartStopBits)UART_STOP_BITS;
        local_cfg.parity = (UartParity)UART_PARITY;
        local_cfg.flow_control = UART_FLOW_CONTROL;
        local_cfg.rx_buffer_size = UART_RX_BUFFER_SIZE;
        local_cfg.tx_buffer_size = UART_TX_BUFFER_SIZE;
        config = &local_cfg;
    }

    /* Apply serial setup. SERCOM6 is already initialized by SYS_Initialize(). */
    USART_SERIAL_SETUP setup;
    setup.baudRate = config->baud_rate;

    switch (config->parity) {
        case UART_PARITY_EVEN:
            setup.parity = USART_PARITY_EVEN;
            break;
        case UART_PARITY_ODD:
            setup.parity = USART_PARITY_ODD;
            break;
        case UART_PARITY_NONE:
        default:
            setup.parity = USART_PARITY_NONE;
            break;
    }

    switch (config->data_bits) {
        case UART_DATA_BITS_7:
            setup.dataWidth = USART_DATA_7_BIT;
            break;
        case UART_DATA_BITS_8:
        default:
            setup.dataWidth = USART_DATA_8_BIT;
            break;
    }

    switch (config->stop_bits) {
        case UART_STOP_BITS_2:
            setup.stopBits = USART_STOP_1_BIT; /* 2 stop bits */
            break;
        case UART_STOP_BITS_1:
        default:
            setup.stopBits = USART_STOP_0_BIT; /* 1 stop bit */
            break;
    }

    if (!SERCOM6_USART_SerialSetup(&setup, 0U)) {
        return UART_SAL_ERROR;
    }

    g_uart_timeout_ms = UART_READ_TIMEOUT_MS;
    g_uart_initialized = true;
    return UART_SAL_OK;
}

UartSalStatus uart_sal_deinit(void)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }

    SERCOM6_USART_Disable();
    g_uart_initialized = false;
    return UART_SAL_OK;
}

UartSalStatus uart_sal_write(const uint8_t *data, size_t length, uint32_t timeout_ms)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }

    if (!data || length == 0) {
        return UART_SAL_INVALID_PARAM;
    }

    /* Harmony PLIB write blocks until each byte is in the shift register. */
    if (!uart_hw_tx_try_write(data, length)) {
        (void)timeout_ms;
        return UART_SAL_TIMEOUT;
    }

    /* Wait for the last byte to be fully shifted out on the wire. */
    while (!SERCOM6_USART_TransmitComplete()) {
        /* spin */
    }

    return UART_SAL_OK;
}

UartSalStatus uart_sal_read(uint8_t *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }

    if (!buffer || !bytes_read) {
        return UART_SAL_INVALID_PARAM;
    }

    *bytes_read = 0;

    if (!uart_hw_rx_ready()) {
        (void)timeout_ms;
        return UART_SAL_TIMEOUT;
    }

    size_t cnt = 0;
    while ((cnt < buffer_size) && uart_hw_rx_ready()) {
        int b = SERCOM6_USART_ReadByte();
        buffer[cnt++] = (uint8_t)(b & 0xFF);
    }

    *bytes_read = cnt;
    return (cnt > 0U) ? UART_SAL_OK : UART_SAL_TIMEOUT;
}

UartSalStatus uart_sal_available(size_t *available)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }

    if (!available) {
        return UART_SAL_INVALID_PARAM;
    }

    *available = uart_hw_rx_ready() ? 1U : 0U;
    return UART_SAL_OK;
}

UartSalStatus uart_sal_flush_tx(uint32_t timeout_ms)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }

    if (SERCOM6_USART_TransmitComplete()) {
        return UART_SAL_OK;
    }

    (void)timeout_ms;
    return UART_SAL_TIMEOUT;
}

UartSalStatus uart_sal_flush_rx(void)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }

    /* Drain available bytes */
    while (uart_hw_rx_ready()) {
        (void)SERCOM6_USART_ReadByte();
    }
    return UART_SAL_OK;
}

UartSalStatus uart_sal_set_timeout(uint32_t timeout_ms)
{
    if (!g_uart_initialized) {
        return UART_SAL_NOT_INITIALIZED;
    }

    g_uart_timeout_ms = timeout_ms;
    return UART_SAL_OK;
}


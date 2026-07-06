/**
 * @file uart_sal_windows.c
 * @brief UART SAL Implementation for Windows
 * 
 * Uses Windows API (CreateFile, ReadFile, WriteFile) for serial communication.
 */

#include "../../uart_sal.h"
#include "uart_config.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Internal State
 * ============================================================================ */

static HANDLE g_com_handle = INVALID_HANDLE_VALUE;
static COMMTIMEOUTS g_original_timeouts;
static uint32_t g_timeout_ms = 5000;

/* ============================================================================
 * UART SAL Implementation
 * ============================================================================ */

UartSalStatus uart_sal_init(void)
{
    /* No global initialization required for Windows serial */
    return UART_SAL_OK;
}

UartSalStatus uart_sal_deinit(void)
{
    if (g_com_handle != INVALID_HANDLE_VALUE) {
        uart_sal_close();
    }
    return UART_SAL_OK;
}

UartSalStatus uart_sal_open(const UartSalConfig *config)
{
    UartSalConfig default_config;
    char port_name[256];
    
    /* If config is NULL, use defaults from uart_config.h */
    if (!config) {
        default_config.baud_rate = UART_BAUD_RATE;
        default_config.data_bits = UART_DATA_BITS_DEFAULT;
        default_config.stop_bits = UART_STOP_BITS_DEFAULT;
        default_config.parity = UART_PARITY_DEFAULT;
        default_config.flow_control = UART_FLOW_CONTROL_DEFAULT;
        strncpy(default_config.port_name, UART_COM_PORT_STR, sizeof(default_config.port_name) - 1);
        default_config.port_name[sizeof(default_config.port_name) - 1] = '\0';
        config = &default_config;
    }
    
    /* Close existing handle if open */
    if (g_com_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(g_com_handle);
        g_com_handle = INVALID_HANDLE_VALUE;
    }
    
    /* Format port name for Windows (add "\\.\" prefix for COM ports) */
    if (strstr(config->port_name, "\\\\.\\") == config->port_name) {
        /* Already has prefix */
        strncpy(port_name, config->port_name, sizeof(port_name) - 1);
    } else {
        /* Add prefix */
        snprintf(port_name, sizeof(port_name), "\\\\.\\%s", config->port_name);
    }
    port_name[sizeof(port_name) - 1] = '\0';
    
    /* Open COM port */
    g_com_handle = CreateFileA(
        port_name,
        GENERIC_READ | GENERIC_WRITE,
        0,                    /* No sharing */
        NULL,                 /* Default security */
        OPEN_EXISTING,
        0,                    /* Non-overlapped I/O */
        NULL
    );
    
    if (g_com_handle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        return UART_SAL_ERROR;
    }
    
    /* Configure DCB (Device Control Block) */
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    
    if (!GetCommState(g_com_handle, &dcb)) {
        CloseHandle(g_com_handle);
        g_com_handle = INVALID_HANDLE_VALUE;
        return UART_SAL_ERROR;
    }
    
    dcb.BaudRate = config->baud_rate;
    dcb.ByteSize = config->data_bits;
    dcb.StopBits = (config->stop_bits == 1) ? ONESTOPBIT : TWOSTOPBITS;
    
    switch (config->parity) {
        case 0: dcb.Parity = NOPARITY; break;
        case 1: dcb.Parity = ODDPARITY; break;
        case 2: dcb.Parity = EVENPARITY; break;
        default: dcb.Parity = NOPARITY; break;
    }
    
    if (config->flow_control) {
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        dcb.fOutxCtsFlow = TRUE;
    } else {
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        dcb.fOutxCtsFlow = FALSE;
    }
    
    dcb.fBinary = TRUE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    
    if (!SetCommState(g_com_handle, &dcb)) {
        CloseHandle(g_com_handle);
        g_com_handle = INVALID_HANDLE_VALUE;
        return UART_SAL_ERROR;
    }
    
    /* Set timeouts */
    COMMTIMEOUTS timeouts = {0};
    GetCommTimeouts(g_com_handle, &g_original_timeouts);
    
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = g_timeout_ms;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = g_timeout_ms;
    
    if (!SetCommTimeouts(g_com_handle, &timeouts)) {
        CloseHandle(g_com_handle);
        g_com_handle = INVALID_HANDLE_VALUE;
        return UART_SAL_ERROR;
    }
    
    /* Flush buffers */
    PurgeComm(g_com_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
    
    return UART_SAL_OK;
}

UartSalStatus uart_sal_close(void)
{
    if (g_com_handle == INVALID_HANDLE_VALUE) {
        return UART_SAL_OK;
    }
    
    CloseHandle(g_com_handle);
    g_com_handle = INVALID_HANDLE_VALUE;
    return UART_SAL_OK;
}

UartSalStatus uart_sal_write(const uint8_t *data, size_t length, size_t *written)
{
    if (!data || length == 0 || !written) {
        return UART_SAL_INVALID_PARAM;
    }
    
    if (g_com_handle == INVALID_HANDLE_VALUE) {
        return UART_SAL_NOT_OPEN;
    }
    
    DWORD bytes_written = 0;
    BOOL result = WriteFile(g_com_handle, data, (DWORD)length, &bytes_written, NULL);
    
    *written = (size_t)bytes_written;
    
    if (!result) {
        return UART_SAL_ERROR;
    }

    return UART_SAL_OK;
}

UartSalStatus uart_sal_read(uint8_t *buffer, size_t buffer_size, size_t *read_count)
{
    if (!buffer || buffer_size == 0 || !read_count) {
        return UART_SAL_INVALID_PARAM;
    }
    
    if (g_com_handle == INVALID_HANDLE_VALUE) {
        return UART_SAL_NOT_OPEN;
    }
    
    DWORD bytes_read = 0;
    BOOL result = ReadFile(g_com_handle, buffer, (DWORD)buffer_size, &bytes_read, NULL);
    
    *read_count = (size_t)bytes_read;
    
    if (!result) {
        DWORD error = GetLastError();
        if (error == ERROR_TIMEOUT) {
            return UART_SAL_TIMEOUT;
        }
        return UART_SAL_ERROR;
    }
    
    if (bytes_read == 0) {
        return UART_SAL_TIMEOUT;
    }

    return UART_SAL_OK;
}

UartSalStatus uart_sal_set_timeout(uint32_t timeout_ms)
{
    g_timeout_ms = timeout_ms;
    
    if (g_com_handle == INVALID_HANDLE_VALUE) {
        return UART_SAL_OK;  /* Will be applied when port is opened */
    }
    
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = timeout_ms;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = timeout_ms;
    
    if (!SetCommTimeouts(g_com_handle, &timeouts)) {
        return UART_SAL_ERROR;
    }
    
    return UART_SAL_OK;
}

UartSalStatus uart_sal_flush(void)
{
    if (g_com_handle == INVALID_HANDLE_VALUE) {
        return UART_SAL_NOT_OPEN;
    }
    
    if (!PurgeComm(g_com_handle, PURGE_RXCLEAR | PURGE_TXCLEAR)) {
        return UART_SAL_ERROR;
    }
    
    return UART_SAL_OK;
}

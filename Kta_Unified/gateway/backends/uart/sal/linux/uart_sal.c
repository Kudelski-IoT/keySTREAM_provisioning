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
 * @file uart_sal.c
 * @brief Linux UART SAL implementation (POSIX termios)
 *
 * NEW FILE added in the "improved" tree to fix the missing implementation
 * for the Linux gateway target. The original tree shipped only
 * uart_config.h here, which produced unresolved symbols at link time.
 *
 * This implementation is intentionally minimal and self-contained:
 *  - Uses POSIX termios (glibc / musl) only.
 *  - Honors the compile-time UART_DEVICE_PATH from uart_config.h.
 *  - Maps UartSalStatus codes consistently with other platforms.
 *
 * It is NOT a hot-path optimized driver; replace with a higher-performance
 * implementation (epoll/select with non-blocking IO) for production use.
 */

#include "../../uart_sal.h"
#include "uart_config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>

/* ---------- internal state ------------------------------------------------ */

static int      g_uart_fd      = -1;
static bool     g_initialized  = false;
static uint32_t g_read_timeout_ms = UART_READ_TIMEOUT_MS;

/* ---------- helpers ------------------------------------------------------- */

static speed_t map_baud(uint32_t baud)
{
    switch (baud) {
        case 9600:    return B9600;
        case 19200:   return B19200;
        case 38400:   return B38400;
        case 57600:   return B57600;
        case 115200:  return B115200;
        case 230400:  return B230400;
        case 460800:  return B460800;
        case 921600:  return B921600;
        default:      return B115200; /* safe default */
    }
}

/* ---------- API ----------------------------------------------------------- */

UartSalStatus uart_sal_init(void)
{
    if (g_initialized) {
        return UART_SAL_OK;
    }
    g_uart_fd     = -1;
    g_initialized = true;
    return UART_SAL_OK;
}

UartSalStatus uart_sal_deinit(void)
{
    if (g_uart_fd >= 0) {
        (void)close(g_uart_fd);
        g_uart_fd = -1;
    }
    g_initialized = false;
    return UART_SAL_OK;
}

UartSalStatus uart_sal_open(const UartSalConfig *config)
{
    if (!g_initialized) {
        return UART_SAL_ERROR;
    }
    if (g_uart_fd >= 0) {
        (void)close(g_uart_fd);
        g_uart_fd = -1;
    }

    const char *path     = (config && config->port_name[0] != '\0') ? config->port_name : UART_DEVICE_PATH;
    uint32_t    baud     = (config && config->baud_rate)            ? config->baud_rate : UART_BAUD_RATE;

    int fd = open(path, O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (fd < 0) {
        return UART_SAL_ERROR;
    }

    struct termios tio;
    if (tcgetattr(fd, &tio) != 0) {
        (void)close(fd);
        return UART_SAL_ERROR;
    }

    cfmakeraw(&tio);
    speed_t s = map_baud(baud);
    (void)cfsetispeed(&tio, s);
    (void)cfsetospeed(&tio, s);

    /* 8N1, no flow control by default */
    tio.c_cflag &= ~(unsigned)(PARENB | CSTOPB | CSIZE | CRTSCTS);
    tio.c_cflag |= CS8 | CLOCAL | CREAD;
    tio.c_iflag &= ~(unsigned)(IXON | IXOFF | IXANY);
    tio.c_cc[VMIN]  = (cc_t)UART_VMIN;
    tio.c_cc[VTIME] = (cc_t)UART_VTIME;

    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        (void)close(fd);
        return UART_SAL_ERROR;
    }

    (void)tcflush(fd, TCIOFLUSH);
    g_uart_fd = fd;
    return UART_SAL_OK;
}

UartSalStatus uart_sal_close(void)
{
    if (g_uart_fd >= 0) {
        (void)close(g_uart_fd);
        g_uart_fd = -1;
    }
    return UART_SAL_OK;
}

UartSalStatus uart_sal_write(const uint8_t *data, size_t length, size_t *written)
{
    if (!data || !written) return UART_SAL_INVALID_PARAM;
    if (g_uart_fd < 0)     return UART_SAL_NOT_OPEN;

    size_t total = 0;
    while (total < length) {
        ssize_t n = write(g_uart_fd, data + total, length - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            *written = total;
            return UART_SAL_ERROR;
        }
        total += (size_t)n;
    }
    *written = total;
    return UART_SAL_OK;
}

UartSalStatus uart_sal_read(uint8_t *buffer, size_t buffer_size, size_t *read_count)
{
    if (!buffer || !read_count) return UART_SAL_INVALID_PARAM;
    if (g_uart_fd < 0)          return UART_SAL_NOT_OPEN;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(g_uart_fd, &rfds);

    struct timeval tv;
    tv.tv_sec  = (time_t)(g_read_timeout_ms / 1000U);
    tv.tv_usec = (suseconds_t)((g_read_timeout_ms % 1000U) * 1000U);

    int sel = select(g_uart_fd + 1, &rfds, NULL, NULL, &tv);
    if (sel == 0) {
        *read_count = 0;
        return UART_SAL_TIMEOUT;
    }
    if (sel < 0) {
        *read_count = 0;
        return (errno == EINTR) ? UART_SAL_TIMEOUT : UART_SAL_ERROR;
    }

    ssize_t n = read(g_uart_fd, buffer, buffer_size);
    if (n < 0) {
        *read_count = 0;
        return UART_SAL_ERROR;
    }
    *read_count = (size_t)n;
    return UART_SAL_OK;
}

UartSalStatus uart_sal_set_timeout(uint32_t timeout_ms)
{
    g_read_timeout_ms = timeout_ms;
    return UART_SAL_OK;
}

UartSalStatus uart_sal_flush(void)
{
    if (g_uart_fd < 0) return UART_SAL_NOT_OPEN;
    return (tcflush(g_uart_fd, TCIOFLUSH) == 0) ? UART_SAL_OK : UART_SAL_ERROR;
}

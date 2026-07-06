#include "app_wincs02.h"

#include "../../App_Config.h"

#include "system/console/sys_console.h"
#include "system/time/sys_time.h"

#include "peripheral/sercom/usart/plib_sercom6_usart.h"

#include "definitions.h"

#include <string.h>

#include "mcu/examples/common/bridge_integration.h"
#include "cryptoauthlib.h"

#if (APP_USE_UART_BRIDGE == 1) && (APP_UART_SMOKE_TEST == 0)
/*
 * MPLAB project metadata currently does not list the MCU bridge sources.
 * Pull them into this translation unit so bridge symbols are linked.
 */

#include "config/default/library/kta_lib/mcu/backends/uart/sal/sg41/uart_sal.c"
#include "config/default/library/kta_lib/mcu/backends/uart/backend_uart.c"
#include "config/default/library/kta_lib/mcu/backends/backend_interface.c"
#include "config/default/library/kta_lib/mcu/bridgeKta/bridge_kta.c"
#include "config/default/library/kta_lib/mcu/examples/common/bridge_integration.c"
#endif

APP_DATA g_appData;

typedef struct
{
    const char *msg;
    size_t len;
    size_t sent;
    bool done;
} APP_CONSOLE_MSG;

static bool APP_ConsoleTrySend(SYS_CONSOLE_HANDLE handle, APP_CONSOLE_MSG *m)
{
    if ((m == NULL) || m->done || (handle == SYS_CONSOLE_HANDLE_INVALID))
    {
        return (m != NULL) ? m->done : false;
    }

    if (m->len == 0U)
    {
        m->len = strlen(m->msg);
    }

    if (m->sent >= m->len)
    {
        m->done = true;
        return true;
    }

    {
        const ssize_t n = SYS_CONSOLE_Write(handle, &m->msg[m->sent], m->len - m->sent);
        if (n > 0)
        {
            m->sent += (size_t)n;
        }
    }

    if (m->sent >= m->len)
    {
        m->done = true;
    }

    return m->done;
}

void APP_WINCS02_Initialize(void)
{
    g_appData.state = APP_STATE_WAIT_USB_ENUMERATION;
}

void APP_WINCS02_Tasks(void)
{
#if (APP_USE_UART_BRIDGE != 1)
#error "UART-only build: APP_USE_UART_BRIDGE must be 1"
#endif

#if (APP_UART_SMOKE_TEST == 1)

#if (APP_UART_HTTP_RELAY == 1)
    /*
     * HTTP Relay mode (APP_UART_HTTP_RELAY=1, APP_UART_SMOKE_TEST=1):
     *
     *  [Tera Term COM13] --> type device ID + Enter
     *  [MCU USB-CDC]     --> sends raw line over SERCOM6 UART (COM12)
     *  [pc_http_relay.py]   makes HTTP request, writes single-line response back to COM12
     *  [MCU SERCOM6 UART]   receives response, prints it on COM13 (USB-CDC)
     *
     * States: 0=PROMPT  1=INPUT  2=SEND  3=RECV  4=PRINT
     */
    static uint8_t  s_hr_state        = 0U;
    static bool     s_hr_prompted     = false;
    static char     s_hr_input[128];
    static size_t   s_hr_input_len    = 0U;
    static size_t   s_hr_tx_offset    = 0U;
    static char     s_hr_response[512];
    static size_t   s_hr_response_len = 0U;
    static uint32_t s_hr_recv_tick    = 0U;

    {
        const SYS_CONSOLE_HANDLE s_con = SYS_CONSOLE_HandleGet(SYS_CONSOLE_INDEX_0);

        switch (s_hr_state)
        {
            case 0U: /* PROMPT */
                if (!s_hr_prompted)
                {
                    SYS_CONSOLE_PRINT("\r\n[HTTP-Relay] Enter device ID and press Enter: ");
                    s_hr_prompted     = true;
                    s_hr_input_len    = 0U;
                    s_hr_tx_offset    = 0U;
                    s_hr_response_len = 0U;
                }
                s_hr_state = 1U;
                break;

            case 1U: /* INPUT - collect device ID from USB-CDC */
            {
                uint8_t rx[32];
                const ssize_t nr = SYS_CONSOLE_Read(s_con, rx, sizeof(rx));
                if (nr > 0)
                {
                    size_t i;
                    for (i = 0U; i < (size_t)nr; i++)
                    {
                        const uint8_t c = rx[i];
                        if ((c == (uint8_t)'\r') || (c == (uint8_t)'\n'))
                        {
                            if (s_hr_input_len > 0U)
                            {
                                s_hr_input[s_hr_input_len++] = '\r';
                                s_hr_input[s_hr_input_len++] = '\n';
                                s_hr_input[s_hr_input_len]   = '\0';
                                SYS_CONSOLE_PRINT("\r\n[HTTP-Relay] Forwarding to PC relay...\r\n");
                                s_hr_state = 2U;
                            }
                        }
                        else if ((c == 0x08U) || (c == 0x7FU)) /* backspace / DEL */
                        {
                            if (s_hr_input_len > 0U)
                            {
                                s_hr_input_len--;
                                {
                                    const char bs[] = "\x08 \x08";
                                    (void)SYS_CONSOLE_Write(s_con, bs, sizeof(bs) - 1U);
                                }
                            }
                        }
                        else
                        {
                            (void)SYS_CONSOLE_Write(s_con, &rx[i], 1U); /* echo */
                            if (s_hr_input_len < (sizeof(s_hr_input) - 3U))
                            {
                                s_hr_input[s_hr_input_len++] = (char)c;
                            }
                        }
                    }
                }
                break;
            }

            case 2U: /* SEND - transmit device-ID line byte-by-byte over SERCOM6 */
            {
                while (s_hr_tx_offset < s_hr_input_len)
                {
                    if (SERCOM6_USART_TransmitterIsReady())
                    {
                        SERCOM6_USART_WriteByte((int)(uint8_t)s_hr_input[s_hr_tx_offset]);
                        s_hr_tx_offset++;
                    }
                    else
                    {
                        break;
                    }
                }
                if (s_hr_tx_offset >= s_hr_input_len)
                {
                    s_hr_response_len = 0U;
                    s_hr_recv_tick    = SYS_TIME_CounterGet();
                    s_hr_state        = 3U;
                }
                break;
            }

            case 3U: /* RECV - collect response bytes from SERCOM6 until '\n' or timeout */
            {
                while (SERCOM6_USART_ReceiverIsReady())
                {
                    const char rc = (char)(uint8_t)SERCOM6_USART_ReadByte();
                    if (s_hr_response_len < (sizeof(s_hr_response) - 1U))
                    {
                        s_hr_response[s_hr_response_len++] = rc;
                    }
                    if (rc == '\n')
                    {
                        s_hr_response[s_hr_response_len] = '\0';
                        s_hr_state = 4U;
                        break;
                    }
                }
                if (s_hr_state == 3U) /* still waiting - check timeout (10 s) */
                {
                    const uint32_t elapsed = SYS_TIME_CounterGet() - s_hr_recv_tick;
                    const uint32_t tmo = (uint32_t)(((uint64_t)SYS_TIME_FrequencyGet() * 10000ULL) / 1000ULL);
                    if (elapsed >= tmo)
                    {
                        SYS_CONSOLE_PRINT("[HTTP-Relay] ERROR: timed out (10 s) waiting for PC relay\r\n");
                        s_hr_state    = 0U;
                        s_hr_prompted = false;
                    }
                }
                break;
            }

            case 4U: /* PRINT - display response on USB-CDC, then loop back */
                SYS_CONSOLE_PRINT("[HTTP-Relay] Response: %s\r\n", s_hr_response);
                s_hr_state        = 0U;
                s_hr_prompted     = false;
                s_hr_input_len    = 0U;
                s_hr_tx_offset    = 0U;
                s_hr_response_len = 0U;
                break;

            default:
                s_hr_state    = 0U;
                s_hr_prompted = false;
                break;
        }
    }

    return;

#else /* APP_UART_HTTP_RELAY == 0: original PING/PONG smoke test */
    // UART smoke test: echo all RX bytes back; if a line equals "PING" (case-insensitive)
    // then respond with "PONG".
    static bool s_banner_printed = false;
    static uint8_t s_usb_line_buf[64];
    static size_t s_usb_line_len = 0U;
    static uint8_t s_line_buf[64];
    static size_t s_line_len = 0U;
    static uint32_t s_uart_rx_total = 0U;
    static bool s_uart_rx_seen_printed = false;

    if (!s_banner_printed)
    {
        SYS_CONSOLE_PRINT("[SMOKE] UART smoke test enabled\r\n");
        SYS_CONSOLE_PRINT("[SMOKE] - USB-CDC: type PING + Enter here -> USB PONG\r\n");
        SYS_CONSOLE_PRINT("[SMOKE] - UART SERCOM6: PC16=TX, PC17=RX, 115200 8N1 -> PONG\r\n");
        s_banner_printed = true;
    }

    // USB-CDC console smoke test (lets you type in the same terminal you already have open)
    {
        uint8_t rx[32];
        const ssize_t nr = SYS_CONSOLE_Read(sysObj.sysConsole0, rx, sizeof(rx));
        if (nr > 0)
        {
            // Echo back what we received so you can see your typing even if local-echo is off
            (void)SYS_CONSOLE_Write(sysObj.sysConsole0, rx, (size_t)nr);

            for (ssize_t i = 0; i < nr; i++)
            {
                const uint8_t b = rx[i];

                if ((b == '\r') || (b == '\n'))
                {
                    if (s_usb_line_len == 4U)
                    {
                        uint8_t p0 = s_usb_line_buf[0];
                        uint8_t p1 = s_usb_line_buf[1];
                        uint8_t p2 = s_usb_line_buf[2];
                        uint8_t p3 = s_usb_line_buf[3];
                        if ((p0 >= 'a') && (p0 <= 'z')) p0 = (uint8_t)(p0 - 'a' + 'A');
                        if ((p1 >= 'a') && (p1 <= 'z')) p1 = (uint8_t)(p1 - 'a' + 'A');
                        if ((p2 >= 'a') && (p2 <= 'z')) p2 = (uint8_t)(p2 - 'a' + 'A');
                        if ((p3 >= 'a') && (p3 <= 'z')) p3 = (uint8_t)(p3 - 'a' + 'A');

                        if ((p0 == 'P') && (p1 == 'I') && (p2 == 'N') && (p3 == 'G'))
                        {
                            SYS_CONSOLE_PRINT("\r\nUSB PONG\r\n");
                        }
                    }
                    s_usb_line_len = 0U;
                }
                else
                {
                    if (s_usb_line_len < sizeof(s_usb_line_buf))
                    {
                        s_usb_line_buf[s_usb_line_len++] = b;
                    }
                    else
                    {
                        s_usb_line_len = 0U;
                    }
                }
            }
        }
    }

    while (SERCOM6_USART_ReceiverIsReady())
    {
        const uint8_t b = (uint8_t)SERCOM6_USART_ReadByte();
        s_uart_rx_total++;

        if (!s_uart_rx_seen_printed)
        {
            SYS_CONSOLE_PRINT("[SMOKE] UART SERCOM6 RX detected\r\n");
            s_uart_rx_seen_printed = true;
        }

        // Echo back (best-effort, non-blocking)
        if (SERCOM6_USART_TransmitterIsReady())
        {
            SERCOM6_USART_WriteByte((int)b);
        }

        if ((b == '\r') || (b == '\n'))
        {
            // Evaluate line
            if (s_line_len == 4U)
            {
                uint8_t p0 = s_line_buf[0];
                uint8_t p1 = s_line_buf[1];
                uint8_t p2 = s_line_buf[2];
                uint8_t p3 = s_line_buf[3];
                if ((p0 >= 'a') && (p0 <= 'z')) p0 = (uint8_t)(p0 - 'a' + 'A');
                if ((p1 >= 'a') && (p1 <= 'z')) p1 = (uint8_t)(p1 - 'a' + 'A');
                if ((p2 >= 'a') && (p2 <= 'z')) p2 = (uint8_t)(p2 - 'a' + 'A');
                if ((p3 >= 'a') && (p3 <= 'z')) p3 = (uint8_t)(p3 - 'a' + 'A');

                if ((p0 == 'P') && (p1 == 'I') && (p2 == 'N') && (p3 == 'G'))
                {
                    const char resp[] = "\r\nPONG\r\n";
                    for (size_t i = 0U; i < (sizeof(resp) - 1U); i++)
                    {
                        while (!SERCOM6_USART_TransmitterIsReady())
                        {
                            // busy wait; smoke test only
                        }
                        SERCOM6_USART_WriteByte((int)resp[i]);
                    }
                }
            }

            s_line_len = 0U;
        }
        else
        {
            if (s_line_len < sizeof(s_line_buf))
            {
                s_line_buf[s_line_len++] = b;
            }
            else
            {
                // overflow: reset
                s_line_len = 0U;
            }
        }
    }

    // If you only see USB PONG but never see this line, the PC is not actually
    // connected to SERCOM6 UART (PC16/PC17).
    if ((s_uart_rx_total == 0U) && (s_usb_line_len == 0U))
    {
        // No-op: keeps unused warnings away if build flags change.
    }

    return;
#endif /* APP_UART_HTTP_RELAY */

#else
    static bool s_bridge_started = false;
    static uint32_t s_next_status_print_tick = 0U;
    static SYS_CONSOLE_HANDLE s_console_handle = SYS_CONSOLE_HANDLE_INVALID;

    static APP_CONSOLE_MSG s_msg_console_ready = {"[UART-Bridge] Console ready\r\n", 0U, 0U, false};
    static APP_CONSOLE_MSG s_msg_bridge_start = {"[UART-Bridge] Starting KTA bridge over UART...\r\n", 0U, 0U, false};
    static APP_CONSOLE_MSG s_msg_bridge_ready = {"[UART-Bridge] Ready\r\n", 0U, 0U, false};

#if (APP_BRIDGE_STATUS_PRINT_PERIOD_MS == 0U)
    (void)s_next_status_print_tick;
#endif

    if (s_console_handle == SYS_CONSOLE_HANDLE_INVALID)
    {
        s_console_handle = SYS_CONSOLE_HandleGet(SYS_CONSOLE_INDEX_0);
    }

    (void)APP_ConsoleTrySend(s_console_handle, &s_msg_console_ready);

    if (!s_bridge_started)
    {
        (void)APP_ConsoleTrySend(s_console_handle, &s_msg_bridge_start);

        /* Initialize ATECC608 secure element (required by KTA SAL layer) */
        {
            extern ATCAIfaceCfg atecc608_0_init_data;
            ATCA_STATUS atca_status = atcab_init(&atecc608_0_init_data);
            if (ATCA_SUCCESS != atca_status)
            {
                const char msg[] = "[UART-Bridge] ERROR: ATECC608 init failed\r\n";
                if (s_console_handle != SYS_CONSOLE_HANDLE_INVALID)
                {
                    (void)SYS_CONSOLE_Write(s_console_handle, msg, (sizeof(msg) - 1U));
                }
                return;
            }
        }

        if (bridge_integration_init(BACKEND_TYPE_UART) != 0)
        {
            const char msg[] = "[UART-Bridge] ERROR: init failed\r\n";
            if (s_console_handle != SYS_CONSOLE_HANDLE_INVALID)
            {
                (void)SYS_CONSOLE_Write(s_console_handle, msg, (sizeof(msg) - 1U));
            }
            return;
        }

        s_bridge_started = true;

#if (APP_BRIDGE_STATUS_PRINT_PERIOD_MS > 0U)
        {
            const uint32_t freq = SYS_TIME_FrequencyGet();
            const uint32_t period_ticks = (uint32_t)(((uint64_t)freq * (uint64_t)APP_BRIDGE_STATUS_PRINT_PERIOD_MS) / 1000ULL);
            s_next_status_print_tick = SYS_TIME_CounterGet() + period_ticks;
        }
#endif
    }

    (void)bridge_integration_process();

    if (s_bridge_started)
    {
        (void)APP_ConsoleTrySend(s_console_handle, &s_msg_bridge_ready);
    }

#if (APP_BRIDGE_STATUS_PRINT_PERIOD_MS > 0U)
    {
        const uint32_t freq = SYS_TIME_FrequencyGet();
        const uint32_t period_ticks = (uint32_t)(((uint64_t)freq * (uint64_t)APP_BRIDGE_STATUS_PRINT_PERIOD_MS) / 1000ULL);
        if (period_ticks != 0U)
        {
            const uint32_t now = SYS_TIME_CounterGet();
            if ((int32_t)(now - s_next_status_print_tick) >= 0)
            {
                SYS_CONSOLE_PRINT("[UART-Bridge] Alive\r\n");
                s_next_status_print_tick = now + period_ticks;
            }
        }
    }
#endif

    return;
#endif
}

/**
 * @file main_windows_async_kta.c
 * @brief Production KTA Gateway with Continuous Monitoring - Windows
 * 
 * Production Features:
 *   - Automatic provisioning on startup
 *   - Continuous status monitoring every 24 hours
 *   - Automatic retry for failed provisioning
 *   - Graceful shutdown handling
 *   - Field management checks (key rotation, updates)
 * 
 * Execution Flow:
 *   1. ktaKeyStreamInit() - One-time initialization
 *   2. ktaKeyStreamFieldMgmt(false) - Initial provisioning
 *   3. Periodic monitoring loop (24-hour interval)
 *   4. ktaKeyStreamFieldMgmt(true) - Periodic field management check
 * 
 * @author Kudelski IoT
 */

#include "../ktaIntegration/ktaFieldMgntHook.h"
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

/* Configuration Constants */
/* Poll the keySTREAM server this often after provisioning.
 * The server may send key-delivery, rotate, renew, or refurbish commands
 * at any time — keep the interval short so the device stays responsive. */
#define KTA_POLL_INTERVAL_SECONDS      5     /* Poll every 5 seconds */
#define KTA_STATUS_DISPLAY_INTERVAL    3600   /* Print heartbeat every hour */

/* Running flag for graceful shutdown */
static volatile bool g_running = true;

/* Signal handler for Ctrl+C */
static void signal_handler(int xSignum)
{
    (void)xSignum;
    (void)printf("\nShutdown requested...\n");
    g_running = false;
}

/* ============================================================================
 * Main - Production Ready
 * ============================================================================ */

int main(void)
{
    TKStatus xStatus;
    TKktaKeyStreamStatus xKsStatus;
    int exit_code = 0;
    
    (void)printf("KTA Field Management Starting...\n");
    
    /* Step 1: Initialize KTA */
    (void)printf("Initializing KTA...\n");
    xStatus = ktaKeyStreamInit();
    
    if (E_K_STATUS_OK != xStatus) {
        (void)printf("ERROR: KTA initialization failed (code: %d)\n", xStatus);
        return 1;
    }
    
    (void)printf("KTA initialized successfully\n");
    
    /* Step 2: Run field management */
    (void)printf("Running field management...\n");
    xStatus = ktaKeyStreamFieldMgmt(false, &xKsStatus);
    
    if (E_K_STATUS_OK != xStatus) {
        (void)printf("ERROR: Field management failed (code: %d)\n", xStatus);
        return 2;
    }
    
    /* Step 3: Report final status */
    (void)printf("Field management completed\n");
    
    switch (xKsStatus) {
        case E_K_KTA_KS_STATUS_NO_OPERATION:
            (void)printf("Status: NO_OPERATION (device operational)\n");
            exit_code = 0;
            break;
        case E_K_KTA_KS_STATUS_RENEW:
            (void)printf("Status: RENEW completed\n");
            exit_code = 0;
            break;
        case E_K_KTA_KS_STATUS_REFURBISH:
            (void)printf("Status: REFURBISH\n");
            exit_code = 2;
            break;
        default:
            (void)printf("Status: UNKNOWN (0x%02X)\n", (unsigned)xKsStatus);
            exit_code = 6;
            break;
    }
    
    (void)printf("KTA Field Management Complete (status: %d)\n", exit_code);

    /* Setup signal handler for graceful shutdown */
    (void)signal(SIGINT, signal_handler);
    (void)signal(SIGTERM, signal_handler);

    /* -----------------------------------------------------------------------
     * Continuous polling loop.
     *
     * After provisioning the keySTREAM server may at any time push:
     *   - Key delivery / rotation
     *   - Certificate renewal
     *   - Refurbish (wipe + re-provision)
     *   - Suspend / reactivate
     *
     * ktaKeyStreamFieldMgmt() handles all of these:
     *   - Runs the full exchange loop (ktaExchangeMessage) until the MCU
     *     signals no more data (empty response).
     *   - Queries ktaKeyStreamStatus and returns the lifecycle state.
     *   - On REFURBISH it resets the init flag and immediately re-runs
     *     the full init → startup → setDeviceInfo → exchange sequence.
     * ----------------------------------------------------------------------- */
    (void)printf("\nGateway running — polling every %d seconds (Press Ctrl+C to shutdown)\n\n", KTA_POLL_INTERVAL_SECONDS);

    uint32_t elapsed_seconds = 0U;

    while (true == g_running) {
        /* Wait for the next poll interval (1-second ticks for clean Ctrl+C) */
        for (uint32_t t = 0U; (t < (uint32_t)KTA_POLL_INTERVAL_SECONDS) && g_running; t++) {
            Sleep(1000U);
            elapsed_seconds++;

            /* Hourly heartbeat */
            if (0U == (elapsed_seconds % (uint32_t)KTA_STATUS_DISPLAY_INTERVAL)) {
                double uptime_h = (double)elapsed_seconds / 3600.0;
                (void)printf("[%.1fh] heartbeat — last status: %d\n",
                             uptime_h, (int)xKsStatus);
            }
        }

        if (false == g_running) { break; }

        xStatus = ktaKeyStreamFieldMgmt(true, &xKsStatus);

        if (E_K_STATUS_OK != xStatus) {
            continue;
        }

        switch (xKsStatus) {
            case E_K_KTA_KS_STATUS_NO_OPERATION:
                exit_code = 0;
                break;

            case E_K_KTA_KS_STATUS_RENEW:
                exit_code = 0;
                break;

            case E_K_KTA_KS_STATUS_REFURBISH:
                exit_code = 2;
                xStatus = ktaKeyStreamFieldMgmt(true, &xKsStatus);
                if (E_K_STATUS_OK == xStatus) {
                    exit_code = (E_K_KTA_KS_STATUS_NO_OPERATION == xKsStatus) ? 0 : (int)xKsStatus;
                }
                break;

            case E_K_KTA_KS_STATUS_NONE:
            default:
                break;
        }
    }

    (void)printf("\nGateway shutdown complete\n");
    return exit_code;
}

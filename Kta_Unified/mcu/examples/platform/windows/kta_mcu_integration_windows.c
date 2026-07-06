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
 * @file kta_mcu_integration_windows.c
 * @brief Windows Example - KTA Bridge Integration
 *
 * Windows-specific implementation of the KTA MCU bridge integration.
 * The bridge runs in a dedicated thread; a second thread represents the
 * user application.  The two threads run concurrently and are joined on
 * a clean shutdown triggered by Ctrl-C or Ctrl-Break.
 *
 * Threading model:
 *   main thread  →  spawns kta_bridge_thread  (runs bridge_integration loop)
 *                →  spawns application_thread (user application placeholder)
 *                →  waits for both threads, then exits
 *
 * Flow: kta_mcu_integration_windows.c → bridge API → HAL → backend → SAL
 *
 * Platform: Windows (Win32 API)
 *   - Thread-based (CreateThread / WaitForMultipleObjects)
 *   - Console Ctrl-C handling (SetConsoleCtrlHandler)
 *   - Standard C library (printf / Sleep)
 */

#include "../kta_mcu_integration.h"
#include "../../common/bridge_integration.h"
#include "../../backends/backend_interface.h"

#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

/* ============================================================================
 * Configuration
 * ============================================================================ */

/** Transport backend used by the bridge (compile-time selection). */
#define BRIDGE_TRANSPORT_TYPE     TRANSPORT_TYPE_UART

/** Bridge poll interval in milliseconds (how often bridge_integration_process
 *  is called when no data is available). */
#define KTA_BRIDGE_POLL_INTERVAL_MS  10U

/** Application thread heartbeat in milliseconds. */
#define KTA_APP_HEARTBEAT_MS         1000U

/* ============================================================================
 * Internal state
 * ============================================================================ */

/** Shared running flag.  Set to FALSE by the Ctrl-C handler to signal both
 *  threads to terminate gracefully. */
static volatile LONG g_running = 1L;

/* ============================================================================
 * Console Ctrl handler (replaces POSIX signal handler)
 * ============================================================================ */

static BOOL WINAPI ctrl_handler(DWORD dwCtrlType)
{
    switch (dwCtrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            printf("\nShutting down...\n");
            (void)InterlockedExchange(&g_running, 0L);
            return TRUE; /* Handled */
        default:
            return FALSE;
    }
}

/* ============================================================================
 * KTA Bridge thread
 * ============================================================================ */

static DWORD WINAPI kta_bridge_thread(LPVOID lpParam)
{
    (void)lpParam;

    printf("[Thread] KTA Bridge thread starting...\n");

    if (bridge_integration_init(BRIDGE_TRANSPORT_TYPE) != 0) {
        printf("[Thread] ERROR: Bridge init failed\n");
        return 1UL;
    }

    printf("[Thread] KTA Bridge thread running\n");

    while (InterlockedCompareExchange(&g_running, 1L, 1L) == 1L) {
        int result = bridge_integration_process();

        if (result > 0) {
            printf("[Thread] Command processed\n");
        } else if (result < 0) {
            printf("[Thread] ERROR: Bridge process failed (%d)\n", result);
        }

        Sleep(KTA_BRIDGE_POLL_INTERVAL_MS);
    }

    (void)bridge_integration_deinit();
    printf("[Thread] KTA Bridge thread stopped\n");

    return 0UL;
}

/* ============================================================================
 * Application thread
 * ============================================================================ */

static DWORD WINAPI application_thread(LPVOID lpParam)
{
    (void)lpParam;

    printf("Application thread starting...\n");

    while (InterlockedCompareExchange(&g_running, 1L, 1L) == 1L) {
        /* Place application logic here.
         * For example: read sensors, manage user interface, etc. */
        Sleep(KTA_APP_HEARTBEAT_MS);
    }

    printf("Application thread stopped\n");

    return 0UL;
}

/* ============================================================================
 * Integration entry point
 * ============================================================================ */

int kta_mcu_integration_entry(void)
{
    HANDLE  hThreads[2];
    DWORD   dwExitCode = 0UL;

    /* Register Ctrl-C / Ctrl-Break handler */
    if (!SetConsoleCtrlHandler(ctrl_handler, TRUE)) {
        printf("WARNING: Failed to register Ctrl-C handler (error %lu)\n",
               GetLastError());
        /* Non-fatal – continue anyway */
    }

    printf("=== KTA Bridge - Windows Integration ===\n");

    /* Spawn KTA bridge thread */
    hThreads[0] = CreateThread(
        NULL,               /* default security attributes */
        0UL,                /* default stack size          */
        kta_bridge_thread,  /* thread function             */
        NULL,               /* no argument                 */
        0UL,                /* run immediately             */
        NULL                /* no thread ID needed         */
    );

    if (NULL == hThreads[0]) {
        printf("ERROR: Failed to create bridge thread (error %lu)\n",
               GetLastError());
        return -1;
    }

    /* Spawn application thread */
    hThreads[1] = CreateThread(
        NULL,
        0UL,
        application_thread,
        NULL,
        0UL,
        NULL
    );

    if (NULL == hThreads[1]) {
        printf("ERROR: Failed to create application thread (error %lu)\n",
               GetLastError());
        (void)InterlockedExchange(&g_running, 0L);
        (void)WaitForSingleObject(hThreads[0], INFINITE);
        (void)CloseHandle(hThreads[0]);
        return -1;
    }

    printf("Threads created. Press Ctrl-C to exit.\n");

    /* Block until both threads have finished */
    (void)WaitForMultipleObjects(2UL, hThreads, TRUE, INFINITE);

    /* Retrieve exit codes for diagnostics */
    for (int i = 0; i < 2; i++) {
        if (GetExitCodeThread(hThreads[i], &dwExitCode) && (dwExitCode != 0UL)) {
            printf("WARNING: thread[%d] exited with code %lu\n", i, dwExitCode);
        }
        (void)CloseHandle(hThreads[i]);
    }

    printf("Shutdown complete\n");

    return 0;
}

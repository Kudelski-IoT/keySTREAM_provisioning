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
********************************************************************************/#include "../../common/kta_mcu_integration.h"
#include "../../common/bridge_integration.h"
#include "../../backends/backend_interface.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#define BRIDGE_TRANSPORT_TYPE TRANSPORT_TYPE_UART
#define KTA_BRIDGE_POLL_INTERVAL_US 10000

static volatile bool g_running = true;

static void signal_handler(int signum) {
    (void)signum;
    printf("\nShutting down...\n");
    g_running = false;
}

static void* kta_bridge_thread(void *arg) {
    (void)arg;
    printf("[Thread] KTA Bridge thread starting...\n");
    if (bridge_integration_init(BRIDGE_TRANSPORT_TYPE) != 0) {
        printf("[Thread] ERROR: Bridge init failed\n");
        return NULL;
    }
    printf("[Thread] KTA Bridge thread running\n");
    while (g_running) {
        int result = bridge_integration_process();
        if (result > 0) {
            printf("[Thread] Command processed\n");
        } else if (result < 0) {
            printf("[Thread] ERROR: Bridge process failed (%d)\n", result);
        }
        usleep(KTA_BRIDGE_POLL_INTERVAL_US);
    }
    bridge_integration_deinit();
    printf("[Thread] KTA Bridge thread stopped\n");
    return NULL;
}

static void* application_thread(void *arg) {
    (void)arg;
    printf("Application thread starting...\n");
    while (g_running) {
        sleep(1);
    }
    printf("Application thread stopped\n");
    return NULL;
}

int kta_mcu_integration_entry(void) {
    pthread_t bridge_thread;
    pthread_t app_thread;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    printf("=== KTA Bridge - Linux Integration ===\n");
    if (pthread_create(&bridge_thread, NULL, kta_bridge_thread, NULL) != 0) {
        printf("ERROR: Failed to create bridge thread\n");
        return -1;
    }
    if (pthread_create(&app_thread, NULL, application_thread, NULL) != 0) {
        printf("ERROR: Failed to create application thread\n");
        g_running = false;
        pthread_join(bridge_thread, NULL);
        return -1;
    }
    printf("Threads created. Press Ctrl+C to exit.\n");
    pthread_join(bridge_thread, NULL);
    pthread_join(app_thread, NULL);
    printf("Shutdown complete\n");
    return 0;
}

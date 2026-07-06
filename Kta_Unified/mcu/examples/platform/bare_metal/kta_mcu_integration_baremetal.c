#include "../kta_mcu_integration.h"
#include "../../common/bridge_integration.h"
#include "../../backends/backend_interface.h"
#include <stdio.h>
#include <stdbool.h>

#define BRIDGE_TRANSPORT_TYPE TRANSPORT_TYPE_UART

/* Optional user application init hook. Provide a weak default so projects
 * that do not define application_init() still link. */
__attribute__((weak)) int application_init(void) { return 0; }

int kta_mcu_integration_entry(void) {
    printf("=== KTA Bridge - Bare Metal Integration ===\n");

    if (application_init() != 0) {
        printf("ERROR: Application initialization failed\n");
        return -1;
    }

    if (bridge_integration_init(BRIDGE_TRANSPORT_TYPE) != 0) {
        printf("ERROR: Bridge initialization failed\n");
        return -1;
    }
    printf("Bridge initialized. Starting main loop...\n");

    /* Allow application code to request a clean shutdown by clearing this
     * flag (e.g. from an ISR or watchdog handler). */
    static volatile bool s_running = true;
    while (s_running) {
        int rc = bridge_integration_process();
        if (rc < 0) {
            printf("WARN: bridge_integration_process returned %d\n", rc);
            /* Continue rather than abort – transport may recover */
        }
        /* Insert application polling or event handling here */
    }

    (void)bridge_integration_deinit();
    return 0;
}

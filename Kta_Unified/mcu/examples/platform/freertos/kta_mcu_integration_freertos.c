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
********************************************************************************/#include "../kta_mcu_integration.h"
#include "../../common/bridge_integration.h"
#include "../../backends/backend_interface.h"
#include "../../bridgeKta/bridge_kta.h"
#include "../../mcu_config.h"
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* Resolve MCU_BACKEND token (uart | ble | usb | zigbee) from mcu_config.h
 * to the BackendType enum value used by backend_interface.h.
 * To change transport: edit MCU_BACKEND in mcu_config.h — do not edit here. */
#define _MCU_BTYPE_uart    BACKEND_TYPE_UART
#define _MCU_BTYPE_ble     BACKEND_TYPE_BLE
#define _MCU_BTYPE_usb     BACKEND_TYPE_USB
#define _MCU_BTYPE_zigbee  BACKEND_TYPE_ZIGBEE
#define _MCU_BTYPE_(x)     _MCU_BTYPE_ ## x
#define _MCU_BTYPE(x)      _MCU_BTYPE_(x)
#define BRIDGE_TRANSPORT_TYPE  _MCU_BTYPE(MCU_BACKEND)

/* FreeRTOS task tuning — adjust for your memory constraints */
#define KTA_BRIDGE_TASK_STACK_SIZE 16384u
#define KTA_BRIDGE_TASK_PRIORITY   (tskIDLE_PRIORITY + 2u)
#define KTA_BRIDGE_TASK_CORE       0

static const char *TAG = "KTA_MCU";

static TaskHandle_t s_bridge_task_handle = NULL;

static void kta_bridge_task(void *arg)
{
    (void)arg;
    KTA_LOG_I(TAG, "Bridge KTA task starting (transport backend: %d)", (int)BRIDGE_TRANSPORT_TYPE);

    if (bridge_integration_init(BRIDGE_TRANSPORT_TYPE) != 0) {
        KTA_LOG_E(TAG, "bridge_integration_init failed");
        vTaskDelete(NULL);
        return;
    }

    for (;;) {
        bridge_integration_process();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    /* Unreachable but kept for symmetry */
    bridge_integration_deinit();
    vTaskDelete(NULL);
}

/**
 * @brief Start the KTA bridge task (FreeRTOS platform implementation).
 *
 * Implements kta_mcu_integration_entry() for FreeRTOS platforms.
 * Call once after hardware peripherals are initialised.
 * Does NOT call vTaskStartScheduler() — the caller is responsible for
 * starting the RTOS scheduler when required by the platform.
 *
 * @return 0 on success, -1 if task creation failed.
 */
int kta_mcu_integration_entry(void)
{
    if (s_bridge_task_handle != NULL) {
        KTA_LOG_W(TAG, "kta_mcu_integration_entry called more than once; ignored");
        return 0;
    }

    KTA_LOG_I(TAG, "Creating KTA bridge task");
    BaseType_t ok = xTaskCreatePinnedToCore(
        kta_bridge_task,
        "bridge_kta",
        KTA_BRIDGE_TASK_STACK_SIZE,
        NULL,
        KTA_BRIDGE_TASK_PRIORITY,
        &s_bridge_task_handle,
        KTA_BRIDGE_TASK_CORE
    );

    if (ok != pdPASS) {
        KTA_LOG_E(TAG, "xTaskCreatePinnedToCore failed");
        return -1;
    }

    KTA_LOG_I(TAG, "Bridge KTA task created (MCU bridge pipeline active)");
    return 0;
}

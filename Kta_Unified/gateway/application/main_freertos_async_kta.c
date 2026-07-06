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
 * @file main_freertos_async_kta.c
 * @brief Production KTA Gateway with Continuous Monitoring - FreeRTOS
 * 
 * Production Features:
 *   - Automatic provisioning on startup
 *   - Continuous status monitoring every 24 hours
 *   - Automatic retry for failed provisioning
 *   - Field management checks (key rotation, updates)
 *   - Runs as FreeRTOS task
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
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdbool.h>

/* Configuration Constants */
#define KTA_POLL_INTERVAL_HOURS        24      /* Check every 24 hours */
#define KTA_RETRY_INTERVAL_MINUTES     10      /* Retry provisioning every 10 minutes */
#define KTA_STATUS_DISPLAY_INTERVAL    3600    /* Display status every hour */

/* Task stack size (words) */
#define KTA_TASK_STACK_SIZE     4096

/* Task priority */
#define KTA_TASK_PRIORITY       (tskIDLE_PRIORITY + 2)

/* Global exit code for status reporting */
static volatile int g_exit_code = 0;

/* ============================================================================
 * KTA Task - Production Ready
 * ============================================================================ */

static void kta_field_mgmt_task(void *pvParameters)
{
    (void)pvParameters;
    
    TKStatus status;
    TKktaKeyStreamStatus ks_status;
    
    printf("KTA Field Management Task Starting...\n");
    
    /* Step 1: Initialize KTA */
    printf("Initializing KTA...\n");
    status = ktaKeyStreamInit();
    
    if (status != E_K_STATUS_OK) {
        printf("ERROR: KTA initialization failed (code: %d)\n", status);
        g_exit_code = 1;
        goto exit_task;
    }
    
    printf("KTA initialized successfully\n");
    
    /* Step 2: Run field management */
    printf("Running field management...\n");
    status = ktaKeyStreamFieldMgmt(false, &ks_status);
    
    if (status != E_K_STATUS_OK) {
        printf("ERROR: Field management failed (code: %d)\n", status);
        g_exit_code = 2;
        goto exit_task;
    }
    
    /* Step 3: Report final status */
    printf("Field management completed\n");
    
    switch (ks_status) {
        case E_K_KTA_KS_STATUS_PROVISIONED:
            printf("Status: PROVISIONED\n");
            g_exit_code = 0;
            break;
        case E_K_KTA_KS_STATUS_NOT_PROVISIONED:
            printf("Status: NOT PROVISIONED\n");
            g_exit_code = 3;
            break;
        case E_K_KTA_KS_STATUS_SUSPENDED:
            printf("Status: SUSPENDED\n");
            g_exit_code = 4;
            break;
        case E_K_KTA_KS_STATUS_REFURBISH:
            printf("Status: REFURBISH REQUIRED\n");
            g_exit_code = 5;
            break;
        default:
            printf("Status: UNKNOWN (0x%02X)\n", ks_status);
            g_exit_code = 6;
            break;
    }
    
    printf("KTA Field Management Complete (status: %d)\n", g_exit_code);

exit_task:
    /* Task keeps running - enter monitoring loop */
    printf("\nKTA task entering production monitoring mode\n");
    printf("Configuration:\n");
    printf("  - Field management check: Every %d hours\n", KTA_POLL_INTERVAL_HOURS);
    printf("  - Provisioning retry: Every %d minutes (if not provisioned)\n", KTA_RETRY_INTERVAL_MINUTES);
    printf("  - Status display: Every hour\n\n");
    
    TickType_t last_wake_time = xTaskGetTickCount();
    uint32_t elapsed_seconds = 0;
    uint32_t last_field_mgmt_check = 0;
    uint32_t last_provisioning_retry = 0;
    
    while (1) {
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000));  /* 1 second */
        elapsed_seconds++;
        
        /* Display heartbeat status every hour */
        if (elapsed_seconds % KTA_STATUS_DISPLAY_INTERVAL == 0) {
            double uptime_hours = elapsed_seconds / 3600.0;
            printf("[%.1fh uptime] Status: ", uptime_hours);
            
            switch (ks_status) {
                case E_K_KTA_KS_STATUS_PROVISIONED:
                    printf("PROVISIONED (operational)\n");
                    break;
                case E_K_KTA_KS_STATUS_NOT_PROVISIONED:
                    printf("NOT PROVISIONED (retrying...)\n");
                    break;
                case E_K_KTA_KS_STATUS_SUSPENDED:
                    printf("SUSPENDED (awaiting reactivation)\n");
                    break;
                case E_K_KTA_KS_STATUS_REFURBISH:
                    printf("REFURBISH REQUIRED (restart needed)\n");
                    break;
                default:
                    printf("UNKNOWN\n");
            }
        }
        
        /* Retry provisioning if not yet provisioned (every 10 minutes) */
        if (ks_status == E_K_KTA_KS_STATUS_NOT_PROVISIONED) {
            uint32_t retry_interval_sec = KTA_RETRY_INTERVAL_MINUTES * 60;
            if ((elapsed_seconds - last_provisioning_retry) >= retry_interval_sec) {
                printf("[Auto-retry] Attempting provisioning...\n");
                status = ktaKeyStreamFieldMgmt(false, &ks_status);
                
                if (status == E_K_STATUS_OK) {
                    if (ks_status == E_K_KTA_KS_STATUS_PROVISIONED) {
                        printf("SUCCESS: Device provisioned!\n");
                        g_exit_code = 0;
                    } else {
                        printf("Provisioning incomplete, status: %d\n", ks_status);
                    }
                } else {
                    printf("WARNING: Provisioning retry failed (code: %d)\n", status);
                }
                
                last_provisioning_retry = elapsed_seconds;
            }
        }
        
        /* Periodic field management check (every 24 hours) */
        uint32_t field_mgmt_interval_sec = KTA_POLL_INTERVAL_HOURS * 3600;
        if ((elapsed_seconds - last_field_mgmt_check) >= field_mgmt_interval_sec) {
            printf("[Periodic Check] Running field management (keys/certs rotation)...\n");
            
            status = ktaKeyStreamFieldMgmt(true, &ks_status);
            
            if (status == E_K_STATUS_OK) {
                printf("Field management complete, status: ");
                switch (ks_status) {
                    case E_K_KTA_KS_STATUS_PROVISIONED:
                        printf("PROVISIONED\n");
                        break;
                    case E_K_KTA_KS_STATUS_SUSPENDED:
                        printf("SUSPENDED - device may require attention\n");
                        break;
                    case E_K_KTA_KS_STATUS_REFURBISH:
                        printf("REFURBISH - system restart recommended\n");
                        break;
                    default:
                        printf("Status: %d\n", ks_status);
                }
            } else {
                printf("WARNING: Field management check failed (code: %d)\n", status);
            }
            
            last_field_mgmt_check = elapsed_seconds;
        }
    }
    
    /* Never reached */
    vTaskSuspend(NULL);
}

/* ============================================================================
 * Main Entry Point
 * ============================================================================ */

int main(void)
{
    BaseType_t xReturned;
    
    printf("KTA FreeRTOS Application Starting...\n");
    
    /* Create KTA field management task */
    xReturned = xTaskCreate(
        kta_field_mgmt_task,         /* Task function */
        "KTA_FieldMgmt",              /* Task name */
        KTA_TASK_STACK_SIZE,          /* Stack size */
        NULL,                         /* Parameters */
        KTA_TASK_PRIORITY,            /* Priority */
        NULL                          /* Task handle */
    );
    
    if (xReturned != pdPASS) {
        printf("ERROR: Failed to create KTA task\n");
        return 1;
    }
    
    printf("Starting FreeRTOS scheduler...\n");
    
    /* Start the scheduler - this should never return */
    vTaskStartScheduler();
    
    /* Should never reach here */
    printf("ERROR: Scheduler returned unexpectedly\n");
    return 1;
}

/* ============================================================================
 * FreeRTOS Hooks (Optional)
 * ============================================================================ */

#if (configUSE_IDLE_HOOK == 1)
void vApplicationIdleHook(void)
{
    /* Idle task hook - can be used for low power mode */
}
#endif

#if (configUSE_TICK_HOOK == 1)
void vApplicationTickHook(void)
{
    /* Tick hook - called every tick */
}
#endif

#if (configUSE_MALLOC_FAILED_HOOK == 1)
void vApplicationMallocFailedHook(void)
{
    printf("FATAL: Malloc failed\n");
    taskDISABLE_INTERRUPTS();
    for (;;) {
        /* Loop forever */
    }
}
#endif

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("FATAL: Stack overflow in task: %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) {
        /* Loop forever */
    }
}
#endif

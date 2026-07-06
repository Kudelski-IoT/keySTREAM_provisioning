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
 * @file main_baremetal_async_kta.c
 * @brief Production KTA Gateway with Continuous Monitoring - Baremetal
 * 
 * Production Features:
 *   - Automatic provisioning on startup
 *   - Continuous status monitoring every 24 hours
 *   - Automatic retry for failed provisioning
 *   - Field management checks (key rotation, updates)
 *   - No RTOS dependency
 * 
 * Execution Flow:
 *   1. ktaKeyStreamInit() - One-time initialization
 *   2. ktaKeyStreamFieldMgmt(false) - Initial provisioning
 *   3. Periodic monitoring loop (24-hour interval)
 *   4. ktaKeyStreamFieldMgmt(true) - Periodic field management check
 * 
 * Platform Requirements:
 *   - Provide printf() implementation (UART/SWO/semihosting)
 *   - Provide system timer for delay functions in ktaFieldMgntHook
 *   - Ensure backend HAL is initialized before calling KTA functions
 * 
 * @author Kudelski IoT
 */

#include "../ktaIntegration/ktaFieldMgntHook.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/* Configuration Constants */
#define KTA_POLL_INTERVAL_HOURS        24      /* Check every 24 hours */
#define KTA_RETRY_INTERVAL_MINUTES     10      /* Retry provisioning every 10 minutes */
#define KTA_STATUS_DISPLAY_INTERVAL    3600    /* Display status every hour */

/* Platform-specific includes (customize for your MCU) */
/* Example for ARM Cortex-M: */
/* #include "stm32f4xx.h" */
/* #include "system_stm32f4xx.h" */

/* ============================================================================
 * Platform Initialization (Customize for your hardware)
 * ============================================================================ */

/**
 * @brief Initialize platform hardware
 * 
 * Customize this function for your specific MCU:
 *   - Clock configuration
 *   - GPIO initialization
 *   - UART/Debug console setup
 *   - Timer initialization
 *   - Interrupt controller setup
 */
static void platform_init(void)
{
    /* TODO: Add your platform-specific initialization here */
    
    /* Example for ARM Cortex-M:
     * SystemInit();              // Configure clocks
     * SystemCoreClockUpdate();   // Update SystemCoreClock variable
     * UART_Init();               // Initialize debug console
     * SysTick_Config(...);       // Configure system timer
     */
}

/**
 * @brief Simple delay (blocking)
 *
 * Note: ktaFieldMgntHook already has internal delays for async operations.
 * This is just for status reporting in the main loop.
 *
 * Implement this for your platform. The default is a calibrated busy-wait
 * that depends on a project-supplied CPU cycles-per-millisecond value
 * (UART_CPU_CYCLES_PER_MS). Override at compile time, e.g.
 *   -DUART_CPU_CYCLES_PER_MS=72000   for a 72 MHz Cortex-M
 *
 * For accurate timing, replace with HAL_Delay() / SysTick polling /
 * platform tick API. The volatile/asm prevents the compiler from
 * optimizing the loop away, but the absolute time is *only* approximate.
 */
#ifndef UART_CPU_CYCLES_PER_MS
#define UART_CPU_CYCLES_PER_MS 10000U /* generic placeholder */
#endif

static void delay_ms(uint32_t ms)
{
#if defined(HAL_GetTick)
    /* STM32 HAL detected */
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < ms) {
        __NOP();
    }
#else
    /* Calibrated busy-wait fallback. Replace with a real timer for production. */
    volatile uint32_t count = ms * UART_CPU_CYCLES_PER_MS;
    while (count--) {
        __asm__ volatile ("nop");
    }
#endif
}

/* ============================================================================
 * Main Entry Point
 * ============================================================================ */

int main(void)
{
    TKStatus status;
    TKktaKeyStreamStatus ks_status;
    int exit_code = 0;
    
    /* Initialize platform hardware */
    platform_init();
    
    printf("\n");
    printf("========================================\n");
    printf("KTA Field Management - Baremetal\n");
    printf("========================================\n");
    
    /* Step 1: Initialize KTA */
    printf("Initializing KTA...\n");
    status = ktaKeyStreamInit();
    
    if (status != E_K_STATUS_OK) {
        printf("ERROR: KTA initialization failed (code: %d)\n", status);
        exit_code = 1;
        goto main_loop;
    }
    
    printf("KTA initialized successfully\n");
    
    /* Step 2: Run field management */
    printf("Running field management...\n");
    status = ktaKeyStreamFieldMgmt(false, &ks_status);
    
    if (status != E_K_STATUS_OK) {
        printf("ERROR: Field management failed (code: %d)\n", status);
        exit_code = 2;
        goto main_loop;
    }
    
    /* Step 3: Report final status */
    printf("Field management completed\n");
    
    switch (ks_status) {
        case E_K_KTA_KS_STATUS_PROVISIONED:
            printf("Status: PROVISIONED\n");
            exit_code = 0;
            break;
        case E_K_KTA_KS_STATUS_NOT_PROVISIONED:
            printf("Status: NOT PROVISIONED\n");
            exit_code = 3;
            break;
        case E_K_KTA_KS_STATUS_SUSPENDED:
            printf("Status: SUSPENDED\n");
            exit_code = 4;
            break;
        case E_K_KTA_KS_STATUS_REFURBISH:
            printf("Status: REFURBISH REQUIRED\n");
            exit_code = 5;
            break;
        default:
            printf("Status: UNKNOWN (0x%02X)\n", ks_status);
            exit_code = 6;
            break;
    }
    
    printf("KTA Field Management Complete (exit: %d)\n", exit_code);
    printf("========================================\n");

main_loop:
    /* Main loop - baremetal applications never exit */
    printf("\nEntering production monitoring mode\n");
    printf("Configuration:\n");
    printf("  - Field management check: Every %d hours\n", KTA_POLL_INTERVAL_HOURS);
    printf("  - Provisioning retry: Every %d minutes (if not provisioned)\n", KTA_RETRY_INTERVAL_MINUTES);
    printf("  - Status display: Every hour\n\n");
    
    uint32_t elapsed_seconds = 0;
    uint32_t last_field_mgmt_check = 0;
    uint32_t last_provisioning_retry = 0;
    
    while (1) {
        delay_ms(1000);  /* 1 second */
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
                        exit_code = 0;
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
                        /* In baremetal, could trigger watchdog reset or set flag for restart */
                        break;
                    default:
                        printf("Status: %d\n", ks_status);
                }
            } else {
                printf("WARNING: Field management check failed (code: %d)\n", status);
            }
            
            last_field_mgmt_check = elapsed_seconds;
        }
    }  /* end of while(1) main monitoring loop */

    /* Never reached */
    return exit_code;
}

/* ============================================================================
 * Optional: Hard Fault Handler (for debugging)
 * ============================================================================ */

#ifdef __ARM_ARCH
void HardFault_Handler(void)
{
    printf("FATAL: Hard Fault occurred\n");
    while (1) {
        /* Loop forever - attach debugger to investigate */
    }
}
#endif

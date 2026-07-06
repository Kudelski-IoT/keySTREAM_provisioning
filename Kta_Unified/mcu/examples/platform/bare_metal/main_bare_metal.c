/**
 * @file main_bare_metal.c
 * @brief Bare Metal Example - KTA Bridge Integration
 * 
 * This example shows how to integrate the KTA bridge in a bare metal
 * environment (no RTOS). The bridge is called periodically from the
 * main loop without blocking the application.
 * 
 * Flow: main_bare_metal.c → bridge API → HAL → backend → SAL
 * 
 * Platform: Bare Metal (No RTOS)
 * - Direct hardware access
 * - Polling-based operation
 * - Single-threaded execution
 */


#include <stdio.h>
#include "../kta_mcu_integration.h"



int main(void)
{
    return kta_mcu_integration_entry();
}

/**
 * @file bridge_integration.h
 * @brief OS-Independent KTA Bridge Integration Layer
 * 
 * This module provides a simple API for integrating the KTA bridge
 * into any platform (bare metal, RTOS, Linux, etc.). It handles:
 * - Transport layer initialization
 * - Message serialization/deserialization
 * - Command processing
 * - Error handling
 * 
 * The integration is completely OS-independent and can be called from:
 * - Bare metal main loops
 * - RTOS tasks (FreeRTOS, Zephyr, etc.)
 * - OS threads (Linux, Windows, etc.)
 * 
 * Flow: Application → bridge_integration → Protocol → HAL → Backend → SAL
 */

#ifndef BRIDGE_INTEGRATION_H
#define BRIDGE_INTEGRATION_H

#include <stdint.h>
#include "../../backends/backend_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the KTA bridge integration layer
 * 
 * This function initializes:
 * 1. Transport protocol layer (TLV message handling)
 * 2. Transport HAL (selects backend: UART/BLE/USB/Zigbee)
 * 3. Opens the transport with default configuration
 * 
 * Call this once at startup before calling bridge_integration_process().
 * 
 * @param transport_type Type of transport to use (UART, BLE, USB, Zigbee)
 * @return 0 on success, -1 on failure
 * 
 * @note This function is thread-safe for initialization
 * 
 * Example:
 * @code
 *   if (bridge_integration_init(BACKEND_TYPE_UART) != 0) {
 *       printf("Bridge init failed\n");
 *       return -1;
 *   }
 * @endcode
 */
int bridge_integration_init(BackendType transport_type);

/**
 * @brief Process one KTA command (non-blocking)
 * 
 * This function:
 * 1. Checks for incoming messages (non-blocking)
 * 2. Deserializes TLV messages to structs
 * 3. Processes commands through KTA bridge
 * 4. Serializes response structs to TLV
 * 5. Sends response back to host
 * 
 * Call this function regularly from your main loop, task, or thread.
 * It returns immediately if no data is available.
 * 
 * @return  1 if a command was processed
 * @return  0 if no data available (timeout)
 * @return -1 on error
 * 
 * @note This function is non-blocking and should be called frequently
 * @note Typical usage: call every 10-50ms from your main loop/task
 * 
 * Example (Bare Metal):
 * @code
 *   while (1) {
 *       bridge_integration_process();  // Non-blocking
 *       application_logic();
 *       delay_ms(10);
 *   }
 * @endcode
 * 
 * Example (RTOS Task):
 * @code
 *   void bridge_task(void *arg) {
 *       while (1) {
 *           bridge_integration_process();
 *           vTaskDelay(pdMS_TO_TICKS(10));
 *       }
 *   }
 * @endcode
 * 
 * Example (Linux Thread):
 * @code
 *   void* bridge_thread(void *arg) {
 *       while (running) {
 *           bridge_integration_process();
 *           usleep(10000);
 *       }
 *       return NULL;
 *   }
 * @endcode
 */
int bridge_integration_process(void);

/**
 * @brief Cleanup and deinitialize the bridge
 * 
 * This function:
 * 1. Closes the transport
 * 2. Deinitializes the HAL layer
 * 3. Deinitializes the protocol layer
 * 4. Releases all resources
 * 
 * Call this before shutting down your application or when
 * switching to a different transport type.
 * 
 * @return 0 on success, -1 on failure
 * 
 * @note Safe to call multiple times
 * @note After calling this, you must call bridge_integration_init()
 *       again before calling bridge_integration_process()
 * 
 * Example:
 * @code
 *   bridge_integration_deinit();
 *   printf("Bridge shutdown complete\n");
 * @endcode
 */
int bridge_integration_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* BRIDGE_INTEGRATION_H */

#ifndef KTA_MCU_INTEGRATION_H
#define KTA_MCU_INTEGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Entry point for KTA MCU integration.
 *
 * This function initializes and runs the KTA bridge and application logic.
 * It should be called from main().
 * This API is platform-independent and will handle all required threads/tasks.
 *
 * @return 0 on success, non-zero on error.
 */
int kta_mcu_integration_entry(void);

#ifdef __cplusplus
}
#endif

#endif // KTA_MCU_INTEGRATION_H

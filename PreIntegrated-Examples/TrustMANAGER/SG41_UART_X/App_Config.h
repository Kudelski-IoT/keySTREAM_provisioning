/******************************************************************************
 *************************keySTREAM Trusted Agent ("KTA")********************
 * (c) 2023-2024 Nagravision Sarl
 *****************************************************************************/
/** \brief  COMMON configuration file - shared by gateway and keystream_connect.
 *
 *  \file App_Config.h
 *
 *  HOW TO USE:
 *    Edit values in THIS file only.
 *    Both projects (gateway and keystream_connect) read from here.
 *
 *  NOTE: The keySTREAM server endpoint (CoAP / HTTP URLs) is defined ONCE in
 *        ktaConfig.h and pulled in at the bottom of this file. Change the URLs
 *        there - this file simply re-exports them to the whole project.
 ******************************************************************************/

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * keySTREAM SERVER URLs
 * The endpoint URLs (C_KTA_APP__KEYSTREAM_HOST_COAP_URL / _HTTP_URL) are
 * DEFINED in ktaConfig.h and imported at the bottom of this file. Do not
 * redefine them here - edit them in ktaConfig.h instead.
 * ============================================================================ */

/* ============================================================================
 * DEVICE IDENTITY
 * Fleet Profile UID from keySTREAM - applies to both gateway and MCU.
 * ============================================================================ */

/**
 * @brief Fleet Profile UID created in keySTREAM.
 * Copy the Fleet Profile UID from keySTREAM and uncomment the following line.
 * Replace xxxxxxxxxxx with the Fleet Profile UID.
 */
//#define KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID             "xxxxxxxxxxx"
#if !defined(KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID)
    #error KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID not defined - set it above.
#endif

/* ============================================================================
 * UART / COM PORT  (Windows gateway only - ignored by MCU build)
 * ============================================================================ */

/** @brief COM port used to communicate with the MCU (wide-char for Win32 API). */
// #define UART_COM_PORT L"xxxxx"
#if !defined(UART_COM_PORT)
    #error UART_COM_PORT not defined - set it above.
#endif

/** @brief COM port name as a plain string (for display / logging). */
// #define UART_COM_PORT_STR "xxxx"
#if !defined(UART_COM_PORT_STR)
    #error UART_COM_PORT_STR not defined - set it above.
#endif

/* ============================================================================
 * MCU / keystream_connect TRANSPORT  (MCU build only - ignored by gateway)
 * ============================================================================ */

/** @brief Set to 1 to run the KTA MCU bridge over UART (no Wi-Fi demo flow). */
#ifndef APP_USE_UART_BRIDGE
#define APP_USE_UART_BRIDGE 1
#endif

/** @brief Set to 1 to run a simple UART echo / PING-PONG smoke test on SERCOM6. */
#ifndef APP_UART_SMOKE_TEST
#define APP_UART_SMOKE_TEST 0
#endif

/** @brief Set to 1 to enable HTTP relay mode (requires APP_UART_SMOKE_TEST = 1). */
#ifndef APP_UART_HTTP_RELAY
#define APP_UART_HTTP_RELAY 1
#endif

/** @brief Periodic status print period (ms) on SYS_CONSOLE. Set to 0 to disable. */
#ifndef APP_BRIDGE_STATUS_PRINT_PERIOD_MS
#define APP_BRIDGE_STATUS_PRINT_PERIOD_MS 5000U
#endif

/**
 * @brief Set to 1 to wipe the KTA NVM lifecycle state slot before ktaInitialize.
 * Use this ONCE when a device is stuck in a partial/broken provisioning state.
 * WARNING: After clearing, the MCU calls exit(1) and halts - power-cycle the
 * board, then reflash with APP_KTA_RESET_LIFECYCLE = 0 to resume normal flow.
 */
#ifndef APP_KTA_RESET_LIFECYCLE
#define APP_KTA_RESET_LIFECYCLE 0
#endif

/* ============================================================================
 * keySTREAM ENDPOINT + LOG LEVEL (single source of truth)
 * Pull in ktaConfig.h LAST - after all the macros above are defined so that
 * ktaConfig.h can reference KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID and honor
 * APP_USE_UART_BRIDGE. ktaConfig.h provides:
 *   - C_KTA_APP__KEYSTREAM_HOST_COAP_URL / _HTTP_URL (the endpoint)
 *   - LOG_KTA_ENABLE (KTA log level used by the gateway log module)
 * Its own re-include of this file is a no-op thanks to the include guard.
 * ============================================================================ */
#include "ktaConfig.h"

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
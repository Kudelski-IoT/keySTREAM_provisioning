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
********************************************************************************/
/**
 * @file App_Config.h
 * @brief Customer Application Configuration — Single Point of Configuration
 *
 * ╔══════════════════════════════════════════════════════════════════╗
 * ║           CUSTOMER-EDITABLE CONFIGURATION FILE                  ║
 * ║                                                                  ║
 * ║  Edit ONLY this file to configure your keySTREAM integration.   ║
 * ║  Do NOT modify any other files in Kta_Unified/.                  ║
 * ║                                                                  ║
 * ║  This file is included automatically by:                         ║
 * ║    - gateway/ktaIntegration/ktaFieldMgntHook.c                   ║
 * ║    - ktaConfig.h (KTA library configuration)                     ║
 * ╚══════════════════════════════════════════════════════════════════╝
 *
 * Configuration sections:
 *   1. Fleet Profile (required)
 *   2. Transport / Backend (MCU only)
 *   3. UART / Serial port settings
 *   4. Logging
 *   5. Device role
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. FLEET PROFILE CONFIGURATION  (REQUIRED)
 *
 * Set KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID to the Fleet Profile Public UID
 * obtained from the keySTREAM portal.
 *
 * Steps:
 *   1. Log in to https://mc.obp.iot.kudelski.com/products/iot
 *   2. Go to Fleet and PKI → FLEET AND CERT
 *   3. Copy the "Fleet Profile Public UID" of your fleet
 *   4. Paste it below (the full string including colons)
 *
 * Example:  "9S4F:example.org:dp:network:device"
 * ============================================================================ */
#define KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID   "REPLACE_WITH_YOUR_FLEET_PROFILE_UID"

/* ============================================================================
 * 2. TRANSPORT / BACKEND SELECTION  (MCU builds only)
 *
 * Select the closed-network transport used between the MCU and the Gateway.
 * This must match the MCU_BACKEND setting in mcu/mcu_config.h.
 *
 * Supported values: BACKEND_TYPE_UART | BACKEND_TYPE_BLE |
 *                   BACKEND_TYPE_USB  | BACKEND_TYPE_ZIGBEE
 *
 * Note: Gateway builds ignore this setting (the gateway always uses HTTP).
 * ============================================================================ */
/* #define APP_TRANSPORT_TYPE   BACKEND_TYPE_UART */

/* ============================================================================
 * 3. UART / SERIAL PORT SETTINGS  (Gateway + UART-connected MCU builds)
 *
 * Windows: set UART_COM_PORT to the numeric COM port number (e.g. 3 for COM3)
 *          and UART_COM_PORT_STR to the matching string (e.g. "\\\\.\\COM3").
 * Linux:   set UART_COM_PORT_STR to the device node (e.g. "/dev/ttyUSB0").
 * ============================================================================ */
#define UART_COM_PORT       3
#define UART_COM_PORT_STR   "\\\\.\\COM3"   /* Windows: change 3 to your port */
/* #define UART_COM_PORT_STR   "/dev/ttyUSB0" */   /* Linux */

/* ============================================================================
 * 4. LOGGING
 *
 * Define LOG_KTA_ENABLE to activate the KTALog framework throughout the
 * KTA library.  Log output goes to the platform sink configured in KTALog.c.
 *
 * For the bridge layer (Kta_Unified/mcu/bridgeKta/), define
 * KTA_BRIDGE_LOG_ENABLE to activate stdout logging on non-ESP32 targets.
 * On ESP32 / ESP-IDF the bridge always logs via ESP_LOG regardless of this flag.
 * ============================================================================ */
/* #define LOG_KTA_ENABLE        */   /* Uncomment to enable KTA library logging */
/* #define KTA_BRIDGE_LOG_ENABLE */   /* Uncomment to enable bridge layer logging */

/* ============================================================================
 * 5. DEVICE ROLE  (future use)
 *
 * Reserved for deployments where the same firmware image can act as either
 * an MCU endpoint or a gateway node.  Not used in the current release.
 * ============================================================================ */
/* #define APP_ROLE_MCU     */
/* #define APP_ROLE_GATEWAY */

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H */

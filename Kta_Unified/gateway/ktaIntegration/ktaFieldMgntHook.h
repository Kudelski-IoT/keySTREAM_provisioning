/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2025 Nagravision Sàrl

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
/** \brief  keySTREAM Trusted Agent - Hook for keySTREAM Trusted Agent
 *          initialization and field management.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file ktaFieldMgntHook.h
 ******************************************************************************/

/**
 * @brief Interface APIs for initialization and field management
 * 
 * ┌────────────────────────────────────────────────────────────────┐
 * │                  PUBLIC CUSTOMER-FACING API                    │
 * │                                                                │
 * │ This is the ONLY header customers need to include.             │
 * │ Just 3 simple functions for complete KTA integration.          │
 * │                                                                │
 * │ Usage:                                                         │
 * │   #include "ktaIntegration/ktaFieldMgntHook.h"                 │
 * │                                                                │
 * │   ktaKeyStreamInit();                                          │
 * │   ktaKeyStreamFieldMgmt(false, &status);                       │
 * │                                                                │
 * │ All internal complexity (threading, backends, transport)       │
 * │ is completely hidden from customer applications.               │
 * └────────────────────────────────────────────────────────────────┘
 */

#ifndef KTA_FIELD_MGNT_HOOK_H
#define KTA_FIELD_MGNT_HOOK_H

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/** @brief KTA Status codes (aligned with k_defs.h) */
typedef enum {
    E_K_STATUS_OK = 0,
    E_K_STATUS_ERROR = -1,
    E_K_STATUS_PARAMETER = -2
} TKStatus;

/** @brief KTA KeyStream Status — matches TKktaKeyStreamStatus in k_kta.h exactly */
typedef enum {
    E_K_KTA_KS_STATUS_NONE         = -1, /* Initial state, never returned by ktaKeyStreamStatus */
    E_K_KTA_KS_STATUS_NO_OPERATION =  0, /* Device operational, nothing to do                   */
    E_K_KTA_KS_STATUS_RENEW        =  1, /* Server issued a renewal command                     */
    E_K_KTA_KS_STATUS_REFURBISH    =  2  /* Server issued a refurbish (wipe + re-provision)     */
} TKktaKeyStreamStatus;

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Hook API for keySTREAM Trusted Agent initialization.
 *
 *   This function performs:
 *     - ktaInitialize()
 *     - ktaStartup()
 *     - ktaSetDeviceInformation()
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input values.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaKeyStreamInit(void);

/**
 * @brief
 *   Hook API for device onboarding and field management.
 *
 *   This function performs:
 *     - ktaExchangeMessage() loop (continues until no more messages)
 *     - ktaKeyStreamStatus()
 *
 * @param[in] xIsFieldMgmtReq
 *   If only onboarding required set it to false.
 *   If certificate/Key rotation or refurbish required set it to true
 *
 * @param[in,out] xpKtaKSCmdStatus
 *   [in] Pointer to the buffer carrying keySTREAM command status.
 *   [out] keySTREAM command status.
 *   Should not be NULL.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaKeyStreamFieldMgmt(
    bool xIsFieldMgmtReq,
    TKktaKeyStreamStatus *xpKtaKSCmdStatus
);

#ifdef __cplusplus
}
#endif

#endif /* KTA_FIELD_MGNT_HOOK_H */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

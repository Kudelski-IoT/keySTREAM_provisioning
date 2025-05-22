/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2024 Nagravision Sàrl

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
 * @brief     Interface apis for intialization and field management
 * @ingroup   g_kta_hook
 */
/** @addtogroup g_kta_hook
 * @{
 */

#ifndef K_KTA_HOOK_H
#define K_KTA_HOOK_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* --------------------------------------------------------------------------------------------- */
/* IMPORTS                                                                                       */
/* --------------------------------------------------------------------------------------------- */
#include <stddef.h>
#include <stdbool.h>
#include "k_defs.h"
#include "k_kta.h"

/* --------------------------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                                        */
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/* VARIABLES                                                                                     */
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/* FUNCTIONS                                                                                     */
/* --------------------------------------------------------------------------------------------- */
/**
 * @ingroup g_kta_hook
 * @brief
 *   Hook API for keySTREAM Trusted Agent initialization.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input values.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaKeyStreamInit
(
  void
);

/**
 * @ingroup g_kta_hook
 * @brief
 *   Hook API for device onboarding and field management.
 *
 * @param[in] xIsFieldMgmtReq
 *   If only onbaording required set it to False.
 *   If certificate/Key rotation or refurbish required set it to True
 * @param[in,out] xpktaKSCmdStatus
 *   [in] Pointer to the buffer carrying keySTREAM command status.
 *   [out] keySTREAM command status.

 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus ktaKeyStreamFieldMgmt
(
  bool                   xIsFieldMgmtReq,
  TKktaKeyStreamStatus*  xpKtaKSCmdStatus
);

#ifdef __cplusplus
}
#endif /* C++ */

/** @defgroup g_kta_hook */
/** @} g_kta_hook */

#endif // K_KTA_HOOK_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */


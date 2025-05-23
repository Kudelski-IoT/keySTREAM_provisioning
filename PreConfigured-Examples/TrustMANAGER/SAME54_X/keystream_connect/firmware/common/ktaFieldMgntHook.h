/******************************************************************************
 * Copyright (c) 2023-2023 Nagravision Sàrl. All rights reserved.
 ******************************************************************************/
/** \brief  keySTREAM Trusted Agent - Hook for keySTREAM Trusted Agent
 *          initialization and field management.
 *
 *  \author Griffin_Team
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
  bool                   isFieldMgntReq,
  TKktaKeyStreamStatus*  xpktaKSCmdStatus
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


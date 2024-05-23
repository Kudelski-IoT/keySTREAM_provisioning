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
/** \brief keySTREAM Trusted Agent - Type definitions.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_defs.h
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - Type definitions.
 */

#ifndef K_DEFS_H
#define K_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#ifndef K_SAL_API
#define K_SAL_API
#endif /* K_SAL_API */

/** @defgroup g_kta_api keySTREAM Trusted Agent Interface */

/** @addtogroup g_kta_api
 * @{
*/

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#ifdef INCLUDE_OBFUSCATE_SYMBOLS
#include "symbols.h"
#endif /* INCLUDE_OBFUSCATE_SYMBOLS */

#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/** @brief keySTREAM Trusted Agent return status codes. */
typedef enum
{
  /**
   * Status OK, everything is fine.
   */
  E_K_STATUS_OK,
  /**
   * Bad parameter
   */
  E_K_STATUS_PARAMETER,
  /**
   * Inconsistent or unsupported data.
   */
  E_K_STATUS_DATA,
  /**
   * Call cannot be done at this time (due to call sequence or life cycle).
   */
  E_K_STATUS_STATE,
  /**
   * Failure on memory allocation.
   */
  E_K_STATUS_MEMORY,
  /**
   * Missing, empty or non-available data.
   */
  E_K_STATUS_MISSING,
  /**
   * Undefined error.
   */
  E_K_STATUS_ERROR,
  /**
   * Decryption failed.
   */
  E_K_STATUS_DECRYPTION,
  /**
   * Already personalized.
   */
  E_K_STATUS_PERSONALIZED,
  /**
   * The requested action is unsupported.
   */
  E_K_STATUS_NOT_SUPPORTED,
  /**
   * Number of status values.
   */
  E_K_NUM_STATUS
} TKStatus;

/** @} g_kta_api */
/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_DEFS_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

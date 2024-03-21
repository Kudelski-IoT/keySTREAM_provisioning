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
/** \brief  Interface for for third party operations.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_platform.h
 ******************************************************************************/

/**
 * @brief Interface for for third party operations.
 */

#ifndef K_SAL_PLATFORM_H
#define K_SAL_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/** @defgroup g_sal_api SAL Interface */

/** @addtogroup g_sal_api
 * @{
*/

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_defs.h"

#include <stddef.h>
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   Process blob of data which is device platform specific.
 *   Can be 3rd Party specific data blob pre-formatted by Server.
 *
 * @param[in] xpInBlob
 *   Should not be NULL.
 *   Address of buffer containing the input data blob.
 *
 * @param[in] xInBlobLen
 *   Length of the buffer containing the input data blob.
 *
 * @param[in] xpOutBlob
 *   Should not be NULL.
 *   Address of buffer where the device platform will return the output data blob.
 *   This parameter is set by caller.
 *
 * @param[in,out] xpOutBlobLen
 *   Address of output buffer length (in Bytes). Should not be NULL.
 *   [in]  Caller set the maximum output buffer length expected.
 *   [out] the function set the actual length of the output buffer;
 *   MAX size = 2-Bytes (set at compile-time).
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus salPlatformProcess
(
  const uint8_t*  xpInBlob,
  size_t          xInBlobLen,
  uint8_t*        xpOutBlob,
  size_t*         xpOutBlobLen
);

/** @} g_sal_api */

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_PLATFORM_H
/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */


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
/** \brief  Interface for storage operation.
*
* \author Kudelski IoT
*
* \date 2023/06/02
*
* \file k_sal_storage.h
*
******************************************************************************/

/**
 * @brief Interface for storage operation.
 */

#ifndef K_SAL_STORAGE_H
#define K_SAL_STORAGE_H

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
#include "k_sal.h"

#include <stdint.h>
#include <stddef.h>

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
 *   To store and lock the data based on ID.
 *
 * @param[in] xStorageDataId
 *   Storage data Identifier. Refer Storage_Ids.
 * @param[in] xpData
 *   Address of buffer containing the input data.
 *   Should not be NULL.
 * @param[in] xDataLen
 *   Length of xpData buffer in bytes.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salStorageSetAndLockValue
(
  uint32_t        xStorageDataId,
  const uint8_t*  xpData,
  size_t          xDataLen
);

/**
 * @brief
 *   To store the data based on ID.
 *
 * @param[in] xStorageDataId
 *   Storage data Identifier. Refer Storage_Ids.
 * @param[in] xpData
 *   Address of buffer containing the input data.
 *   Should not be NULL.
 * @param[in] xDataLen
 *   Length of xpData buffer in bytes.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salStorageSetValue
(
  uint32_t        xStorageDataId,
  const uint8_t*  xpData,
  size_t          xDataLen
);

/**
 * @brief
 *   To get the data based on ID.
 *
 * @param[in] xStorageDataId
 *   Storage data Identifier. Refer Storage_Ids.
 * @param[out] xpData
 *   Address of buffer where the device platform will return the output data.
 *   Should not be NULL.
 * @param[in,out] xpDataLen
 *   Address of output buffer length (in Bytes).
 *   [in] Caller set the maximum output buffer length expected.
 *   [out] The function set the actual length of the output buffer.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salStorageGetValue
(
  uint32_t  xStorageDataId,
  uint8_t*  xpData,
  size_t*   xpDataLen
);

/** @} g_sal_api */

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_STORAGE_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */


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
/** \brief Interface for socket communication.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/02
 *
 *  \file k_sal_com.h
 *
 ******************************************************************************/
/**
 * @brief     Interface for socket communication.
 * @ingroup   g_sal
 */
/**
 * @addtogroup g_sal
 * @{
 */
/**
 * @defgroup g_sal_com  com in usage.
 */
/**
 * @} g_sal
 */

#ifndef K_SAL_COM_H
#define K_SAL_COM_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_comm_defs.h"

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
 * @ingroup
 *   g_sal_com
 *
 * @brief
 *   Initalize COM.
 *
 * @post
 *   Call salComTerm().
 *
 * @param[in] xConnectTimeoutInMs
 *   Connection timeout in milliseconds.
 * @param[in] xReadTimeoutInMs
 *   Read timeout in milliseconds.
 * @param[out] xppComInfo
 *   Pointer com info data. Should not be NULL.
 *
 * @return
 * - E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComInit
(
  uint32_t  xConnectTimeoutInMs,
  uint32_t  xReadTimeoutInMs,
  void**    xppComInfo
);

/**
 * @ingroup
 *   g_sal_com
 *
 * @brief
 *   Establish connection with server.
 *
 * @param[in] xpComInfo
 *   Com Info data. Should not be NULL.
 * @param[in] xpHost
 *   Server Host name. Should not be NULL. Must have '\0' at the end.
 * @param[in] xpPort
 *   Server Port. Should not be NULL. Must have '\0' at the end.
 *
 * @return
 * - E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComConnect
(
  void*           xpComInfo,
  const uint8_t*  xpHost,
  const uint8_t*  xpPort
);

/**
 * @ingroup
 *   g_sal_com
 *
 * @brief
 *   Send data to the server.
 *
 * @pre
 *   salComConnect should be successfully executed prior to this function.
 *
 * @param[in] xpComInfo
 *   Com info data; Should not be NULL.
 * @param[in] xpBuffer
 *   Data buffer to send; must point to *xpBufferLen bytes.
 * @param[in] xBufferLen
 *   Size of the data buffer, in bytes.
 *
 * @return
 * - E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComWrite
(
  void*           xpComInfo,
  const uint8_t*  xpBuffer,
  size_t          xBufferLen
);

/**
 * @ingroup
 *   g_sal_com
 *
 * @brief
 *   Receive data from the server.
 *
 * @pre
 *   salComConnect should be successfully executed prior to this function.
 *
 * @param[in] xpComInfo
 *   Com Info data; Should not be NULL.
 * @param[out] xpBuffer
 *   Data buffer to fill; must point to *xpBufferLen bytes.
 * @param[in,out] xBufferLen
 *   [in] Size of the data buffer, in bytes.
 *   [out] Size of the received data, in bytes.
 *
 * @return
 * - E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComRead
(
  void*     xpComInfo,
  uint8_t*  xpBuffer,
  size_t*   xpBufferLen
);

/**
 * @ingroup
 *   g_sal_com
 *
 * @brief
 *   Terminating communication.
 *
 * @param[in] xpComInfo
 *   Com Info data. Should not be NULL.
 *
 * @return
 * - E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComTerm
(
  void*  xpComInfo
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_COM_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

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
/** \brief Communication stack interface utility.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file comm_interface_util.h
 ******************************************************************************/
/**
 * @brief Communication stack interface utility.
 */

#ifndef COMM_INTERFACE_UTILS_H
#define COMM_INTERFACE_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "comm_interface.h"
#include "k_sal_socket.h"
#include "k_sal_os.h"
#include "k_sal_random.h"

#include <stdio.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */
/**
 * SUPPRESS: MISRA_DEV_KTA_003 : misra_c2012_rule_21.6_violation
 * SUPPRESS: MISRA_DEV_KTA_001 : misra_c2012_rule_17.1_violation
 * Using printf for logging.
 * Not checking the return status of printf, since not required.
 **/
/* Macro to enable debug logs. */
#ifdef ENABLE_COMM_DEBUG_PRINTS
#define M_COMM__LOG(x_print)   printf x_print; printf("\r")
#else
#define M_COMM__LOG(x_print)
#endif /* ENABLE_COMM_DEBUG_PRINTS */

/* Macro to enable info logs. */
#ifdef ENABLE_COMM_DEBUG_PRINTS
#define M_COMM__INFO(x_print) \
  printf("\nCOMM> "); \
  M_COMM__LOG(x_print)
#else
#define M_COMM__INFO(x_print)
#endif /* ENABLE_COMM_DEBUG_PRINTS */

/* Macro to enable error logs. */
#ifdef ENABLE_COMM_DEBUG_PRINTS
#define M_COMM__ERROR(x_print) \
  printf("\nCOMM> "); \
  M_COMM__LOG(x_print)
#else
#define M_COMM__ERROR(x_print)
#endif /* ENABLE_COMM_DEBUG_PRINTS */

/* Macro to print API start. */
#define M_COMM__API_START()    M_COMM__INFO((">Start of %s", __func__))

/* Macro to print API end. */
#define M_COMM__API_END()      M_COMM__INFO(("<End of %s", __func__))

/**
 * @brief Boolean enums.
 */
typedef enum
{
  E_FALSE,
  E_TRUE,
  E_UNDEF
} TBoolean;

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Convert the IP address to sal socket structure.
 *
 * @param[in] xpAddress
 *   Ip address of the server.
 *   Should not be NULL.
 * @param[out] xpSocketIp
 *   [in] Pointer to structure holding socket info.
 *   [out] Actual structure holding socket info.
 *   Should not be NULL.
 *
 * @return
 * - E_TRUE in case of success.
 * - E_FALSE in case of failure.
 */
TBoolean commUtilConvertSocketIp
(
  const uint8_t*  xpAddress,
  TKSocketIp*     xpSocketIp
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // COMM_INTERFACE_UTILS_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

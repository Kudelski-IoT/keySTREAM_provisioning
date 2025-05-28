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
/** \brief  SAL for socket commmunication, based on BSD socket API.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_socket.h
 ******************************************************************************/
/**
 * @brief SAL for socket commmunication, based on BSD socket API.
 */

#ifndef K_SAL_SOCKET_H
#define K_SAL_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_comm_defs.h"
#ifdef SAL_SOCKET_WRAP
#include "sal_socket_wrap.h"
#endif /* SAL_SOCKET_WRAP */

#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/* Maximum IP4 address length. */
#define C_SAL__MAX_IP4_ADDRESS_LENGTH (16u)

/** @brief Socket type. */
typedef enum
{
  E_SAL_SOCKET_TYPE_UDP,
  /* UDP */
  E_SAL_SOCKET_TYPE_TCP,
  /* TCP */
  E_SAL_SOCKET_NUM_TYPES
  /* Number of items in this enum. */
} TKSalSocketType;

/** @brief Socket (hidden type) */
typedef struct SKSalSocket TKSalSocket;

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Create a socket instance to exchange data.
 *
 * @param[in] xType
 *   Socket type.
 * @param[in,out] xppSocket
 *   [in] Pointer to socket info instance.
 *   [out] Actual socket info instance.
 *   Should not be NULL.
 *
 * @return
 * - E_K_COMM_STATUS_OK in case of success.
 * - E_K_COMM_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_COMM_STATUS_DATA for unexpected data.
 * - E_K_COMM_STATUS_MISSING for missing data.
 * - E_K_COMM_STATUS_ERROR for other errors.
 */
K_SAL_API TKCommStatus salSocketCreate
(
  const TKSalSocketType  xType,
  TKSalSocket**          xppSocket
);

/**
 * @brief
 *   Send bytes through a socket.
 *
 * @param[in] xpSocket
 *   Socket info.
 *   Should not be NULL.
 * @param[in] xpBuffer
 *   Data buffer to use.
 *   Must point to xBufferLength bytes.
 *   Should not be NULL.
 * @param[in] xBufferLength
 *   Size of the data to send, in bytes.
 * @param[in] xpAddress
 *   Address of the destination.
 *   Should not be NULL.
 * @return
 * - E_K_COMM_STATUS_OK in case of success.
 * - E_K_COMM_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_COMM_STATUS_DATA for unexpected data.
 * - E_K_COMM_STATUS_NETWORK for network related error.
 * - E_K_COMM_STATUS_ERROR for other errors.
 */
K_SAL_API TKCommStatus salSocketSendTo
(
  TKSalSocket*           xpSocket,
  const unsigned char*   xpBuffer,
  const size_t           xBufferLength,
  const TKSocketIp*      xpAddress
);

/**
 * @brief
 *   Receive data from keySTREAM.
 *
 * @param[in] xpSocket
 *   Socket info.
 *   Should not be NULL.
 * @param[in,out] xpBuffer
 *   [in] Pointer to buffer hoding revceivced message.
 *   [out] Actual data receivced from keySTREAM.
 *   Should not be NULL.
 * @param[in,out] xpBufferLength
 *   [in] Pointer to buffer hoding revceivced message.
 *   [out] Actual data receivced from keySTREAM.
 *   Should not be NULL.
 * @param[in,out] xpAddress
 *   [in] Pointer to buffer hoding source address.
 *   [out] Actual source address.
 *   Should not be NULL.
 *
 * @return
 * - E_K_COMM_STATUS_OK in case of success.
 * - E_K_COMM_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_COMM_STATUS_MISSING for missing data.
 * - E_K_COMM_STATUS_NETWORK for network related error.
 * - E_K_COMM_STATUS_ERROR for other errors.
 */
K_SAL_API TKCommStatus salSocketReceiveFrom
(
  TKSalSocket*      xpSocket,
  unsigned char*    xpBuffer,
  size_t*           xpBufferLength,
  TKSocketIp*       xpAddress
);

/**
 * @brief
 *   Dispose a socket instance.
 *
 * @param[in] xpSocket
 *   Socket instace to dispose.
 */
K_SAL_API void salSocketDispose
(
  TKSalSocket*  xpSocket
);

/**
 * @brief
 *   Get the network network MTU (maximum transmission unit).
 *   If for some reason the value is not available,
 *   the function shall return E_K_COMM_STATUS_MISSING.
 *
 * @param[in,out] xpValue
 *   Pointer to hold network value.
 *   Actual value for MTU.
 *   Should not be NULL.
 *
 * - E_K_COMM_STATUS_OK in case of success.
 * - E_K_COMM_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_COMM_STATUS_MISSING for missing data.
 */
K_SAL_API TKCommStatus salSocketGetNetworkMtu
(
  size_t*   xpValue
);

/**
 * @brief
 *   Convert Host URL to IP address.
 *
 * @param[in] xpHost
 *   Host URL, string encdode.
 *   Should not be NULL.
 * @param[in] xpIpAddress
 *   [in] Pointer to hold IP address.
 *   [out] Actual ip address.
 *   Should not be NULL.
 * @return
 * - E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salGetHostByName
(
  const char*  xpHost,
  uint8_t*     xpIpAddress
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_SOCKET_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

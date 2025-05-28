/******************************************************************************
 * Copyright (c) 2023-2023 Nagravision Sàrl. All rights reserved.
 ******************************************************************************/
/** \brief Interface for socket communication.
 *
 *  \author Griffin_Team
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

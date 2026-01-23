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
/** \brief  Interface for socket communication.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/07/19
 *
 *  \file k_sal_com.c
 ******************************************************************************/
/**
 * @brief Interface for socket communication.
 */

#include "k_sal_com.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "iot_secure_sockets.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Max read retry count. */
#define C_SAL_COM_MAX_READ_RETRY_COUNT   (3u)

#ifdef DEBUG
/** @brief Enable sal com logs. */
#define M_INTL_SAL_COM_DEBUG(__PRINT__)  do { \
                                              printf("\tCOM %d>", __LINE__); \
                                              printf __PRINT__; \
                                              printf("\r\n"); \
                                          } while (0)
#define M_INTL_SAL_COM_ERROR(__PRINT__)  do { \
                                              printf("\tCOM %d> ERROR ", __LINE__); \
                                              printf __PRINT__; \
                                              printf("\r\n"); \
                                          } while (0)
#else
#define M_INTL_SAL_COM_DEBUG(__PRINT__)
#define M_INTL_SAL_COM_ERROR(__PRINT__)
#endif /* DEBUG. */

/**
 * @ingroup g_k_types
 *
 * @brief Communication info.
 */
typedef struct
{
  Socket_t  socketId;
  /* ID of unique socket used. */
  uint32_t  writeTimeOut;
  /* Write timeout. */
  uint32_t  readTimeOut;
  /* Read timeout. */
  uint32_t  isConnected;
  /* Flag to check whether already connection estabilished with server or not. */
  uint8_t ptrId;
  /* Magic number to validate Pointer */
} TKComInfo;

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
static TKComInfo  gComInfo = {SOCKETS_INVALID_SOCKET, 0, 0, 0, 2u};
/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief  implement salComInit
 *
 */
K_SAL_API TKCommStatus salComInit
(
  uint32_t  xConnectTimeoutInMs,
  uint32_t  xReadTimeoutInMs,
  void**    xppComInfo
)
{
  TKCommStatus      status = E_K_COMM_STATUS_ERROR;
  TKComInfo*        pComInfo = NULL;

  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));
  for (;;)
  {
    if (NULL == xppComInfo)
    {
      M_INTL_SAL_COM_ERROR(("Invalid parameter"));
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }
    pComInfo = (TKComInfo *)*xppComInfo;
    pComInfo = &gComInfo;
    pComInfo->socketId = SOCKETS_INVALID_SOCKET;
    pComInfo->writeTimeOut = xConnectTimeoutInMs;
    pComInfo->readTimeOut = xReadTimeoutInMs;
    pComInfo->isConnected = 0;

    *xppComInfo = pComInfo;
    status = E_K_COMM_STATUS_OK;
    break;
  }
  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));
  return status;
}

/**
 * @brief  implement salComConnect
 *
 */
K_SAL_API TKCommStatus salComConnect
(
  void*          xpComInfo,
  const uint8_t* xpHost,
  const uint8_t* xpPort
)
{
  TKComInfo*          pComInfo = (TKComInfo *)xpComInfo;
  TKCommStatus        status = E_K_COMM_STATUS_ERROR;
  int32_t             socStatus = SOCKETS_SOCKET_ERROR;
  SocketsSockaddr_t   socketAddress = { 0 };
  uint16_t            port = 0;
  uint32_t            retVal = 0;
  char*               pEnd = NULL;

  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));

  for (;;)
  {
    if ((NULL == xpComInfo) || (NULL == xpHost) || (NULL == xpPort))
    {
      M_INTL_SAL_COM_ERROR(("Invalid parameter"));
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }
    if (pComInfo->ptrId != 2u)
    {
      M_INTL_SAL_COM_ERROR(("Invalid parameter"));
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }
    if (1u == pComInfo->isConnected)
    {
      M_INTL_SAL_COM_DEBUG(("Socket Already in Use"));
      status = E_K_COMM_STATUS_ERROR;
      break;
    }
    errno = 0;
    retVal = (uint32_t)strtol((const char *)xpPort, &pEnd, 10);
    if (retVal > 0 || retVal <= 0xFFFF)
    {
      port = (uint16_t)retVal;
    }
    if (0 != errno)
    {
      M_INTL_SAL_COM_ERROR(("Error Occurred %d", errno));
      break;
    }

    if (SOCKETS_INVALID_SOCKET == pComInfo->socketId)
    {
      /* Create socket. */
      pComInfo->socketId = SOCKETS_Socket(SOCKETS_AF_INET,
                                          SOCKETS_SOCK_STREAM,
                                          SOCKETS_IPPROTO_TCP);
      if (pComInfo->socketId == SOCKETS_INVALID_SOCKET)
      {
        M_INTL_SAL_COM_ERROR(("SOCKETS_Socket failed"));
        status = E_K_COMM_STATUS_RESOURCE;
        break;
      }
      /* Set send timeout for the socket. */
      (void)SOCKETS_SetSockOpt(pComInfo->socketId,
                               0,
                               SOCKETS_SO_SNDTIMEO,
                               &pComInfo->writeTimeOut,
                               sizeof(pComInfo->writeTimeOut));

      /* Set receive timeout for the socket. */
      (void)SOCKETS_SetSockOpt(pComInfo->socketId,
                               0,
                               SOCKETS_SO_RCVTIMEO,
                               &pComInfo->readTimeOut,
                               sizeof(pComInfo->readTimeOut));
    }
    socketAddress.ucLength = sizeof(SocketsSockaddr_t);
    socketAddress.ucSocketDomain = SOCKETS_AF_INET;
    socketAddress.ulAddress = SOCKETS_GetHostByName((const char *)xpHost);
    socketAddress.usPort = SOCKETS_htons(port);

    M_INTL_SAL_COM_DEBUG(("Connect To Add [%d %x %x]",
                          socketAddress.ucSocketDomain,
                          socketAddress.usPort,
                          (unsigned int)socketAddress.ulAddress));

    socStatus = SOCKETS_Connect(pComInfo->socketId,
                                &socketAddress,
                                (uint32_t)sizeof(socketAddress));
    if (socStatus != SOCKETS_ERROR_NONE)
    {
      M_INTL_SAL_COM_ERROR(("Connect to server failed"));
      status = E_K_COMM_STATUS_RESOURCE;
      (void)SOCKETS_Close(pComInfo->socketId);
      pComInfo->socketId = SOCKETS_INVALID_SOCKET;
      break;
    }
    pComInfo->isConnected = 1;
    status = E_K_COMM_STATUS_OK;
    break;
  }

  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));
  return status;
}

/**
 * @brief  implement salComWrite
 *
 */
K_SAL_API TKCommStatus salComWrite
(
  void*          xpComInfo,
  const uint8_t* xpBuffer,
  size_t         xBufferLen
)
{
  TKCommStatus  status = E_K_COMM_STATUS_ERROR;
  TKComInfo*    pComInfo = (TKComInfo *)xpComInfo;
  uint32_t      bytesSent = 0;
  int32_t       socStatus = SOCKETS_SOCKET_ERROR;

  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));

  for (;;)
  {
    if ((NULL == xpComInfo) || (NULL == xpBuffer) || (0U == xBufferLen))
    {
      M_INTL_SAL_COM_ERROR(("Invalid parameter"));
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }
    if (pComInfo->ptrId != 2u)
    {
      M_INTL_SAL_COM_ERROR(("Invalid parameter"));
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }
    if (0 == pComInfo->isConnected)
    {
      M_INTL_SAL_COM_DEBUG(("Socket Not Connected"));
      status = E_K_COMM_STATUS_ERROR;
      break;
    }
    while (bytesSent < xBufferLen)
    {
      /* Try sending the remaining data. */
      socStatus = SOCKETS_Send(pComInfo->socketId,
                               (const unsigned char *)&xpBuffer[bytesSent],
                               (size_t)(xBufferLen - bytesSent),
                               (uint32_t)0);

      /* A negative return value from SOCKETS_Send means some error occurred. */
      if (socStatus < 0)
      {
        break;
      }
      else
      {
        /* Update the count of sent bytes. */
        bytesSent += (uint32_t)socStatus;
      }
    }

    if (bytesSent == xBufferLen)
    {
      status = E_K_COMM_STATUS_OK;
    }
    else
    {
      M_INTL_SAL_COM_ERROR(("SOCKETS_Send failed"));
    }
    break;
  }

  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));
  return status;
}

/**
 * @brief  implement salComRead
 *
 */
K_SAL_API TKCommStatus salComRead
(
  void*     xpComInfo,
  uint8_t*  xpBuffer,
  size_t*   xpBufferLen
)
{
  TKComInfo*    pComInfo   = (TKComInfo *)xpComInfo;
  TKCommStatus  status  = E_K_COMM_STATUS_ERROR;
  int32_t       socStatus = SOCKETS_SOCKET_ERROR;
  uint16_t      retryCount;

  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));

  for (;;)
  {
    if ((NULL == xpComInfo) || (NULL == xpBuffer) || (NULL == xpBufferLen) || (0 == *xpBufferLen))
    {
      M_INTL_SAL_COM_ERROR(("Invalid parameter"));
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }
    if (pComInfo->ptrId != 2u)
    {
      M_INTL_SAL_COM_ERROR(("Invalid parameter"));
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }
    if (0 == pComInfo->isConnected)
    {
      M_INTL_SAL_COM_DEBUG(("Socket Not Connected"));
      status = E_K_COMM_STATUS_ERROR;
      break;
    }
    for (retryCount = 0; retryCount < (uint16_t)C_SAL_COM_MAX_READ_RETRY_COUNT; retryCount++)
    {
      socStatus = SOCKETS_Recv(pComInfo->socketId, (unsigned char *)xpBuffer,
                               (uint32_t)(*xpBufferLen), (uint32_t)0);

      /* Check if it is a Timeout. */
      if (socStatus != 0)
      {
        if (socStatus < 0)
        {
          M_INTL_SAL_COM_ERROR(("SOCKETS_Recv error, %ld", socStatus));
        }
        else
        {
          *xpBufferLen = socStatus;
          status = E_K_COMM_STATUS_OK;
        }

        /* If it is not a timeout, break. */
        break;
      }
      else
      {
        /* It is a timeout, retry. */
        M_INTL_SAL_COM_ERROR(("SOCKETS_Recv Timeout"));
      }
    }

    if (retryCount == (uint16_t)C_SAL_COM_MAX_READ_RETRY_COUNT)
    {
      M_INTL_SAL_COM_ERROR(("SOCKETS_Recv number of Timeout exceeded"));
    }

    break;
  }

  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));

  return status;
}

/**
 * @brief  implement salComTerm
 *
 */
K_SAL_API TKCommStatus salComTerm
(
  void*  xpComInfo
)
{
  TKCommStatus  status = E_K_COMM_STATUS_ERROR;
  TKComInfo*    pComInfo = (TKComInfo *)xpComInfo;

  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));

  for (;;)
  {
    if (NULL == xpComInfo)
    {
      M_INTL_SAL_COM_ERROR(("Invalid parameter"));
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }
    if (pComInfo->socketId != SOCKETS_INVALID_SOCKET)
    {
      (void)SOCKETS_Close(pComInfo->socketId);
      pComInfo->socketId = SOCKETS_INVALID_SOCKET;
      pComInfo->isConnected = 0;
    }
    pComInfo->ptrId = 0u;
    status = E_K_COMM_STATUS_OK;
    break;
  }

  M_INTL_SAL_COM_DEBUG(("End of %s", __func__));

  return status;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

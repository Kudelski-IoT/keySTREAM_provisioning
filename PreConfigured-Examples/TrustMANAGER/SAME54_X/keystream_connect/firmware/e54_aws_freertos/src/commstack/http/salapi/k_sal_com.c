/******************************************************************************
 * Copyright (c) 2023-2023 Nagravision Sàrl. All rights reserved.
 ******************************************************************************/
/** \brief  Interface for socket communication.
 *
 *  \author Griffin_Team
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
//#include "iot_secure_sockets.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

#define SOCKETS_ERROR_NONE               ( 0 )     /*!< No error. */

#define socketsconfigBYTE_ORDER              pdLITTLE_ENDIAN

typedef struct SocketsSockaddr
{
    uint8_t ucLength;       /**< Length of SocketsSockaddr structure. */
    uint8_t ucSocketDomain; /**< Only SOCKETS_AF_INET is supported. */
    uint16_t usPort;        /**< Port number. Convention is to call this sin_port. */
    uint32_t ulAddress;     /**< IP Address. Convention is to call this sin_addr. */
} SocketsSockaddr_t;

#if defined( socketsconfigBYTE_ORDER ) && ( socketsconfigBYTE_ORDER == pdLITTLE_ENDIAN )
    #define SOCKETS_htons( usIn )    ( ( uint16_t ) ( ( ( usIn ) << 8U ) | ( ( usIn ) >> 8U ) ) )
#else
    #define SOCKETS_htons( usIn )    ( ( uint16_t ) ( usIn ) )
#endif

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
} TKComInfo;

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */

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
  static TKComInfo  gComInfo = { 0 };
  TKCommStatus      status = E_K_COMM_STATUS_ERROR;
  TKComInfo*        pComInfo = &gComInfo;

  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));
  for (;;)
  {
    if (NULL == xppComInfo)
    {
      M_INTL_SAL_COM_ERROR(("Invalid parameter"));
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }

    pComInfo->socketId = FREERTOS_INVALID_SOCKET;
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
  int32_t             socStatus = FREERTOS_SOCKET_ERROR;
  struct              freertos_sockaddr socketAddress = { 0 };
  uint16_t            port = 0;
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

    errno = 0;
    port = (uint16_t)strtol((const char *)xpPort, &pEnd, 10);
    if (0 != errno)
    {
      M_INTL_SAL_COM_ERROR(("Error Occurred %d", errno));
      break;
    }

    if (FREERTOS_INVALID_SOCKET == pComInfo->socketId)
    {
      
      /* Create socket. */
      pComInfo->socketId = FreeRTOS_socket(FREERTOS_AF_INET,
                                          FREERTOS_SOCK_STREAM,
                                          FREERTOS_IPPROTO_TCP);
      if (pComInfo->socketId == FREERTOS_INVALID_SOCKET)
      {
        M_INTL_SAL_COM_ERROR(("SOCKETS_Socket failed"));
        status = E_K_COMM_STATUS_RESOURCE;
        break;
      }
      /* Set send timeout for the socket. */
      (void)FreeRTOS_setsockopt(pComInfo->socketId,
                               0,
                               FREERTOS_SO_SNDTIMEO,
                               &pComInfo->writeTimeOut,
                               sizeof(pComInfo->writeTimeOut));

      /* Set receive timeout for the socket. */
      (void)FreeRTOS_setsockopt(pComInfo->socketId,
                               0,
                               FREERTOS_SO_RCVTIMEO,
                               &pComInfo->readTimeOut,
                               sizeof(pComInfo->readTimeOut));
      pComInfo->isConnected = 0;
    }
    if (1U == pComInfo->isConnected)
    {
      M_INTL_SAL_COM_DEBUG(("Already Connected"));
      status = E_K_COMM_STATUS_OK;
      break;
    }
    socketAddress.sin_port = SOCKETS_htons(port);
    socketAddress.sin_addr = FreeRTOS_gethostbyname((const char *)xpHost);

    M_INTL_SAL_COM_DEBUG(("Connect To Add [%d %x %x]",
                          FREERTOS_AF_INET,
                          socketAddress.sin_port,
                          (unsigned int)socketAddress.sin_addr));

    socStatus = FreeRTOS_connect(pComInfo->socketId,
                                &socketAddress,
                                (uint32_t)sizeof(socketAddress));

    if (socStatus != SOCKETS_ERROR_NONE)
    {
      M_INTL_SAL_COM_ERROR(("Connect to server failed"));
      status = E_K_COMM_STATUS_RESOURCE;
      (void)FreeRTOS_closesocket(pComInfo->socketId);
      pComInfo->socketId = FREERTOS_INVALID_SOCKET;
      break;
    }

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
  int32_t       socStatus = FREERTOS_SOCKET_ERROR;

  M_INTL_SAL_COM_DEBUG(("Start of %s", __func__));

  for (;;)
  {
    if ((NULL == xpComInfo) || (NULL == xpBuffer) || (0U == xBufferLen))
    {
      M_INTL_SAL_COM_ERROR(("Invalid parameter"));
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }

    while (bytesSent < xBufferLen)
    {
      /* Try sending the remaining data. */
      socStatus = FreeRTOS_send(pComInfo->socketId,
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
      pComInfo->isConnected = 1;
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
  int32_t       socStatus = FREERTOS_SOCKET_ERROR;
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
    for (retryCount = 0; retryCount < (uint16_t)C_SAL_COM_MAX_READ_RETRY_COUNT; retryCount++)
    {
      socStatus = FreeRTOS_recv(pComInfo->socketId, (unsigned char *)xpBuffer,
                               (uint32_t)xpBufferLen, (uint32_t)0);

      /* Check if it is a Timeout. */
      if (socStatus != 0)
      {
        if (socStatus < 0)
        {
          M_INTL_SAL_COM_ERROR(("SOCKETS_Recv error, %ld", socStatus));
        }
        else
        {
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
  TKCommStatus  status = E_K_COMM_STATUS_OK;
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
    if (pComInfo->socketId != FREERTOS_INVALID_SOCKET)
    {
      (void)FreeRTOS_closesocket(pComInfo->socketId);
      pComInfo->socketId = FREERTOS_INVALID_SOCKET;
      pComInfo->isConnected = 0;
    }
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

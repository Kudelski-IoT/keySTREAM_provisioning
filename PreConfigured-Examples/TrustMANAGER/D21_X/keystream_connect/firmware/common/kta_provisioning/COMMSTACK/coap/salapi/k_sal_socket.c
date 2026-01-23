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
/** \brief SAL for socket commmunication, based on BSD socket API.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_socket.c
 ******************************************************************************/
/**
 * @brief SAL for socket commmunication, based on BSD socket API.
 */

#include "k_sal_socket.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_log_extended.h"
#include "k_sal_os.h"
#include "k_sal_log.h"
#include "cryptoauthlib.h"
#include "cloud_wifi_task.h"
#include "socket.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>


/* MISRA C:2012 Rule 8.5 deviation:
 * Object/function definition in this header is required for compatibility with legacy code
 * and/or to support platform-specific initialization. This deviation is documented and justified
 * because refactoring would break existing integration or platform abstraction layers.
 */
extern int32_t kta_gRxBufferLength;
extern enum wifi_status kta_gSockRecvStatus;
extern uint8_t* kta_getHostByName(const char* xpUrl);

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Maximal number of sockets used in parallel. */
#define C_SAL_SOCKET_MAX_INSTANCES (4u)

/** @brief Length of the IP address in string format. */
#define C_SAL_SOCKET_ADDR_STRING_SIZE (32u)

/** @brief POSIX error return value. */
#define C_SAL_SOCKET_ERROR_RET (-1)

/** @brief To enable or disable log for this module. */
#define C_SAL_SOCKET_ENABLE_LOG 0

/** @brief Module name to display. */
#define C_SAL_SOCKET_MODULE_NAME "SOCKET"

/** @brief Command used to discover the interfaces with UP state. */
#define C_SAL_SOCKET_READ_NETWORK_INFO_CMD "ip addr ls up"

/** @brief Max adaptaters available. */
#define C_SAL_SOCKET_MAX_NB_ADAPTER (10u)

/** @brief Socket info. */
struct KTA_SalSocket
{
  bool            isUsed;
  /* True if the socket is in use. */
  bool            isCreated;
  /* True if the socket is created. */
  int             socketId;
  /* ID of unique socket used. */
  TKSalSocketType type;
  /* Socket type. */
};

/** @brief SAL socket log macro. */
#define M_SAL_SOCKET_LOG(...)

/** @brief SAL socket log with variable macro. */
#define M_SAL_SOCKET_LOG_VAR(...)

/** @brief SAL socket log with buffer macro. */
#define M_SAL_SOCKET_LOG_BUFF(...)

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */

/** @brief Socket class variable. */
static TKSalSocket gaSalSocketTable[C_SAL_SOCKET_MAX_INSTANCES] = { 0 };

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   Check if socket instance belongs to our sockets table and is used.
 *
 * @param[in] xpThis
 *   Pointer to the socket to check.
 *
 * @return
 * - true if valid.
 * - false if invalid.
*/
static bool salSocketIsValidInstance
(
  const TKSalSocket*  xpThis
);

/**
 * @brief
 *   Build the socket address.
 *
 * @param[in] xpIp
 *   IP address info;
 *   Should not be NULL.
 * @param[in,out] xpAddress
 *   [in] Pointer to sockaddr_in structre.
 *   [out] sockaddr_in structre filled with necessary info.
 *
 * @return
 * - E_K_COMM_STATUS_OK in case of success.
 * - E_K_COMM_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_COMM_STATUS_DATA for unexpected data.
 * - E_K_COMM_STATUS_ERROR for other errors.
 */
static TKCommStatus salSocketBuildAddr
(
  const TKSocketIp*    xpIp,
  struct sockaddr_in*  xpAddress
);

/**
 * @brief
 *   Create the socket if not yet done.
 *
 * @param[in,out] xpThis
 *   [in] Pointer to socket info instance.
 *   [out] Actual socket info instance.
 *   Should not be NULL.
 * @param[in] xProtocol
 *   Protocol to use.
 *
 * @return
 * - E_K_COMM_STATUS_OK in case of success.
 * - E_K_COMM_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_COMM_STATUS_DATA for unexpected data.
 * - E_K_COMM_STATUS_RESOURCE for resouce related error.
 * - E_K_COMM_STATUS_ERROR for other errors.
 */
static TKCommStatus salSocketCreateIfNeeded
(
  TKSalSocket*        xpThis,
  const TKIpProtocol  xProtocol
);

/**
 * @brief
 *   Return network information.
 *   This function reads the output of shell command "ip addr is up" to build
 *   a list of IP adapter connected (string="UP") with their
 *   respective MTUs. If at least one adapter is connected, this function
 *   returns *xpBConnected = true and the smallest mtu size found
 *   (string="mtu").
 *   Otherwise, it returns *xpBConnected = false and *x_pusMtu = 0.
 *
 * @param[in,out] xpBConnected
 *   [in] Pointer to info for status of adpater(connected or not).
 *   [out] Actual info for status of adpater(connected or not).
 *   Should not be NULL.
 * @param[in,out] x_pusMtu
 *   [in] Pointer to info for MTU.
 *   [out] Actual info for MTU.
 *   Should not be NULL. *
 * @return
 * - E_K_COMM_STATUS_OK or an error status.
 */
static TKCommStatus salSocketGetNetworkInfo
(
  bool*      xpBConnected,
  uint32_t*  xpU32Mtu
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief  implement salSocketCreate
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_002 : misra_c2012_rule_17.7_violation
 * Not using the return value of functions
 */
TKCommStatus salSocketCreate
(
  const TKSalSocketType  xType,
  TKSalSocket**          xppSocket
)
{
  TKSalSocket*  pThis = NULL;
  TKCommStatus  status = E_K_COMM_STATUS_ERROR;

  M_SAL_SOCKET_LOG_VAR("Start of %s", __func__);

    for (;;)
    {
      if (NULL == xppSocket)
      {
        M_SAL_SOCKET_LOG("ERROR : Created socket is NULL");
        status = E_K_COMM_STATUS_PARAMETER;
        break;
      }

      if ((E_SAL_SOCKET_TYPE_UDP != xType) && (E_SAL_SOCKET_TYPE_TCP != xType))
      {
        M_SAL_SOCKET_LOG_VAR("ERROR: Invalid type %d.", xType);
        status = E_K_COMM_STATUS_DATA;
        break;
      }

      /* Look for a free instance */
      for (size_t i = 0; i < C_SAL_SOCKET_MAX_INSTANCES; i++)
      {
        if (false == gaSalSocketTable[i].isUsed)
        {
          pThis = gaSalSocketTable + i;
          break;
        }
      }

      if (NULL == pThis)
      {
        M_SAL_SOCKET_LOG_VAR("ERROR: No available instance (max %d).", C_SAL_SOCKET_MAX_INSTANCES);
        status = E_K_COMM_STATUS_MISSING;
        break;
      }

      (void)memset(pThis, 0, sizeof(TKSalSocket));
      pThis->isCreated = false;
      pThis->type = xType;
      pThis->isUsed = true;
      pThis->socketId = C_SAL_SOCKET_ERROR_RET;
      *xppSocket = pThis;
      status = E_K_COMM_STATUS_OK;
      break;
    }

  M_SAL_SOCKET_LOG_VAR("End of %s", __func__);

  return status;
}

/**
 * @brief  implement salSocketSendTo
 *
 */
TKCommStatus salSocketSendTo
(
  TKSalSocket*          xpSocket,
  const unsigned char*  xpBuffer,
  const size_t          xBufferLength,
  const TKSocketIp*     xpAddress
)
{
  struct sockaddr_in address;
  TKCommStatus       status = E_K_COMM_STATUS_ERROR;
  uint32_t           mtuValue = 0;
  bool               bConnected = false;

  M_SAL_SOCKET_LOG_VAR("Start of %s", __func__);
  (void)memset(&address, 0, sizeof(address));

  for (;;)
  {
    if (
      (NULL == xpSocket)        ||
      (NULL == xpBuffer)      ||
      (0U    == xBufferLength) ||
      (NULL == xpAddress)
    )
    {
      M_SAL_SOCKET_LOG("ERROR: Socket or Buffer or BufferLength or Socket IP are invalid");
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }

    if (
      (xpAddress->protocol != E_K_IP_PROTOCOL_V4) &&
      (xpAddress->protocol != E_K_IP_PROTOCOL_V6)
    )
    {
      M_SAL_SOCKET_LOG("ERROR: Invalid Socket type, should be ipv4 or ipv6");
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }

    if (true != salSocketIsValidInstance(xpSocket))
    {
      M_SAL_SOCKET_LOG("ERROR: Invalid socket instance");
      status = E_K_COMM_STATUS_DATA;
      break;
    }

    status = salSocketCreateIfNeeded(xpSocket, xpAddress->protocol);

    if (E_K_COMM_STATUS_OK != status)
    {
      M_SAL_SOCKET_LOG("ERROR: Socket creation has failed");
      break;
    }

    if (E_K_COMM_STATUS_OK != salSocketBuildAddr(xpAddress, &address))
    {
      M_SAL_SOCKET_LOG_VAR("ERROR: Bad IP protocol %d.", xpAddress->protocol);
      break;
    }

#ifndef ENABLE_PC_SOCKETS
    /* Check IP adapter info. */
    status = salSocketGetNetworkInfo(&bConnected, &mtuValue);

    if (E_K_COMM_STATUS_OK != status)
    {
      M_SAL_SOCKET_LOG("ERROR: Retrieving network information has failed");
      break;
    }

    if (false == bConnected)
    {
      M_SAL_SOCKET_LOG("ERROR: No IP connection.");
      status = E_K_COMM_STATUS_NETWORK;
      break;
    }

#endif /* ENABLE_PC_SOCKETS */
    /* Write data to socket. */
    ssize_t size = sendto(xpSocket->socketId,
                  (void*)xpBuffer,
                  xBufferLength,
                  0 /* flags */,
                  (struct sockaddr*)&address,
                  sizeof(address));

    if ((C_SAL_SOCKET_ERROR_RET == size) || (size != xBufferLength))
    {
      /**
       * With some architectures or kernel, the socket might return an 101 error
       * We don't really care about this error if UDP is used.
       */
      if (E_SAL_SOCKET_TYPE_UDP == xpSocket->type)
      {
        status = E_K_COMM_STATUS_OK;
        break;
      }

      M_SAL_SOCKET_LOG_VAR("ERROR: Network is unreachable, errno=%d.", errno);
      status = E_K_COMM_STATUS_ERROR;
      break;
    }

    status = E_K_COMM_STATUS_OK;
    break;
  }

  M_SAL_SOCKET_LOG_VAR("End of %s", __func__);

  return status;
}

/**
 * @brief  implement salSocketReceiveFrom
 *
 */
TKCommStatus salSocketReceiveFrom
(
  TKSalSocket*    xpSocket,
  unsigned char*  xpBuffer,
  size_t*         xpBufferLength,
  TKSocketIp*     xpAddress
)
{
  ssize_t size = 0;
  TKCommStatus status = E_K_COMM_STATUS_ERROR;
  uint32_t mtuValue = 0;
  bool bConnected = false;

  M_SAL_SOCKET_LOG_VAR("Start of %s", __func__);

  for (;;)
  {
    if (
      (NULL == xpSocket)       ||
      (NULL == xpBuffer)       ||
      (NULL == xpBufferLength)
    )
    {
      M_SAL_SOCKET_LOG("ERROR: Socket or Buffer or BufferLength are invalid");
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }

    if (true != salSocketIsValidInstance(xpSocket))
    {
      M_SAL_SOCKET_LOG("ERROR: Invalid socket instance");
      status = E_K_COMM_STATUS_DATA;
      break;
    }

    if (0 == *xpBufferLength)
    {
      M_SAL_SOCKET_LOG("ERROR: BufferLength is equal to 0");
      status = E_K_COMM_STATUS_DATA;
      break;
    }

    /* WARNING: if socket never created, will be created as V4 socket */
    status = salSocketCreateIfNeeded(xpSocket, E_K_IP_PROTOCOL_V4);

    if (E_K_COMM_STATUS_OK != status)
    {
      M_SAL_SOCKET_LOG("ERROR: Socket has not been vreated properly");
      status = E_K_COMM_STATUS_DATA;
      break;
    }

#ifndef ENABLE_PC_SOCKETS
    /* Check IP adapter info. */
    status = salSocketGetNetworkInfo(&bConnected, &mtuValue);

    if (E_K_COMM_STATUS_OK != status)
    {
      break;
    }

    if (false == bConnected)
    {
      M_SAL_SOCKET_LOG("ERROR: No IP connection.");
      status = E_K_COMM_STATUS_NETWORK;
      break;
    }

#endif /* ENABLE_PC_SOCKETS */
    kta_gSockRecvStatus = WIFI_STATUS_UNKNOWN;
    kta_gRxBufferLength = 0;
    /* Read from socket. */
    M_SAL_SOCKET_LOG_VAR("Receiving data, size=%d.", *xpBufferLength);
    errno = 0;
    size = recvfrom(xpSocket->socketId,
                    (void*)xpBuffer,
                    *xpBufferLength,
                    15000);

    /* For WINC1500, recvfrom is async - wait for callback */
    while (kta_gSockRecvStatus != WIFI_STATUS_MESSAGE_RECEIVED)
    {
      if ((kta_gSockRecvStatus == WIFI_STATUS_TIMEOUT) ||
          (kta_gSockRecvStatus == WIFI_STATUS_ERROR))
      {
        M_SAL_SOCKET_LOG("Invalid status %d - marking socket error", kta_gSockRecvStatus);
        status = E_K_COMM_STATUS_ERROR;
        break;
      }

      atca_delay_ms(1);  /* Reduced from 5ms for faster polling */
      m2m_wifi_handle_events();
    }

    /* Check if we successfully received data */
    if (kta_gSockRecvStatus == WIFI_STATUS_MESSAGE_RECEIVED)
    {
      *xpBufferLength = kta_gRxBufferLength;
      M_SAL_SOCKET_LOG_VAR("Data received, length=%d.", (int)*xpBufferLength);
      M_SAL_SOCKET_LOG_BUFF("xpBuffer", xpBuffer, *xpBufferLength);
      status = E_K_COMM_STATUS_OK;
    }
    else
    {
      M_SAL_SOCKET_LOG("ERROR: Failed to receive data.");
      *xpBufferLength = 0;
      status = E_K_COMM_STATUS_ERROR;
    }
    break;
  }

  M_SAL_SOCKET_LOG_VAR("End of %s", __func__);

  /* Explicitly cast to void if unused to suppress warning */
  (void)size;

  return status;
}

/**
 * @brief  implement salSocketDispose
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_002 : misra_c2012_rule_17.7_violation
 * Not using the return value of snprintf
 */
void salSocketDispose
(
  TKSalSocket*  xpSocket
)
{
  int retVal = SOCK_ERR_NO_ERROR;
  M_SAL_SOCKET_LOG_VAR("Start of %s", __func__);

  if (salSocketIsValidInstance(xpSocket) && (true == xpSocket->isUsed))
  {
    retVal = shutdown(xpSocket->socketId);
    if (retVal != SOCK_ERR_NO_ERROR)
    {
      M_SAL_SOCKET_LOG_VAR("ERROR: shutdown failed with code %d - continuing cleanup", retVal);
    }
    
    /* Give WiFi module time to actually close the socket */
    atca_delay_ms(20);  /* Reduced from 100ms for faster cleanup */
    
    /* Always cleanup socket structure regardless of shutdown result */
    xpSocket->socketId = C_SAL_SOCKET_ERROR_RET;
    xpSocket->isCreated = false;
    (void)memset(xpSocket, 0, sizeof(TKSalSocket));
    /* Already done by previous memset, just for readability */
    xpSocket->isUsed = false;
  }

  M_SAL_SOCKET_LOG_VAR("End of %s", __func__);
  /* Single exit point at end of function */

  /* Explicitly cast to void to suppress unused variable warning */
  (void)retVal;
}

/**
 * @brief  implement salSocketGetNetworkMtu
 *
 */
TKCommStatus salSocketGetNetworkMtu
(
  size_t*  xpValue
)
{
  uint32_t      mtuValue = 0;
  TKCommStatus  status = E_K_COMM_STATUS_ERROR;
  bool          bConnected = false;

  M_SAL_SOCKET_LOG_VAR("Start of %s", __func__);

  for (;;)
  {
    if (NULL == xpValue)
    {
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }

    /* Check IP adapter info. */
    status = salSocketGetNetworkInfo(&bConnected, &mtuValue);

    if (E_K_COMM_STATUS_OK != status)
    {
      status = E_K_COMM_STATUS_ERROR;
      break;
    }

    if (false == bConnected)
    {
      M_SAL_SOCKET_LOG("ERROR interface is not connected.");
      status = E_K_COMM_STATUS_MISSING;
      break;
    }

    *xpValue = mtuValue;

    break;
  }

  M_SAL_SOCKET_LOG_VAR("End of %s", __func__);

  return status;
}

/**
 * @brief  implement salGetHostByName
 *
 */
 /**
 * SUPPRESS: MISRA_DEV_KTA_002 : misra_c2012_rule_17.7_violation
 * Not using the return value of snprintf
 */
K_SAL_API TKCommStatus salGetHostByName
(
    const char* xpHost,
    uint8_t* xpIpAddress
)
{
  TKCommStatus status = E_K_COMM_STATUS_OK;

  if ((NULL == xpHost) || (NULL == xpIpAddress))
  {
    M_SAL_SOCKET_LOG("ERROR: Invalid Parameter");
    status = E_K_COMM_STATUS_PARAMETER;
  } 
  else
  {
    M_SAL_SOCKET_LOG_VAR("Start of %s", __func__);
    (void)memcpy(xpIpAddress, kta_getHostByName(xpHost), C_SAL__MAX_IP4_ADDRESS_LENGTH);
    M_SAL_SOCKET_LOG(("xpIpAddress[%s]", xpIpAddress));
    M_SAL_SOCKET_LOG_VAR("End of %s", __func__);
  }

  return status;
}
/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */
/**
 * @implements salSocketIsValidInstance
 *
 */
static bool salSocketIsValidInstance
(
  const TKSalSocket*  xpThis
)
{
  M_SAL_SOCKET_LOG_VAR("Start of %s", __func__);

  size_t index = 0;
  bool   isValid = false;

  /* Iterate through our socket class variable. */
  for (index = 0; index < C_SAL_SOCKET_MAX_INSTANCES; index++)
  {
    /* The socket belongs to the table. */
    if ((xpThis) == (gaSalSocketTable + index))
    {
      /* Test if the instance is used. */
      if (true == xpThis->isUsed)
      {
        /* The socket instance is valid. */
        isValid = true;
      }

      break;
    }
  }

  M_SAL_SOCKET_LOG_VAR("End of %s", __func__);

  return isValid;
}

/**
 * @implements salSocketBuildAddr
 *
 */
static TKCommStatus salSocketBuildAddr
(
  const TKSocketIp*    xpIp,
  struct sockaddr_in*  xpAddress
)
{
  char          aAddrIPv4Char[C_SAL_SOCKET_ADDR_STRING_SIZE] = { 0 };
  TKCommStatus  status = E_K_COMM_STATUS_ERROR;
  uint32_t      addrIPv4 = 0u;

  M_SAL_SOCKET_LOG_VAR("Start of %s", __func__);

  for (;;)
  {
    if ((NULL == xpIp) || (NULL == xpAddress))
    {
      M_SAL_SOCKET_LOG("ERROR : Invalid Ip/socket address");
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }

    switch (xpIp->protocol)
    {
      case E_K_IP_PROTOCOL_V4:
        addrIPv4 = xpIp->address.v4.address;

        /* Build the string IP address. */
        /**
         * SUPPRESS: MISRA_DEV_KTA_002 : misra_c2012_rule_17.7_violation
         * Not using the return value of snprintf
         */
        (void)snprintf
        (
          aAddrIPv4Char,
          C_SAL_SOCKET_ADDR_STRING_SIZE,
          "%u.%u.%u.%u",
          (unsigned int)((addrIPv4 & 0xFF000000u) >> 24u),
          (unsigned int)((addrIPv4 & 0x00FF0000u) >> 16u),
          (unsigned int)((addrIPv4 & 0x0000FF00u) >> 8u),
          (unsigned int)(addrIPv4 & 0x000000FFu)
        );
        xpAddress->sin_family = AF_INET;
        xpAddress->sin_port  = _htons(xpIp->address.v4.port);
        xpAddress->sin_addr.s_addr = _htonl(addrIPv4);

        status = E_K_COMM_STATUS_OK;
        break;

      case E_K_IP_PROTOCOL_V6:
        /* Not supported. */
        status = E_K_COMM_STATUS_DATA;
        break;

      default:
        status = E_K_COMM_STATUS_DATA;
        break;
    }

    break;
  }

  M_SAL_SOCKET_LOG_VAR("End of %s", __func__);

  return status;
}

/**
 * @implements salSocketCreateIfNeeded
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_002 : misra_c2012_rule_17.7_violation
 * Not using the return value of functions
 */
static TKCommStatus salSocketCreateIfNeeded
(
  TKSalSocket*        xpThis,
  const TKIpProtocol  xProtocol
)
{
  struct sockaddr_in  address;
  TKCommStatus        status = E_K_COMM_STATUS_ERROR;
  uint8_t             type = 0;

  M_SAL_SOCKET_LOG_VAR("Start of %s", __func__);
  (void)memset(&address, 0, sizeof(address));

  for (;;)
  {
    if (true != salSocketIsValidInstance(xpThis))
    {
      M_SAL_SOCKET_LOG("ERROR : Invalid socket instance");
      status = E_K_COMM_STATUS_DATA;
      break;
    }

    if (true == xpThis->isCreated)
    {
      /* Socket already created - verify it's still valid */
      status = E_K_COMM_STATUS_OK;
      break;
    }

    /* If socket ID exists but not marked as created, clean it up */
    if (xpThis->socketId != C_SAL_SOCKET_ERROR_RET && xpThis->socketId > 0)
    {
      M_SAL_SOCKET_LOG("Cleaning up stale socket before creating new one");
      (void)shutdown(xpThis->socketId);
      xpThis->socketId = C_SAL_SOCKET_ERROR_RET;
    }

    switch (xpThis->type)
    {
      case E_SAL_SOCKET_TYPE_UDP:
        type = SOCK_DGRAM;
        break;

      case E_SAL_SOCKET_TYPE_TCP:
        type = SOCK_STREAM;
        break;

      default:
        M_SAL_SOCKET_LOG("ERROR : Invalid socket type, should be UDP or TCP");
        status = E_K_COMM_STATUS_PARAMETER;
        break;
    }

    if ((E_K_COMM_STATUS_DATA == status) || (E_K_COMM_STATUS_PARAMETER == status))
    {
      break;
    }

    switch (xProtocol)
    {
      case E_K_IP_PROTOCOL_V4:
        address.sin_family = AF_INET;
        break;

      case E_K_IP_PROTOCOL_V6:
        /* address.sin_family = AF_INET6. */
        break;

      default:
        M_SAL_SOCKET_LOG("ERROR : Invalid protocol, should be ipv4 or ipv6");
        status = E_K_COMM_STATUS_PARAMETER;
        break;
    }

    if (E_K_COMM_STATUS_PARAMETER == status)
    {
      break;
    }

    /* Create socket. */
    xpThis->socketId = socket(address.sin_family, type, 0);

    if (C_SAL_SOCKET_ERROR_RET == xpThis->socketId)
    {
      M_SAL_SOCKET_LOG("ERROR : Iinvalid socket id");
      status = E_K_COMM_STATUS_RESOURCE;
      break;
    }

    xpThis->isCreated = true;
    status = E_K_COMM_STATUS_OK;
    break;
  }

  M_SAL_SOCKET_LOG_VAR("End of %s", __func__);

  return status;
}

/**
 * @implements salSocketGetNetworkInfo
 *
 */
static TKCommStatus salSocketGetNetworkInfo
(
  bool*      xpBConnected,
  uint32_t*  xpU32Mtu
)
{
  TKCommStatus eStatus = E_K_COMM_STATUS_OK;

  M_SAL_SOCKET_LOG_VAR("Start of %s", __func__);
  /* Initialize param to return. */
  *xpBConnected = false;
  *xpU32Mtu = 0;

  /* Build the list of adapter connected with their MTU values. */
  for (;;) /* Dummy loop */
  {
    /* Should get the mtu value from the device. */
    *xpU32Mtu = 2048u;
    *xpBConnected = true;
    /* End of dummy loop */
    break;
  }

  M_SAL_SOCKET_LOG_VAR("End of %s", __func__);

  return eStatus;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

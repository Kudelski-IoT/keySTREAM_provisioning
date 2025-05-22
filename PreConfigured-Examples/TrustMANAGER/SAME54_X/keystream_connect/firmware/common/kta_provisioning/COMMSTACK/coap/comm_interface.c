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
/** \brief Communication interface.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file comm_interface.c
 ******************************************************************************/
/**
 * @brief Communication interface.
 */

#include "comm_interface.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "comm_interface_util.h"
/* mbed coap headers. */
#include "sn_coap_header.h"
#include "sn_coap_protocol.h"
/* Next one is needed for outgoing block-wise - prepare_blockwise_message(). */
#include "sn_coap_protocol_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Coap max receive buffer size. */
#define C_COMM_INTERFACE_COAP_MAX_RECEIVE_BUFFER_SIZE        (1472u)

/** @brief Coap resending counter. */
#define C_COMM_INTERFACE_COAP_MAX_RESENDING_COUNT            (3u)

/** @brief Coap resending interval in seconds. */
#define C_COMM_INTERFACE_COAP_RESENDING_INTERVAL_IN_SECS     (2u)

/** @brief Wait for next CoAP response from server in ms. */
#define C_COMM_INTERFACE_COAP_WAIT_FOR_RESPONSE              (200u)

/** @brief Max retransmission if no response from server. */
#define C_COMM_INTERFACE_COAP_MAX_RESENDING_RETRIES          (20u)

/** @brief Max IP4 address length. */
#define C_COMM_INTERFACE_MAX_IP_ADDRESS_LENGTH              (16u)

/** @brief Macro for memory allocation */
#define M_COMM_INTERFACE_MALLOC(x_size)     pCommCoapMalloc(x_size)

/** @brief Macro to free allocated memory */
#define M_COMM_INTERFACE_FREE(x_ptr)        commCoapFree(x_ptr)

/** @brief Set an argument/return value as unused */
#define M_UNUSED(xArg)            (void)(xArg)

/** @brief Communicaion interface object. */
typedef struct
{
  TBoolean          isInitialized;
  /* True, if the interface is initialized successfully. */
  TBoolean          isExchangeTerminated;
  /* True, if the message transfer completed or error occured. */
  TKCommStatus          exchangeStatus;
  /* Message exchange status. */
  struct coap_s*    pCoapHandle;
  /* Handle of the CoAP. */
  uint8_t*          pServerIp;
  /* Destination server IP. */
  uint16_t          serverPort;
  /* Destination server port. */
  TKSalSocket*       pSocket;
  /* UDP Socket used for Coap. */
  TKSocketIp        socketIP;
  /* Destination server IP in SAL socket format. */
  sn_nsdl_addr_s    dstAddress;
  /* Destination server address. */
  uint8_t*           pFirstPayload;
  /* Payload of the first message received. */
  uint16_t          firstPayloadLength;
  /* Payload length first message in bytes. */
  uint8_t*           pPayload;
  /* Payload of the remaining messages received. */
  uint16_t          payloadLength;
  /* Payload length remaining messages in bytes. */
  uint16_t          coapBlockSize;
  /* Coap Message block size. */
  uint32_t          maxRetries;
  /** Max retries for retransmission.
   * Should not exceed C_COMM_INTERFACE_COAP_MAX_RESENDING_RETRIES.
   */
  size_t            mtuSize;
  /* MTU size in bytes. */
  uint8_t*          pResponseBuffer;
  /* Response buffer. */
  uint8_t*          pCoapUri;
  /* Coap Server Uri. */
  uint16_t          coapUriLength;
  /* Coap Server Uri length. */
  uint16_t          lastRecivedMessageId;
  /* Response buffer to receive the data from the socket. it should be mtu length. */
} TCommInterface;

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/** @brief TCommInterface structure object */
static TCommInterface gCommInterfaceObj;

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
#ifdef ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS
/**
 * @brief
 *   Prints the buffer in hex format.
 *
 * @param[in] xpBuffer
 *   Buffer to print.
 * @param[in] xBufferLength
 *   Length of data to print.
 */
static void debugPrintBufferInHex
(
  const uint8_t*  xpBuffer,
  const size_t  xBufferLength
);

/**
 * @brief
 *   Prints the CAOP header.
 *
 * @param[in] xpCoapHeader
 *   COAP header structure to print.
 */
static void debugPrintCoapHeader
(
  const sn_coap_hdr_s* xpCoapHeader
);

/**
 * @brief
 *   Prints the Payload in hex format.
 *
 * @param[in] xpMessageReceived
 *   Buffer to print.
 * @param[in] xReceivedMsgSize
 *   Length of data to print.
 */
static void debugPrintPayload
(
  const uint8_t*  xpMessageReceived,
  const size_t    xReceivedMsgSize
);
#endif /* ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS */

/**
 * @brief
 *   Allocate a block of memory.
 *
 * @param[in] xSize
 *   Size in bytes to allocate;
 *   Should not be 0.
 *
 * @return
 *   Pointer to the allocated memory block if successful, NULL otherwise.
 */
static void* pCommCoapMalloc
(
  uint16_t  xSize
);

/**
 * @brief
 *   Free the previously allocated memory block.
 *
 * @param[in] xpAddr
 *   Pointer to the memory block to be freed.
 */
static void commCoapFree
(
  void*  xpAddr
);

/**
 * @brief
 *   Tx function for sending coap messages.
 *
 * @param[in] xpSendBuffer
 *   Date to send.
 * @param[in] xSendBufferSize
 *   Length of data to send.
 * @param[in] xpDstAddress
 *   UNUSED.
 * @param[in] xpUserData
 *   UNUSED.
 *
 * @return
 * - E_K_COMM_STATUS_OK in case of success.
 * - E_K_COMM_STATUS_RESOURCE if the IP address is invalid.
 * - E_K_COMM_STATUS_NETWORK if the network connectivity is not available.
 */
static uint8_t commCoapTxCb
(
  uint8_t*         xpSendBuffer,
  uint16_t         xSendBufferSize,
  sn_nsdl_addr_s*  xpDstAddress,
  void*            xpUserData
);

/**
 * @brief
 *   Dummy Rx function for receiving coap messages.
 *
 * @param[in] xpCoapHeader
 *   UNUSED.
 * @param[in] xpDstAddress
 *   UNUSED.
 * @param[in] xpUserData
 *   UNUSED.
 */
static int8_t commCoapRxCb
(
  sn_coap_hdr_s*   xpCoapHeader,
  sn_nsdl_addr_s*  xpDstAddress,
  void*            xpUserData
);

/**
 * @brief
 *   Terminate the coap protocol and release the memory.
 */
static void terminateCoapProtocol
(
  void
);

/**
 * @brief
 *  Prepare coap message with required params.
 *
 * @param[in]  xpPayload
 *   Payload to send.
 *   Can be null, if no data to send.
 * @param[in]  xPayloadLen
 *   Size of the payload.
 *   Can be 0, if no data to send.
 *
 * @return sn_coap_hdr_s
 *   Structure containing CoAP header info.
 */
static sn_coap_hdr_s* pGetDefaultCoapHeader
(
  const uint16_t  xPayloadLen,
  const uint8_t*  xpPayload
);

/**
 * @brief
 *  Get the MTU size in bytes.
 *
 * @return
 *  Size in bytes.
 */
static size_t getMtuSize
(
  void
);

/**
 * @brief
 *   Get the CoAP block size based on MTU.
 *
 * @param[in] xMtuValue
 *   MTU value.
 *
 * @return
 *  CoAP block size in bytes.
 */
static uint16_t getCoapBlockSizeUsingMtu
(
  const size_t  xMtuValue
);

/**
 * @brief
 *  Get the coap retransmission count.
 *
 * @return
 *  Resending count.
 */
static uint8_t getCoapResendingCount
(
  void
);

/**
 * @brief
 *  Get the coap retransmission interval.
 *
 * @return
 *  Interval in sec.
 */
static uint8_t getCoapResendingInterval
(
  void
);

/**
 * @brief
 *  Get system relative time in sec.
 *
 * @return
 *  Time in sec.
 */
static uint32_t getRelativeTimeInSec
(
  void
);

/**
 * @brief
 *   Build and send the coap message to the keySTREAM.
 *
 * @param[in]  xpMessageToSend
 *   Payload to send, may be NULL if no data to send.
 * @param[in]  xSendSize
 *   Size of the payload, may be 0 if no data to send.
 *
 * @return
 * - E_K_COMM_STATUS_OK in case of success.
 * - E_K_COMM_STATUS_MEMORY failure on memory allocation.
 * - E_K_COMM_STATUS_DATA if internal failure in coap.
 * - E_K_COMM_STATUS_RESOURCE if the IP address is invalid.
 * - E_K_COMM_STATUS_NETWORK if the network connectivity is not available.
 */
static TKCommStatus commCoapBuildAndSendMessage
(
  const uint8_t*  xpMessageToSend,
  const size_t    xSendSize
);

/**
 * @brief
 *   Build and send the coap message with block2 option and next block of the previous message. it
 *   mustn't have the payload.
 *
 * @param[in] xBlock2
 *   Block number received from the previous request.
 *
 * @return
 * - E_K_COMM_STATUS_OK in case of success.
 * - E_K_COMM_STATUS_MEMORY failure on memory allocation.
 * - E_K_COMM_STATUS_DATA if internal failure in coap.
 * - E_K_COMM_STATUS_RESOURCE if the IP address is invalid.
 * - E_K_COMM_STATUS_NETWORK if the network connectivity is not available.
 */
static TKCommStatus commCoapPrepareAndSendBlock2Message
(
  const int32_t  xBlock2
);

/**
 * @brief
 *  Convert the internal error code to communication error code.
 *
 * @param[in]  status
 *   Internal status.
 */
static TCommIfStatus commConvertError
(
  TKCommStatus  xStatus
);

/**
 * @brief
 *  Check and retransmit the CoAP packet and wait for milliseconds.
 *
 * @param[in]  xpCoapHandle
 *   Coap handle.
 */
static void commCoapWaitForData
(
  struct coap_s*  xpCoapHandle
);

/**
 * @brief
 *  Retry the block2 packet sending until max retry reached.
 *
 * @param[in]  xBlock2
 *  Block number received from the previous request.
 */
static void commCoapRetrySendBlock2
(
  const int32_t  xBlock2
);

/**
 * @brief
 *  Get the response from the received CoAP packet. Blockwise response handling done in CoAP.
 *
 * @param[in] xpResponseBuffer
 *   Response buffer received from the server.
 * @param[in] xResponseBufferLength
 *   Length of the response buffer.
 * @param[in] xpCoapHandle
 *   coap handle.
 * @param[in,out] IsPayloadFreeRequired
 *   [in]Pointer to hold status.
 *   [out]Set to ture if payload free is required otherwise false.
 */
static void commCoapGetResponse
(
  uint8_t*        xpResponseBuffer,
  size_t          xResponseBufferLength,
  struct coap_s*  xpCoapHandle,
  TBoolean*       xpIsPayloadFreeRequired
);

/**
 * @brief
 *  Copy the received payload to the destination buffer.
 *
 * @param[in,out] xpDstMsgBuffer
 *   [in] Pointer to destination message buffer to copy payload in.
 *   [out] Actual destination message buffer after payload is copied.
 * @param[in] xResponseBufferLength
 *   Length of the destination message buffer.
 * @param[in] xIsPayloadFreeRequired
 *   Set to ture if payload free is required otherwise false.
 *
 * @return
 *   Size of the payload in bytes.
 */
static uint32_t copyPayloadToMessageBuffer
(
  uint8_t*        xpDstMsgBuffer,
  size_t          xDstMsgBufferLength,
  const TBoolean  xIsPayloadFreeRequired
);

/**
 * @brief
 *    Get the IP address from host name.
 *
 * @param[in] Host
 *    Host name to convert in IPV4 address.
 * @param[in,out] xIpAddress
 *   [in] Pointer to hold IP Address.
 *   [out] Actual IP address, string encoded.
 *
 * @return
 * - E_K_COMM_STATUS_OK in case of success.
 * - E_K_COMM_STATUS_ERROR for other errors.
 */
static TKCommStatus lgetIPAddress
(
  const uint8_t*  xpHost,
  uint8_t*        xpIpAddress
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief  implement commInitProtocol
 *
 */
TCommIfStatus commInitProtocol
(
  const TCommIfIpProtocol  xIpProtocol,
  const uint8_t*           xpUri,
  const uint8_t*           xpHost,
  const uint16_t           xPort
)
{
  TCommIfStatus   status = E_COMM_IF_STATUS_ERROR;
  TKCommStatus    commStatus = E_K_COMM_STATUS_ERROR;
  TBoolean        isValidAdd = E_FALSE;
  size_t          mtuSize = 0;
  uint16_t        uriLength = 0;
  uint8_t         aIpAddress[C_COMM_INTERFACE_MAX_IP_ADDRESS_LENGTH] = { 0 };
  uint8_t         ipAddressLength = 0;

  M_COMM__API_START();

  for (;;)
  {
    if (
      (NULL == xpHost) ||
      (0U == xPort) ||
      ((E_COMM_IF_IP_PROTOCOL_V4 != xIpProtocol) && (E_COMM_IF_IP_PROTOCOL_V6 != xIpProtocol)) ||
      (E_TRUE == gCommInterfaceObj.isInitialized) ||
      (NULL == xpUri)
    )
    {
      M_COMM__ERROR(("Invalid parameters or already initialized"));
      status = E_COMM_IF_STATUS_PARAMETER;
      break;
    }

    status = lgetIPAddress(xpHost, aIpAddress);

    if (E_COMM_IF_STATUS_OK != status)
    {
      M_COMM__ERROR(("lgetIPAddress failed %d", status));
      break;
    }

    ipAddressLength = strlen((char*)aIpAddress);
    uriLength = strlen((const char*)xpUri);

    gCommInterfaceObj.isInitialized = E_FALSE;
    gCommInterfaceObj.pCoapHandle = sn_coap_protocol_init(pCommCoapMalloc,
                                                          commCoapFree,
                                                          commCoapTxCb,
                                                          commCoapRxCb);

    if (NULL == gCommInterfaceObj.pCoapHandle)
    {
      M_COMM__ERROR(("sn_coap_protocol_init failed"));
      break;
    }

    gCommInterfaceObj.pCoapUri = M_COMM_INTERFACE_MALLOC(uriLength);

    if (NULL == gCommInterfaceObj.pCoapUri)
    {
      terminateCoapProtocol();
      M_COMM__ERROR(("Memory Allocation failed Size[%d]", uriLength));
      status = E_COMM_IF_STATUS_MEMORY;
      break;
    }

    (void)memcpy(gCommInterfaceObj.pCoapUri, xpUri, uriLength);
    gCommInterfaceObj.coapUriLength = uriLength;

    gCommInterfaceObj.pServerIp = M_COMM_INTERFACE_MALLOC(ipAddressLength);

    if (NULL == gCommInterfaceObj.pServerIp)
    {
      terminateCoapProtocol();
      M_COMM__ERROR(("Memory Allocation failed Size[%d]", ipAddressLength));
      status = E_COMM_IF_STATUS_MEMORY;
      break;
    }

    (void)memcpy(gCommInterfaceObj.pServerIp, aIpAddress, ipAddressLength);
    gCommInterfaceObj.serverPort = xPort;

    /* Creating the socket. */
    commStatus = salSocketCreate(E_SAL_SOCKET_TYPE_UDP, &gCommInterfaceObj.pSocket);

    if (E_K_COMM_STATUS_OK != commStatus)
    {
      M_COMM__ERROR(("salSocketCreate failed Status[%d]", commStatus));
      terminateCoapProtocol();
      break;
    }

    if (E_COMM_IF_IP_PROTOCOL_V4 == xIpProtocol)
    {
      isValidAdd = commUtilConvertSocketIp(aIpAddress, &gCommInterfaceObj.socketIP);

      if (E_TRUE != isValidAdd)
      {
        status = E_COMM_IF_STATUS_PARAMETER;
        M_COMM__ERROR(("Invalid ip4 socket IP[%s]", aIpAddress));
        terminateCoapProtocol();
        break;
      }

      gCommInterfaceObj.socketIP.address.v4.port = xPort;
      gCommInterfaceObj.dstAddress.type = SN_NSDL_ADDRESS_TYPE_IPV4;
    }
    else
    {
      gCommInterfaceObj.dstAddress.type = SN_NSDL_ADDRESS_TYPE_IPV6;
      gCommInterfaceObj.socketIP.address.v6.port = xPort;
      (void)memcpy(gCommInterfaceObj.socketIP.address.v6.address,
                   aIpAddress,
                   C_COMM_INTERFACE_MAX_IP_ADDRESS_LENGTH);
    }

    gCommInterfaceObj.dstAddress.addr_ptr = gCommInterfaceObj.pServerIp;
    gCommInterfaceObj.dstAddress.addr_len = ipAddressLength;
    gCommInterfaceObj.dstAddress.port = gCommInterfaceObj.serverPort;

    commStatus = sn_coap_protocol_set_retransmission_parameters(gCommInterfaceObj.pCoapHandle,
                                                                getCoapResendingCount(),
                                                                getCoapResendingInterval());

    if (0 != commStatus)
    {
      M_COMM__ERROR(("sn_coap_protocol_set_retransmission_parameters failed Status[%d]",
                     TKCommStatus));
      terminateCoapProtocol();
      break;
    }

    mtuSize = getMtuSize();
    gCommInterfaceObj.coapBlockSize = getCoapBlockSizeUsingMtu(mtuSize);

    gCommInterfaceObj.pResponseBuffer = (uint8_t*)M_COMM_INTERFACE_MALLOC(mtuSize);

    if (NULL == gCommInterfaceObj.pResponseBuffer)
    {
      M_COMM__ERROR(("Memory Allocation failed Size[%ld]", mtuSize));
      status = E_COMM_IF_STATUS_MEMORY;
      terminateCoapProtocol();
      break;
    }

    gCommInterfaceObj.mtuSize = mtuSize;

    (void)memset(gCommInterfaceObj.pResponseBuffer, 0, mtuSize);

    status = E_COMM_IF_STATUS_OK;
    gCommInterfaceObj.isInitialized = E_TRUE;
    break;
  }

  M_COMM__API_END();

  return status;
}

/**
 * @brief  implement commTerminateProtocol
 *
 */
TCommIfStatus commTerminateProtocol
(
  void
)
{
  M_COMM__API_START();

  terminateCoapProtocol();

  M_COMM__API_END();

  return E_COMM_IF_STATUS_OK;
}

/**
 * @brief  implement commMessageExchange
 *
 */
TCommIfStatus commMessageExchange
(
  const  uint8_t*  xpMessageToSend,
  const  size_t    xSendSize,
  uint8_t*         xpReceiveMsgBuffer,
  size_t*          xpReceiveMsgBufferLength
)
{
  TCommIfStatus   commStatus = E_COMM_IF_STATUS_ERROR;
  TKCommStatus    status = E_K_COMM_STATUS_OK;
  TKSocketIp      ip = {0};
  size_t          responseBufferLength = 0;
  TBoolean        isPayloadFreeRequired = E_FALSE;

  M_COMM__API_START();

  for (;;)
  {
    if (
      (NULL == xpMessageToSend) ||
      (0U == xSendSize) ||
      (NULL == xpReceiveMsgBuffer) ||
      (NULL == xpReceiveMsgBufferLength) ||
      (0 == *xpReceiveMsgBufferLength) ||
      (E_FALSE == gCommInterfaceObj.isInitialized)
    )
    {
      M_COMM__ERROR(("Invalid paramertes or not initialized"));
      commStatus = E_COMM_IF_STATUS_PARAMETER;
      break;
    }

    gCommInterfaceObj.maxRetries = C_COMM_INTERFACE_COAP_MAX_RESENDING_RETRIES;
    gCommInterfaceObj.lastRecivedMessageId = 0;

    status = commCoapBuildAndSendMessage(xpMessageToSend, xSendSize);

    if (E_K_COMM_STATUS_OK != status)
    {
      M_COMM__ERROR(("commCoapBuildAndSendMessage Failed %d", status));
      commStatus = commConvertError(status);
      break;
    }

    M_COMM__INFO(("First message send successfully"));
    gCommInterfaceObj.isExchangeTerminated = E_FALSE;

    do
    {
      responseBufferLength = gCommInterfaceObj.mtuSize;
      gCommInterfaceObj.exchangeStatus = E_K_COMM_STATUS_ERROR;
      status = salSocketReceiveFrom(gCommInterfaceObj.pSocket,
                                    gCommInterfaceObj.pResponseBuffer,
                                    &responseBufferLength,
                                    &ip);

      switch (status)
      {
        case E_K_COMM_STATUS_OK:
        {
          commCoapGetResponse(gCommInterfaceObj.pResponseBuffer,
                              responseBufferLength,
                              gCommInterfaceObj.pCoapHandle,
                              &isPayloadFreeRequired);
        }
        break;

        case E_K_COMM_STATUS_NETWORK:
        {
          /* Network not available. */
          M_COMM__ERROR(("salSocketReceiveFrom Failed E_K_COMM_STATUS_NETWORK"));
          gCommInterfaceObj.exchangeStatus = E_K_COMM_STATUS_NETWORK;
          gCommInterfaceObj.isExchangeTerminated = E_TRUE;
        }
        break;

        case E_K_COMM_STATUS_MISSING:
        {
          M_COMM__ERROR(("salSocketReceiveFrom Failed E_K_COMM_STATUS_MISSING"));

          /* No data available, resending */
          if (gCommInterfaceObj.maxRetries > 0u)
          {
            --gCommInterfaceObj.maxRetries;
            M_COMM__INFO(("Retry Count%d", gCommInterfaceObj.maxRetries));
            commCoapWaitForData(gCommInterfaceObj.pCoapHandle);
          }
          else
          {
            /* Max retry reached, break the communication. */
            gCommInterfaceObj.exchangeStatus = E_K_COMM_STATUS_RESOURCE;
            gCommInterfaceObj.isExchangeTerminated = E_TRUE;
            M_COMM__ERROR(("Max Retry Count reached%d Stopping.", gCommInterfaceObj.maxRetries));
          }

          M_COMM__INFO(("sn_coap_protocol_exec %d", status));
        }
        break;

        case E_K_COMM_STATUS_ERROR:
        default:
        {
          M_COMM__ERROR(("Unknow status code %d", status));
          gCommInterfaceObj.isExchangeTerminated = E_TRUE;
          gCommInterfaceObj.exchangeStatus = E_K_COMM_STATUS_DATA;
        }
        break;
      }

#ifdef ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS
      debugPrintBufferInHex(gCommInterfaceObj.pResponseBuffer, responseBufferLength);
#endif /* ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS */
    } while (E_FALSE == gCommInterfaceObj.isExchangeTerminated);

    commStatus = commConvertError(gCommInterfaceObj.exchangeStatus);

    if (E_K_COMM_STATUS_OK == gCommInterfaceObj.exchangeStatus)
    {
      *xpReceiveMsgBufferLength = copyPayloadToMessageBuffer(xpReceiveMsgBuffer,
                                                             *xpReceiveMsgBufferLength,
                                                             isPayloadFreeRequired);
      break;
    }

    *xpReceiveMsgBufferLength = 0;
    break;
  }

  /* Free blockwise messages memory. */
  sn_coap_protocol_clear_sent_blockwise_messages(gCommInterfaceObj.pCoapHandle);
  sn_coap_protocol_clear_received_blockwise_messages(gCommInterfaceObj.pCoapHandle);
  sn_coap_protocol_clear_retransmission_buffer(gCommInterfaceObj.pCoapHandle);

  M_COMM__API_END();

  return commStatus;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

#ifdef ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS
/**
 * @implements debugPrintBufferInHex
 *
 */
static void debugPrintBufferInHex
(
  const uint8_t*  xpBuffer,
  const size_t  xBufferLength
)
{
  size_t loopCount;

  M_COMM__INFO(("======================================"));
  M_COMM__INFO(("Hex "));

  for (loopCount = 0; loopCount < xBufferLength; loopCount++)
  {
    M_COMM_LOG(("%02X", (int)xpBuffer[loopCount]));
  }

  M_COMM__INFO(("======================================"));
}

/**
 * @implements debugPrintCoapHeader
 *
 */
static void debugPrintCoapHeader
(
  const sn_coap_hdr_s* xpCoapHeader
)
{
  if (NULL != xpCoapHeader)
  {
    M_COMM__INFO(("----------------------------"));
    M_COMM__INFO(("Token Length   %d", xpCoapHeader->token_len));
    M_COMM__INFO(("Coap Status    %d", xpCoapHeader->coap_status));
    M_COMM__INFO(("Msg Code       %d", xpCoapHeader->msg_code));
    M_COMM__INFO(("Msg Type       %d", xpCoapHeader->msg_type));
    M_COMM__INFO(("Content Format %d", xpCoapHeader->content_format));
    M_COMM__INFO(("Msg Id         %d", xpCoapHeader->msg_id));
    M_COMM__INFO(("Payload Len    %d", xpCoapHeader->payload_len));
    M_COMM__INFO(("----------------------------"));
  }
}

/**
 * @implements debugPrintPayload
 *
 */
static void debugPrintPayload
(
  const uint8_t*  xpMessageReceived,
  const size_t    xReceivedMsgSize
)
{
  size_t     loopCount;

  M_COMM__INFO(("Payload Received :"));

  for (loopCount = 0; loopCount < xReceivedMsgSize; loopCount++)
  {
    M_COMM_LOG(("%02x", (int)xpMessageReceived[loopCount]));
  }

  M_COMM__INFO(("EOM"));
}
#endif /* ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS */

/**
 * @implements pCommCoapMalloc
 *
 */
static void* pCommCoapMalloc
(
  uint16_t xSize
)
{
  return kta_pSalMemoryAllocate(xSize);
}

/**
 * @implements commCoapFree
 *
 */
static void commCoapFree
(
  void* xpAddr
)
{
  salMemoryFree(xpAddr);
}

/**
 * @implements commCoapTxCb
 *
 */
static uint8_t commCoapTxCb
(
  uint8_t*         xpSendBuffer,
  uint16_t         xSendBufferSize,
  sn_nsdl_addr_s*  xpDstAddress,
  void*            xpUserData
)
{
  TKCommStatus socketStatus = E_K_COMM_STATUS_ERROR;

  M_COMM__API_START();
  M_COMM__INFO(("commCoapTxCb Sending Message...Size %d", xSendBufferSize));

#ifdef ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS
  debugPrintBufferInHex(xpSendBuffer, xSendBufferSize);
#endif /* ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS */

  socketStatus = salSocketSendTo(gCommInterfaceObj.pSocket,
                                 xpSendBuffer,
                                 xSendBufferSize,
                                 &gCommInterfaceObj.socketIP);
  M_COMM__API_END();
  M_UNUSED(xpDstAddress);
  M_UNUSED(xpUserData);
  return socketStatus;
}

/**
 * @implements commCoapRxCb
 *
 */
static int8_t commCoapRxCb
(
  sn_coap_hdr_s*     xpCoapHeader,
  sn_nsdl_addr_s*    xpDstAddress,
  void*              xpUserData)
{
  M_COMM__API_START();
  M_COMM__INFO(("coap rx cb"));
  M_COMM__API_END();
  M_UNUSED(xpCoapHeader);
  M_UNUSED(xpDstAddress);
  M_UNUSED(xpUserData);
  return 0;
}

/**
 * @implements terminateCoapProtocol
 *
 */
static void terminateCoapProtocol
(
  void
)
{
  M_COMM__API_START();

  if (NULL != gCommInterfaceObj.pCoapHandle)
  {
    sn_coap_protocol_destroy(gCommInterfaceObj.pCoapHandle);
    gCommInterfaceObj.pCoapHandle = NULL;
  }

  if (NULL != gCommInterfaceObj.pServerIp)
  {
    M_COMM_INTERFACE_FREE(gCommInterfaceObj.pServerIp);
    gCommInterfaceObj.pServerIp = NULL;
  }

  if (NULL != gCommInterfaceObj.pCoapUri)
  {
    M_COMM_INTERFACE_FREE(gCommInterfaceObj.pCoapUri);
    gCommInterfaceObj.pCoapUri = NULL;
  }

  if (NULL != gCommInterfaceObj.pResponseBuffer)
  {
    M_COMM_INTERFACE_FREE(gCommInterfaceObj.pResponseBuffer);
    gCommInterfaceObj.pResponseBuffer = NULL;
  }

  salSocketDispose(gCommInterfaceObj.pSocket);

  gCommInterfaceObj.serverPort = 0;
  gCommInterfaceObj.isInitialized = E_FALSE;

  M_COMM__API_END();
}

/**
 * @implements pGetDefaultCoapHeader
 *
 */
static sn_coap_hdr_s* pGetDefaultCoapHeader
(
  const uint16_t  xPayloadLen,
  const uint8_t*  xpPayload
)
{
  uint8_t*        pCoapUriPath  = NULL;
  sn_coap_hdr_s*  pCoapHeader   = NULL;
  uint32_t        coapToken     = 0;
  TKCommStatus    status        = E_K_COMM_STATUS_ERROR;
  const uint8_t*  pCoapUri      = gCommInterfaceObj.pCoapUri;
  const uint16_t  coapUriLength = gCommInterfaceObj.coapUriLength;

  M_COMM__API_START();

  for (;;)
  {
    status = salRandomGet((unsigned char*)&coapToken, sizeof(coapToken));

    if (E_K_COMM_STATUS_OK != status)
    {
      M_COMM__ERROR(("salRandomGet failed"));
      break;
    }

    status = E_K_COMM_STATUS_ERROR;

    M_COMM__INFO(("Coap Token Value %u", coapToken));
    pCoapHeader = (sn_coap_hdr_s*)M_COMM_INTERFACE_MALLOC(sizeof(sn_coap_hdr_s));

    if (NULL == pCoapHeader)
    {
      M_COMM__ERROR(("Coap header memory allocation failed"));
      break;
    }

    (void)memset(pCoapHeader, 0, sizeof(sn_coap_hdr_s));

    pCoapUriPath = M_COMM_INTERFACE_MALLOC(coapUriLength);

    if (NULL == pCoapUriPath)
    {
      M_COMM__ERROR(("Coap uri path memory allocation failed"));
      break;
    }

    (void)memcpy(pCoapUriPath, pCoapUri, coapUriLength);

    pCoapHeader->uri_path_ptr = pCoapUriPath;               /* Uri path. */
    pCoapHeader->uri_path_len = coapUriLength;              /* Length of Uri path. */
    pCoapHeader->msg_code = COAP_MSG_CODE_REQUEST_POST;     /* CoAP method. */
    pCoapHeader->content_format = COAP_CT_OCTET_STREAM;     /* CoAP content type. */
    pCoapHeader->payload_len = xPayloadLen;                 /* Body length. */
    pCoapHeader->payload_ptr = (uint8_t*)xpPayload;         /* Body pointer. */
    pCoapHeader->options_list_ptr = 0;                      /* Optional: options list. */
    pCoapHeader->token_len = sizeof(coapToken);
    pCoapHeader->token_ptr = M_COMM_INTERFACE_MALLOC(pCoapHeader->token_len);

    if (NULL == pCoapHeader->token_ptr)
    {
      M_COMM__ERROR(("Coap token memory allocation failed"));
      break;
    }

    (void)memcpy(pCoapHeader->token_ptr, &coapToken, pCoapHeader->token_len);

    /* Message ID is used to track request->response patterns, because we're
     * using UDP (so everything is unconfirmed).
     * See the receive code to verify that we get the same message ID back.
     *
     * Setting message id to 0. So that mbed coap will internally generate the new message ID.
     */
    pCoapHeader->msg_id = 0;
    status = E_K_COMM_STATUS_OK;
    break;
  }

  if ((E_K_COMM_STATUS_OK != status) && (NULL != pCoapHeader))
  {
    if (NULL != pCoapHeader->uri_path_ptr)
    {
      M_COMM_INTERFACE_FREE(pCoapHeader->uri_path_ptr);
      pCoapHeader->uri_path_ptr = NULL;
    }

    M_COMM_INTERFACE_FREE(pCoapHeader);
    pCoapHeader = NULL;
  }

  M_COMM__API_END();
  return pCoapHeader;
}

/**
 * @implements getMtuSize
 *
 */
static size_t getMtuSize
(
  void
)
{
  size_t value = 0;
  TKCommStatus status = E_K_COMM_STATUS_ERROR;

  M_COMM__API_START();
  status = salSocketGetNetworkMtu(&value);
  value = (E_K_COMM_STATUS_OK == status) ? value : C_COMM_INTERFACE_COAP_MAX_RECEIVE_BUFFER_SIZE;

  M_COMM__INFO(("MTU Size %ld", value));
  M_COMM__API_END();

  return value;
}

/**
 * @implements getCoapBlockSizeUsingMtu
 *
 */
static uint16_t getCoapBlockSizeUsingMtu
(
  const size_t  xMtuValue
)
{
  /* valid CoAP block sizes are 16, 32, 64, 128, 256, 512 and 1024. */
  uint16_t aBlockSizeArray[] = {16, 32, 64, 128, 256, 512, 1024};
  uint32_t blockIndex = 0;
  uint32_t blockEndIndex = sizeof(aBlockSizeArray) / sizeof(aBlockSizeArray[0]) - 1U;

  M_COMM__API_START();

  /* Pick the nearest coap block size for the received MTU size. */
  for (blockIndex = blockEndIndex; blockIndex >= 0U; --blockIndex)
  {
    if (xMtuValue >= aBlockSizeArray[blockIndex])
    {
      break;
    }
  }

  M_COMM__INFO(("Block Size %d", aBlockSizeArray[blockIndex]));
  M_COMM__API_END();

  /* Default block size is 16. */
  return aBlockSizeArray[blockIndex];
}

/**
 * @implements getCoapResendingCount
 *
 */
static uint8_t getCoapResendingCount
(
  void
)
{
  M_COMM__API_START();
  M_COMM__API_END();

  return C_COMM_INTERFACE_COAP_MAX_RESENDING_COUNT;
}

/**
 * @implements getCoapResendingInterval
 *
 */
static uint8_t getCoapResendingInterval
(
  void
)
{
  M_COMM__API_START();
  M_COMM__API_END();

  return C_COMM_INTERFACE_COAP_RESENDING_INTERVAL_IN_SECS;
}

/**
 * @implements getRelativeTimeInSec
 *
 */
static uint32_t getRelativeTimeInSec
(
  void
)
{
  TKSalMsTime timeInSec = 0;

  M_COMM__API_START();
  timeInSec = salTimeGetRelative() / 1000;
  M_COMM__API_END();

  return timeInSec;
}

/**
 * @implements commCoapBuildAndSendMessage
 *
 */
static TKCommStatus commCoapBuildAndSendMessage
(
  const uint8_t*  xpMessageToSend,
  const size_t    xSendSize
)
{
  TKCommStatus    status = E_K_COMM_STATUS_ERROR;
  sn_coap_hdr_s*  pCoapResponsePtr = NULL;
  uint8_t*        pTxMessageBuffer = NULL;
  uint16_t        txBufferSize = 0;
  int16_t         lengthAndStatus = -1;
  int8_t          coapStatus = -1;

  M_COMM__API_START();

  for (;;)
  {
    gCommInterfaceObj.isExchangeTerminated = E_FALSE;

    pCoapResponsePtr = pGetDefaultCoapHeader(xSendSize, xpMessageToSend);

    if (NULL == pCoapResponsePtr)
    {
      M_COMM__ERROR(("pGetDefaultCoapHeader Failed"));
      status = E_K_COMM_STATUS_MEMORY;
      break;
    }

    coapStatus = sn_coap_protocol_set_block_size(gCommInterfaceObj.pCoapHandle,
                                                 gCommInterfaceObj.coapBlockSize);

    if (0 != coapStatus)
    {
      M_COMM__ERROR(("sn_coap_protocol_set_block_size Failed status[%d]", coapStatus));
      status = E_K_COMM_STATUS_DATA;
      break;
    }

    coapStatus = prepare_blockwise_message(gCommInterfaceObj.pCoapHandle, pCoapResponsePtr);

    if (0 != coapStatus)
    {
      M_COMM__ERROR(("prepare_blockwise_message Failed status[%d]", coapStatus));
      status = E_K_COMM_STATUS_DATA;
      break;
    }

    txBufferSize = sn_coap_builder_calc_needed_packet_data_size_2(pCoapResponsePtr,
                   gCommInterfaceObj.coapBlockSize);

    if (0U == txBufferSize)
    {
      M_COMM__ERROR(("sn_coap_builder_calc_needed_packet_data_size_2 Failed"));
      status = E_K_COMM_STATUS_DATA;
      break;
    }

    pTxMessageBuffer = (uint8_t*)M_COMM_INTERFACE_MALLOC(txBufferSize);

    if (NULL == pTxMessageBuffer)
    {
      M_COMM__ERROR(("Memory Alloc Failed Size[%d]", txBufferSize));
      status = E_K_COMM_STATUS_MEMORY;
      break;
    }

    (void)memset(pTxMessageBuffer, 0, txBufferSize);

    lengthAndStatus = sn_coap_protocol_build(gCommInterfaceObj.pCoapHandle,
                                             &gCommInterfaceObj.dstAddress,
                                             pTxMessageBuffer,
                                             pCoapResponsePtr,
                                             NULL,
                                             getRelativeTimeInSec());

    if (lengthAndStatus <= 0x00)
    {
      M_COMM__ERROR(("sn_coap_protocol_build Failed"));
      status = E_K_COMM_STATUS_DATA;
      break;
    }

    /* Sending message to the server. */
    status = commCoapTxCb(pTxMessageBuffer, txBufferSize, NULL, NULL);
    break;
  }

  sn_coap_parser_release_allocated_coap_msg_mem(gCommInterfaceObj.pCoapHandle, pCoapResponsePtr);
  M_COMM_INTERFACE_FREE(pTxMessageBuffer);
  pTxMessageBuffer = NULL;

  M_COMM__API_END();

  return status;
}

/**
 * @implements commCoapPrepareAndSendBlock2Message
 *
 */
static TKCommStatus commCoapPrepareAndSendBlock2Message
(
  const int32_t  xBlock2
)
{
  TKCommStatus             status = E_K_COMM_STATUS_ERROR;
  uint8_t*                 pTxMessageBuffer = NULL;
  sn_coap_hdr_s*           pCoapResponsePtr = NULL;
  sn_coap_options_list_s*  pOptionList = NULL;
  uint32_t                 blockNumber = 0;
  uint16_t                 txBufferSize = 0;
  int16_t                  coapStatus = 0;
  int16_t                  lengthAndStatus = -1;
  uint8_t                  blockTemp = 0;

  M_COMM__API_START();

  for (;;)
  {
    pCoapResponsePtr = pGetDefaultCoapHeader(0, NULL);

    if (NULL == pCoapResponsePtr)
    {
      M_COMM__ERROR(("pGetDefaultCoapHeader Failed"));
      status = E_K_COMM_STATUS_MEMORY;
      break;
    }

    coapStatus = prepare_blockwise_message(gCommInterfaceObj.pCoapHandle, pCoapResponsePtr);

    if (0 != coapStatus)
    {
      M_COMM__ERROR(("prepare_blockwise_message Failed status[%d]", coapStatus));
      status = E_K_COMM_STATUS_DATA;
      break;
    }

    pOptionList = sn_coap_parser_alloc_options(gCommInterfaceObj.pCoapHandle, pCoapResponsePtr);

    if (NULL == pOptionList)
    {
      M_COMM__ERROR(("sn_coap_parser_alloc_options Failed"));
      status = E_K_COMM_STATUS_MEMORY;
      break;
    }

    blockTemp = ((uint8_t)xBlock2 & 0x07u);
    blockNumber = ((uint32_t)xBlock2 >> 4u);
    blockNumber++;
    pOptionList->block2 = (blockNumber << 4) | blockTemp;

    txBufferSize = sn_coap_builder_calc_needed_packet_data_size_2(
                     pCoapResponsePtr,
                     gCommInterfaceObj.coapBlockSize);

    if (0U == txBufferSize)
    {
      M_COMM__ERROR(("sn_coap_builder_calc_needed_packet_data_size_2 Failed"));
      status = E_K_COMM_STATUS_DATA;
      break;
    }

    pTxMessageBuffer = (uint8_t*)M_COMM_INTERFACE_MALLOC(txBufferSize);

    if (NULL == pTxMessageBuffer)
    {
      M_COMM__ERROR(("Memory Alloc Failed Size[%d]", txBufferSize));
      status = E_K_COMM_STATUS_MEMORY;
      break;
    }

    (void)memset(pTxMessageBuffer, 0, txBufferSize);

    lengthAndStatus = sn_coap_protocol_build(gCommInterfaceObj.pCoapHandle,
                                             &gCommInterfaceObj.dstAddress,
                                             pTxMessageBuffer,
                                             pCoapResponsePtr,
                                             NULL,
                                             getRelativeTimeInSec());

    if (0x00 >= lengthAndStatus)
    {
      M_COMM__ERROR(("sn_coap_protocol_build Failed"));
      status = E_K_COMM_STATUS_DATA;
      break;
    }

    M_COMM__INFO(("Send the first block2 message"));
    status = commCoapTxCb(pTxMessageBuffer, txBufferSize, NULL, NULL);
    break;
  }

  sn_coap_parser_release_allocated_coap_msg_mem(gCommInterfaceObj.pCoapHandle, pCoapResponsePtr);
  pCoapResponsePtr = NULL;
  M_COMM_INTERFACE_FREE(pTxMessageBuffer);
  pTxMessageBuffer = NULL;

  M_COMM__API_END();

  return status;
}

/**
 * @implements commConvertError
 *
 */
static TCommIfStatus commConvertError
(
  TKCommStatus  xStatus
)
{
  TCommIfStatus commStatus;

  M_COMM__API_START();

  switch (xStatus)
  {
    case E_K_COMM_STATUS_OK:
    {
      commStatus = E_COMM_IF_STATUS_OK;
    }
    break;

    case E_K_COMM_STATUS_NETWORK:
    {
      commStatus = E_COMM_IF_STATUS_NETWORK;
    }
    break;

    case E_K_COMM_STATUS_PARAMETER:
    {
      commStatus = E_COMM_IF_STATUS_PARAMETER;
    }
    break;

    case E_K_COMM_STATUS_DATA:
    {
      commStatus = E_COMM_IF_STATUS_DATA;
    }
    break;

    case E_K_COMM_STATUS_TIMEOUT:
    {
      commStatus = E_COMM_IF_STATUS_TIMEOUT;
    }
    break;

    case E_K_COMM_STATUS_MEMORY:
    {
      commStatus = E_COMM_IF_STATUS_MEMORY;
    }
    break;

    case E_K_COMM_STATUS_ERROR:
    {
      commStatus = E_COMM_IF_STATUS_ERROR;
    }
    break;

    default:
    {
      M_COMM__ERROR(("Unknow xStatus code %d", xStatus));
      commStatus = E_COMM_IF_STATUS_ERROR;
    }
    break;
  }

  M_COMM__INFO(("xStatus %d ConvertedStatus %d", xStatus, commStatus));
  M_COMM__API_END();

  return commStatus;
}

/**
 * @implements commCoapWaitForData
 *
 */
static void commCoapWaitForData
(
  struct coap_s*  xpCoapHandle
)
{
  uint32_t  currentTime = 0;
  int8_t    execStatus = -1;

  M_COMM__API_START();

  if ((NULL != xpCoapHandle) && (E_TRUE == gCommInterfaceObj.isInitialized))
  {
    currentTime = getRelativeTimeInSec();
    execStatus = sn_coap_protocol_exec(xpCoapHandle, currentTime);

    if (0 == execStatus)
    {
      M_COMM__INFO(("wait for data - %d ms", C_COMM_INTERFACE_COAP_WAIT_FOR_RESPONSE));
      salTimeMilliSleep(C_COMM_INTERFACE_COAP_WAIT_FOR_RESPONSE);
    }
    else
    {
      M_COMM__ERROR(("wait for data - invalid behaviour"));
      gCommInterfaceObj.exchangeStatus = E_K_COMM_STATUS_RESOURCE;
      gCommInterfaceObj.isExchangeTerminated = E_TRUE;
    }
  }

  M_COMM__API_END();
}

/**
 * @implements commCoapRetrySendBlock2
 *
 */
static void commCoapRetrySendBlock2
(
  const int32_t  xBlock2
)
{
  TKCommStatus  status = E_K_COMM_STATUS_ERROR;

  M_COMM__API_START();

  gCommInterfaceObj.maxRetries = C_COMM_INTERFACE_COAP_MAX_RESENDING_RETRIES;

  do
  {
    status = commCoapPrepareAndSendBlock2Message(xBlock2);

    if (E_K_COMM_STATUS_OK == status)
    {
      M_COMM__INFO(("commCoapPrepareAndSendBlock2Message success"));
      break;
    }

    --gCommInterfaceObj.maxRetries;
    salTimeMilliSleep(C_COMM_INTERFACE_COAP_WAIT_FOR_RESPONSE);
  } while (gCommInterfaceObj.maxRetries > 0u);

  if (0U == gCommInterfaceObj.maxRetries)
  {
    gCommInterfaceObj.isExchangeTerminated = E_TRUE;
    gCommInterfaceObj.exchangeStatus = status;
    M_COMM__ERROR(("Max Retry[%d] reached Status[%d]",
                   C_COMM_INTERFACE_COAP_MAX_RESENDING_RETRIES, status));
  }
  else
  {
    gCommInterfaceObj.isExchangeTerminated = E_FALSE;
    gCommInterfaceObj.maxRetries = C_COMM_INTERFACE_COAP_MAX_RESENDING_RETRIES;
    gCommInterfaceObj.exchangeStatus = status;
  }

  M_COMM__API_END();
}

/**
 * @implements commCoapGetResponse
 *
 */
static void commCoapGetResponse
(
  uint8_t*        xpResponseBuffer,
  size_t          xResponseBufferLength,
  struct coap_s*  xpCoapHandle,
  TBoolean*       xpIsPayloadFreeRequired
)
{
  sn_coap_hdr_s*  pCoapResponseData = NULL;
  TKCommStatus    status            = E_K_COMM_STATUS_ERROR;

  M_COMM__API_START();

  for (;;)
  {
    gCommInterfaceObj.isExchangeTerminated = E_TRUE;
    *xpIsPayloadFreeRequired = E_FALSE;
    gCommInterfaceObj.maxRetries = C_COMM_INTERFACE_COAP_MAX_RESENDING_RETRIES;

    pCoapResponseData = sn_coap_protocol_parse(xpCoapHandle,
                                               &gCommInterfaceObj.dstAddress,
                                               xResponseBufferLength,
                                               xpResponseBuffer,
                                               NULL);

    if (NULL == pCoapResponseData)
    {
      M_COMM__ERROR(("sn_coap_protocol_parse failed"));
      break;
    }

    /* Check for message duplication. */
    if (gCommInterfaceObj.lastRecivedMessageId == pCoapResponseData->msg_id)
    {
      M_COMM__ERROR(("Duplicate message detected Ignoring"));
      gCommInterfaceObj.isExchangeTerminated = E_FALSE;
      break;
    }

    gCommInterfaceObj.lastRecivedMessageId = pCoapResponseData->msg_id;

#ifdef ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS
    debugPrintCoapHeader(pCoapResponseData);
#endif /* ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS */

    gCommInterfaceObj.exchangeStatus = E_K_COMM_STATUS_ERROR;

    switch (pCoapResponseData->coap_status)
    {
      case COAP_STATUS_OK:
      {
        gCommInterfaceObj.isExchangeTerminated = E_TRUE;
        gCommInterfaceObj.exchangeStatus = E_K_COMM_STATUS_OK;

        /**
         * In some cases,the mbed-coap returns the first payload in the last response of the
         * send request. So storing the first payload and later combining the first and remaining
         * payload while copying it to result buffer.
         */
        if (pCoapResponseData->payload_len > 0)
        {
          gCommInterfaceObj.pFirstPayload =
            (uint8_t*)M_COMM_INTERFACE_MALLOC(pCoapResponseData->payload_len);

          if (NULL == gCommInterfaceObj.pFirstPayload)
          {
            M_COMM__ERROR(("Memory Alloc Failed Size[%d]", pCoapResponseData->payload_len));
            gCommInterfaceObj.firstPayloadLength = 0;
            gCommInterfaceObj.exchangeStatus = E_K_COMM_STATUS_MEMORY;
          }
          else
          {
            (void)memcpy(gCommInterfaceObj.pFirstPayload,
                         pCoapResponseData->payload_ptr,
                         pCoapResponseData->payload_len);

            gCommInterfaceObj.firstPayloadLength = pCoapResponseData->payload_len;
            M_COMM__INFO(("First Payload %p length %d",
                          gCommInterfaceObj.pFirstPayload,
                          gCommInterfaceObj.firstPayloadLength));
          }
        }

        if (
          (NULL != pCoapResponseData->options_list_ptr) &&
          (COAP_OPTION_BLOCK_NONE != pCoapResponseData->options_list_ptr->block2)
        )
        {
          gCommInterfaceObj.exchangeStatus = E_K_COMM_STATUS_ERROR;

          status = commCoapPrepareAndSendBlock2Message(
                     pCoapResponseData->options_list_ptr->block2);

          if (E_K_COMM_STATUS_OK == status)
          {
            gCommInterfaceObj.isExchangeTerminated = E_FALSE;
            gCommInterfaceObj.maxRetries = C_COMM_INTERFACE_COAP_MAX_RESENDING_RETRIES;
          }
          else
          {
            M_COMM__ERROR(("commCoapPrepareAndSendBlock2Message Failed %d Retrying...", status));
            commCoapRetrySendBlock2(pCoapResponseData->options_list_ptr->block2);
          }
        }
      }
      break;

      case COAP_STATUS_PARSER_BLOCKWISE_ACK:
      case COAP_STATUS_PARSER_BLOCKWISE_MSG_RECEIVING:
      {
        gCommInterfaceObj.isExchangeTerminated = E_FALSE;
      }
      break;

      case COAP_STATUS_PARSER_BLOCKWISE_MSG_RECEIVED:
      {
        gCommInterfaceObj.exchangeStatus = E_K_COMM_STATUS_OK;
        gCommInterfaceObj.isExchangeTerminated = E_TRUE;
        *xpIsPayloadFreeRequired = E_TRUE;
        gCommInterfaceObj.pPayload = pCoapResponseData->payload_ptr;
        gCommInterfaceObj.payloadLength = pCoapResponseData->payload_len;
        M_COMM__INFO(("Remaining Payload %p length %d",
                      gCommInterfaceObj.pPayload,
                      gCommInterfaceObj.payloadLength));
      }
      break;

      default:
      {
        M_COMM__ERROR(("Unknow status code %d", pCoapResponseData->coap_status));
        gCommInterfaceObj.isExchangeTerminated = E_TRUE;
      }
      break;
    }

    /* Free the memory. */
    sn_coap_parser_release_allocated_coap_msg_mem(xpCoapHandle, pCoapResponseData);
    pCoapResponseData = NULL;
    break;
  }

  M_COMM__API_END();
}

/**
 * @implements copyPayloadToMessageBuffer
 *
 */
static uint32_t copyPayloadToMessageBuffer
(
  uint8_t*  xpDstMsgBuffer,
  size_t    xDstMsgBufferLength,
  const TBoolean  xIsPayloadFreeRequired
)
{
  uint32_t  payloadLength = gCommInterfaceObj.firstPayloadLength + gCommInterfaceObj.payloadLength;
  uint8_t*  pDstBuffer = xpDstMsgBuffer;
  size_t    dstMsgBufferLength = xDstMsgBufferLength;

  dstMsgBufferLength = (payloadLength > dstMsgBufferLength) ? dstMsgBufferLength : payloadLength;

  payloadLength = 0;

  if ((gCommInterfaceObj.firstPayloadLength > 0u) && (NULL != gCommInterfaceObj.pFirstPayload))
  {
    if (dstMsgBufferLength > gCommInterfaceObj.firstPayloadLength)
    {
      (void)memcpy(pDstBuffer,
                   gCommInterfaceObj.pFirstPayload,
                   gCommInterfaceObj.firstPayloadLength);
      dstMsgBufferLength -= gCommInterfaceObj.firstPayloadLength;
      pDstBuffer = &pDstBuffer[gCommInterfaceObj.firstPayloadLength];
      payloadLength += gCommInterfaceObj.firstPayloadLength;
    }
    else
    {
      (void)memcpy(pDstBuffer, gCommInterfaceObj.pFirstPayload, dstMsgBufferLength);
      payloadLength += (uint32_t)dstMsgBufferLength;
      dstMsgBufferLength = 0;
    }

    M_COMM_INTERFACE_FREE(gCommInterfaceObj.pFirstPayload);
    gCommInterfaceObj.firstPayloadLength = 0;
    gCommInterfaceObj.pFirstPayload = NULL;
  }

  if (
    (gCommInterfaceObj.payloadLength > 0u) &&
    (NULL != gCommInterfaceObj.pPayload) &&
    (dstMsgBufferLength > 0u)
  )
  {
    (void)memcpy(pDstBuffer, gCommInterfaceObj.pPayload, dstMsgBufferLength);
    payloadLength += (uint32_t)dstMsgBufferLength;

    if (E_TRUE == xIsPayloadFreeRequired)
    {
      M_COMM_INTERFACE_FREE(gCommInterfaceObj.pPayload);
    }

    gCommInterfaceObj.pPayload = NULL;
    gCommInterfaceObj.payloadLength = 0;
  }

#ifdef ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS
  debugPrintPayload(xpDstMsgBuffer, payloadLength);
#endif /* ENABLE_COMM_COAP_PACKET_DEBUG_PRINTS */
  return payloadLength;
}

/**
 * @implements lgetIPAddress
 *
 */
static TKCommStatus lgetIPAddress
(
  const uint8_t* xpHost,
  uint8_t*       xpIpAddress
)
{
  TCommIfStatus  status = E_COMM_IF_STATUS_ERROR;

  M_COMM__API_START();

  status = salGetHostByName((const char*)xpHost, xpIpAddress);
  M_COMM__INFO(("New xpIpAddress[%s]", xpIpAddress));

  M_COMM__API_END();

  return status;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

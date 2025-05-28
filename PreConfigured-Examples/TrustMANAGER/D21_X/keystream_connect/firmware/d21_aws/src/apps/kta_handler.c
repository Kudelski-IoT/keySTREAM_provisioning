/** \brief  Handler for keySTREAM Trusted Agent to integrate with
 *          keySTREAM aws connect application.
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file kta_handler.c
 ******************************************************************************/

/**
 * @brief  Handler for keySTREAM Trusted Agent to integrate with 
 *         keySTREAM aws connect application.
 */

#include "app.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "wdrv_winc_client_api.h"
#include "cloud_wifi_ecc_process.h"
#include "cloud_wifi_task.h"
#include "cryptoauthlib.h"
#include "cust_def_signer.h"
#include "slotConfig.h"

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */
/** @brief Maximium length allowed for IPV4 address. */
#define C_MAX_IP4_ADDRESS_LENGTH            (16u)

/** @brief Maximium length allowed for url. */
#define C_MAX_URL_LENGTH                    (70u)

/** @brief Log for debug. */
#ifdef ENABLE_HANDLER_DEBUG
#define M_KTA_HANDLER_LOG(__PRINT__) do { \
    APP_DebugPrintf("KHNDLR> "); \
    APP_DebugPrintf (__PRINT__); \
    APP_DebugPrintf("\r\n"); \
  } while (0)
#else
#define M_KTA_HANDLER_LOG(__PRINT__)
#endif /* ENABLE_HANDLER_DEBUG */

/** @brief Log for start and end. */
#ifdef ENABLE_HANDLER_DEBUG
/* Macro to print api start */
#define M_HANDLER_API_START()      APP_DebugPrintf(">Start of %s\r\n", __func__)
/* Macro to print api end. */
#define M_HANDLER_API_END()        APP_DebugPrintf("<End of %s\r\n", __func__)
#else
#define M_HANDLER_API_START()
#define M_HANDLER_API_END()
#endif /* ENABLE_HANDLER_DEBUG */

/** @breif Host by name info. */
typedef struct hostByName
{
  uint8_t aUrl[C_MAX_URL_LENGTH];
  /* URL. */
  uint8_t aIpAddress[C_MAX_IP4_ADDRESS_LENGTH];
  /* IP address. */
} THostByName;

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
static THostByName gHostByName = { 0 };
static tpfAppSocketCb gPfSocketCb = NULL;
static tpfAppResolveCb gPfAppResolveCb = NULL;
static WDRV_WINC_DHCP_ADDRESS_EVENT_HANDLER gFDHCPAddressEventCallback = NULL;
static bool gIsIPAvailable = false;

enum wifi_status kta_gSockRecvStatus = WIFI_STATUS_UNKNOWN;
enum wifi_status kta_gSockSendStatus = WIFI_STATUS_UNKNOWN;
int32_t kta_gRxBufferLength = 0;
uint32_t kta_gTxSize = 0;
extern enum cloud_iot_state kta_gCloudWifiState;
extern struct socket_connection kta_gSocketConnection;

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
/**
* @brief
*   Get host IP address by URL.
**/
uint8_t* kta_getHostByName
(
  char* xpUrl
);

/**
 * @brief
 *   Socket callback handler.
 */
static void ktaSocketCallbackHandler
(
  SOCKET  xSocket,
  uint8_t xMessageType,
  void*   xpMessage
);

/**
 * @brief
 *   DNS resolution callback handler.
 */
static void ktaCloudDnsResolveHandler
(
  uint8_t* xpU8DomainName,
  uint32_t xU32ServerIP
);

/**
 * @brief
 *   DNS enable callback handler.
 */
static void ktaDHCPAddressEventCallback
(
  DRV_HANDLE  xHandle,
  uint32_t    xIpAddress
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
* @brief implements ktaSocketRegisterEventCallback.
*
**/
WDRV_WINC_STATUS ktaSocketRegisterEventCallback
(
  DRV_HANDLE      xHandle,
  tpfAppSocketCb  xFAppSocketCb
)
{
  M_HANDLER_API_START();
  gPfSocketCb = xFAppSocketCb;
  return WDRV_WINC_SocketRegisterEventCallback(xHandle, &ktaSocketCallbackHandler);
}

/**
* @brief implements ktaSocketRegisterResolverCallbacks.
*
**/
WDRV_WINC_STATUS ktaSocketRegisterResolverCallback
(
  DRV_HANDLE      xHandle,
  tpfAppResolveCb xFAppResolveCb
)
{
  M_HANDLER_API_START();
  gPfAppResolveCb = xFAppResolveCb;
  return WDRV_WINC_SocketRegisterResolverCallback(xHandle, &ktaCloudDnsResolveHandler);
}

/**
* @brief implements ktaIPUseDHCPSet.
*
**/
WDRV_WINC_STATUS ktaIPUseDHCPSet
(
  DRV_HANDLE xHandle,
  const WDRV_WINC_DHCP_ADDRESS_EVENT_HANDLER xFDHCPAddressEventCallback
)
{
  gFDHCPAddressEventCallback = xFDHCPAddressEventCallback;
  return WDRV_WINC_IPUseDHCPSet(xHandle, &ktaDHCPAddressEventCallback);
}


/**
* @brief implements ktaTransferEccCertsToWinc.
*
*/
int8_t ktaTransferEccCertsToWinc(void)
{
  int8_t retValue;
  int atcaStatus = ATCACERT_E_SUCCESS;

  M_HANDLER_API_START();
  atcaStatus = atcab_read_bytes_zone(DEVZONE_DATA,
                                     C_KTA__SIGNER_PUB_KEY_STORAGE_SLOT, C_KTA__SIGNER_PUBLIC_KEY_STORAGE_OFFSET,
                                     g_cert_ca_public_key_1_signer, C_KTA__SIGNER_PUBLIC_KEY_LENGTH);

  if (ATCACERT_E_SUCCESS != (atcaStatus))
  {
    M_KTA_HANDLER_LOG(("atcab_read_bytes_zone failed: \r\n"));
    retValue =  M2M_ERR_FAIL;
  }
  else
  {
    retValue = transfer_ecc_certs_to_winc();
  }

  M_HANDLER_API_END();
  return retValue;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */
/**
* @implements kta_getHostByName.
*
**/
uint8_t* kta_getHostByName
(
  char* xpUrl
)
{
  M_HANDLER_API_START();

  gIsIPAvailable = false;

  size_t len = sizeof(gHostByName.aUrl);
  (void)strncpy((char*)gHostByName.aUrl, xpUrl, len);
  gethostbyname(xpUrl);

  while (gIsIPAvailable == false)
  {
    atca_delay_ms(5);
    m2m_wifi_handle_events();
  }

  M_KTA_HANDLER_LOG(("gethostbyname Ip is %s", gHostByName.aIpAddress));
  M_HANDLER_API_END();

  return gHostByName.aIpAddress;
}

/**
* @implements ktaSocketCallbackHandler.
*
**/
static void ktaSocketCallbackHandler
(
  SOCKET  xSocket,
  uint8_t xMessageType,
  void*   xpMessage
)
{
  tstrSocketRecvMsg* pSocketReceiveMessage = NULL;
  int16_t* pBytesSent = NULL;

  M_HANDLER_API_START();

  if ((NULL != gPfSocketCb) && (xSocket == kta_gSocketConnection.socket))
  {
    M_KTA_HANDLER_LOG(("Calling socket_callback_handler %d", xSocket));
    gPfSocketCb(xSocket, xMessageType, xpMessage);
  }
  else
  {
    M_KTA_HANDLER_LOG(("Ignoring socket_callback_handler %d", xSocket));

    switch (xMessageType)
    {
      case SOCKET_MSG_CONNECT:
      {
        M_KTA_HANDLER_LOG(("Ignoring SOCKET_MSG_CONNECT %d", xSocket));
        break;
      }

      case SOCKET_MSG_RECV:
      case SOCKET_MSG_RECVFROM:
      {
        pSocketReceiveMessage = (tstrSocketRecvMsg*)xpMessage;

        if (pSocketReceiveMessage != NULL)
        {
          if (pSocketReceiveMessage->s16BufferSize >= 0)
          {
            kta_gRxBufferLength += pSocketReceiveMessage->s16BufferSize;

            /* The message was received. */
            if (pSocketReceiveMessage->u16RemainingSize == 0)
            {
              kta_gSockRecvStatus = WIFI_STATUS_MESSAGE_RECEIVED;
            }

            M_KTA_HANDLER_LOG(("%s: SOCKET_MSG_RECV %d %d", __FUNCTION__,
                               (int)pSocketReceiveMessage->s16BufferSize,
                               pSocketReceiveMessage->u16RemainingSize));
          }
          else
          {
            if (pSocketReceiveMessage->s16BufferSize == SOCK_ERR_TIMEOUT)
            {
              /* A timeout has occurred. */
              kta_gSockRecvStatus = WIFI_STATUS_TIMEOUT;
            }
            else
            {
              /* An error has occurred. */
              kta_gSockRecvStatus = WIFI_STATUS_ERROR;
            }
          }
        }

        break;
      }

      case SOCKET_MSG_SEND:
        pBytesSent = (int16_t*)xpMessage;

        M_KTA_HANDLER_LOG(("%s: SOCKET_MSG_SEND %d", __FUNCTION__, (int)*pBytesSent));

        if ((*pBytesSent <= 0) || (*pBytesSent > (int32_t)kta_gTxSize))
        {
          /**
           * Seen an odd instance where pBytesSent is way more than the requested bytes sent.
           * This happens when we're expecting an error, so were assuming this is an error
           * condition.
           */
          kta_gSockSendStatus = WIFI_STATUS_ERROR;
        }
        else
        {
          /* The message was sent. */
          M_KTA_HANDLER_LOG(("%s: SOCKET_MSG_SEND %d", __FUNCTION__, (int)*pBytesSent));
          kta_gSockSendStatus = WIFI_STATUS_MESSAGE_SENT;
        }

        break;

      default:
        M_KTA_HANDLER_LOG(("%s: unhandled message %d", __FUNCTION__, (int)xMessageType));
        /* Do nothing. */
        break;
    }
  }

  M_HANDLER_API_END();
}

/**
* @implements ktaCloudDnsResolveHandler.
*
**/
static void ktaCloudDnsResolveHandler
(
  uint8_t* xpU8DomainName,
  uint32_t xU32ServerIP
)
{
  char aMessage[128] = { 0 };
  uint8_t aGHostIpAddress[4] = { 0 };

  M_HANDLER_API_START();

  if (strcmp((char*)xpU8DomainName, (char*)gHostByName.aUrl) == 0)
  {
    aGHostIpAddress[0] = xU32ServerIP & 0xFFU;
    aGHostIpAddress[1] = (xU32ServerIP >> 8U) & 0xFFU;
    aGHostIpAddress[2] = (xU32ServerIP >> 16U) & 0xFFU;
    aGHostIpAddress[3] = (xU32ServerIP >> 24U) & 0xFFU;

    (void)sprintf(&aMessage[0], "%u.%u.%u.%u", aGHostIpAddress[0], aGHostIpAddress[1],
                  aGHostIpAddress[2], aGHostIpAddress[3]);

    (void)strncpy((char*)gHostByName.aIpAddress, &aMessage[0], C_MAX_IP4_ADDRESS_LENGTH);
    gIsIPAvailable = true;
  }
  else
  {
    gPfAppResolveCb(xpU8DomainName, xU32ServerIP);
  }

  M_HANDLER_API_END();
}

/**
* @implements ktaDHCPAddressEventCallback.
*
**/
static void ktaDHCPAddressEventCallback
(
  DRV_HANDLE  xHandle,
  uint32_t    xIpAddress
)
{
  M_HANDLER_API_START();

  gFDHCPAddressEventCallback(xHandle, xIpAddress);
  kta_gCloudWifiState = CLOUD_STATE_WIFI_CONNECTED;

  M_HANDLER_API_END();
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

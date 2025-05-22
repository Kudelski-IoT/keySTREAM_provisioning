/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2024 Nagravision SÃ rl

* Subject to your compliance with these terms, you may use the Nagravision SÃ rl
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
 *  \date 2023/06/12
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
#if 0
#include "cloud_wifi_task.h"
#include "socket.h"
#endif
#include "cryptoauthlib.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "KTALog.h"

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Invalid Socket Id. */
#define C_SAL_COM_SOCKET_INVALID (-1)

/** @brief Connect wait time in ms. */
#define C_SAL_COM_CONNECT_DELAY_IN_MS       (500u)

/**
 * @brief
 * Reset read time out value in ms(this will override the read timeout
 * after first successful read. This is added to avoid more dealy in read because
 * read will always hit the timeout while checking any bytes are available or not.
 */
#define C_SAL_COM_RESET_READ_TIMEOUT_IN_MS  (100u)

/** @brief Length of an address in IP V6 protocol, in bytes. */
#define C_SAL_COM_V6_ADDRESS_NUM_BYTES      (16u)

/******************************************************************************/
/* LOCAL MACROS                                                               */
/******************************************************************************/

#ifdef DEBUG
/** @brief Enable sal com logs. */
/** 
 * SUPPRESS: MISRA_DEV_KTA_003 : misra_c2012_rule_21.6_violation
 * SUPPRESS: MISRA_DEV_KTA_001 : misra_c2012_rule_17.1_violation
 * Using printf for logging.
 * Not checking the return status of printf, since not required.
 **/
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

/******************************************************************************/
/* TYPES & STRUCTURES                                                         */
/******************************************************************************/

/** @brief Boolean Types. */
typedef enum
{
  E_FALSE,
  /* TRUE. */
  E_TRUE,
  /* FALSE. */
  E_UNDEF
  /* Undefined. */
} TBoolean;

/**
 * @ingroup g_k_types
 *
 * @brief Supported IP protocols.
 */
typedef enum
{
  E_K_IP_PROTOCOL_V4,
  /* IP V4. */
  E_K_IP_PROTOCOL_V6,
  /* IP V6. */
  E_K_IP_NUM_PROTOCOLS
  /* Number of supported IP protocols. */
} TKIpProtocol;

/**
 * @ingroup g_k_types
 *
 * @brief Socket IP address, expressed for protocol IP V4.
*/
typedef struct
{
  uint32_t       address;
  /* IP V4 address, MSBF; e.g. 192.168.15.4 = 0xC0A80F04 */
  uint16_t       port;
  /* IP port. */
} TKSocketAddressV4;

/**
 * @ingroup g_k_types
 *
 * @brief Socket IP.
 */
typedef struct
{
  TKIpProtocol         protocol;
  /* IP protocol. */
  TKSocketAddressV4    v4;
  /* IP address; the used field depends on protocol value. */
} TKSocketIp;

/**
 * @ingroup g_k_types
 *
 * @brief Communication info.
 */
typedef struct
{
  int       socketId;
  /* ID of unique socket used. */
  uint32_t  connectTimeOut;
  /* Connection timeout. */
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

/**
 * @brief
 *   Convert the ip to sal socket structure.
 *
 * @param[in] xpAddress
 *   Ip address of the server. Should not be NULL
 * @param[out] xpSocketIp
 *   Converted SAL socket ip.
 * @return
 * - E_TRUE for success
 * - E_FALSE for failure
 */
#if 0
static TBoolean lConvertSocketIp
(
  const uint8_t*  xpAddress,
  TKSocketIp*     xpSocketIp
);

#endif

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

extern int32_t kta_gRxBufferLength;
extern enum wifi_status kta_gSockRecvStatus;
extern enum wifi_status kta_gSockSendStatus;
extern uint32_t kta_gTxSize;
extern uint8_t* kta_getHostByName(const char* xpUrl);

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
  M_UNUSED(xConnectTimeoutInMs);
  M_UNUSED(xReadTimeoutInMs);
  M_UNUSED(xppComInfo);
  return E_K_COMM_STATUS_OK;
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
  M_UNUSED(xpComInfo);
  M_UNUSED(xpHost);
  M_UNUSED(xpPort);
  return E_K_COMM_STATUS_OK;
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
  M_UNUSED(xpComInfo);
  M_UNUSED(xpBuffer);
  M_UNUSED(xBufferLen);
  return E_K_COMM_STATUS_OK;
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
  M_UNUSED(xpComInfo);
  M_UNUSED(xpBuffer);
  M_UNUSED(xpBufferLen);
  return E_K_COMM_STATUS_OK;
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
  M_UNUSED(xpComInfo);
  return E_K_COMM_STATUS_OK;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @implements lConvertSocketIp
 *
 **/
#if 0
static TBoolean lConvertSocketIp
(
  const uint8_t*  xpAddress,
  TKSocketIp*     xpSocketIp
)
{
  return E_FALSE;
}

#endif
/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

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
/** \brief Socket communication - Baremetal/Custom TCP/IP stack implementation.
 *
 *  \author Kudelski IoT
 *
 *  \date 2025/01/08
 *
 *  \file k_sal_com.c
 *
 *  This is a template for baremetal implementations using custom TCP/IP stacks
 *  such as Microchip WINC WiFi, custom Ethernet drivers, or other embedded
 *  network stacks. Users must adapt this to their specific hardware/stack.
 ******************************************************************************/
/**
 * @brief Baremetal socket communication template implementation.
 */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_com.h"
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------------- */
/* PLATFORM-SPECIFIC IMPORTS                                                  */
/* -------------------------------------------------------------------------- */
/* TODO: Include your network stack headers here
 * Examples for different platforms:
 *
 * Microchip WINC WiFi:
 * #include "system/wifi/sys_wincs_wifi_service.h"
 * #include "system/net/sys_wincs_net_service.h"
 *
 * Custom Ethernet:
 * #include "ethernet_driver.h"
 * #include "tcp_stack.h"
 *
 * ESP32:
 * #include "esp_wifi.h"
 * #include "esp_netif.h"
 */

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Invalid Socket Id. */
#define C_SAL_COM_SOCKET_INVALID            (-1)

/** @brief Magic number to validate pointer */
#define C_SAL_COM_MAGIC_NUMBER              (0x53414C32U)  /* "SAL2" */

/** @brief Connection state flags */
#define C_SAL_COM_STATE_INITIALIZED         (0x01U)
#define C_SAL_COM_STATE_CONNECTED           (0x02U)

/* TODO: Define your platform-specific socket constants */

/* -------------------------------------------------------------------------- */
/* TYPES & STRUCTURES                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Communication info structure.
 */
typedef struct
{
  int           socketId;           /**< Socket handle (platform-specific) */
  uint32_t      connectTimeOut;     /**< Connection timeout in ms */
  uint32_t      readTimeOut;        /**< Read timeout in ms */
  uint32_t      state;              /**< Connection state flags */
  uint32_t      magicNumber;        /**< Validation magic number */
  /* TODO: Add platform-specific fields as needed */
} TKComInfo;

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */

/** @brief Global communication info structure */
static TKComInfo g_comInfo = {C_SAL_COM_SOCKET_INVALID, 0U, 0U, 0U, 0U};

/* TODO: Add platform-specific state variables */

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Validate com info pointer.
 *
 * @param[in] xpComInfo Pointer to validate. Should not be NULL.
 *
 * @return true if valid, false otherwise.
 */
static bool lIsValidComInfo
(
  const void* xpComInfo
);

/* TODO: Add platform-specific helper function prototypes
 * Examples:
 * - static TKCommStatus lWaitForNetworkReady(uint32_t xTimeoutMs);
 * - static TKCommStatus lHandleNetworkEvents(void);
 * - static void lSocketEventCallback(int socket, int event);
 */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize COM (Baremetal implementation).
 *
 * @param[in] xConnectTimeoutInMs Connection timeout in milliseconds.
 * @param[in] xReadTimeoutInMs Read timeout in milliseconds.
 * @param[out] xppComInfo Pointer to com info data. Should not be NULL.
 *
 * @return E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComInit
(
  uint32_t  xConnectTimeoutInMs,
  uint32_t  xReadTimeoutInMs,
  void**    xppComInfo
)
{
  TKCommStatus xStatus = E_K_COMM_STATUS_ERROR;

  if (NULL == xppComInfo)
  {
    xStatus = E_K_COMM_STATUS_PARAMETER;
  }
  else if (C_SAL_COM_MAGIC_NUMBER == g_comInfo.magicNumber)
  {
    /* Already initialized */
    xStatus = E_K_COMM_STATUS_OK;
    *xppComInfo = &g_comInfo;
  }
  else
  {
    /* TODO: Initialize your network stack
     * Examples:
     * - Initialize WiFi driver
     * - Initialize Ethernet controller
     * - Register callbacks
     * - Configure network interface
     */
    
    g_comInfo.socketId = C_SAL_COM_SOCKET_INVALID;
    g_comInfo.connectTimeOut = xConnectTimeoutInMs;
    g_comInfo.readTimeOut = xReadTimeoutInMs;
    g_comInfo.state = C_SAL_COM_STATE_INITIALIZED;
    g_comInfo.magicNumber = C_SAL_COM_MAGIC_NUMBER;
    *xppComInfo = &g_comInfo;
    xStatus = E_K_COMM_STATUS_OK;
  }

  return xStatus;
}

/**
 * @brief Establish connection with server (Baremetal implementation).
 *
 * @param[in] xpComInfo Com Info data. Should not be NULL.
 * @param[in] xpHost Server Host name. Should not be NULL. Must have '\0' at the end.
 * @param[in] xpPort Server Port. Should not be NULL. Must have '\0' at the end.
 *
 * @return E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComConnect
(
  void*          xpComInfo,
  const uint8_t* xpHost,
  const uint8_t* xpPort
)
{
  TKComInfo*    xpInfo = (TKComInfo*)xpComInfo;
  TKCommStatus  xStatus = E_K_COMM_STATUS_ERROR;

  if ((NULL == xpComInfo) || (NULL == xpHost) || (NULL == xpPort) ||
      (false == lIsValidComInfo(xpComInfo)))
  {
    xStatus = E_K_COMM_STATUS_PARAMETER;
  }
  else if (0U != (xpInfo->state & C_SAL_COM_STATE_CONNECTED))
  {
    /* Already connected */
    xStatus = E_K_COMM_STATUS_ERROR;
  }
  else
  {
    /* TODO: Implement connection logic for your platform
     * 
     * Example steps:
     * 1. Create socket using platform API
     * 2. Configure socket options (timeouts, etc.)
     * 3. Resolve hostname to IP address (DNS lookup)
     * 4. Connect to server
     * 5. Wait for connection establishment
     * 6. Update state flags
     *
     * Example (Microchip WINC):
     * - Call SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_TCP_OPEN, &config)
     * - Wait for connection callback
     * - Store socket ID
     * 
     * Example (Custom stack):
     * - socket_id = tcp_socket_create();
     * - tcp_socket_connect(socket_id, host, port);
     * - tcp_wait_connected(socket_id, timeout);
     */
    
    /* Placeholder implementation */
    xpInfo->socketId = C_SAL_COM_SOCKET_INVALID;
    xStatus = E_K_COMM_STATUS_RESOURCE;
  }

  return xStatus;
}

/**
 * @brief Send data to the server (Baremetal implementation).
 *
 * @param[in] xpComInfo Com info data. Should not be NULL.
 * @param[in] xpBuffer Data buffer to send. Should not be NULL.
 * @param[in] xBufferLen Size of the data buffer, in bytes.
 *
 * @return E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComWrite
(
  void*          xpComInfo,
  const uint8_t* xpBuffer,
  size_t         xBufferLen
)
{
  TKComInfo*    xpInfo = (TKComInfo*)xpComInfo;
  TKCommStatus  xStatus = E_K_COMM_STATUS_ERROR;

  if ((NULL == xpComInfo) || (NULL == xpBuffer) || (0U == xBufferLen) ||
      (false == lIsValidComInfo(xpComInfo)))
  {
    xStatus = E_K_COMM_STATUS_PARAMETER;
  }
  else if (0U == (xpInfo->state & C_SAL_COM_STATE_CONNECTED))
  {
    xStatus = E_K_COMM_STATUS_ERROR;
  }
  else
  {
    /* TODO: Implement data send for your platform
     * 
     * Example:
     * - bytes_sent = tcp_socket_send(socket_id, buffer, length);
     * - Handle partial sends in loop
     * - Check for errors (disconnection, timeout)
     * 
     * Example (Microchip WINC):
     * - SYS_WINCS_NET_TcpSockWrite(socket_id, buffer, length);
     * - Wait for send complete event
     */
    
    xStatus = E_K_COMM_STATUS_NETWORK;
  }

  return xStatus;
}

/**
 * @brief Read data from the server (Baremetal implementation).
 *
 * @param[in] xpComInfo Com info data. Should not be NULL.
 * @param[out] xpBuffer Buffer to store received data. Should not be NULL.
 * @param[in,out] xpBufferLen In: Buffer size, Out: Bytes received.
 *
 * @return E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComRead
(
  void*    xpComInfo,
  uint8_t* xpBuffer,
  size_t*  xpBufferLen
)
{
  TKComInfo*    xpInfo = (TKComInfo*)xpComInfo;
  TKCommStatus  xStatus = E_K_COMM_STATUS_ERROR;

  if ((NULL == xpComInfo) || (NULL == xpBuffer) || (NULL == xpBufferLen) ||
      (0U == *xpBufferLen) || (false == lIsValidComInfo(xpComInfo)))
  {
    xStatus = E_K_COMM_STATUS_PARAMETER;
  }
  else if (0U == (xpInfo->state & C_SAL_COM_STATE_CONNECTED))
  {
    xStatus = E_K_COMM_STATUS_ERROR;
  }
  else
  {
    /* TODO: Implement data receive for your platform
     * 
     * Example:
     * - bytes_received = tcp_socket_recv(socket_id, buffer, max_length);
     * - Check for timeout
     * - Check for connection closed (0 bytes)
     * - Update *xpBufferLen with actual bytes received
     * 
     * Example (Microchip WINC):
     * - Wait for read event callback
     * - SYS_WINCS_NET_TcpSockRead(socket_id, buffer, length);
     */
    
    *xpBufferLen = 0U;
    xStatus = E_K_COMM_STATUS_TIMEOUT;
  }

  return xStatus;
}

/**
 * @brief Terminate COM and close connection (Baremetal implementation).
 *
 * @param[in] xpComInfo Com info data. Should not be NULL.
 *
 * @return E_K_COMM_STATUS_OK or the error status.
 */
K_SAL_API TKCommStatus salComTerm
(
  void* xpComInfo
)
{
  TKComInfo*    xpInfo = (TKComInfo*)xpComInfo;
  TKCommStatus  xStatus = E_K_COMM_STATUS_ERROR;

  if ((NULL == xpComInfo) || (false == lIsValidComInfo(xpComInfo)))
  {
    xStatus = E_K_COMM_STATUS_PARAMETER;
  }
  else
  {
    /* TODO: Implement socket close for your platform
     * 
     * Example:
     * - tcp_socket_close(socket_id);
     * - Wait for close complete
     * - Cleanup resources
     * 
     * Example (Microchip WINC):
     * - SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_CLOSE, &socket_id);
     */
    
    if ((0U != (xpInfo->state & C_SAL_COM_STATE_CONNECTED)) &&
        (C_SAL_COM_SOCKET_INVALID != xpInfo->socketId))
    {
      /* Close socket */
    }

    /* Reset state */
    xpInfo->socketId = C_SAL_COM_SOCKET_INVALID;
    xpInfo->state = 0U;
    xpInfo->magicNumber = 0U;
    
    xStatus = E_K_COMM_STATUS_OK;
  }

  return xStatus;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Validate com info pointer.
 *
 * @param[in] xpComInfo Pointer to validate. Should not be NULL.
 *
 * @return true if valid, false otherwise.
 */
static bool lIsValidComInfo
(
  const void* xpComInfo
)
{
  const TKComInfo* xpInfo = (const TKComInfo*)xpComInfo;
  bool xIsValid = false;

  if ((NULL != xpComInfo) && (C_SAL_COM_MAGIC_NUMBER == xpInfo->magicNumber))
  {
    xIsValid = true;
  }

  return xIsValid;
}

/* -------------------------------------------------------------------------- */
/* TODO: Implement platform-specific helper functions                         */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

/*******************************************************************************
  WINCS Host Assisted Net Service Implementation

  File Name:
    sys_wincs_net_service.c

  Summary:
    Source code for the WINCS Host Assisted Net Service implementation.

  Description:
    This file contains the source code for the WINCS Host Assisted Net Service
    implementation.
 *******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (C) 2020 released Microchip Technology Inc.  All rights reserved.

 
Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
//DOM-IGNORE-END


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This section lists the other files that are included in this file.
 */
#include "system/net/sys_wincs_net_service.h"
#include "system/wifi/sys_wincs_wifi_service.h"


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

//    Array to store socket callback handlers.
static SYS_WINCS_NET_SOCK_CALLBACK_t  g_SocketCallBackHandler[SYS_WINCS_NET_SOCK_SERVICE_CB_MAX];

//    Variable to store the DHCP callback handler.
static SYS_WINCS_NET_DHCP_CALLBACK_t  g_DHCPCallBackHandler;

//    Array to store connection IDs.
static SYS_WINCS_NET_CID_TYPE         g_connIDs[SYS_WINCS_NET_NUM_CONN_IDS];

//    Variable to store the TLS handle.
static WDRV_WINC_TLS_HANDLE           g_tlsHandle = WDRV_WINC_TLS_INVALID_HANDLE;


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */
/**
 * @brief Add a Connection ID
 *
 * Summary:
 *    This function adds a new connection ID to the global connection ID array.
 *
 * Description:
 *    This function searches for an available slot in the global connection ID array,
 *    initializes it with the provided parameters, and returns a pointer to the newly
 *    added connection ID structure.
 *
 * Parameters:
 *    @param sockfd          Socket file descriptor.
 *    @param socketType      Type of the socket.
 *    @param pLocalBindAddr  Pointer to the local bind address (can be NULL).
 *    @param pRemoteBindAddr Pointer to the remote bind address (can be NULL).
 *    @param httpChkSum      Flag indicating whether HTTP checksum is enabled.
 *
 * Returns:
 *    SYS_WINCS_NET_CID_TYPE*  Pointer to the newly added connection ID structure,
 *                             or NULL if no available slot was found.
 */
static SYS_WINCS_NET_CID_TYPE* SYS_WINCS_NET_ConnIDAdd
(
    int sockfd, 
    SYS_WINCS_NET_SOCKET_TYPE socketType, 
    SYS_WINCS_NET_SOCK_ADDR_TYPE *pLocalBindAddr, 
    SYS_WINCS_NET_SOCK_ADDR_TYPE *pRemoteBindAddr, 
    bool httpChkSum
)
{
    uint8_t i;

    for (i = 0; i < SYS_WINCS_NET_NUM_CONN_IDS; i++)
    {
        if (false == g_connIDs[i].inUse)
        {
            memset(&g_connIDs[i], 0, sizeof(SYS_WINCS_NET_CID_TYPE));

            g_connIDs[i].inUse            = true;
            g_connIDs[i].id               = i + 1;
            g_connIDs[i].sockfd           = sockfd;
            g_connIDs[i].socketType       = socketType;
            g_connIDs[i].doHTTPChecksum   = httpChkSum;

            if (NULL != pLocalBindAddr)
            {
                memcpy(&g_connIDs[i].localBindAddress, pLocalBindAddr, sizeof(SYS_WINCS_NET_SOCK_ADDR_TYPE));
            }

            if (NULL != pRemoteBindAddr)
            {
                memcpy(&g_connIDs[i].remoteBindAddress, pRemoteBindAddr, sizeof(SYS_WINCS_NET_SOCK_ADDR_TYPE));
            }

            #ifdef SYS_WINCS_NET_DEBUG_LOGS
            SYS_WINCS_NET_DBG_MSG("Adding CID %d\r\n", g_connIDs[i].id);
            #endif

            return &g_connIDs[i];
        }
    }

    return NULL;
}

/**
 * @brief Find Connection ID by Socket
 *
 * Summary:
 *    This function finds a connection ID in the global connection ID array by socket file descriptor.
 *
 * Description:
 *    This function searches the global connection ID array for a connection ID that matches
 *    the provided socket file descriptor and returns a pointer to the corresponding connection
 *    ID structure.
 *
 * Parameters:
 *    @param sockfd  Socket file descriptor.
 *
 * Returns:
 *    SYS_WINCS_NET_CID_TYPE*  Pointer to the found connection ID structure,
 *                             or NULL if no matching connection ID was found.
 */
static SYS_WINCS_NET_CID_TYPE* SYS_WINCS_NET_ConnIDFindBySocket
(
    int sockfd
)
{
    uint8_t i;

    for (i = 0; i < SYS_WINCS_NET_NUM_CONN_IDS; i++)
    {
        if ((true == g_connIDs[i].inUse) && (g_connIDs[i].sockfd == sockfd))
        {
            return &g_connIDs[i];
        }
    }

    return NULL;
}

/**
 * @brief Delete Connection ID by Socket
 *
 * Summary:
 *    This function deletes a connection ID from the global connection ID array by socket file descriptor.
 *
 * Description:
 *    This function searches the global connection ID array for a connection ID that matches
 *    the provided socket file descriptor, clears the corresponding entry, and returns a flag
 *    indicating whether the deletion was successful.
 *
 * Parameters:
 *    @param sockfd  Socket file descriptor.
 *
 * Returns:
 *    bool  True if the connection ID was successfully deleted, false otherwise.
 */
static bool SYS_WINCS_NET_ConnIDDeleteBySocket
(
    int sockfd
)
{
    uint8_t i;

    for (i = 0; i < SYS_WINCS_NET_NUM_CONN_IDS; i++)
    {
        if ((true == g_connIDs[i].inUse) && (g_connIDs[i].sockfd == sockfd))
        {
            memset(&g_connIDs[i], 0, sizeof(SYS_WINCS_NET_CID_TYPE));
            return true;
        }
    }

    return false;
}



#ifdef SYS_WINCS_NET_DEBUG_LOGS
static void SYS_WINCS_NET_connIDPrint
(
    void
)
{
    uint8_t i;
    uint8_t n = 0;

    SYS_WINCS_NET_DBG_MSG("Available CIDs:\r\n");

    for (i=0; i<SYS_WINCS_NET_NUM_CONN_IDS; i++)
    {
        if (true == g_connIDs[i].inUse)
        {
            in_port_t port;
            char addrStr[64] = {""};

            if (0 != (g_connIDs[i].socketType & SYS_WINCS_NET_SKT_SERVER))
            {
                port = ntohs(g_connIDs[i].localBindAddress.sin_port);
            }
            else
            {
                port = ntohs(g_connIDs[i].remoteBindAddress.sin_port);
            }

            if (AF_INET == g_connIDs[i].remoteBindAddress.sin_family)
            {
                inet_ntop(AF_INET, &g_connIDs[i].remoteBindAddress.v4.sin_addr.s_addr, addrStr, sizeof(addrStr));
            }
            else
            {
                inet_ntop(AF_INET6, &g_connIDs[i].remoteBindAddress.v6.sin6_addr.s6_addr, addrStr, sizeof(addrStr));
            }

            SYS_WINCS_NET_DBG_MSG("CID: %d, sock:%d, port:%u, ip: %s\r\n", g_connIDs[i].id, g_connIDs[i].sockfd, port, addrStr);
            n++;
        }
    }

    if (0 == n)
    {
        SYS_WINCS_NET_DBG_MSG("The list is empty\r\n");
    }
}
#endif

/**
 * @brief Socket Event Callback
 *
 * Summary:
 *    This function is a callback that handles socket events.
 *
 * Description:
 *    This function is called when a socket event occurs. It processes the event
 *    based on the event type and the associated socket file descriptor.
 *
 * Parameters:
 *    @param context  User-defined context information.
 *    @param sockfd   Socket file descriptor associated with the event.
 *    @param event    Type of the socket event.
 *
 * Returns:
 *    None.
 */
            
static void SYS_WINCS_NET_SocketEventCallback
(
    uintptr_t context, 
    int sockfd, 
    WINC_SOCKET_EVENT event, 
    WINC_SOCKET_STATUS status
)
{
    SYS_WINCS_NET_CID_TYPE *pConnID = SYS_WINCS_NET_ConnIDFindBySocket(sockfd);
    SYS_WINCS_NET_SOCK_CALLBACK_t net_cb_func = g_SocketCallBackHandler[0];
    
    
    #ifdef SYS_WINCS_NET_DEBUG_LOGS
    SYS_WINCS_NET_DBG_MSG("f SYS_WINCS_NET_SocketEventCallback, sockfd : %d event : %d\r\n",sockfd,event);
    #endif
    if(event ==  WINC_SOCKET_EVENT_CLOSE)
    {
        net_cb_func(sockfd, SYS_WINCS_NET_SOCK_EVENT_CLOSED, NULL);
        return;
    }
    
    if (NULL == pConnID)
    {
        SYS_WINCS_NET_DBG_MSG("ERROR : Net Callback NULL");
        return;
    }
    
    if (WINC_SOCKET_STATUS_OK != status)
    {
        SYS_WINCS_NET_DBG_MSG("ERROR: Socket error %d from event %d\r\n", status, event);
        SYS_WINCS_NET_ConnIDDeleteBySocket(sockfd);
        shutdown(sockfd, SHUT_RDWR);
        return;
    }

    switch (event)
    {
        case WINC_SOCKET_EVENT_OPEN:
        {
            if (PF_UNSPEC == pConnID->remoteBindAddress.sin_family)
            {
                if (-1 == bind(sockfd, (struct sockaddr*)&pConnID->localBindAddress, sizeof(SYS_WINCS_NET_SOCK_ADDR_TYPE)))
                {
                    SYS_WINCS_NET_DBG_MSG("ERROR: Failed to bind socket\r\n");
                    SYS_WINCS_NET_ConnIDDeleteBySocket(sockfd);
                    shutdown(sockfd, SHUT_RDWR);
                    break;
                }

                if (SYS_WINCS_NET_SKT_STREAM == (pConnID->socketType & SYS_WINCS_NET_SKT_STREAM))
                {
                    listen(sockfd, SYS_WINCS_NET_NO_OF_CLIENT_SOCKETS);
                }

                if (SYS_WINCS_NET_SKT_DATAGRAM == (pConnID->socketType & SYS_WINCS_NET_SKT_DATAGRAM))
                {
                    pConnID->isConnected = true;
                    #ifdef SYS_WINCS_NET_DEBUG_LOGS
                    SYS_WINCS_NET_DBG_MSG("Socket %d session ID = %d\r\n", sockfd, 0);
                    #endif
                }
            }
            else
            {
                if(SYS_WINCS_NET_SKT_MULTICAST == (pConnID->socketType & SYS_WINCS_NET_SKT_MULTICAST))
                {
                    if (AF_INET == pConnID->remoteBindAddress.sin_family)
                    {
                        pConnID->localBindAddress.v4.sin_addr.s_addr = htonl(INADDR_ANY);
                    }
                    else
                    {
                        memcpy(&pConnID->localBindAddress.v6.sin6_addr, &in6addr_any, sizeof(in6addr_any));
                    }

                    pConnID->localBindAddress.sin_family = pConnID->remoteBindAddress.sin_family;
                    pConnID->localBindAddress.sin_port   = pConnID->remoteBindAddress.sin_port;

                    if (-1 == bind(sockfd, (struct sockaddr*)&pConnID->localBindAddress, sizeof(SYS_WINCS_NET_SOCK_ADDR_TYPE)))
                    {
                        SYS_WINCS_NET_DBG_MSG("ERROR: Failed to bind socket\r\n");
                        SYS_WINCS_NET_ConnIDDeleteBySocket(sockfd);
                        shutdown(sockfd, SHUT_RDWR);
                        break;
                    }

                    if (AF_INET == pConnID->remoteBindAddress.sin_family)
                    {
                        struct ip_mreqn group;
                        memcpy(&group.imr_multiaddr, &pConnID->remoteBindAddress.v4.sin_addr, sizeof(struct in_addr));
                        memcpy(&group.imr_address, &pConnID->localBindAddress.v4.sin_addr, sizeof(struct in_addr));

                        group.imr_ifindex = 0;
                        setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group));
                    }
                    else
                    {
                        struct ipv6_mreq group;
                        memcpy(&group.ipv6mr_multiaddr, &pConnID->remoteBindAddress.v6.sin6_addr, sizeof(struct in6_addr));
                        group.ipv6mr_interface = 0;
                        setsockopt(sockfd, IPPROTO_IP, IPV6_ADD_MEMBERSHIP, &group, sizeof(group));
                    }
                    pConnID->isConnected = true;
                }
                else
                {
                    if(SYS_WINCS_NET_SKT_ENCRYPTED == (pConnID->socketType & SYS_WINCS_NET_SKT_ENCRYPTED))
                    {
                        if(0 == setsockopt(sockfd, IPPROTO_TLS, TLS_CONF_IDX, &g_tlsHandle, sizeof(int)))
                        {
                            SYS_WINCS_NET_DBG_MSG("setsockopt SUCCESS\r\n");
                        }
                        else
                        {
                            SYS_WINCS_NET_DBG_MSG("setsockopt FAIL  errno : %d\r\n",errno);
                        }
                    }
                    errno = 0;

                    if (-1 == connect(sockfd, (struct sockaddr*)&pConnID->remoteBindAddress, sizeof(SYS_WINCS_NET_SOCK_ADDR_TYPE)))
                    {
                        if (EINPROGRESS != errno)
                        {
                            SYS_WINCS_NET_DBG_MSG("ERROR: Failed to connect to the server!\r\n");
                            SYS_WINCS_NET_ConnIDDeleteBySocket(sockfd);
                            shutdown(sockfd, SHUT_RDWR);
                            break;
                        }
                    }
                }
            }

            break;
        }

        case WINC_SOCKET_EVENT_LISTEN:
        {
            if(SYS_WINCS_NET_SKT_MULTICAST != (pConnID->socketType & SYS_WINCS_NET_SKT_MULTICAST))
            {
                #ifdef SYS_WINCS_NET_DEBUG_LOGS
                SYS_WINCS_NET_DBG_MSG("Socket %d session ID = %d\r\n", sockfd, 0);
                #endif
                
                #ifdef SYS_WINCS_NET_DEBUG_LOGS
                SYS_WINCS_NET_connIDPrint();
                #endif
                SYS_WINCS_NET_DBG_MSG("Socket Listening......\r\n");
            }
            break;
        }

        case WINC_SOCKET_EVENT_CONNECT_REQ:
        {
            #ifdef SYS_WINCS_NET_DEBUG_LOGS
            SYS_WINCS_NET_DBG_MSG("WINC_SOCKET_EVENT_CONNECT_REQ\r\n");
            #endif
            int newSocket;
            SYS_WINCS_NET_SOCK_ADDR_TYPE addr;
            socklen_t addrLen = sizeof(SYS_WINCS_NET_SOCK_ADDR_TYPE);

            errno = 0;
            newSocket = accept(sockfd, (struct sockaddr*)&addr, &addrLen);

            if (-1 == newSocket)
            {
                if ((EWOULDBLOCK == errno) || (EAGAIN == errno))
                {
                    break;
                }

                SYS_WINCS_NET_DBG_MSG("Accept error!\r\n");
                shutdown(sockfd, SHUT_RDWR);
                break;
            }

            pConnID = SYS_WINCS_NET_ConnIDAdd(newSocket, pConnID->socketType, &pConnID->localBindAddress, &addr, false);

            if (NULL != pConnID)
            {
                #ifdef SYS_WINCS_NET_DEBUG_LOGS
                SYS_WINCS_NET_connIDPrint();
                #endif
                pConnID->isConnected = true;
            }
            net_cb_func(sockfd, SYS_WINCS_NET_SOCK_EVENT_CLIENT_CONNECTED, NULL);
            break;
        }

        case WINC_SOCKET_EVENT_CONNECT:
        {
            #ifdef SYS_WINCS_NET_DEBUG_LOGS
            SYS_WINCS_NET_DBG_MSG("WINC_SOCKET_EVENT_CONNECT\r\n");
            #endif
            if(SYS_WINCS_NET_SKT_ENCRYPTED != (pConnID->socketType & SYS_WINCS_NET_SKT_ENCRYPTED))
            {
                pConnID->isConnected = true;

                #ifdef SYS_WINCS_NET_DEBUG_LOGS
                SYS_WINCS_NET_connIDPrint();
                #endif
                net_cb_func(sockfd, SYS_WINCS_NET_SOCK_EVENT_CONNECTED, NULL);
            }
            break;
        }

        case WINC_SOCKET_EVENT_TLS_CONNECT:
        {
            SYS_WINCS_NET_DBG_MSG("WINC_SOCKET_EVENT_TLS_CONNECT\r\n");
            pConnID->isConnected = true;
            #ifdef SYS_WINCS_NET_DEBUG_LOGS
            SYS_WINCS_NET_connIDPrint();
            #endif
            net_cb_func(sockfd, SYS_WINCS_NET_SOCK_EVENT_TLS_DONE, NULL);
            break;
        }
        
        case WINC_SOCKET_EVENT_SEND:
        {
             net_cb_func(sockfd, SYS_WINCS_NET_SOCK_EVENT_SEND_COMPLETE, NULL);
            #ifdef SYS_WINCS_NET_DEBUG_LOGS
            SYS_WINCS_NET_DBG_MSG("Socket %d send completed\r\n", sockfd);
            #endif
            
            break;
        }
        
        case WINC_SOCKET_EVENT_RECV:
        {
            #ifdef SYS_WINCS_NET_DEBUG_LOGS
            SYS_WINCS_NET_DBG_MSG("WINC_SOCKET_EVENT_RECV from soc : %d\r\n",sockfd);
            #endif
            
            net_cb_func(sockfd, SYS_WINCS_NET_SOCK_EVENT_READ, NULL);
            break;
        }

        case WINC_SOCKET_EVENT_ERROR:
        {
            net_cb_func(sockfd, SYS_WINCS_NET_SOCK_EVENT_ERROR, NULL);
            break;
        }
        

        default:
        {
            break;
        }
    }
}

/**
 * @brief DHCP Server Event Callback
 *
 * Summary:
 *    This function is a callback that handles DHCP server events.
 *
 * Description:
 *    This function is called when a DHCP server event occurs. It processes the event
 *    based on the event type and the associated event information.
 *
 * Parameters:
 *    @param handle      Handle to the driver instance.
 *    @param eventType   Type of the DHCP server event.
 *    @param pEventInfo  Pointer to the event information.
 *
 * Returns:
 *    None.
 */

static void SYS_WINCS_NET_ApDhcpsEventCallback
(
    DRV_HANDLE handle, 
    WDRV_WINC_DHCPS_EVENT_TYPE eventType, 
    void *pEventInfo
)
{
    SYS_WINCS_NET_DHCP_CALLBACK_t net_dhcp_func = g_DHCPCallBackHandler;
    switch (eventType)
    {
        case WDRV_WINC_DHCPS_EVENT_LEASE_ASSIGNED:
        {
            char s[20] = {0};

            WDRV_WINC_UtilsIPAddressToString((WDRV_WINC_IPV4_ADDR*)pEventInfo, s, sizeof(s));
            net_dhcp_func(SYS_WINCS_NET_STA_DHCP_DONE,(uint8_t *) s);
            break;
        }

        default:
        {
            break;
        }
    }
}


/**
 * @brief Create a Network Socket
 *
 * Summary:
 *    This function creates a network socket based on the provided configuration.
 *
 * Description:
 *    This function takes a pointer to a socket configuration structure and
 *    initializes a network socket accordingly. The function returns a result
 *    indicating the success or failure of the socket creation process.
 *
 * Parameters:
 *    @param socketCfg  Pointer to the socket configuration structure.
 *
 * Returns:
 *    SYS_WINCS_RESULT_t  The result of the socket creation process.
 *                        Possible values include:
 *                        - SYS_WINCS_RESULT_SUCCESS: Socket created successfully.
 *                        - SYS_WINCS_RESULT_FAILURE: Failed to create socket.
 */

static SYS_WINCS_RESULT_t SYS_WINCS_NET_CreateSocket
(
    SYS_WINCS_NET_SOCKET_t *socketCfg
)
{
    int domain = AF_UNSPEC;
    bool httpChkSum = false;
    SYS_WINCS_NET_SOCK_ADDR_TYPE addr;
    SYS_WINCS_NET_SOCKET_TYPE socketType = SYS_WINCS_NET_SKT_UNENCRYPTED;
    static WDRV_WINC_IP_MULTI_ADDRESS IPAddress;

    SYS_WINCS_RESULT_t result = SYS_WINCS_FAIL;
           
    /* socket type : TLS or Non TLS*/
    if(socketCfg->tlsEnable)
    {
        socketType &= ~SYS_WINCS_NET_SKT_UNENCRYPTED;
        socketType |= SYS_WINCS_NET_SKT_ENCRYPTED;
    }
    else
    {
        socketType = SYS_WINCS_NET_SKT_UNENCRYPTED;
    }
    
    /* SOcket type : TCP or UDP */
    if(socketCfg->sockType == SYS_WINCS_NET_SOCK_TYPE_TCP)
    {
        socketType |= SYS_WINCS_NET_SKT_STREAM;
    }
    else if (socketCfg->sockType == SYS_WINCS_NET_SOCK_TYPE_UDP)
    {
        socketType |= SYS_WINCS_NET_SKT_DATAGRAM;
    }
    
    /* SOcket type :Client or Server  */
    if(socketCfg->bindType == SYS_WINCS_NET_BIND_REMOTE) //client
    {
        socketType |= SYS_WINCS_NET_SKT_CLIENT;
    }
    else if (socketCfg->bindType == SYS_WINCS_NET_BIND_LOCAL)//server
    {
        socketType |= SYS_WINCS_NET_SKT_SERVER;
    }
    else if (socketCfg->bindType == SYS_WINCS_NET_BIND_MCAST)//multicast
    {
        socketType |= SYS_WINCS_NET_SKT_MULTICAST;
    }

    /* Addr type : IPV4 or IPV6*/
    if(socketCfg->ipType == SYS_WINCS_NET_IPV4)
    {
        domain = AF_INET;
        if(socketCfg->sockAddr != NULL)
        {
            if (1 != inet_pton(AF_INET, socketCfg->sockAddr, &IPAddress.v4))
            {
                return result;
            }
        }
        else
        {
            IPAddress.v4.Val = 0;
        }
    }
    else
    {
        domain = AF_INET6;
        if(socketCfg->sockAddr != NULL)
        {
            if(1 != inet_pton(AF_INET6, socketCfg->sockAddr, &IPAddress.v6))
            {
                return result;
            }
        }
        else
        {
        }
    }
    
    /* Create TCP or UDP socket*/
    if(socketCfg->sockType == SYS_WINCS_NET_SOCK_TYPE_TCP)
    {
        if(socketCfg->tlsEnable)
        {
            socketCfg->sockID = socket(domain, SOCK_STREAM, IPPROTO_TLS);
        }
        else
        {
            socketCfg->sockID = socket(domain, SOCK_STREAM, IPPROTO_TCP);
        }
    }
    else if(socketCfg->sockType == SYS_WINCS_NET_SOCK_TYPE_UDP)
    {
        socketCfg->sockID = socket(domain, SOCK_DGRAM, IPPROTO_UDP);
    }
    
    if (socketCfg->sockID < 0)
    {
        return SYS_WINCS_FAIL;
    }
    
    if (AF_INET == domain)
    {
        if (0 == IPAddress.v4.Val)
        {
            addr.v4.sin_family      = domain;
            addr.v4.sin_port        = htons(socketCfg->sockPort);
            addr.v4.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else
        {
            addr.v4.sin_family      = domain;
            addr.v4.sin_port        = htons(socketCfg->sockPort);
            addr.v4.sin_addr.s_addr = IPAddress.v4.Val;
        }
    }
    else
    {
        if (0 == memcmp(&IPAddress.v6, &in6addr_any, sizeof(in6addr_any)))
        {
            addr.v6.sin6_family = AF_INET6;
            addr.v6.sin6_port   = htons(socketCfg->sockPort);
            memcpy(&addr.v6.sin6_addr, &in6addr_any, sizeof(in6addr_any));
        }
        else
        {
            addr.v6.sin6_family = AF_INET6;
            addr.v6.sin6_port   = htons(socketCfg->sockPort);
            memcpy(&addr.v6.sin6_addr, &IPAddress.v6, sizeof(IPAddress.v6));
        }
    }
    
    /* SOcket type :Client or Server  */
    if(socketCfg->bindType == SYS_WINCS_NET_BIND_REMOTE) //client
    {
        if (NULL == SYS_WINCS_NET_ConnIDAdd(socketCfg->sockID, socketType, NULL, &addr, httpChkSum))
        {
            shutdown(socketCfg->sockID, SHUT_RDWR);
            return SYS_WINCS_FAIL;
        }
    }
    else if (socketCfg->bindType == SYS_WINCS_NET_BIND_LOCAL)//server
    {
        if (NULL == SYS_WINCS_NET_ConnIDAdd(socketCfg->sockID, socketType, &addr, NULL, httpChkSum))
        {
            shutdown(socketCfg->sockID, SHUT_RDWR);
            return SYS_WINCS_FAIL;
        }
    }
    
    return SYS_WINCS_PASS;
}



/*This function is used to write into TCP Socket*/
SYS_WINCS_RESULT_t SYS_WINCS_NET_TcpSockWrite
( 
    uint32_t socket, 
    uint16_t length, 
    uint8_t *input
)
{
    SYS_WINCS_RESULT_t result = SYS_WINCS_PASS;

    int16_t sent_data_len = 0;
    errno = 0;
    
    sent_data_len = send(socket, input, length, 0);
    if (-1 == sent_data_len)
    {
        #ifdef SYS_WINCS_NET_DEBUG_LOGS
        SYS_WINCS_NET_DBG_MSG("ERROR: Failed to send to socket %d (%d)\r\n", socket, errno);
        #endif
        result = SYS_WINCS_FAIL;
    }
    return result;
}


/*This function is used to read from the socket*/
int16_t SYS_WINCS_NET_SockRead
(
    uint32_t socket, 
    uint16_t length, 
    uint8_t *buffer
) 
{                
    int16_t recvd_data_len = 0;
    recvd_data_len = recv(socket, buffer, length, 0);

    if (-1 == recvd_data_len)
    {
        if ((EWOULDBLOCK != errno) && (EAGAIN != errno))
        {
            #ifdef SYS_WINCS_NET_DEBUG_LOGS
            SYS_WINCS_NET_DBG_MSG("Socket recv error %d %d\r\n",socket, errno);
            SYS_WINCS_NET_ConnIDDeleteBySocket(socket);
            #endif
        }
    }
    return recvd_data_len;
}


/*This function is used to read from the TCP socket*/
int16_t SYS_WINCS_NET_TcpSockRead
( 
    uint32_t socket, 
    uint16_t  length, 
    uint8_t *buffer
)  
{
    return SYS_WINCS_NET_SockRead(socket, length, buffer);
}


/*This function is used to read from the UDP socket*/
int16_t SYS_WINCS_NET_UdpSockRead
( 
    uint32_t socket, 
    uint16_t length, 
    uint8_t *buffer
)  
{                   
    return SYS_WINCS_NET_SockRead(socket, length, buffer);
}



/*This function is used to write into UDP Socket*/
SYS_WINCS_RESULT_t SYS_WINCS_NET_UdpSockWrite
(
    uint32_t socket, 
    const char *addr, 
    uint32_t port, 
    uint16_t length, 
    uint8_t *input
)
{
    SYS_WINCS_RESULT_t result = SYS_WINCS_FAIL;
    WDRV_WINC_IP_MULTI_ADDRESS remoteIPAddress;
    SYS_WINCS_NET_SOCK_ADDR_TYPE ip_addr;
        
    if (1 == inet_pton(AF_INET, (const char *)addr, &remoteIPAddress.v4))
    {
        ip_addr.v4.sin_family      = AF_INET;
        ip_addr.v4.sin_port        = htons(port);
        ip_addr.v4.sin_addr.s_addr = remoteIPAddress.v4.Val;
    }
    else if (1 == inet_pton(AF_INET6,(const char *) addr, &remoteIPAddress.v6))
    {
        ip_addr.v6.sin6_family = AF_INET6;
        ip_addr.v6.sin6_port   = htons(port);
        memcpy(&ip_addr.v6.sin6_addr, &remoteIPAddress.v6, sizeof(remoteIPAddress.v6));
    }
    else
    {
        return result;
    }
    
    if (-1 == sendto(socket, input, length, 0, (struct sockaddr*)&ip_addr, sizeof(ip_addr)))
    {
        #ifdef SYS_WINCS_NET_DEBUG_LOGS
        SYS_WINCS_NET_DBG_MSG("ERROR: Failed to send to socket %d (%d)\r\n", socket, errno);
        #endif
        return result;
    }
    return SYS_WINCS_PASS;
}



/* This function is used for Network and Socket service Control*/
SYS_WINCS_RESULT_t SYS_WINCS_NET_SockSrvCtrl
( 
    SYS_WINCS_NET_SOCK_SERVICE_t request, 
    SYS_WINCS_WIFI_HANDLE_t netHandle
)
{
    DRV_HANDLE wdrvHandle = DRV_HANDLE_INVALID;
    
    SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_GET_DRV_HANDLE,&wdrvHandle);
    WDRV_WINC_STATUS status = WDRV_WINC_STATUS_OK;
    
    switch(request)
    {
        /**<Enable the DHCP server */
        case SYS_WINCS_NET_DHCP_SERVER_ENABLE:
        {
            if(netHandle == NULL)
                break;
            
            WDRV_WINC_IP_MULTI_ADDRESS apIPv4Addr;
            WDRV_WINC_IPV4_ADDR dhcpsIPv4PoolAddr;
            
            const char **dhcps_cfg_list = netHandle; 
            
            if (false == WDRV_WINC_UtilsStringToIPAddress(dhcps_cfg_list[0], &apIPv4Addr.v4))
            {
                return SYS_WINCS_FAIL;
            }
            
            if (false == WDRV_WINC_UtilsStringToIPAddress(dhcps_cfg_list[1], &dhcpsIPv4PoolAddr))
            {
                return SYS_WINCS_FAIL;
            }

            status = WDRV_WINC_NetIfIPAddrSet(wdrvHandle, WDRV_WINC_NETIF_IDX_0, WDRV_WINC_IP_ADDRESS_TYPE_IPV4, &apIPv4Addr, 24);
            if(status != WDRV_WINC_STATUS_OK)
            {
                return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
            }

            status = WDRV_WINC_DHCPSPoolAddressSet(wdrvHandle, WDRV_WINC_DHCPS_IDX_0, &dhcpsIPv4PoolAddr);
            if(status != WDRV_WINC_STATUS_OK)
            {
                return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
            } 

            status = WDRV_WINC_DHCPSEnableSet(wdrvHandle, WDRV_WINC_DHCPS_IDX_0, true, SYS_WINCS_NET_ApDhcpsEventCallback);
            if(status != WDRV_WINC_STATUS_OK)
            {
                return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
            }
            
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
	
        /**<Disable the DHCP server */ 
        case SYS_WINCS_NET_DHCP_SERVER_DISABLE:
        {
            status =  WDRV_WINC_DHCPSEnableSet(wdrvHandle, WDRV_WINC_DHCPS_IDX_0, false, NULL);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
	
        /**<Open TCP Socket*/ 
        case SYS_WINCS_NET_SOCK_TCP_OPEN: 
        {  
            WDRV_WINC_SocketRegisterEventCallback(wdrvHandle, &SYS_WINCS_NET_SocketEventCallback);
            if (SYS_WINCS_PASS != SYS_WINCS_NET_CreateSocket((SYS_WINCS_NET_SOCKET_t*)(netHandle)))
            {
                return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_INVALID_ARG, __FUNCTION__, __LINE__);
            }
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__);  
        }
            
        /**<Close the socket*/    
        case SYS_WINCS_NET_SOCK_CLOSE:
        {
            uint32_t socket = *((uint32_t *)netHandle);
            SYS_WINCS_NET_ConnIDDeleteBySocket(socket);
            
            if(0 == shutdown(socket, SHUT_RDWR))
            {
                return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
            }
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__); 
        }
	
        case SYS_WINCS_NET_OPEN_TLS_CTX:
        {
            g_tlsHandle = WDRV_WINC_TLSCtxOpen(wdrvHandle);

            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__); 
        }
        
        case SYS_WINCS_NET_GET_TLS_CTX_HANDLE:
        {
            *(WDRV_WINC_TLS_HANDLE *)netHandle = g_tlsHandle;
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__); 
        }
        
        /**<Use the TLS configurations */
        case SYS_WINCS_NET_TLS_CONFIG:
        {
            SYS_WINCS_NET_TLS_SOC_PARAMS *tls_cfg_list = netHandle;
            if(tls_cfg_list->tlsCACertificate != NULL)
            {
                status = WDRV_WINC_TLSCtxCACertFileSet(wdrvHandle, g_tlsHandle, tls_cfg_list->tlsCACertificate , tls_cfg_list->tlsPeerAuth);
                if (WDRV_WINC_STATUS_OK != status)
                {
                    return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
                }
            }
            
            if(tls_cfg_list->tlsCertificate != NULL)
            {
                status =  WDRV_WINC_TLSCtxCertFileSet(wdrvHandle, g_tlsHandle, tls_cfg_list->tlsCertificate);
                if (WDRV_WINC_STATUS_OK != status)
                {
                    return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
                }
            }
            
            if((tls_cfg_list->tlsKeyName != NULL) && (tls_cfg_list->tlsKeyName != NULL))
            {
                status =  WDRV_WINC_TLSCtxPrivKeySet(wdrvHandle, g_tlsHandle, tls_cfg_list->tlsKeyName, tls_cfg_list->tlsKeyPassword);
                if (WDRV_WINC_STATUS_OK != status)
                {
                    return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
                }
            }
            
            if(tls_cfg_list->tlsServerName != NULL)
            {
                status =  WDRV_WINC_TLSCtxSNISet(wdrvHandle, g_tlsHandle, tls_cfg_list->tlsServerName);
                if (WDRV_WINC_STATUS_OK != status)
                {
                    return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
                }
            }
            
            if(tls_cfg_list->tlsDomainName != NULL)
            {
                status =  WDRV_WINC_TLSCtxHostnameCheckSet(wdrvHandle, g_tlsHandle, tls_cfg_list->tlsDomainName, tls_cfg_list->tlsDomainNameVerify);
                if (WDRV_WINC_STATUS_OK != status)
                {
                    return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
                }
            }
     
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }

        /**<Register application callback for sockets*/
        case SYS_WINCS_NET_SOCK_SET_CALLBACK:
	    {
            g_SocketCallBackHandler[0] = (SYS_WINCS_NET_SOCK_CALLBACK_t)(netHandle);                        
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__);
	    }    
        
        /**<Register application callback for sockets*/
        case SYS_WINCS_NET_SOCK_SET_SRVC_CALLBACK:
	    {
            g_DHCPCallBackHandler  = (SYS_WINCS_NET_DHCP_CALLBACK_t)(netHandle);  
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__);
	    }   
	    
        /*<Get Function callback data*/        
        case SYS_WINCS_NET_SOCK_GET_CALLBACK:
        {
            SYS_WINCS_NET_SOCK_CALLBACK_t *socketCallBackHandler;
            socketCallBackHandler = (SYS_WINCS_NET_SOCK_CALLBACK_t *) netHandle;
            socketCallBackHandler[0] = g_SocketCallBackHandler[0];
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__);
        }
        
        default:
	    {
            SYS_WINCS_NET_DBG_MSG("ERROR : Unknown Net Service Request\r\n");
        }
    } 

    return SYS_WINCS_FAIL;
}

/* *****************************************************************************
 End of File
 */

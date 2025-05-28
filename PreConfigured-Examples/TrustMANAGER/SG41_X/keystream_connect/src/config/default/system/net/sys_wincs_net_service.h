/*******************************************************************************
  WINCS Host Assisted Net Service Header file 

  File Name:
    sys_wincs_net_service.h

  Summary:
    Header file for the WINCS Host Assisted Net Service implementation.

  Description:
    This file contains the header file for the WINCS Host Assisted Net Service
    implementation.
 *******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (C) 2020 released Microchip Technology Inc.  All rights reserved.

 * Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef SYS_WINCS_NET_SERVICE_H
#define	SYS_WINCS_NET_SERVICE_H

#include <xc.h>
#include "wdrv_winc.h"
#include "wdrv_winc_client_api.h"
#include "system/wifi/sys_wincs_wifi_service.h"  


/* Handle for Net Configurations */
typedef void * SYS_WINCS_NET_HANDLE_t;

#define SYS_WINCS_NET_DBG_MSG(args, ...)         SYS_CONSOLE_PRINT("[NET]:"args, ##__VA_ARGS__)

/* Number of clients connecting to the server socket, with the maximum value being WINC_CMD_PARAM_MAX_SOCKET_PEND_SKTS. */
#define SYS_WINCS_NET_NO_OF_CLIENT_SOCKETS       2

/*WINCS Network Socket max callback service */
#define SYS_WINCS_NET_SOCK_SERVICE_CB_MAX        2

#define SYS_WINCS_NET_SOCK_ID_LEN_MAX            8
#define SYS_WINCS_NET_SOCK_ADDR_LEN_MAX          32
#define SYS_WINCS_NET_SOCK_TLS_CFG_LEN_MAX       64

#define SYS_WINCS_NET_NUM_CONN_IDS               10
#define SYS_WINCS_NET_SOCK_RCV_BUF_SIZE          2048

// *****************************************************************************
// Network and Socket Service List
//
// Summary:
//    Enumeration of network and socket services.
//
// Description:
//    This enumeration defines various services related to network and socket
//    operations, such as TLS configuration, DHCP server control, socket operations,
//    and callback registration.
//
// Remarks:
//    None.
// *****************************************************************************

typedef enum 
{
    /**< Use the TLS configuration 1 */
    SYS_WINCS_NET_TLS_CONFIG = 1,        

    /**< Network Interface configuration */        
    SYS_WINCS_NET_IF_CONFIG,                 

    /**< Enable the DHCP server */
    SYS_WINCS_NET_DHCP_SERVER_ENABLE,

    /**< Disable the DHCP server */        
    SYS_WINCS_NET_DHCP_SERVER_DISABLE,  

    /**< Open TCP Socket */        
    SYS_WINCS_NET_SOCK_TCP_OPEN,     

    /**< Open UDP Socket */        
    SYS_WINCS_NET_SOCK_UDP_OPEN,     

    /**< Close the socket */        
    SYS_WINCS_NET_SOCK_CLOSE,     

    /**< Register application callback for sockets */
    SYS_WINCS_NET_SOCK_SET_CALLBACK,         
    SYS_WINCS_NET_SOCK_SET_SRVC_CALLBACK,

    /**< Get Function callback data */        
    SYS_WINCS_NET_SOCK_GET_CALLBACK,

    /**< Open TLS context */
    SYS_WINCS_NET_OPEN_TLS_CTX,

    /**< Get TLS context handle */
    SYS_WINCS_NET_GET_TLS_CTX_HANDLE,

} SYS_WINCS_NET_SOCK_SERVICE_t;


// *****************************************************************************
// Socket Type
//
// Summary:
//    Enumeration of socket types.
//
// Description:
//    This enumeration defines the types of sockets, such as UDP and TCP.
//
// Remarks:
//    None.
// *****************************************************************************

typedef enum   
{    
    /**< UDP Socket type */
    SYS_WINCS_NET_SOCK_TYPE_UDP = 1,                  

    /**< TCP Socket type */
    SYS_WINCS_NET_SOCK_TYPE_TCP,  

} SYS_WINCS_NET_SOCK_TYPE_t;


// *****************************************************************************
// Socket Bind Type
//
// Summary:
//    Enumeration of socket bind types.
//
// Description:
//    This enumeration defines the types of socket bindings, such as local (server),
//    remote (client), multicast, and none.
//
// Remarks:
//    None.
// *****************************************************************************

typedef enum    
{ 
    /**< Bind as a server Socket */
    SYS_WINCS_NET_BIND_LOCAL,         

    /**< Bind as a client Socket */        
    SYS_WINCS_NET_BIND_REMOTE,          

    /**< Bind as a multicast Socket */
    SYS_WINCS_NET_BIND_MCAST,             

    /**< Bind None */        
    SYS_WINCS_NET_BIND_NONE,

} SYS_WINCS_BIND_TYPE_t;


// *****************************************************************************
// Socket Mode
//
// Summary:
//    Enumeration of socket modes.
//
// Description:
//    This enumeration defines the modes of sockets, such as ASCII and Binary.
//
// Remarks:
//    None.
// *****************************************************************************

typedef enum    
{ 
    /**< ASCII Socket mode */
    SYS_WINCS_NET_SOCKMODE_ASCII = 1,      

    /**< Binary Socket mode */        
    SYS_WINCS_NET_SOCKMODE_BINARY,  

} SYS_WINCS_NET_SOCKMODE_t;


// *****************************************************************************
// Network Socket Events
//
// Summary:
//    Enumeration of network socket events.
//
// Description:
//    This enumeration defines various events that can occur for a network socket,
//    such as connection, disconnection, data read, and error events.
//
// Remarks:
//    None.
// *****************************************************************************

typedef enum 
{
    /**< Socket connected event */
    SYS_WINCS_NET_SOCK_EVENT_CONNECTED,  

    /**< TLS handshake done event */
    SYS_WINCS_NET_SOCK_EVENT_TLS_DONE,

    /**< Socket disconnected event */
    SYS_WINCS_NET_SOCK_EVENT_DISCONNECTED,   

    /* Socket event send completed */
    SYS_WINCS_NET_SOCK_EVENT_SEND_COMPLETE,

    /**< Socket data read event */        
    SYS_WINCS_NET_SOCK_EVENT_READ,          

    /**< Socket error event */        
    SYS_WINCS_NET_SOCK_EVENT_ERROR,

    /**< Socket event undefined */
    SYS_WINCS_NET_SOCK_EVENT_UNDEFINED,

    /* Client Connected */
    SYS_WINCS_NET_SOCK_EVENT_CLIENT_CONNECTED,

    SYS_WINCS_NET_SOCK_EVENT_CLOSED,

} SYS_WINCS_NET_SOCK_EVENT_t;



// *****************************************************************************
// DHCP Events
//
// Summary:
//    Enumeration of DHCP events.
//
// Description:
//    This enumeration defines events related to DHCP, such as when a STA IP
//    address is received.
//
// Remarks:
//    None.
// *****************************************************************************

typedef enum 
{
    /* STA IP address received */
    SYS_WINCS_NET_STA_DHCP_DONE,

} SYS_WINCS_NET_DHCP_EVENT_t;


// *****************************************************************************
// Network Socket IP Types
//
// Summary:
//    Enumeration of network socket IP types.
//
// Description:
//    This enumeration defines the types of IP addresses that can be used to open
//    a socket, including IPv4, IPv6 local, and IPv6 global addresses.
//
// Remarks:
//    Note that both IPv6 local and global addresses are represented by the same
//    value (6).
// *****************************************************************************

typedef enum 
{
    /**< Open socket with IPv4 address */
    SYS_WINCS_NET_IPV4          = 4,

    /**< Open socket with IPv6 local address */
    SYS_WINCS_NET_IPV6_LOCAL    = 6,

    /**< Open socket with IPv6 global address */
    SYS_WINCS_NET_IPV6_GLOBAL   = 6,

} SYS_WINCS_NET_IP_TYPE_t;



// *****************************************************************************
// Enumeration for Different Types of Network Sockets in the SYS_WINCS System
//
// Summary:
//    This enumeration defines various socket types and their properties.
//
// Description:
//    The enumeration includes types for client and server sockets, stream and
//    datagram sockets, encrypted and unencrypted sockets, and multicast sockets.
//
// Remarks:
//    None.
// *****************************************************************************

typedef enum
{
    // Represents a client socket type
    SYS_WINCS_NET_SKT_CLIENT        = 0x0001,

    // Represents a server socket type
    SYS_WINCS_NET_SKT_SERVER        = 0x0002,

    // Represents a stream socket type (TCP)
    SYS_WINCS_NET_SKT_STREAM        = 0x0004,

    // Represents a datagram socket type (UDP)
    SYS_WINCS_NET_SKT_DATAGRAM      = 0x0008,

    // Represents an unencrypted socket
    SYS_WINCS_NET_SKT_UNENCRYPTED   = 0x0010,

    // Represents an encrypted socket
    SYS_WINCS_NET_SKT_ENCRYPTED     = 0x0020,

    // Represents a multicast socket
    SYS_WINCS_NET_SKT_MULTICAST     = 0x0040,

} SYS_WINCS_NET_SOCKET_TYPE;



// *****************************************************************************
// Socket Data Type
//
// Summary:
//    Socket data type structure.
//
// Description:
//    This structure defines the properties and configuration of a network socket,
//    including its binding type, protocol type, port number, address, and other
//    relevant settings.
//
// Remarks:
//    None.
// *****************************************************************************

typedef struct 
{    
    /**< Bind type of socket */
    SYS_WINCS_BIND_TYPE_t        bindType;  

    /**< UDP or TCP socket type */
    SYS_WINCS_NET_SOCK_TYPE_t    sockType;        

    /**< Server or Client port number */
    uint16_t                     sockPort;          

    /**< Socket Address (IPv4 / IPv6 Address) */
    const char                   *sockAddr;         

    /* Socket ID */
    uint32_t                     sockID;       

    /**< TLS configuration */
    bool                         tlsEnable; 

    /* Socket address Type IPv4 - 4 / IPv6 - 6 */
    uint8_t                      ipType;

} SYS_WINCS_NET_SOCKET_t;


// *****************************************************************************
// Advanced Socket Settings
//
// Summary:
//    Structure for advanced socket settings.
//
// Description:
//    This structure defines advanced settings for a socket, including socket ID,
//    keep-alive option, and Nagle's algorithm (NoDelay) option.
//
// Remarks:
//    None.
// *****************************************************************************

typedef struct 
{  
    /**< Socket ID */
    uint32_t sockID;                

    /**< Keep-Alive option */
    uint8_t sockKeepAlive;         

    /**< Socket NAGLE/NoDelay */
    uint8_t sockNoDelay;  

} SYS_WINCS_NET_SOCKET_CONFIG_t;


// *****************************************************************************
// Socket Address Types
//
// Summary:
//    Union for socket address types.
//
// Description:
//    This union defines different types of socket addresses, including common
//    address family and port number, IPv4 address structure, and IPv6 address
//    structure.
//
// Remarks:
//    None.
// *****************************************************************************

typedef union
{
    // Common structure for address family and port number
    struct
    {
        sa_family_t sin_family;    /* Address family (e.g., AF_INET, AF_INET6) */
        in_port_t sin_port;        /* Port number */
    };

    // IPv4 address structure
    struct sockaddr_in v4;         /* IPv4 address structure */

    // IPv6 address structure
    struct sockaddr_in6 v6;        /* IPv6 address structure */

} SYS_WINCS_NET_SOCK_ADDR_TYPE;


// *****************************************************************************
// Network Connection Information
//
// Summary:
//    Structure for network connection information.
//
// Description:
//    This structure defines information about a network connection, including
//    connection ID, socket file descriptor, connection status, socket type, local
//    and remote bind addresses, HTTP checksum option, and bytes received.
//
// Remarks:
//    None.
// *****************************************************************************

typedef struct
{
    /**< Connection ID */
    uint8_t                        id;                    

    /**< Socket file descriptor */
    int                            sockfd;                

    /**< Connection in use flag */
    bool                           inUse;                 

    /**< Connection status flag */
    bool                           isConnected;           

    /**< Type of socket (TCP/UDP) */
    SYS_WINCS_NET_SOCKET_TYPE      socketType;            

    /**< Local bind address */
    SYS_WINCS_NET_SOCK_ADDR_TYPE   localBindAddress;      

    /**< Remote bind address */
    SYS_WINCS_NET_SOCK_ADDR_TYPE   remoteBindAddress;     

    /**< HTTP checksum option */
    bool                           doHTTPChecksum;        

    /**< Number of HTTP bytes received */
    size_t                         httpBytesReceived;     
} SYS_WINCS_NET_CID_TYPE;



// *****************************************************************************
// Network Socket TLS Configurations
//
// Summary:
//    Structure for network socket TLS configurations.
//
// Description:
//    This structure defines TLS configurations for a network socket, including
//    peer authentication, CA certificate, certificate name, key name, key password,
//    server name, domain name, domain name verification, and TLS handle.
//
// Remarks:
//    None.
// *****************************************************************************

typedef struct 
{
    /**< TLS Peer authentication */
    bool        tlsPeerAuth;             

    /**< TLS CA Certificate */
    char        *tlsCACertificate;       

    /**< TLS Certificate Name */         
    char        *tlsCertificate;         

    /**< TLS Key name  */            
    char        *tlsKeyName;             

    /**< TLS Key password  */
    char        *tlsKeyPassword;         

    /**< TLS Server name  */
    char        *tlsServerName;          

    /**< TLS Domain Name */
    char        *tlsDomainName;          

    /**< TLS Domain Name Verify */
    bool        tlsDomainNameVerify;     

    /**< TLS handle */
    uint8_t     tlsHandle;               

} SYS_WINCS_NET_TLS_SOC_PARAMS;



// *****************************************************************************
// DHCP Callback Function Type
//
// Summary:
//    Typedef for DHCP callback function.
//
// Description:
//    This typedef defines the function signature for a DHCP callback function,
//    which takes a DHCP event and a network handle as parameters.
//
// Remarks:
//    None.
// *****************************************************************************

typedef SYS_WINCS_RESULT_t (*SYS_WINCS_NET_DHCP_CALLBACK_t)(SYS_WINCS_NET_DHCP_EVENT_t, SYS_WINCS_NET_HANDLE_t netHandle);


// *****************************************************************************
// Network Socket Events Callback Function Type
//
// Summary:
//    Typedef for network socket events callback function.
//
// Description:
//    This typedef defines the function signature for a network socket events
//    callback function, which takes a socket ID, a socket event, and a network
//    handle as parameters.
//
// Remarks:
//    None.
// *****************************************************************************

typedef SYS_WINCS_RESULT_t (*SYS_WINCS_NET_SOCK_CALLBACK_t)(uint32_t sock, SYS_WINCS_NET_SOCK_EVENT_t, SYS_WINCS_NET_HANDLE_t netHandle);


// *****************************************************************************
// NET Sock Service Layer API
//
// Summary:
//    API to handle system operations for network sockets.
//
// Description:
//    This function handles various system operations for network sockets based
//    on the requested service.
//
// Parameters:
//    request - Requested service ::SYS_WINCS_NET_SOCK_SERVICE_t
//    netHandle - Network handle
//
// Returns:
//    ::SYS_WINCS_PASS - Requested service is handled successfully
//    ::SYS_WINCS_FAIL - Requested service has failed
//
// Remarks:
//    None.
// *****************************************************************************

SYS_WINCS_RESULT_t SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_SERVICE_t request, SYS_WINCS_NET_HANDLE_t netHandle);


// *****************************************************************************
// NET Socket Write API for TCP
//
// Summary:
//    API to send data over a TCP socket.
//
// Description:
//    This function sends data over a TCP socket.
//
// Parameters:
//    socket - Socket ID
//    length - Length of data to be sent
//    input - Input buffer containing the data to be sent
//
// Returns:
//    ::SYS_WINCS_PASS - Data is sent successfully
//    ::SYS_WINCS_FAIL - Data sending has failed
//
// Remarks:
//    None.
// *****************************************************************************

SYS_WINCS_RESULT_t SYS_WINCS_NET_TcpSockWrite(uint32_t socket, uint16_t length, uint8_t *input);


// *****************************************************************************
// NET Socket Write API for UDP
//
// Summary:
//    API to send data over a UDP socket.
//
// Description:
//    This function sends data over a UDP socket.
//
// Parameters:
//    socket - Socket ID
//    addr - IP address of the UDP peer
//    port - Port address of the UDP peer
//    length - Length of data to be sent
//    input - Input buffer containing the data to be sent
//
// Returns:
//    ::SYS_WINCS_PASS - Data is sent successfully
//    ::SYS_WINCS_FAIL - Data sending has failed
//
// Remarks:
//    None.
// *****************************************************************************

SYS_WINCS_RESULT_t SYS_WINCS_NET_UdpSockWrite(uint32_t socket, const char *addr, uint32_t port, uint16_t length, uint8_t *input);


// *****************************************************************************
// NET Socket Read API for TCP
//
// Summary:
//    API to read data from a TCP socket.
//
// Description:
//    This function reads data from a TCP socket.
//
// Parameters:
//    socket - Socket ID
//    length - Length of data to be read
//    input - Input buffer to store the read data
//
// Returns:
//    ::SYS_WINCS_PASS - Data is read successfully
//    ::SYS_WINCS_FAIL - Data reading has failed
//
// Remarks:
//    None.
// *****************************************************************************

int16_t SYS_WINCS_NET_TcpSockRead(uint32_t socket, uint16_t length, uint8_t *input);


// *****************************************************************************
// NET Socket Read API for UDP
//
// Summary:
//    API to read data from a UDP socket.
//
// Description:
//    This function reads data from a UDP socket.
//
// Parameters:
//    socket - Socket ID
//    length - Length of data to be read
//    input - Input buffer to store the read data
//
// Returns:
//    ::SYS_WINCS_PASS - Data is read successfully
//    ::SYS_WINCS_FAIL - Data reading has failed
//
// Remarks:
//    None.
// *****************************************************************************

int16_t SYS_WINCS_NET_UdpSockRead(uint32_t socket, uint16_t length, uint8_t *input);


#endif	/* SYS_WINCS_NET_SERVICE_H */



/*******************************************************************************
  File Name:
    socket.h

  Summary:
    WINC1500 BSD Compatible Socket Interface

  Description:
    WINC1500 BSD Compatible Socket Interface
 *******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2019 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
//DOM-IGNORE-END

#ifndef __SOCKET_H__
#define __SOCKET_H__

#ifdef  __cplusplus
extern "C" {
#endif

/** @defgroup SocketHeader Socket
 *          BSD compatible socket interface between the host layer and the network
 *          protocol stacks in the firmware.
 *          These functions are used by the host application to send or receive
 *          packets and to do other socket operations.
 */

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#include "nm_common.h"
#include "m2m_types.h"
#include "netinet_in.h"

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/**
 * @defgroup  SocketDefines Defines
 * @ingroup SocketHeader
 */

/** @defgroup  IPDefines TCP/IP Defines
 * @ingroup SocketDefines
 * The following list of macros are used to define constants used throughout the socket layer.
 * @{
 */

#define HOSTNAME_MAX_SIZE                                   64
/*!<
    Maximum allowed size for a host domain name passed to the function gethostbyname @ref gethostbyname.
    command value. Used with the setsockopt function.

*/

#define SOCKET_BUFFER_MAX_LENGTH                            1500
/*!<
    Maximum allowed size for a socket data buffer. Used with @ref send socket
    function to ensure that the buffer sent is within the allowed range.
*/

#define  AF_INET                                            2
/*!<
    The AF_INET is the address family used for IPv4. An IPv4 transport address is specified with the @ref sockaddr_in structure.
    (It is the only supported type for the current implementation.)
*/


#define  SOCK_STREAM                                        1
/*!<
     One of the IPv4 supported socket types for reliable connection-oriented stream connection.
     Passed to the @ref socket function for the socket creation operation.
*/

#define  SOCK_DGRAM                                         2
/*!<
     One of the IPv4 supported socket types for unreliable connectionless datagram connection.
     Passed to the @ref socket function for the socket creation operation.
*/

#define SOCKET_FLAGS_SSL                                    0x01
/*!<
    This flag shall be passed to the socket API for SSL session.
*/

#define TCP_SOCK_MAX                                        (7)
/*!<
    Maximum number of simultaneous TCP sockets.
*/

#define UDP_SOCK_MAX                                        4
/*!<
    Maximum number of simultaneous UDP sockets.
*/

#define MAX_SOCKET                                          (TCP_SOCK_MAX + UDP_SOCK_MAX)
/*!<
    Maximum number of simultaneous sockets.
*/

#define SOL_SOCKET                                          1
/*!<
    Socket option.
    Used with the @ref setsockopt function
*/

#define SOL_SSL_SOCKET                                      2
/*!<
    SSL Socket option level.
    Used with the @ref setsockopt function
*/

#define SO_SET_UDP_SEND_CALLBACK                            0x00
/*!<
    Socket option used by the application to enable/disable
    the use of UDP send callbacks.
    Used with the @ref setsockopt function.
*/

#define IP_ADD_MEMBERSHIP                                   0x01
/*!<
    Set Socket Option Add Membership command value (to join a multicast group).
    Used with the @ref setsockopt function.
*/

#define IP_DROP_MEMBERSHIP                                  0x02
/*!<
    Set Socket Option Drop Membership command value (to leave a multicast group).
    Used with the @ref setsockopt function.
*/

#define SO_KEEPALIVE                                        0x04
/*!<
    Enable or disable TCP keep-alive.
    Used with the @ref setsockopt function.
*/

#define TCP_KEEPIDLE                                        0x05
/*!<
    Duration between two keepalive transmissions in idle condition (in 500ms increments, so 4 would be 2 seconds). Max 2^32.
    Used with the @ref setsockopt function.
*/

#define TCP_KEEPINTVL                                       0x06
/*!<
    Duration between two successive keepalive retransmissions, if acknowledgment to the previous keepalive
    transmission is not received  (in 500ms increments, so 4 would be 2 seconds). Max 255 (127.5 seconds).
    Used with the @ref setsockopt function.
*/

#define TCP_KEEPCNT                                         0x07
/*!<
    Number of retransmissions to be carried out before declaring that the remote end is not available. Max 255.
    Used with the @ref setsockopt function.
*/
/**@}*/     //IPDefines

/**
 * @defgroup  TLSDefines TLS Defines
 * @ingroup SocketDefines
 * @ingroup SSLAPI
 */

/** @defgroup  SSLSocketOptions TLS Socket Options
 * @ingroup TLSDefines
 * The following list of macros are used to define SSL Socket options.
 * @{
 * @sa setsockopt
 */

#define SO_SSL_BYPASS_X509_VERIF                            0x01
/*!<
    Allow an opened SSL socket to bypass the X509 certificate
    verification process.
    It is highly required NOT to use this socket option in production
    software applications. It is supported for debugging and testing
    purposes.
    The option value should be cast to int32_t type and it is handled
    as a boolean flag.
*/

#define SO_SSL_SNI                                          0x02
/*!<
    Set the Server Name Indicator (SNI) for an SSL socket. The
    SNI is a NULL terminated string containing the server name
    associated with the connection. It must not exceed the size
    of HOSTNAME_MAX_SIZE.
*/

#define SO_SSL_ENABLE_SESSION_CACHING                       0x03
/*!<
    This option allow the TLS to cache the session information for fast
    TLS session establishment in future connections using the
    TLS Protocol session resume features.
*/

#define SO_SSL_ENABLE_SNI_VALIDATION                        0x04
/*!<
    Enable SNI validation against the server's certificate subject
    common name. If there is no SNI provided (via the SO_SSL_SNI
    option), setting this option does nothing.
*/
/**@}*/     //SSLSocketOptions

/** @defgroup  LegacySSLCipherSuite Legacy names for TLS Cipher Suite IDs
 * @ingroup TLSDefines
 * The following list of macros MUST NOT be used. Instead use the new names under SSLCipherSuiteID
 * @sa m2m_ssl_set_active_ciphersuites
 * @{
 */

#define SSL_ENABLE_RSA_SHA_SUITES                           0x01
/*!<
    Enable RSA Hmac_SHA based Cipher suites. For example,
        TLS_RSA_WITH_AES_128_CBC_SHA
*/

#define SSL_ENABLE_RSA_SHA256_SUITES                        0x02
/*!<
    Enable RSA Hmac_SHA256 based Cipher suites. For example,
        TLS_RSA_WITH_AES_128_CBC_SHA256
*/

#define SSL_ENABLE_DHE_SHA_SUITES                           0x04
/*!<
    Enable DHE Hmac_SHA based Cipher suites. For example,
        TLS_DHE_RSA_WITH_AES_128_CBC_SHA
*/

#define SSL_ENABLE_DHE_SHA256_SUITES                        0x08
/*!<
    Enable DHE Hmac_SHA256 based Cipher suites. For example,
        TLS_DHE_RSA_WITH_AES_128_CBC_SHA256
*/

#define SSL_ENABLE_RSA_GCM_SUITES                           0x10
/*!<
    Enable RSA AEAD based Cipher suites. For example,
        TLS_RSA_WITH_AES_128_GCM_SHA256
*/

#define SSL_ENABLE_DHE_GCM_SUITES                           0x20
/*!<
    Enable DHE AEAD based Cipher suites. For example,
        TLS_DHE_RSA_WITH_AES_128_GCM_SHA256
*/

#define SSL_ENABLE_ALL_SUITES                               0x0000003F
/*!<
    Enable all possible supported cipher suites.
*/
/**@}*/     //LegacySSLCipherSuite

/** @defgroup  SSLCipherSuiteID TLS Cipher Suite IDs
 * @ingroup TLSDefines
 * The following list of macros defined the list of supported TLS Cipher suites.
 * Each MACRO defines a single Cipher suite.
 * @sa m2m_ssl_set_active_ciphersuites
 * @{
 */

#define SSL_CIPHER_RSA_WITH_AES_128_CBC_SHA                 NBIT0
#define SSL_CIPHER_RSA_WITH_AES_128_CBC_SHA256              NBIT1
#define SSL_CIPHER_DHE_RSA_WITH_AES_128_CBC_SHA             NBIT2
#define SSL_CIPHER_DHE_RSA_WITH_AES_128_CBC_SHA256          NBIT3
#define SSL_CIPHER_RSA_WITH_AES_128_GCM_SHA256              NBIT4
#define SSL_CIPHER_DHE_RSA_WITH_AES_128_GCM_SHA256          NBIT5
#define SSL_CIPHER_RSA_WITH_AES_256_CBC_SHA                 NBIT6
#define SSL_CIPHER_RSA_WITH_AES_256_CBC_SHA256              NBIT7
#define SSL_CIPHER_DHE_RSA_WITH_AES_256_CBC_SHA             NBIT8
#define SSL_CIPHER_DHE_RSA_WITH_AES_256_CBC_SHA256          NBIT9
#define SSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA           NBIT10
#define SSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA           NBIT11
#define SSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA256        NBIT12
#define SSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256      NBIT13
#define SSL_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256        NBIT14
#define SSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256      NBIT15

#define SSL_ECC_ONLY_CIPHERS        \
(\
    SSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256  |   \
    SSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256      \
)
/*!<
    All ciphers that use ECC crypto only. This excludes ciphers that use RSA. They use ECDSA instead.
    These ciphers are turned off by default at startup.
    The application may enable them if it has an ECC math engine (like ATECC508).
*/
#define SSL_ECC_ALL_CIPHERS     \
(\
    SSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA       |   \
    SSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA256    |   \
    SSL_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256    |   \
    SSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256  |   \
    SSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256      \
)
/*!<
    All supported ECC Ciphers including those ciphers that depend on RSA and ECC.
    These ciphers are turned off by default at startup.
    The application may enable them if it has an ECC math engine (like ATECC508).
*/

#define SSL_NON_ECC_CIPHERS_AES_128 \
(\
    SSL_CIPHER_RSA_WITH_AES_128_CBC_SHA             |   \
    SSL_CIPHER_RSA_WITH_AES_128_CBC_SHA256          |   \
    SSL_CIPHER_DHE_RSA_WITH_AES_128_CBC_SHA         |   \
    SSL_CIPHER_DHE_RSA_WITH_AES_128_CBC_SHA256      |   \
    SSL_CIPHER_RSA_WITH_AES_128_GCM_SHA256          |   \
    SSL_CIPHER_DHE_RSA_WITH_AES_128_GCM_SHA256          \
)
/*!<
    All supported AES-128 Ciphers (ECC ciphers are not counted). This is the default active group after startup.
*/

#define SSL_ECC_CIPHERS_AES_256     \
(\
    SSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA   \
)
/*!<
    ECC AES-256 supported ciphers.
*/

#define SSL_NON_ECC_CIPHERS_AES_256 \
(\
    SSL_CIPHER_RSA_WITH_AES_256_CBC_SHA         |   \
    SSL_CIPHER_RSA_WITH_AES_256_CBC_SHA256      |   \
    SSL_CIPHER_DHE_RSA_WITH_AES_256_CBC_SHA     |   \
    SSL_CIPHER_DHE_RSA_WITH_AES_256_CBC_SHA256      \
)
/*!<
    AES-256 Ciphers.
    This group is disabled by default at startup because the WINC HW Accelerator
    supports only AES-128. If the application needs to force AES-256 cipher support,
    it could enable them (or any of them) explicitly by calling m2m_ssl_set_active_ciphersuites.
*/

#define SSL_CIPHER_ALL              \
(\
    SSL_CIPHER_RSA_WITH_AES_128_CBC_SHA             |   \
    SSL_CIPHER_RSA_WITH_AES_128_CBC_SHA256          |   \
    SSL_CIPHER_DHE_RSA_WITH_AES_128_CBC_SHA         |   \
    SSL_CIPHER_DHE_RSA_WITH_AES_128_CBC_SHA256      |   \
    SSL_CIPHER_RSA_WITH_AES_128_GCM_SHA256          |   \
    SSL_CIPHER_DHE_RSA_WITH_AES_128_GCM_SHA256      |   \
    SSL_CIPHER_RSA_WITH_AES_256_CBC_SHA             |   \
    SSL_CIPHER_RSA_WITH_AES_256_CBC_SHA256          |   \
    SSL_CIPHER_DHE_RSA_WITH_AES_256_CBC_SHA         |   \
    SSL_CIPHER_DHE_RSA_WITH_AES_256_CBC_SHA256      |   \
    SSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA       |   \
    SSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA256    |   \
    SSL_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256    |   \
    SSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256  |   \
    SSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256  |   \
    SSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA           \
)
/*!<
    Turn On All TLS Ciphers.
*/
/**@}*/     //SSLCipherSuiteID

/**************
Socket Errors
**************/
/**@defgroup  SocketErrorCode Error Codes
 * @ingroup SocketHeader
 * The following list of macros are used to define the possible error codes.
 * Errors are listed in numerical order with the error macro name.
 * @{
 */
#define SOCK_ERR_NO_ERROR                                   0
/*!<
    Successful socket operation. This code is also used with event @ref SOCKET_MSG_RECV if a socket connection is closed.
    In that case, the application should call @ref shutdown().
*/

#define SOCK_ERR_INVALID_ADDRESS                            -1
/*!<
    Socket address is invalid. The socket operation cannot be completed successfully without specifying a specific address
    For example: bind is called without specifying a port number
*/

#define SOCK_ERR_ADDR_ALREADY_IN_USE                        -2
/*!<
    Socket operation cannot bind on the given address. With socket operations, only one IP address per socket is permitted.
    Any attempt for a new socket to bind with an IP address already bound to another open socket,
    will return the following error code. States that bind operation failed.
*/

#define SOCK_ERR_MAX_TCP_SOCK                               -3
/*!<
    Exceeded the maximum number of TCP sockets. A maximum number of TCP sockets opened simultaneously is defined through TCP_SOCK_MAX.
    It is not permitted to exceed that number at socket creation. Identifies that @ref socket operation failed.
*/

#define SOCK_ERR_MAX_UDP_SOCK                               -4
/*!<
    Exceeded the maximum number of UDP sockets. A maximum number of UDP sockets opened simultaneously is defined through UDP_SOCK_MAX.
    It is not permitted to exceed that number at socket creation. Identifies that @ref socket operation failed
*/

#define SOCK_ERR_INVALID_ARG                                -6
/*!<
    An invalid argument is passed to a function.
*/

#define SOCK_ERR_MAX_LISTEN_SOCK                            -7
/*!<
    Exceeded the maximum number of TCP passive listening sockets.
    Identifies Identifies that @ref listen operation failed.
*/

#define SOCK_ERR_INVALID                                    -9
/*!<
    The requested socket operation is not valid in the
    current socket state.
    For example: @ref accept is called on a TCP socket before @ref bind or @ref listen.
*/

#define SOCK_ERR_ADDR_IS_REQUIRED                           -11
/*!<
    Destination address is required. Failure to provide the socket address required for the socket operation to be completed.
    It is generated as an error to the @ref sendto function when the address required to send the data to is not known.
*/

#define SOCK_ERR_CONN_ABORTED                               -12
/*!<
    The socket is closed (reset) by the peer. If this error is received, the application should call @ref shutdown().
*/

#define SOCK_ERR_TIMEOUT                                    -13
/*!<
    The socket pending operation has timed out. The socket remains open.
*/

#define SOCK_ERR_BUFFER_FULL                                -14
/*!<
    No buffer space available to be used for the requested socket operation.
*/
/**@}*/     //SocketErrorCode

/** @addtogroup SocketDefines
 *  @{
 */
#ifdef _NM_BSP_BIG_END
#define _htonl(m)               (m)
#define _htons(A)               (A)

#else

#define _htonl(m)       \
    (uint32_t)(((uint32_t)(m << 24)) | ((uint32_t)((m & 0x0000FF00) << 8)) | ((uint32_t)((m & 0x00FF0000) >> 8)) | ((uint32_t)(((uint32_t)m) >> 24)))
/*!<
    Convert a 4-byte integer from the host representation to the Network byte order representation.
*/

#define _htons(A)       (uint16_t)((((uint16_t) (A)) << 8) | (((uint16_t) (A)) >> 8))
/*!<
    Convert a 2-byte integer (short) from the host representation to the Network byte order representation.
*/
#endif

#define _ntohl              _htonl
/*!<
    Convert a 4-byte integer from the Network byte order representation to the host representation .
*/

#define _ntohs              _htons
/*!<
    Convert a 2-byte integer from the Network byte order representation to the host representation .
*/
/**@}*/     //SocketDefines

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
DATA TYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/** @defgroup  SocketEnums DataTypes
 * @ingroup SocketHeader
 * Specific Enumeration-typedefs used for socket operations
 * @{ */

/*!
@typedef    \
    SOCKET

@brief
    Definition for socket handler data type.
    Socket ID,used with all socket operations to uniquely identify the socket handler.
    Such an ID is uniquely assigned at socket creation when calling @ref socket operation.
*/
typedef int8_t  SOCKET;

/*!
@struct \
    sockaddr

@brief
    Generic socket address structure.

@see
      sockaddr_in
*/
struct sockaddr {
    uint16_t        sa_family;
    /*!<
        Socket address family.
    */
    uint8_t     sa_data[14];
    /*!<
        Maximum size of all the different socket address structures.
    */
};

 //@}
/**@defgroup  AsyncCallback Asynchronous Events
 * @ingroup SocketEnums
 * Specific Enumeration used for asynchronous operations
 * @{ */
/*!
@enum   \
    tenuSocketCallbackMsgType

@brief
    Asynchronous APIs make use of callback functions in-order to return back the results once the corresponding socket operation is completed.
    Hence resuming the normal execution of the application code while the socket operation returns the results.
    Callback functions expect event messages to be passed in, in-order to identify the operation they're returning the results for.
    The following enum identifies the type of events that are received in the callback function.

    Application Use:
    In order for application developers to handle the pending events from the network controller through the callback functions,
    a function call must be made to the function @ref m2m_wifi_handle_events at least once for each socket operation.

@see     bind
@see     listen
@see     accept
@see     connect
@see     send
@see     recv
*/

typedef enum {
    SOCKET_MSG_BIND = 1,
    /*!<
        Bind socket event.
    */
    SOCKET_MSG_LISTEN,
    /*!<
        Listen socket event.
    */
    SOCKET_MSG_DNS_RESOLVE,
    /*!<
        DNS Resolution event.
    */
    SOCKET_MSG_ACCEPT,
    /*!<
        Accept socket event.
    */
    SOCKET_MSG_CONNECT,
    /*!<
        Connect socket event.
    */
    SOCKET_MSG_RECV,
    /*!<
        Receive socket event.
    */
    SOCKET_MSG_SEND,
    /*!<
        Send socket event.
    */
    SOCKET_MSG_SENDTO,
    /*!<
        sendto socket event.
    */
    SOCKET_MSG_RECVFROM
    /*!<
        Recvfrom socket event.
    */
} tenuSocketCallbackMsgType;


/*!
@struct \
    tstrSocketBindMsg

@brief  Socket bind status.

    An asynchronous call to the @ref bind socket operation, returns information through this structure in response.
    This structure together with the event @ref SOCKET_MSG_BIND are passed in parameters to the callback function.
@see
     bind
*/
typedef struct {
    int8_t      status;
    /*!<
        The result of the bind operation.
        Holding a value of ZERO for a successful bind or otherwise a negative
        error code corresponding to the type of error.
    */
} tstrSocketBindMsg;

/*!
@struct \
    tstrSocketListenMsg

@brief  Socket listen status.

    Socket listen information is returned through this structure in response to the asynchronous call to the @ref listen function.
    This structure together with the event @ref SOCKET_MSG_LISTEN are passed-in parameters to the callback function.
@see
      listen
*/
typedef struct {
    int8_t      status;
    /*!<
        Holding a value of ZERO for a successful listen or otherwise a negative
        error code corresponding to the type of error.
    */
} tstrSocketListenMsg;

/*!
@struct \
    tstrSocketAcceptMsg

@brief  Socket accept status.

    Socket accept information is returned through this structure in response to the asynchronous call to the @ref accept function.
    This structure together with the event @ref SOCKET_MSG_ACCEPT are passed-in parameters to the callback function.
*/
typedef struct {
    SOCKET      sock;
    /*!<
        On a successful @ref accept operation, the return information is the socket ID for the accepted connection with the remote peer.
        Otherwise a negative error code is returned to indicate failure of the accept operation.
    */
    struct      sockaddr_in strAddr;
    /*!<
        Socket address structure for the remote peer.
    */
} tstrSocketAcceptMsg;

/*!
@struct \
    tstrSocketConnectMsg

@brief  Socket connect status.

    Socket connect information is returned through this structure in response to the asynchronous call to the @ref connect socket function.
    This structure together with the event @ref SOCKET_MSG_CONNECT are passed-in parameters to the callback function.
    If the application receives this structure with a negative value in s8Error, the application should call @ref shutdown().
*/
typedef struct {
    SOCKET  sock;
    /*!<
        Socket ID referring to the socket passed to the connect function call.
    */
    int8_t      s8Error;
    /*!<
        Connect error code.
        Holding a value of ZERO for a successful connect or otherwise a negative
        error code corresponding to the type of error.
    */
} tstrSocketConnectMsg;

/*!
@struct \
    tstrSocketRecvMsg

@brief  Socket recv status.

    Socket receive information is returned through this structure in response to the asynchronous call to the recv or recvfrom socket functions.
    This structure together with the events @ref SOCKET_MSG_RECV or @ref SOCKET_MSG_RECVFROM are passed-in parameters to the callback function.
@remark
    In case the received data from the remote peer is larger than the USER buffer size defined during the asynchronous call to the @ref recv function,
    delivered to the user in a number of consecutive chunks according to the USER Buffer size.
    A negative or zero buffer size indicates an error with the following code:
    @ref SOCK_ERR_NO_ERROR          : Socket connection closed. The application should now call @ref shutdown().
    @ref SOCK_ERR_CONN_ABORTED      : Socket connection aborted. The application should now call @ref shutdown().
    @ref SOCK_ERR_TIMEOUT           : Socket receive timed out. The socket connection remains open.
*/
typedef struct {
    uint8_t                 *pu8Buffer;
    /*!<
        Pointer to the USER buffer (passed to @ref recv and @ref recvfrom function) containing the received data chunk.
    */
    int16_t                 s16BufferSize;
    /*!<
        The received data chunk size.
        Holds a negative value if there is a receive error or ZERO on success upon reception of close socket message.
    */
    uint16_t                    u16RemainingSize;
    /*!<
        The number of bytes remaining in the current @ref  recv operation.
    */
    struct sockaddr_in      strRemoteAddr;
    /*!<
        Socket address structure for the remote peer. It is valid for @ref SOCKET_MSG_RECVFROM event.
    */
} tstrSocketRecvMsg;
/**@}*/     //AsyncCallback

/** @defgroup SocketCallbacks Callbacks
 *  @ingroup SocketHeader
 */
/**@{*/
/*!
@typedef \
    tpfAppSocketCb

@brief
                The main socket application callback function. Applications register their main socket application callback through this function by calling  @ref registerSocketCallback.
                In response to events received, the following callback function is called to handle the corresponding asynchronous function called. Example: @ref bind, @ref connect,...etc.

@param [in] sock
                Socket ID for the callback.

                The socket callback function is called whenever a new event is received in response
                to socket operations.

@param [in] u8Msg
                 Socket event type. Possible values are:
                  - @ref SOCKET_MSG_BIND
                  - @ref SOCKET_MSG_LISTEN
                  - @ref SOCKET_MSG_ACCEPT
                  - @ref SOCKET_MSG_CONNECT
                  - @ref SOCKET_MSG_RECV
                  - @ref SOCKET_MSG_SEND
                  - @ref SOCKET_MSG_SENDTO
                  - @ref SOCKET_MSG_RECVFROM

@param [in] pvMsg
                Pointer to message structure. Existing types are:
                  - tstrSocketBindMsg
                  - tstrSocketListenMsg
                  - tstrSocketAcceptMsg
                  - tstrSocketConnectMsg
                  - tstrSocketRecvMsg

@see
    tenuSocketCallbackMsgType
    tstrSocketRecvMsg
    tstrSocketConnectMsg
    tstrSocketAcceptMsg
    tstrSocketListenMsg
    tstrSocketBindMsg
*/
typedef void (*tpfAppSocketCb) (SOCKET sock, uint8_t u8Msg, void * pvMsg);

/*!
@typedef    \
    tpfAppResolveCb

@brief
        DNS resolution callback function.
    Applications requiring DNS resolution should register their callback through this function by calling @ref registerSocketCallback.
    The following callback is triggered in response to asynchronous call to the @ref gethostbyname function (DNS Resolution callback).

@param[in]  pu8DomainName
                Domain name of the host.

@param[in]  u32ServerIP
                Server IPv4 address encoded in NW byte order format. If it is Zero, then the DNS resolution failed.
*/
typedef void (*tpfAppResolveCb) (uint8_t* pu8DomainName, uint32_t u32ServerIP);

/*!
@typedef \
    tpfPingCb

@brief  PING Callback

    The function delivers the ping statistics for the sent ping triggered by calling
    m2m_ping_req.

@param[in]  u32IPAddr
                Destination IP.

@param[in]  u32RTT
                Round Trip Time.

@param[in]  u8ErrorCode
                Ping error code. It may be one of:
                - PING_ERR_SUCCESS
                - PING_ERR_DEST_UNREACH
                - PING_ERR_TIMEOUT
*/
typedef void (*tpfPingCb)(uint32_t u32IPAddr, uint32_t u32RTT, uint8_t u8ErrorCode);
/**@}*/     //SocketCallbacks

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
FUNCTION PROTOTYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/** \defgroup SocketAPI Function
 *   @ingroup SocketHeader
 */

/** @defgroup SocketInitializationFn socketInit
 *  @ingroup SocketAPI
 *     The function performs the necessary initializations for the socket library through the following steps:
    - A check made by the global variable gbSocketInit, ensuring that initialization for sockets is performed only once,
     in-order to prevent resetting the socket instances already created in the global socket array (gastrSockets).
    - Zero initializations to the global socket array (gastrSockets), which holds the list of TCP sockets.
    - Registers the socket (Host Interface)hif callback function through the call to the hif_register_cb function.
       This facilitates handling  all of the socket related functions received through interrupts from the firmware.
 */
/**@{*/
/*!
@fn \
    void socketInit(void);

@return     void

@remarks
    This initialization function must be invoked before any socket operation is performed.
    No error codes from this initialization function since the socket array is statically allocated based in the maximum number of
    sockets @ref MAX_SOCKET based on the systems capability.
\section socketinit example
This example demonstrates the use of the socketinit for socket initialization for an mqtt chat application.
 \code
    tstrWifiInitParam   param;
    tstrNetworkId       strNetworkId;
    tstrAuthPsk         strAuthPsk;
    int8_t ret;
    char topic[strlen(MAIN_CHAT_TOPIC) + MAIN_CHAT_USER_NAME_SIZE + 1];

    //Initialize the board.
    system_init();

    //Initialize the UART console.
    configure_console();

    // Initialize the BSP.
    nm_bsp_init();

    ----------

    // Initialize socket interface.
    socketInit();
    registerSocketCallback(socket_event_handler, socket_resolve_handler);

    // Connect to router.
    strNetworkId.pu8Bssid = NULL;
    strNetworkId.pu8Ssid = MAIN_WLAN_SSID;
    strNetworkId.u8SsidLen = sizeof(MAIN_WLAN_SSID);
    strNetworkId.u8Channel = M2M_WIFI_CH_ALL;

    strAuthPsk.pu8Psk = NULL;
    strAuthPsk.pu8Passphrase = MAIN_WLAN_PSK;
    strAuthPsk.u8PassphraseLen = (uint8_t)strlen((char*)MAIN_WLAN_PSK);

    m2m_wifi_connect_psk(WIFI_CRED_SAVE_ENCRYPTED, &strNetworkId, &strAuthPsk);
\endcode
*/
void socketInit(void);


/*!
@fn \
    void socketDeinit(void);

@brief  Socket Layer De-initialization

    The function performs the necessary cleanup for the socket library static data
    It must be invoked as the last any socket operation is performed on any active sockets.
*/
void socketDeinit(void);

/*!
@fn \
    uint8_t IsSocketReady(void);

@see            socketInit
                socketDeinit
@return         If the socket has been initialized and is ready.
                Should return 1 after @ref socketInit and 0 after @ref socketDeinit
*/
uint8_t IsSocketReady(void);
/** @} */     //SocketInitializationFn

/** @defgroup SocketCallbackFn registerSocketCallback
 *    @ingroup SocketAPI
 *    Register two callback functions one for asynchronous socket events and the other one for DNS callback registering function.
 *    The registered callback functions are used to retrieve information in response to the asynchronous socket functions called.
 */
/**@{*/


/*!
@fn \
    void registerSocketCallback(tpfAppSocketCb socket_cb, tpfAppResolveCb resolve_cb);

@param[in]  socket_cb   tpfAppSocketCb
                Assignment of callback function to the global callback @ref tpfAppSocketCb gpfAppSocketCb. Delivers
                socket messages to the host application. In response to the asynchronous function calls, such as @ref bind
                @ref listen @ref accept @ref connect

@param[in]  resolve_cb  tpfAppResolveCb
                Assignment of callback function to the global callback @ref tpfAppResolveCb gpfAppResolveCb.
                Used for DNS resolving functionalities. The DNS resolving technique is determined by the application
                registering the callback.
                NULL is assigned when, DNS resolution is not required.

@return     void
@remarks
        If any of the socket functionalities is not to be used, NULL is passed in as a parameter.
        It must be invoked after socketinit and before other socket layer operations.

\section registerSocketCallback example
    This example demonstrates the use of the registerSocketCallback to register a socket callback function with DNS resolution CB set to null
    for a simple UDP server example.
 \code
    tstrWifiInitParam   param;
    tstrNetworkId       strNetworkId;
    tstrAuthPsk         strAuthPsk;
    int8_t ret;
    struct sockaddr_in addr;

    // Initialize the board
    system_init();

    //Initialize the UART console.
    configure_console();

    // Initialize the BSP.
    nm_bsp_init();

    // Initialize socket address structure.
    addr.sin_family = AF_INET;
    addr.sin_port = _htons(MAIN_WIFI_M2M_SERVER_PORT);
    addr.sin_addr.s_addr = _htonl(MAIN_WIFI_M2M_SERVER_IP);

    // Initialize Wi-Fi parameters structure.
    memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

    // Initialize Wi-Fi driver with data and status callbacks.
    param.pfAppWifiCb = wifi_cb;
    ret = m2m_wifi_init(&param);
    if (M2M_SUCCESS != ret) {
        printf("main: m2m_wifi_init call error!(%d)\r\n", ret);
        while (1) {
        }
    }

    // Initialize socket module
    socketInit();
    registerSocketCallback(socket_cb, NULL);

    // Connect to router.
    strNetworkId.pu8Bssid = NULL;
    strNetworkId.pu8Ssid = MAIN_WLAN_SSID;
    strNetworkId.u8SsidLen = sizeof(MAIN_WLAN_SSID);
    strNetworkId.u8Channel = M2M_WIFI_CH_ALL;

    strAuthPsk.pu8Psk = NULL;
    strAuthPsk.pu8Passphrase = MAIN_WLAN_PSK;
    strAuthPsk.u8PassphraseLen = (uint8_t)strlen((char*)MAIN_WLAN_PSK);

    m2m_wifi_connect_psk(WIFI_CRED_SAVE_ENCRYPTED, &strNetworkId, &strAuthPsk);
\endcode
*/
void registerSocketCallback(tpfAppSocketCb socket_cb, tpfAppResolveCb resolve_cb);
void registerSocketEventCallback(tpfAppSocketCb socket_cb);
void registerSocketResolveCallback(tpfAppResolveCb resolve_cb);
/** @} */     //SocketCallbackFn

/** @defgroup SocketFn socket
 *    @ingroup SocketAPI
 *  Synchronous socket allocation function based on the specified socket type. Created sockets are non-blocking and their possible types are either TCP or a UDP sockets.
 *  The maximum allowed number of TCP sockets is @ref TCP_SOCK_MAX sockets while the maximum number of UDP sockets that can be created simultaneously is @ref UDP_SOCK_MAX sockets.
 *
 */
/**@{*/
/*!
@fn \
    SOCKET socket(uint16_t u16Domain, uint8_t u8Type, uint8_t u8Flags);


@param [in] u16Domain
                Socket family. The only allowed value is AF_INET (IPv4.0) for TCP/UDP sockets.

@param[in]  u8Type
                Socket type. Allowed values are:
                - @ref SOCK_STREAM
                - @ref SOCK_DGRAM

@param[in]  u8Flags
                Used to specify the socket creation flags. It shall be set to zero for normal TCP/UDP sockets.
                It could be @ref SOCKET_FLAGS_SSL if the socket is used for SSL session. The use of the flag
                @ref SOCKET_FLAGS_SSL has no meaning in case of UDP sockets.

@pre
    The @ref socketInit function must be called once at the beginning of the application to initialize the socket handler.
    before any call to the socket function can be made.

@see
    connect
    bind
    listen
    accept
    recv
    recvfrom
    send
    sendto
    shutdown
    setsockopt
    getsockopt

@return
    On successful socket creation, a non-blocking socket type is created and a socket ID is returned
    In case of failure the function returns a negative value, identifying one of the socket error codes defined.
    For example: @ref SOCK_ERR_INVALID for invalid argument or @ref SOCK_ERR_MAX_TCP_SOCK  if the number of TCP
    allocated sockets exceeds the number of available sockets.

@remarks
        The socket function must be called a priori to any other related socket functions "e.g. send, recv, shutdown  ..etc"
\section Socket allocation example
    This example demonstrates the use of the socket function to allocate the socket, returning the socket handler to be used for other
socket operations. Socket creation is dependent on the socket type.

UDP example

@code
    SOCKET UdpServerSocket = -1;

    UdpServerSocket = socket(AF_INET, SOCK_DGRAM, 0);

@endcode

TCP example

@code
    static SOCKET tcp_client_socket = -1;

    tcp_client_socket = socket(AF_INET, SOCK_STREAM, 0));
@endcode

SSL example

@code
static SOCKET ssl_socket = -1;

ssl_socket = socket(AF_INET, SOCK_STREAM, SOCK_FLAGS_SSL));
@endcode
*/
SOCKET socket(uint16_t u16Domain, uint8_t u8Type, uint8_t u8Flags);
/** @} */     //SocketFn

/** @defgroup BindFn bind
 *  @ingroup SocketAPI
*   Asynchronous bind function associates the provided address and local port to the socket.
*   The function can be used with both TCP and UDP sockets. It is mandatory to call the @ref bind function before starting any UDP or TCP server operation.
*   Upon socket bind completion, the application will receive a @ref SOCKET_MSG_BIND message in the socket callback.
*/
/**@{*/
/*!
\fn \
    int8_t bind(SOCKET sock, struct sockaddr *pstrAddr, uint8_t u8AddrLen);

@param[in]  sock
                Socket ID, must hold a non negative value.
                A negative value will return a socket error @ref SOCK_ERR_INVALID_ARG. Indicating that an invalid argument is passed in.

@param[in]  pstrAddr
                Pointer to socket address structure @ref sockaddr_in.

@param[in]  u8AddrLen
                Size of the given socket address structure in bytes.

@pre
    The socket function must be called to allocate a socket before passing the socket ID to the bind function.

@see    socket
@see    connect
@see    listen
@see    accept
@see    recv
@see    recvfrom
@see    send
@see    sendto

@return
    The function returns ZERO for successful operations and a negative value otherwise.
    The possible error values are:
    - @ref SOCK_ERR_NO_ERROR
        Indicating that the operation was successful.

    - @ref SOCK_ERR_INVALID_ARG
        Indicating passing invalid arguments such as negative socket ID or NULL socket address structure.

    - @ref SOCK_ERR_INVALID
        Indicate socket bind failure.
\section bind example
    This example demonstrates the call of the bind socket operation after a successful socket operation.
@code
    struct sockaddr_in  addr;
    SOCKET udpServerSocket =-1;
    int ret = -1;

    if(udpServerSocket == -1)
    {
        udpServerSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if(udpServerSocket >= 0)
        {
            addr.sin_family         = AF_INET;
            addr.sin_port           = _htons(1234);
            addr.sin_addr.s_addr    = 0;
            ret = bind(udpServerSocket,(struct sockaddr*)&addr,sizeof(addr));

            if(ret != 0)
            {
                printf("Bind Failed. Error code = %d\n",ret);
                shutdown(udpServerSocket);
            }
        }
        else
        {
            printf("UDP Server Socket Creation Failed\n");
            return;
        }
    }
@endcode
*/
int8_t bind(SOCKET sock, struct sockaddr *pstrAddr, uint8_t u8AddrLen);
/**@}*/     //BindFn

/** @defgroup ListenFn listen
 *   @ingroup SocketAPI
 *  After successful socket binding to an IP address and port on the system, start listening on a passive socket for incoming connections.
       The socket must be bound on a local port or the listen operation fails.
       Upon the call to the asynchronous listen function, response is received through the event @ref SOCKET_MSG_LISTEN
    in the socket callback.
    A successful listen means the TCP server operation is active. If a connection is accepted, then the application socket callback function is
    notified with the new connected socket through the event @ref SOCKET_MSG_ACCEPT. Hence there is no need to call the @ref accept function
    after calling @ref listen.

    After a connection is accepted, the user is then required to call @ref recv to receive any packets transmitted by the remote host or to receive notification of socket connection
    termination.
 */
/**@{*/
/*!
@fn \
    int8_t listen(SOCKET sock, uint8_t backlog);

@param[in]  sock
                Socket ID, must hold a non negative value.
                A negative value will return a socket error @ref SOCK_ERR_INVALID_ARG. Indicating that an invalid argument is passed in.

@param[in]  backlog
                Not used by the current implementation.

@pre
    The bind function must be called to assign the port number and IP address to the socket before the listen operation.

@see    bind
@see    accept
@see    recv
@see    recvfrom
@see    send
@see    sendto

@return
    The function returns ZERO for successful operations and a negative value otherwise.
    The possible error values are:
    - @ref SOCK_ERR_NO_ERROR
        Indicating that the operation was successful.

    - @ref SOCK_ERR_INVALID_ARG
        Indicating passing invalid arguments such as negative socket ID.

    - @ref SOCK_ERR_INVALID
        Indicate socket listen failure.
\section listen example
This example demonstrates the call of the listen socket operation after a successful socket operation.
@code
    static void TCP_Socketcallback(SOCKET sock, uint8_t u8Msg, void * pvMsg)
    {
        int ret =-1;

        switch(u8Msg)
        {
        case SOCKET_MSG_BIND:
            {
                tstrSocketBindMsg   *pstrBind = (tstrSocketBindMsg*)pvMsg;
                if(pstrBind != NULL)
                {
                    if(pstrBind->status == 0)
                    {
                        ret = listen(sock, 0);

                        if(ret <0)
                            printf("Listen failure! Error = %d\n",ret);
                    }
                    else
                    {
                        M2M_ERR("bind Failure!\n");
                        shutdown(sock);
                    }
                }
            }
            break;

        case SOCKET_MSG_LISTEN:
            {

                tstrSocketListenMsg *pstrListen = (tstrSocketListenMsg*)pvMsg;
                if(pstrListen != NULL)
                {
                    if(pstrListen->status == 0)
                    {
                        ret = accept(sock,NULL,0);
                    }
                    else
                    {
                        M2M_ERR("listen Failure!\n");
                        shutdown(sock);
                    }
                }
            }
            break;

        case SOCKET_MSG_ACCEPT:
            {
                tstrSocketAcceptMsg *pstrAccept = (tstrSocketAcceptMsg*)pvMsg;

                if(pstrAccept->sock >= 0)
                {
                    TcpNotificationSocket = pstrAccept->sock;
                    recv(pstrAccept->sock,gau8RxBuffer,sizeof(gau8RxBuffer),TEST_RECV_TIMEOUT);
                }
                else
                {
                    M2M_ERR("accept failure\n");
                }
            }
            break;

        default:
            break;
        }
    }

@endcode
*/
int8_t listen(SOCKET sock, uint8_t backlog);
/**@}*/     //ListenFn

/** @defgroup AcceptFn accept
 *    @ingroup SocketAPI
 *  The function has no current implementation. An empty declaration is used to prevent errors when legacy application code is used.
 *     For recent application use, the accept function can be safer as it has no effect and could be safely removed from any application using it.
 */
/**@{*/
/*!
@fn \
    int8_t accept(SOCKET sock, struct sockaddr *addr, uint8_t *addrlen);

@param[in]  sock
                Socket ID, must hold a non negative value.
                A negative value will return a socket error @ref SOCK_ERR_INVALID_ARG. Indicating that an invalid argument is passed in.
@param[in]  addr
                Not used in the current implementation.

@param[in]  addrlen
                Not used in the current implementation.

@return
    The function returns ZERO for successful operations and a negative value otherwise.
    The possible error values are:
    - @ref SOCK_ERR_NO_ERROR
        Indicating that the operation was successful.

    - @ref SOCK_ERR_INVALID_ARG
        Indicating passing invalid arguments such as negative socket ID.
*/
int8_t accept(SOCKET sock, struct sockaddr *addr, uint8_t *addrlen);
/** @} */
/** @defgroup ConnectFn connect
    @ingroup SocketAPI
       Establishes a TCP connection with a remote server.
    The asynchronous connect function must be called after receiving a valid socket ID from the @ref socket function.
    The application socket callback function is notified of the result of the connection attempt through the event @ref SOCKET_MSG_CONNECT,
    along with a structure @ref tstrSocketConnectMsg.
    If socket connection fails, the application should call @ref shutdown().
    A successful connect means the TCP session is active. The application is then required to make a call to the @ref recv
    to receive any packets transmitted by the remote server, unless the application is interrupted by a notification of socket connection
    termination.
 */
/**@{*/
/*!
@fn \
    int8_t connect(SOCKET sock, struct sockaddr *pstrAddr, uint8_t u8AddrLen);

@param[in]  sock
                Socket ID, must hold a non negative value.
                A negative value will return a socket error @ref SOCK_ERR_INVALID_ARG. Indicating that an invalid argument is passed in.

@param[in]  pstrAddr
                Address of the remote server.

@param[in]  pstrAddr
                Pointer to socket address structure @ref sockaddr_in.

@param[in]  u8AddrLen
                Size of the given socket address structure in bytes.
                Not currently used, implemented for BSD compatibility only.

@pre
    The socket function must be called to allocate a TCP socket before passing the socket ID to the bind function.
    If the socket is not bound, you do NOT have to call bind before the "connect" function.

@see
    socket
    recv
    send
    shutdown

@return
    The function returns ZERO for successful operations and a negative value otherwise.
    The possible error values are:
    - @ref SOCK_ERR_NO_ERROR
        Indicating that the operation was successful.

    - @ref SOCK_ERR_INVALID_ARG
        Indicating passing invalid arguments such as negative socket ID or NULL socket address structure.

    - @ref SOCK_ERR_INVALID
        Indicate socket connect failure.
\section connect example
   The example demonstrates a TCP application, showing how the asynchronous call to the connect function is made through the main function and how the
   callback function handles the @ref SOCKET_MSG_CONNECT event.

   Main Function

@code
    struct sockaddr_in  Serv_Addr;
    SOCKET TcpClientSocket =-1;
    int ret = -1

    TcpClientSocket = socket(AF_INET,SOCK_STREAM,0);
    Serv_Addr.sin_family = AF_INET;
    Serv_Addr.sin_port = _htons(1234);
    Serv_Addr.sin_addr.s_addr = inet_addr(SERVER);
    printf("Connected to server via socket %u\n",TcpClientSocket);

    do
    {
        ret = connect(TcpClientSocket,(sockaddr_in*)&Serv_Addr,sizeof(Serv_Addr));
        if(ret != 0)
        {
            printf("Connection Error\n");
        }
        else
        {
            printf("Connection successful.\n");
            break;
        }
    }while(1)
@endcode

Socket Callback

@code
    if(u8Msg == SOCKET_MSG_CONNECT)
    {
        tstrSocketConnectMsg    *pstrConnect = (tstrSocketConnectMsg*)pvMsg;
        if(pstrConnect->s8Error == 0)
        {
            uint8_t  acBuffer[GROWL_MSG_SIZE];
            uint16_t u16MsgSize;

            printf("Connect success!\n");

            u16MsgSize = FormatMsg(u8ClientID, acBuffer);
            send(sock, acBuffer, u16MsgSize, 0);
            recv(pstrNotification->Socket, (void*)au8Msg,GROWL_DESCRIPTION_MAX_LENGTH, GROWL_RX_TIMEOUT);
            u8Retry = GROWL_CONNECT_RETRY;
        }
        else
        {
            M2M_DBG("Connection Failed, Error: %d\n",pstrConnect->s8Error");
            shutdown(pstrNotification->Socket);
        }
    }
@endcode
*/
int8_t connect(SOCKET sock, struct sockaddr *pstrAddr, uint8_t u8AddrLen);
/**@}*/     //AcceptFn

/** @defgroup ReceiveFn recv
    @ingroup SocketAPI
        An asynchronous receive function, used to retrieve data from a TCP stream.
    Before calling the recv function, a successful socket connection status must have been received through any of the two socket events
    @ref SOCKET_MSG_CONNECT or @ref SOCKET_MSG_ACCEPT, from  the socket callback. Hence, indicating that the socket is already connected to a remote
    host.
    The application receives the required data in response to this asynchronous call through the reception of the event @ref SOCKET_MSG_RECV in the
    socket callback.

    Receiving the SOCKET_MSG_RECV message in the callback with zero or negative buffer length indicates the following:
    @ref SOCK_ERR_NO_ERROR          : Socket connection closed. The application should now call @ref shutdown().
    @ref SOCK_ERR_CONN_ABORTED      : Socket connection aborted. The application should now call @ref shutdown().
    @ref SOCK_ERR_TIMEOUT           : Socket receive timed out. The socket connection remains open.
    The application code is expected to shutdown the socket through the call to the @ref shutdown function upon the appearance of the above mentioned  errors.
 */
/**@{*/
/*!
@fn \
    int16_t recv(SOCKET sock, void *pvRecvBuf, uint16_t u16BufLen, uint32_t u32Timeoutmsec);

@param[in]  sock
                Socket ID, must hold a non negative value.
                A negative value will return a socket error @ref SOCK_ERR_INVALID_ARG. Indicating that an invalid argument is passed in.


@param[in]  pvRecvBuf
                Pointer to a buffer that will hold the received data. The buffer is used
                in the recv callback to deliver the received data to the caller. The buffer must
                be resident in memory (heap or global buffer).

@param[in]  u16BufLen
                The buffer size in bytes.

@param[in]  u32Timeoutmsec
                Timeout for the recv function in milli-seconds. If the value is set to ZERO, the timeout
                will be set to infinite (the recv function waits forever). If the timeout period is
                elapsed with no data received, the socket will get a timeout error.
@pre
    - The socket function must be called to allocate a TCP socket before passing the socket ID to the recv function.
    - The socket in a connected state is expected to receive data through the socket interface.

@see    socket
@see    connect
@see    bind
@see    listen
@see    recvfrom
@see    shutdown

@return
    The function returns ZERO for successful operations and a negative value otherwise.
    The possible error values are:
    - @ref SOCK_ERR_NO_ERROR
        Indicating that the operation was successful.

    - @ref SOCK_ERR_INVALID_ARG
        Indicating passing invalid arguments such as negative socket ID or NULL Receive buffer.

    - @ref SOCK_ERR_BUFFER_FULL
        Indicate socket receive failure.
\section recv example
   The example demonstrates a code snippet for the calling of the recv function in the socket callback upon notification of the accept or connect events, and the parsing of the
   received data when the @ref SOCKET_MSG_RECV event is received.
@code
    switch(u8Msg)
    {

    case SOCKET_MSG_ACCEPT:
        {
            tstrSocketAcceptMsg *pstrAccept = (tstrSocketAcceptMsg*)pvMsg;

            if(pstrAccept->sock >= 0)
            {
                recv(pstrAccept->sock,gau8RxBuffer,sizeof(gau8RxBuffer),TEST_RECV_TIMEOUT);
            }
            else
            {
                M2M_ERR("accept\n");
            }
        }
        break;


    case SOCKET_MSG_RECV:
        {
            tstrSocketRecvMsg   *pstrRx = (tstrSocketRecvMsg*)pvMsg;

            if(pstrRx->s16BufferSize > 0)
            {
                recv(sock,gau8RxBuffer,sizeof(gau8RxBuffer),TEST_RECV_TIMEOUT);
            }
            else
            {
                printf("Socket recv Error: %d\n",pstrRx->s16BufferSize);
                shutdown(sock);
            }
        }
        break;

    default:
        break;
    }
}
@endcode
*/
int16_t recv(SOCKET sock, void *pvRecvBuf, uint16_t u16BufLen, uint32_t u32Timeoutmsec);
/**@}*/     //ReceiveFn

/** @defgroup ReceiveFromSocketFn recvfrom
 *   @ingroup SocketAPI
 *  Receives data from a UDP Socket.
*
*   The asynchronous recvfrom function is used to retrieve data from a UDP socket. The socket must already be bound to
*   a local port before a call to the recvfrom function is made (i.e message @ref SOCKET_MSG_BIND is received
*   with successful status in the socket callback).
*
*   Upon calling the recvfrom function with a successful return code, the application is expected to receive a notification
*   in the socket callback whenever a message is received through the @ref SOCKET_MSG_RECVFROM event.
*
*   Receiving the SOCKET_MSG_RECVFROM message in the callback with zero, indicates that the socket is closed.
*   Whereby a negative buffer length indicates one of the socket error codes such as socket timeout error @ref SOCK_ERR_TIMEOUT
*
*   The recvfrom callback can also be used to show the IP address of the remote host that sent the frame by
*   using the "strRemoteAddr" element in the @ref tstrSocketRecvMsg structure. (refer to the code example)
 */
/**@{*/
/*!
@fn \
    int16_t recvfrom(SOCKET sock, void *pvRecvBuf, uint16_t u16BufLen, uint32_t u32TimeoutSeconds);

@param[in]  sock
                Socket ID, must hold a non negative value.
                A negative value will return a socket error @ref SOCK_ERR_INVALID_ARG. Indicating that an invalid argument is passed in.

@param[in]  pvRecvBuf
                Pointer to a buffer that will hold the received data. The buffer shall be used
                in the recv callback to deliver the received data to the caller. The buffer must
                be resident in memory (heap or global buffer).

@param[in]  u16BufLen
                The buffer size in bytes.

@param[in]  u32TimeoutSeconds
                Timeout for the recv function in milli-seconds. If the value is set to ZERO, the timeout
                will be set to infinite (the recv function waits forever).

@pre
    - The socket function must be called to allocate a UDP socket before passing the socket ID to the recvfrom function.
    - The socket corresponding to the socket ID must be successfully bound to a local port through the call to a @ref bind function.

@see
    socket
    bind
    shutdown

@return
    The function returns ZERO for successful operations and a negative value otherwise.
    The possible error values are:
    - @ref SOCK_ERR_NO_ERROR
        Indicating that the operation was successful.

    - @ref SOCK_ERR_INVALID_ARG
        Indicating passing invalid arguments such as negative socket ID or NULL Receive buffer.

    - @ref SOCK_ERR_BUFFER_FULL
        Indicate socket receive failure.
\section recvfrom example
   The example demonstrates a code snippet for the calling of the recvfrom function in the socket callback upon notification of a successful bind event, and the parsing of the
   received data when the @ref SOCKET_MSG_RECVFROM event is received.
@code
    switch(u8Msg)
    {
    case SOCKET_MSG_BIND:
        {
            tstrSocketBindMsg   *pstrBind = (tstrSocketBindMsg*)pvMsg;

            if(pstrBind != NULL)
            {
                if(pstrBind->status == 0)
                {
                    recvfrom(sock, gau8SocketTestBuffer, TEST_BUFFER_SIZE, 0);
                }
                else
                {
                    M2M_ERR("bind\n");
                }
            }
        }
        break;

    case SOCKET_MSG_RECVFROM:
        {
            tstrSocketRecvMsg   *pstrRx = (tstrSocketRecvMsg*)pvMsg;

            if(pstrRx->s16BufferSize > 0)
            {
                //get the remote host address and port number
                uint16_t u16port = pstrRx->strRemoteAddr.sin_port;
                uint32_t strRemoteHostAddr = pstrRx->strRemoteAddr.sin_addr.s_addr;

                printf("Received frame with size = %d.\tHost address=%x, Port number = %d\n\n",pstrRx->s16BufferSize,strRemoteHostAddr, u16port);

                ret = recvfrom(sock,gau8SocketTestBuffer,sizeof(gau8SocketTestBuffer),TEST_RECV_TIMEOUT);
            }
            else
            {
                printf("Socket recv Error: %d\n",pstrRx->s16BufferSize);
                ret = shutdown(sock);
            }
        }
        break;

    default:
        break;
    }
}
@endcode
*/
int16_t recvfrom(SOCKET sock, void *pvRecvBuf, uint16_t u16BufLen, uint32_t u32Timeoutmsec);
/**@}*/     //ReceiveFromSocketFn

/** @defgroup SendFn send
 *   @ingroup SocketAPI
*  Asynchronous sending function, used to send data on a TCP/UDP socket.

*  Called by the application code when there is outgoing data available required to be sent on a specific socket handler.
*  The only difference between this function and the similar @ref sendto function, is the type of socket the data is sent on and the parameters passed in.
*  @ref send function is most commonly called for sockets in a connected state.
*  After the data is sent, the socket callback function registered using registerSocketCallback(), is expected to receive an event of type
*  @ref SOCKET_MSG_SEND holding information containing the number of data bytes sent.
 */
/**@{*/
/*!
@fn \
    int16_t send(SOCKET sock, void *pvSendBuffer, uint16_t u16SendLength, uint16_t u16Flags);

@param[in]  sock
                Socket ID, must hold a non negative value.
                A negative value will return a socket error @ref SOCK_ERR_INVALID_ARG. Indicating that an invalid argument is passed in.

@param[in]  pvSendBuffer
                Pointer to a buffer  holding data to be transmitted.

@param[in]  u16SendLength
                The buffer size in bytes.

@param[in]  u16Flags
                Not used in the current implementation.

@pre
    Sockets must be initialized using socketInit. \n

    For TCP Socket:\n
        Must use a successfully connected Socket (so that the intended recipient address is known ahead of sending the data).
        Hence this function is expected to be called after a successful socket connect operation(in client case or accept in the
        the server case).\n

    For UDP Socket:\n
        UDP sockets most commonly use @ref sendto function, where the destination address is defined. However, in-order to send outgoing data
        using the @ref send function, at least one successful call must be made to the @ref sendto function a priori the consecutive calls to the @ref send function,
        to ensure that the destination address is saved in the firmware.


@warning
    u16SendLength must not exceed @ref SOCKET_BUFFER_MAX_LENGTH.\n
    Use a valid socket identifier through the a prior call to the @ref socket function.
    Must use a valid buffer pointer.
    Successful  completion of a call to send() does not guarantee delivery of the message,
    A negative return value indicates only locally-detected errors

@see
    socketInit
    recv
    sendto
    socket
    connect
    accept
    sendto

@return
    The function shall return @ref SOCK_ERR_NO_ERROR for successful operation and a negative value (indicating the error) otherwise.
*/
int16_t send(SOCKET sock, void *pvSendBuffer, uint16_t u16SendLength, uint16_t u16Flags);
/**@}*/     //SendFn

/** @defgroup SendToSocketFn sendto
 *  @ingroup SocketAPI
*    Asynchronous sending function, used to send data on a UDP socket.
*    Called by the application code when there is data required to be sent on a UDP socket handler.
*    The application code is expected to receive data from a successful bounded socket node.
*    The only difference between this function and the similar @ref send function, is the type of socket the data is received on. This function works
*    only with UDP sockets.
*    After the data is sent, the socket callback function registered using registerSocketCallback(), is expected to receive an event of type
*    @ref SOCKET_MSG_SENDTO.
*/
/**@{*/
/*!
@fn \
    int16_t sendto(SOCKET sock, void *pvSendBuffer, uint16_t u16SendLength, uint16_t flags, struct sockaddr *pstrDestAddr, uint8_t u8AddrLen);

@param[in]  sock
                Socket ID, must hold a non negative value.
                A negative value will return a socket error @ref SOCK_ERR_INVALID_ARG. Indicating that an invalid argument is passed in.

@param[in]  pvSendBuffer
                Pointer to a buffer holding data to be transmitted.
                A NULL value will return a socket error @ref SOCK_ERR_INVALID_ARG. Indicating that an invalid argument is passed in.

@param[in]  u16SendLength
                The buffer size in bytes. It must not exceed @ref SOCKET_BUFFER_MAX_LENGTH.

@param[in]  flags
                Not used in the current implementation

@param[in]  pstrDestAddr
                The destination address.

@param[in]  u8AddrLen
                Destination address length in bytes.
                Not used in the current implementation, only included for BSD compatibility.
@pre
        Sockets must be initialized using socketInit.

@warning
    u16SendLength must not exceed @ref SOCKET_BUFFER_MAX_LENGTH. \n
    Use a valid socket (returned from socket ).
    A valid buffer pointer must be used (not NULL). \n
    Successful  completion of a call to sendto() does not guarantee delivery of the message,
    A negative return value indicates only locally-detected errors

@see
    socketInit
    recvfrom
    sendto
    socket
    connect
    accept
    send

@return
    The function  returns @ref SOCK_ERR_NO_ERROR for successful operation and a negative value (indicating the error) otherwise.
*/
int16_t sendto(SOCKET sock, void *pvSendBuffer, uint16_t u16SendLength, uint16_t flags, struct sockaddr *pstrDestAddr, uint8_t u8AddrLen);
/**@}*/     //SendToSocketFn

/** @defgroup CloseSocketFn shutdown
 *  @ingroup SocketAPI
 *  Synchronous close function, releases all the socket assigned resources.
 */
/**@{*/
/*!
@fn \
    int8_t shutdown(SOCKET sock);

@param[in]  sock
                Socket ID, must hold a non negative value.
                A negative value will return a socket error @ref SOCK_ERR_INVALID_ARG. Indicating that an invalid argument is passed in.

@pre
        Sockets must be initialized through the call of the socketInit function.
        @ref shutdown is called only for valid socket identifiers created through the @ref socket function.

@warning
    If @ref shutdown is called while there are still pending messages (sent or received ) they will be discarded.

@see
    socketInit
    socket

@return
    The function returned @ref SOCK_ERR_NO_ERROR for successful operation and a negative value (indicating the error) otherwise.
*/
int8_t shutdown(SOCKET sock);
/**@}*/     //CloseSocketFn

/** @defgroup gethostbynameFn gethostbyname
 *  @ingroup SocketAPI
*   Asynchronous DNS resolving function. This function use DNS to resolve a domain name into the corresponding IP address.
*   A call to this function will cause a DNS request to be sent and the response will be delivered to the DNS callback function registered using registerSocketCallback()
 */
/**@{*/
/*!
@fn \
    int8_t gethostbyname(const char *pcHostName);

@param[in]  pcHostName
                NULL terminated string containing the domain name for the remote host.
                Its size must not exceed @ref HOSTNAME_MAX_SIZE.

@warning
    Successful completion of a call to gethostbyname() does not guarantee success of the DNS request,
    a negative return value indicates only locally-detected errors

@see
    registerSocketCallback

@return
    - @ref SOCK_ERR_NO_ERROR
    - @ref SOCK_ERR_INVALID_ARG
*/
int8_t gethostbyname(const char *pcHostName);
/**@}*/     //gethostbynameFn

/** @defgroup sslEnableCertExpirationCheckFn sslEnableCertExpirationCheck
 *  @ingroup SocketAPI
 *  Configure the behavior of the SSL Library for Certificate Expiry Validation.
 */
/**@{*/
/*!
@fn \
int8_t sslEnableCertExpirationCheck(tenuSslCertExpSettings enuValidationSetting);

@param [in] enuValidationSetting
                See @ref tenuSslCertExpSettings for details.

@sa     tenuSslCertExpSettings

@return
    - @ref SOCK_ERR_NO_ERROR for successful operation and negative error code otherwise.
*/
int8_t sslEnableCertExpirationCheck(tenuSslCertExpSettings enuValidationSetting);
/**@}*/     //sslEnableCertExpirationCheckFn

/** @defgroup SetSocketOptionFn setsockopt
 *  @ingroup SocketAPI
*The setsockopt() function shall set the option specified by the option_name
*   argument, at the protocol level specified by the level argument, to the value
*   pointed to by the option_value argument for the socket specified by the socket argument.
*
* <p>Possible protocol level values supported are @ref SOL_SOCKET and @ref SOL_SSL_SOCKET.
* Possible options when the protocol level is @ref SOL_SOCKET :</p>
* <table style="width: 100%">
*   <tr>
*       <td style="height: 22px"><strong>@ref SO_SET_UDP_SEND_CALLBACK</strong></td>
*       <td style="height: 22px">Enable/Disable callback messages for sendto().
*       Since UDP is unreliable by default the user maybe interested (or not) in
*       receiving a message of @ref SOCKET_MSG_SENDTO for each call of sendto().
*       Enabled if option value equals TRUE, disabled otherwise.</td>
*   </tr>
*   <tr>
*       <td><strong>@ref IP_ADD_MEMBERSHIP</strong></td>
*       <td>Valid for UDP sockets. This option is used to receive frames sent to
*       a multicast group. option_value shall be a pointer to Unsigned 32-bit
*       integer containing the multicast IPv4 address. </td>
*   </tr>
*   <tr>
*       <td><strong>@ref IP_DROP_MEMBERSHIP</strong></td>
*       <td>Valid for UDP sockets. This option is used to stop receiving frames
*       sent to a multicast group. option_value shall be a pointer to Unsigned
*       32-bit integer containing the multicast IPv4 address.</td>
*   </tr>
* </table>
* <p>Possible options when the protocol level&nbsp; is @ref SOL_SSL_SOCKET</p>
* <table style="width: 100%">
*   <tr>
*       <td style="height: 22px"><strong>
*       @ref SO_SSL_BYPASS_X509_VERIF</strong></td>
*       <td style="height: 22px">Allow an opened SSL socket to bypass the X509
*       certificate verification process. It is highly recommended <strong>NOT</strong> to use
*       this socket option in production software applications. The option is
*       supported for debugging and testing purposes. The option value should be
*       casted to int type and it is handled as a boolean flag.</td>
*   </tr>
*   <tr>
*       <td><strong>@ref SO_SSL_SNI</strong></td>
*       <td>Set the Server Name Indicator (SNI) for an SSL socket. The SNI is a
*       null terminated string containing the server name associated with the
*       connection. It must not exceed the size of @ref HOSTNAME_MAX_SIZE.</td>
*   </tr>
*   <tr>
*       <td><strong>@ref SO_SSL_ENABLE_SESSION_CACHING</strong></td>
*       <td>This option allow the TLS to cache the session information for fast
*       TLS session establishment in future connections using the TLS Protocol
*       session resume features.</td>
*   </tr>
* </table>
 */
 /**@{*/
/*!
@fn \
    int8_t setsockopt(SOCKET socket, uint8_t u8Level, uint8_t option_name,
       const void *option_value, uint16_t u16OptionLen);

@param[in]  socket
                Socket handler.

@param[in]  u8Level
                Protocol level. See description above.

@param[in]  option_name
                Option to be set. See description above.

@param[in]  option_value
                Pointer to user provided value.

@param[in]  u16OptionLen
                Length of the option value in bytes.

@sa SOL_SOCKET, SOL_SSL_SOCKET, IP_ADD_MEMBERSHIP, IP_DROP_MEMBERSHIP

@return
    The function shall return \ref SOCK_ERR_NO_ERROR for successful operation
    and a negative value (indicating the error) otherwise.
*/
int8_t setsockopt(SOCKET socket, uint8_t u8Level, uint8_t option_name,
       const void *option_value, uint16_t u16OptionLen);
/**@}*/     //SetSocketOptionFn

/** @defgroup GetSocketOptionsFn getsockopt
 *  @ingroup SocketAPI
 *   Get socket options retrieves
*    This Function isn't implemented yet but this is the form that will be released later.
 */
/**@{*/
/*!
@fn \
    int8_t getsockopt(SOCKET sock, uint8_t u8Level, uint8_t u8OptName, const void *pvOptValue, uint8_t * pu8OptLen);

@brief

@param[in]  sock
                Socket Identifier.
@param[in]  u8Level
                The protocol level of the option.
@param[in]  u8OptName
                The u8OptName argument specifies a single option to get.
@param[out] pvOptValue
                The pvOptValue argument contains pointer to a buffer containing the option value.
@param[out] pu8OptLen
                Option value buffer length.
@return
    The function shall return ZERO for successful operation and a negative value otherwise.
*/
int8_t getsockopt(SOCKET sock, uint8_t u8Level, uint8_t u8OptName, const void *pvOptValue, uint8_t* pu8OptLen);
/**@}*/     //GetSocketOptionsFn

/**@}*/
/** @defgroup PingFn m2m_ping_req
 *   @ingroup SocketAPI
 *      The function sends ping request to the given IP Address.
 */
/**@{*/
/*!
@fn \
        int8_t m2m_ping_req(uint32_t u32DstIP, uint8_t u8TTL, tpfPingCb fpPingCb);

@param[in]  u32DstIP
                Target Destination IP Address for the ping request. It must be represented in Network byte order.
                The function inet_addr could be used to translate the dotted decimal notation IP
                to its Network bytes order integer representative.

@param[in]  u8TTL
                IP TTL value for the ping request. If set to ZERO, the default value SHALL be used.

@param[in]  fpPingCb
                Callback will be called to deliver the ping statistics.

@see        inet_addr
@return     The function returns @ref M2M_SUCCESS for successful operations and a negative value otherwise.
*/
int8_t m2m_ping_req(uint32_t u32DstIP, uint8_t u8TTL, tpfPingCb fpPingCb);
/**@}*/     //PingFn

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __SOCKET_H__ */

/**
 * @file http_downloader.h
 * @brief HTTP file downloader 
 * @author Adithya Vishnu Sankar
 */

#ifndef HTTP_DOWNLOADER_H
#define HTTP_DOWNLOADER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "system/wifi/sys_wincs_wifi_service.h"
#include "system/net/sys_wincs_net_service.h"
/* -------------------------------------------------------------------------- */
/* CONSTANTS                                                                  */
/* -------------------------------------------------------------------------- */

/** @brief Maximum URL length */
#define HTTP_MAX_URL_LENGTH         (2512)

/** @brief Maximum hostname length */
#define HTTP_MAX_HOSTNAME_LENGTH    (128)

/** @brief Maximum path length */
#define HTTP_MAX_PATH_LENGTH        (2512)

/** @brief HTTP default port */
#define HTTP_DEFAULT_PORT           (80)

/** @brief Maximum HTTP header length */
#define HTTP_MAX_HEADER_LENGTH      (2512)

/** @brief HTTP read buffer size */
#define HTTP_READ_BUFFER_SIZE       (1024)

/** @brief DNS resolution timeout */
#define HTTP_DNS_TIMEOUT_MS         (30000)

/** @brief HTTP connection timeout */
#define HTTP_CONNECT_TIMEOUT_MS     (30000)

/** @brief HTTP read timeout */
#define HTTP_READ_TIMEOUT_MS        (200000)

/* -------------------------------------------------------------------------- */
/* TYPES                                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief HTTP downloader status codes
 */
typedef enum
{
    HTTP_STATUS_OK = 0,
    HTTP_STATUS_ERROR,
    HTTP_STATUS_INVALID_URL,
    HTTP_STATUS_DNS_FAILED,
    HTTP_STATUS_CONNECT_FAILED,
    HTTP_STATUS_REQUEST_FAILED,
    HTTP_STATUS_RESPONSE_ERROR
} THttpStatus;

/**
 * @brief Parsed URL structure
 */
typedef struct
{
    char hostname[HTTP_MAX_HOSTNAME_LENGTH];
    char path[HTTP_MAX_PATH_LENGTH];
    uint16_t port;
} THttpUrl;

/* -------------------------------------------------------------------------- */
/* FUNCTION PROTOTYPES                                                        */
/* -------------------------------------------------------------------------- */

/**
 * @brief Parse HTTP URL into components
 * 
 * @param url Full HTTP URL string
 * @param parsedUrl Pointer to store parsed components
 * @return HTTP status code
 */
THttpStatus httpParseUrl(const char* url, THttpUrl* parsedUrl);

/**
 * @brief Resolve hostname to IPv4 address
 * 
 * @param hostname Hostname to resolve
 * @param ipAddress Buffer to store resolved IP (minimum 16 bytes)
 * @return HTTP status code
 */
THttpStatus httpResolveHostname(const char* hostname, char* ipAddress);

/**
 * @brief Download file from HTTP URL and print content
 * 
 * @param parsedUrl parsed url structure
 * @param ipAddress ipaddress of host
 * @return HTTP status code
 */
THttpStatus httpDownloadFile(const THttpUrl* parsedUrl, const char* ipAddress);

/**
 * @brief Send HTTP GET request
 * 
 * @param socketId Socket ID for the connection
 * @param hostname Server hostname
 * @param path File path on server
 * @return HTTP status code
 */
THttpStatus httpSendGetRequest(uint32_t socketId, const char* hostname, const char* path);

/**
 * @brief Read and parse HTTP response
 * 
 * @param socketId Socket ID for the connection
 * @return HTTP status code
 */
THttpStatus httpReadResponse(uint32_t socketId);

/**
 * @brief Socket callback function
 * 
 * @param called from k_sal_com.c
 * @return void
 */
void httpSocketCallback(uint32_t socket, SYS_WINCS_NET_SOCK_EVENT_t event, SYS_WINCS_NET_HANDLE_t netHandle);

#endif /* HTTP_DOWNLOADER_H */
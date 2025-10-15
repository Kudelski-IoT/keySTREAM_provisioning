/*******************************************************************************
********************************************************************************
* File Downloader for WINCS02 WiFi Module on PIC32CX1025SG41
* Based on the provided k_sal_com.c socket implementation
********************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "system/wifi/sys_wincs_wifi_service.h"
#include "system/net/sys_wincs_net_service.h"

/* -------------------------------------------------------------------------- */
/* CONSTANTS AND DEFINITIONS                                                  */
/* -------------------------------------------------------------------------- */

#define DOWNLOAD_BUFFER_SIZE        2048
#define HTTP_RESPONSE_BUFFER_SIZE   4096
#define MAX_URL_LENGTH              1024
#define MAX_HOST_LENGTH             256
#define MAX_PATH_LENGTH             512
#define HTTP_PORT                   80
#define HTTPS_PORT                  443
#define DOWNLOAD_TIMEOUT_MS         30000

/* HTTP Response codes */
#define HTTP_OK                     200
#define HTTP_MOVED_PERMANENTLY      301
#define HTTP_FOUND                  302

/* -------------------------------------------------------------------------- */
/* TYPES AND STRUCTURES                                                       */
/* -------------------------------------------------------------------------- */

typedef enum {
    DOWNLOAD_STATUS_OK = 0,
    DOWNLOAD_STATUS_ERROR,
    DOWNLOAD_STATUS_PARAMETER_ERROR,
    DOWNLOAD_STATUS_NETWORK_ERROR,
    DOWNLOAD_STATUS_HTTP_ERROR,
    DOWNLOAD_STATUS_TIMEOUT,
    DOWNLOAD_STATUS_INSUFFICIENT_MEMORY
} TDownloadStatus;

typedef struct {
    char host[MAX_HOST_LENGTH];
    char path[MAX_PATH_LENGTH];
    uint16_t port;
    bool isHttps;
} TUrlComponents;

typedef struct {
    uint32_t socketId;
    bool isConnected;
    volatile bool socketEventComplete;
    volatile bool socketSendComplete;
    volatile bool socketReadEvent;
    uint32_t totalBytesDownloaded;
    uint8_t* downloadBuffer;
} TDownloadContext;

/* -------------------------------------------------------------------------- */
/* GLOBAL VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

static TDownloadContext g_downloadContext = {0};
static SYS_WINCS_NET_SOCKET_t g_downloadSocket;

/* -------------------------------------------------------------------------- */
/* FUNCTION PROTOTYPES                                                        */
/* -------------------------------------------------------------------------- */

static TDownloadStatus parseUrl(const char* url, TUrlComponents* components);
static TDownloadStatus createHttpGetRequest(const TUrlComponents* components, char* request, size_t requestSize);
static TDownloadStatus parseHttpResponse(const uint8_t* response, size_t responseSize, uint32_t* contentLength, uint32_t* headerLength);
static void downloadSocketCallback(uint32_t socket, SYS_WINCS_NET_SOCK_EVENT_t event, SYS_WINCS_NET_HANDLE_t netHandle);
static TDownloadStatus connectToServer(const TUrlComponents* components);
static TDownloadStatus sendHttpRequest(const char* request);
static TDownloadStatus receiveHttpResponse(uint8_t* buffer, size_t bufferSize, size_t* bytesReceived);
static void printDownloadProgress(uint32_t downloaded, uint32_t total);

/* -------------------------------------------------------------------------- */
/* SOCKET CALLBACK IMPLEMENTATION                                             */
/* -------------------------------------------------------------------------- */

static void downloadSocketCallback(uint32_t socket, SYS_WINCS_NET_SOCK_EVENT_t event, SYS_WINCS_NET_HANDLE_t netHandle)
{
    switch(event)
    {
        case SYS_WINCS_NET_SOCK_EVENT_CONNECTED:
            printf("[DOWNLOAD] Connected to server\r\n");
            g_downloadContext.isConnected = true;
            g_downloadContext.socketEventComplete = true;
            break;

        case SYS_WINCS_NET_SOCK_EVENT_DISCONNECTED:
            printf("[DOWNLOAD] Disconnected from server\r\n");
            g_downloadContext.isConnected = false;
            g_downloadContext.socketEventComplete = true;
            break;

        case SYS_WINCS_NET_SOCK_EVENT_ERROR:
            printf("[DOWNLOAD] Socket error occurred\r\n");
            g_downloadContext.socketEventComplete = true;
            break;

        case SYS_WINCS_NET_SOCK_EVENT_READ:
            g_downloadContext.socketReadEvent = true;
            break;

        case SYS_WINCS_NET_SOCK_EVENT_SEND_COMPLETE:
            g_downloadContext.socketSendComplete = true;
            break;

        case SYS_WINCS_NET_SOCK_EVENT_CLOSED:
            printf("[DOWNLOAD] Socket closed\r\n");
            g_downloadContext.socketEventComplete = true;
            break;

        default:
            break;
    }
}

/* -------------------------------------------------------------------------- */
/* URL PARSING IMPLEMENTATION                                                 */
/* -------------------------------------------------------------------------- */

static TDownloadStatus parseUrl(const char* url, TUrlComponents* components)
{
    if (!url || !components) {
        return DOWNLOAD_STATUS_PARAMETER_ERROR;
    }

    // Initialize components
    memset(components, 0, sizeof(TUrlComponents));
    components->port = HTTP_PORT;
    components->isHttps = false;

    // Check for protocol
    const char* protocolEnd = strstr(url, "://");
    if (!protocolEnd) {
        return DOWNLOAD_STATUS_PARAMETER_ERROR;
    }

    // Determine if HTTPS
    if (strncmp(url, "https", 5) == 0) {
        components->isHttps = true;
        components->port = HTTPS_PORT;
    }

    // Move past protocol
    const char* hostStart = protocolEnd + 3;
    
    // Find the end of host (either '/' for path or ':' for port)
    const char* hostEnd = hostStart;
    while (*hostEnd && *hostEnd != '/' && *hostEnd != ':') {
        hostEnd++;
    }

    // Copy host
    size_t hostLen = hostEnd - hostStart;
    if (hostLen >= MAX_HOST_LENGTH) {
        return DOWNLOAD_STATUS_PARAMETER_ERROR;
    }
    strncpy(components->host, hostStart, hostLen);
    components->host[hostLen] = '\0';

    // Check for custom port
    if (*hostEnd == ':') {
        hostEnd++; // Skip ':'
        components->port = (uint16_t)atoi(hostEnd);
        
        // Find path start
        while (*hostEnd && *hostEnd != '/') {
            hostEnd++;
        }
    }

    // Copy path
    if (*hostEnd == '/') {
        strncpy(components->path, hostEnd, MAX_PATH_LENGTH - 1);
        components->path[MAX_PATH_LENGTH - 1] = '\0';
    } else {
        strcpy(components->path, "/");
    }

    return DOWNLOAD_STATUS_OK;
}

/* -------------------------------------------------------------------------- */
/* HTTP REQUEST CREATION                                                      */
/* -------------------------------------------------------------------------- */

static TDownloadStatus createHttpGetRequest(const TUrlComponents* components, char* request, size_t requestSize)
{
    if (!components || !request) {
        return DOWNLOAD_STATUS_PARAMETER_ERROR;
    }

    int result = snprintf(request, requestSize,
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: WINCS02-Downloader/1.0\r\n"
        "Connection: close\r\n"
        "Accept: */*\r\n"
        "\r\n",
        components->path,
        components->host);

    if (result < 0 || (size_t)result >= requestSize) {
        return DOWNLOAD_STATUS_PARAMETER_ERROR;
    }

    return DOWNLOAD_STATUS_OK;
}

/* -------------------------------------------------------------------------- */
/* HTTP RESPONSE PARSING                                                      */
/* -------------------------------------------------------------------------- */

static TDownloadStatus parseHttpResponse(const uint8_t* response, size_t responseSize, uint32_t* contentLength, uint32_t* headerLength)
{
    if (!response || !contentLength || !headerLength) {
        return DOWNLOAD_STATUS_PARAMETER_ERROR;
    }

    *contentLength = 0;
    *headerLength = 0;

    // Find end of headers (double CRLF)
    const char* headerEnd = strstr((const char*)response, "\r\n\r\n");
    if (!headerEnd) {
        return DOWNLOAD_STATUS_HTTP_ERROR;
    }

    *headerLength = (headerEnd - (const char*)response) + 4;

    // Check HTTP status code
    if (strncmp((const char*)response, "HTTP/1.1 200", 12) != 0 &&
        strncmp((const char*)response, "HTTP/1.0 200", 12) != 0) {
        printf("[DOWNLOAD] HTTP Error - Non-200 status code\r\n");
        return DOWNLOAD_STATUS_HTTP_ERROR;
    }

    // Find Content-Length header
    const char* contentLengthStr = strstr((const char*)response, "Content-Length:");
    if (contentLengthStr && contentLengthStr < headerEnd) {
        contentLengthStr += 15; // Skip "Content-Length:"
        while (*contentLengthStr == ' ') contentLengthStr++; // Skip spaces
        *contentLength = (uint32_t)atoi(contentLengthStr);
    }

    return DOWNLOAD_STATUS_OK;
}

/* -------------------------------------------------------------------------- */
/* NETWORK COMMUNICATION                                                      */
/* -------------------------------------------------------------------------- */

static TDownloadStatus connectToServer(const TUrlComponents* components)
{
    // Configure socket
    memset(&g_downloadSocket, 0, sizeof(g_downloadSocket));
    g_downloadSocket.bindType = SYS_WINCS_NET_BIND_REMOTE;
    strncpy(g_downloadSocket.sockAddr, components->host, sizeof(g_downloadSocket.sockAddr) - 1);
    g_downloadSocket.sockType = SYS_WINCS_NET_SOCK_TYPE0;
    g_downloadSocket.sockPort = components->port;
    g_downloadSocket.tlsEnable = components->isHttps ? 1 : 0;
    g_downloadSocket.ipType = SYS_WINCS_NET_SOCK_TYPE_IPv4_0;

    // Set callback
    SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_SET_CALLBACK, downloadSocketCallback);

    // Reset flags
    g_downloadContext.socketEventComplete = false;
    g_downloadContext.isConnected = false;

    // Create socket
    if (SYS_WINCS_FAIL == SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_TCP_OPEN, &g_downloadSocket)) {
        printf("[DOWNLOAD] Failed to create socket\r\n");
        return DOWNLOAD_STATUS_NETWORK_ERROR;
    }

    // Wait for connection
    uint32_t timeout = DOWNLOAD_TIMEOUT_MS / 10; // 100ms intervals
    while (!g_downloadContext.socketEventComplete && timeout > 0) {
        WDRV_WINC_Tasks(sysObj.drvWifiWinc);
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout--;
    }

    if (!g_downloadContext.isConnected) {
        printf("[DOWNLOAD] Connection timeout or failed\r\n");
        return DOWNLOAD_STATUS_TIMEOUT;
    }

    g_downloadContext.socketId = g_downloadSocket.sockID;
    printf("[DOWNLOAD] Connected to %s:%d\r\n", components->host, components->port);
    
    return DOWNLOAD_STATUS_OK;
}

static TDownloadStatus sendHttpRequest(const char* request)
{
    if (!request) {
        return DOWNLOAD_STATUS_PARAMETER_ERROR;
    }

    g_downloadContext.socketSendComplete = false;
    
    size_t requestLen = strlen(request);
    if (SYS_WINCS_FAIL == SYS_WINCS_NET_TcpSockWrite(g_downloadContext.socketId, 
                                                      (uint16_t)requestLen, 
                                                      (uint8_t*)request)) {
        printf("[DOWNLOAD] Failed to send HTTP request\r\n");
        return DOWNLOAD_STATUS_NETWORK_ERROR;
    }

    // Wait for send completion
    uint32_t timeout = 5000; // 5 second timeout
    while (!g_downloadContext.socketSendComplete && timeout > 0) {
        WDRV_WINC_Tasks(sysObj.drvWifiWinc);
        vTaskDelay(pdMS_TO_TICKS(1));
        timeout--;
    }

    if (!g_downloadContext.socketSendComplete) {
        printf("[DOWNLOAD] Send timeout\r\n");
        return DOWNLOAD_STATUS_TIMEOUT;
    }

    printf("[DOWNLOAD] HTTP request sent\r\n");
    return DOWNLOAD_STATUS_OK;
}

static TDownloadStatus receiveHttpResponse(uint8_t* buffer, size_t bufferSize, size_t* bytesReceived)
{
    if (!buffer || !bytesReceived) {
        return DOWNLOAD_STATUS_PARAMETER_ERROR;
    }

    *bytesReceived = 0;
    uint32_t timeout = 10000; // 10 second timeout

    // Wait for data
    while (!g_downloadContext.socketReadEvent && timeout > 0) {
        WDRV_WINC_Tasks(sysObj.drvWifiWinc);
        vTaskDelay(pdMS_TO_TICKS(1));
        timeout--;
    }

    if (!g_downloadContext.socketReadEvent) {
        return DOWNLOAD_STATUS_TIMEOUT;
    }

    g_downloadContext.socketReadEvent = false;

    // Read data
    int bytesRead = SYS_WINCS_NET_TcpSockRead(g_downloadContext.socketId,
                                              bufferSize,
                                              buffer);

    if (bytesRead <= 0) {
        return DOWNLOAD_STATUS_NETWORK_ERROR;
    }

    *bytesReceived = (size_t)bytesRead;
    return DOWNLOAD_STATUS_OK;
}

/* -------------------------------------------------------------------------- */
/* PROGRESS DISPLAY                                                           */
/* -------------------------------------------------------------------------- */

static void printDownloadProgress(uint32_t downloaded, uint32_t total)
{
    if (total > 0) {
        uint32_t percentage = (downloaded * 100) / total;
        printf("[DOWNLOAD] Progress: %lu/%lu bytes (%lu%%)\r\n", 
               (unsigned long)downloaded, (unsigned long)total, (unsigned long)percentage);
    } else {
        printf("[DOWNLOAD] Downloaded: %lu bytes\r\n", (unsigned long)downloaded);
    }
}

/* -------------------------------------------------------------------------- */
/* MAIN DOWNLOAD FUNCTION                                                     */
/* -------------------------------------------------------------------------- */

TDownloadStatus downloadFile(const char* url, uint8_t* outputBuffer, size_t outputBufferSize, size_t* downloadedSize)
{
    TDownloadStatus status = DOWNLOAD_STATUS_OK;
    TUrlComponents urlComponents;
    char httpRequest[1024];
    uint8_t* responseBuffer = NULL;
    size_t totalDownloaded = 0;
    uint32_t contentLength = 0;
    uint32_t headerLength = 0;
    bool headersParsed = false;

    printf("[DOWNLOAD] Starting download from: %s\r\n", url);

    // Validate parameters
    if (!url || !outputBuffer || !downloadedSize || outputBufferSize == 0) {
        return DOWNLOAD_STATUS_PARAMETER_ERROR;
    }

    *downloadedSize = 0;

    // Allocate response buffer
    responseBuffer = malloc(HTTP_RESPONSE_BUFFER_SIZE);
    if (!responseBuffer) {
        printf("[DOWNLOAD] Failed to allocate response buffer\r\n");
        return DOWNLOAD_STATUS_INSUFFICIENT_MEMORY;
    }

    // Parse URL
    status = parseUrl(url, &urlComponents);
    if (status != DOWNLOAD_STATUS_OK) {
        printf("[DOWNLOAD] Failed to parse URL\r\n");
        goto cleanup;
    }

    printf("[DOWNLOAD] Connecting to %s:%d\r\n", urlComponents.host, urlComponents.port);

    // Connect to server
    status = connectToServer(&urlComponents);
    if (status != DOWNLOAD_STATUS_OK) {
        goto cleanup;
    }

    // Create HTTP request
    status = createHttpGetRequest(&urlComponents, httpRequest, sizeof(httpRequest));
    if (status != DOWNLOAD_STATUS_OK) {
        printf("[DOWNLOAD] Failed to create HTTP request\r\n");
        goto cleanup;
    }

    // Send HTTP request
    status = sendHttpRequest(httpRequest);
    if (status != DOWNLOAD_STATUS_OK) {
        goto cleanup;
    }

    // Receive and process response
    while (g_downloadContext.isConnected && totalDownloaded < outputBufferSize) {
        size_t bytesReceived = 0;
        status = receiveHttpResponse(responseBuffer, HTTP_RESPONSE_BUFFER_SIZE, &bytesReceived);
        
        if (status == DOWNLOAD_STATUS_TIMEOUT) {
            // Check if we've received some data
            if (totalDownloaded > 0) {
                printf("[DOWNLOAD] Download completed (connection closed by server)\r\n");
                status = DOWNLOAD_STATUS_OK;
            }
            break;
        }
        
        if (status != DOWNLOAD_STATUS_OK) {
            break;
        }

        // Parse headers on first response
        if (!headersParsed) {
            status = parseHttpResponse(responseBuffer, bytesReceived, &contentLength, &headerLength);
            if (status != DOWNLOAD_STATUS_OK) {
                printf("[DOWNLOAD] Failed to parse HTTP response\r\n");
                break;
            }

            headersParsed = true;
            printf("[DOWNLOAD] Content-Length: %lu bytes\r\n", (unsigned long)contentLength);

            // Check if output buffer is large enough
            if (contentLength > 0 && contentLength > outputBufferSize) {
                printf("[DOWNLOAD] Output buffer too small. Need %lu bytes, have %lu\r\n", 
                       (unsigned long)contentLength, (unsigned long)outputBufferSize);
                status = DOWNLOAD_STATUS_INSUFFICIENT_MEMORY;
                break;
            }

            // Copy data after headers
            size_t dataInFirstResponse = bytesReceived - headerLength;
            if (dataInFirstResponse > 0) {
                size_t copySize = (dataInFirstResponse > (outputBufferSize - totalDownloaded)) ? 
                                  (outputBufferSize - totalDownloaded) : dataInFirstResponse;
                memcpy(outputBuffer + totalDownloaded, responseBuffer + headerLength, copySize);
                totalDownloaded += copySize;
            }
        } else {
            // Copy received data
            size_t copySize = (bytesReceived > (outputBufferSize - totalDownloaded)) ? 
                              (outputBufferSize - totalDownloaded) : bytesReceived;
            memcpy(outputBuffer + totalDownloaded, responseBuffer, copySize);
            totalDownloaded += copySize;
        }

        // Print progress
        printDownloadProgress(totalDownloaded, contentLength);

        // Check if download is complete
        if (contentLength > 0 && totalDownloaded >= contentLength) {
            printf("[DOWNLOAD] Download completed successfully\r\n");
            break;
        }
    }

    *downloadedSize = totalDownloaded;

cleanup:
    // Close socket
    if (g_downloadContext.isConnected) {
        g_downloadContext.socketEventComplete = false;
        SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_CLOSE, &g_downloadContext.socketId);
        
        // Wait for socket to close
        uint32_t timeout = 5000;
        while (!g_downloadContext.socketEventComplete && timeout > 0) {
            WDRV_WINC_Tasks(sysObj.drvWifiWinc);
            vTaskDelay(pdMS_TO_TICKS(1));
            timeout--;
        }
    }

    // Free allocated memory
    if (responseBuffer) {
        free(responseBuffer);
    }

    if (status == DOWNLOAD_STATUS_OK && totalDownloaded > 0) {
        printf("[DOWNLOAD] Successfully downloaded %lu bytes\r\n", (unsigned long)totalDownloaded);
    } else {
        printf("[DOWNLOAD] Download failed with status: %d\r\n", status);
    }

    return status;
}

/* -------------------------------------------------------------------------- */
/* EXAMPLE USAGE FUNCTION                                                     */
/* -------------------------------------------------------------------------- */

void downloadFileExample(void)
{
    const char* downloadUrl = "http://iot-bds-fota-236185153589.s3.eu-west-1.amazonaws.com/prod/b4659ba7-d84c-4363-a0b6-4af7d0e75604/ATECC608_0123795E6947FD3E01.zip?X-Amz-Security-Token=IQoJb3JpZ2luX2VjELz%2F%2F%2F%2F%2F%2F%2F%2F%2F%2FwEaCWV1LXdlc3QtMSJIMEYCIQCCvb0V5LzvtFz%2FeW%2BhQo9oa%2Fd%2Bz5yz2wC1dRg6GIdLrQIhAM3Ps2f4re4La2N%2FV3HcdIlNG03SvkPLVqhNWxtTXMf2KoQECKX%2F%2F%2F%2F%2F%2F%2F%2F%2F%2FwEQBBoMMjM2MTg1MTUzNTg5IgyvFtTjJw5lkMFRtVsq2AMVjo3y0eIOZJnejCnM5%2FAAKJZMvQFNnIVMRhn0qjBgK%2BCIiGJLWQqbBg%2FkdIqMPH3YVMFFawoSsSqbzadS%2BXzxlyGl6AMgFPnTfVGV0x%2FgGJWWIjfZA%2BxS%2BmsNE3lUJb0M0eFPjRoS6YUHeqHmTL9D9p%2Fw9j2HUJgzlvTZm4%2FM%2FBFfnPhS3boQ47pWABTReUVjEfF4A3h8SG7d%2BIqb47dRtURcui0MRfudEwhtBuatGWqfBdQMM3buC6l1j9fBLcjPlYMDZysvnkaOCAFFRGxii%2BgyCoYUeppQ5jDH9TtaimPpcrTjdWh3z%2FeHG3kklCziKePvKLu2F4R0mYfQLOo2IbYDdk%2Fwt3odfM1gakSVciHl1DEwVAx%2F8eDSPEvVJRGSe7RW0QNSTSuSNJfQ%2ByXL8Uw5JUBQK9ezZ8TtYoRyw64zcX3EdNZ0%2FTz8gOmxDbUisChBiseBkrho98CsCfR8mO%2Fq4z4MJaicG4bEWdjrK4ZgR9hkyBqlt4oYy4ya0Cvv1J7BcuP12Bb5WSFn0gLCHacecpS8gqHg%2FstCeBzBOlHCeqcb2%2B5ayye6iltp1zMLvKZmlOQ0nH97nQ18TYtvjtQ%2B94dKFynjSL232ROIbBa2096EkAfUMNPoz8IGOqQBk%2FLhPnEpaM%2FaZm4NXTGmVqbtHGwY%2BlhHArWb%2BJhUWghGGtZesjb5p3kklZiyLcMA6K3AILawZGrlCwVdMCh1NG%2FE2CiVOnQFqjOze3VdYzczJpm%2BOUmidahBvZq7TOcY7KJda9qbi1%2FzuEGr%2BVhpjwQyEMTLxS3rUENVzxdJBLcNxqcEoFhKXhuJ9N1r4djaen50ne4jLlXk5BijRsWQHZMOddY%3D&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Date=20250619T113246Z&X-Amz-SignedHeaders=host&X-Amz-Expires=604800&X-Amz-Credential=ASIATN7N3OA2Q3VJRR4K%2F20250619%2Feu-west-1%2Fs3%2Faws4_request&X-Amz-Signature=c533d178b41f568611cf44ba83d2fe1723d4a9e33dbf7e69d9d3be918e75ffe8";
    
    // Allocate buffer for downloaded file (adjust size as needed)
    uint8_t* downloadBuffer = malloc(1024 * 1024); // 1MB buffer
    size_t downloadedSize = 0;
    
    if (!downloadBuffer) {
        printf("[DOWNLOAD] Failed to allocate download buffer\r\n");
        return;
    }
    
    TDownloadStatus result = downloadFile(downloadUrl, downloadBuffer, 1024 * 1024, &downloadedSize);
    
    if (result == DOWNLOAD_STATUS_OK && downloadedSize > 0) {
        printf("[DOWNLOAD] File downloaded successfully!\r\n");
        printf("[DOWNLOAD] File size: %lu bytes\r\n", (unsigned long)downloadedSize);
        
        // Here you can process the downloaded file
        // For example, save to flash memory, parse the ZIP file, etc.
        
        // Example: Print first few bytes as hex
        printf("[DOWNLOAD] First 16 bytes: ");
        for (int i = 0; i < 16 && i < downloadedSize; i++) {
            printf("%02X ", downloadBuffer[i]);
        }
        printf("\r\n");
        
    } else {
        printf("[DOWNLOAD] Download failed with status: %d\r\n", result);
    }
    
    // Clean up
    free(downloadBuffer);
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
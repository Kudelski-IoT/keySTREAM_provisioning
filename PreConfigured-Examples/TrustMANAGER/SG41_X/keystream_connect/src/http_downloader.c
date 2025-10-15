/**
 * @file http_downloader.c
 * @brief HTTP Range Request Downloader with Flash Writing - Optimized for 2048-byte chunks
 * @author Adithya Vishnu Sankar
 */

#include "http_downloader.h"
#include "system/wifi/sys_wincs_wifi_service.h"
#include "system/net/sys_wincs_net_service.h"
#include "definitions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//#define HTTP_DOWNLOADER_DEBUG

/* -------------------------------------------------------------------------- */
/* CONSTANTS                                                                  */
/* -------------------------------------------------------------------------- */
#define HTTP_RANGE_CHUNK_SIZE 2048
#define MAX_HEADER_SIZE 1024
#define READ_BUFFER_SIZE 2048

// Flash constants
#define FLASH_WRITE_START_ADDR    0x00090000UL  // Corrected address
#define NVMCTRL_FLASH_PAGESIZE    512U
#define PAGES_PER_CHUNK           (HTTP_RANGE_CHUNK_SIZE / NVMCTRL_FLASH_PAGESIZE)  // 4 pages per chunk


//#define HTTP_DOWNLOADER_DEBUG
/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
static volatile bool g_socketConnected = false;
static volatile bool g_socketReadReady = false;
static volatile bool g_socketSendComplete = false;
static volatile bool g_socketClosed = false;

// Global variable to store detected file size
int g_detectedFileSize = 0;

// Flash write tracking
static uint32_t g_currentFlashAddr = FLASH_WRITE_START_ADDR;
static uint32_t g_totalBytesWritten = 0;

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTION PROTOTYPES                                                  */
/* -------------------------------------------------------------------------- */
void httpSocketCallback(uint32_t socket, SYS_WINCS_NET_SOCK_EVENT_t event, SYS_WINCS_NET_HANDLE_t netHandle);
static void httpPrintHex(const uint8_t* data, size_t length);
static void httpTrimWhitespace(char* str);
static int httpExtractFileSize(const char* headers);
static bool httpSendRangeRequest(uint32_t socketId, const char* hostname, const char* path, int startByte, int endByte);
static int httpReadChunk(uint32_t socketId, uint8_t* buffer, int expectedBytes);
static bool httpConnect(const char* ipAddress, uint16_t port, uint32_t* socketId);
static void httpDisconnect(uint32_t socketId);

// Flash writing functions
static bool httpWriteChunkToFlash(const uint8_t* chunkData, int chunkSize);
static bool httpWritePageToFlash(const uint8_t* pageData, uint32_t flashAddr);
static void httpShowFlashProgress(uint32_t bytesWritten, uint32_t totalBytes);

/* -------------------------------------------------------------------------- */
/* FLASH WRITING FUNCTIONS                                                    */
/* -------------------------------------------------------------------------- */

static bool httpWriteChunkToFlash(const uint8_t* chunkData, int chunkSize)
{
    uint8_t pageBuffer[NVMCTRL_FLASH_PAGESIZE];
    int remainingBytes = chunkSize;
    int sourceOffset = 0;
    
#ifdef HTTP_DOWNLOADER_DEBUG
    printf("Writing %d bytes to flash at 0x%08lX\r\n", chunkSize, g_currentFlashAddr);
#endif
    
    // Write full pages (512 bytes each)
    while (remainingBytes >= NVMCTRL_FLASH_PAGESIZE) {
        memcpy(pageBuffer, chunkData + sourceOffset, NVMCTRL_FLASH_PAGESIZE);
        
        if (!httpWritePageToFlash(pageBuffer, g_currentFlashAddr)) {
#ifdef HTTP_DOWNLOADER_DEBUG
            printf("Failed to write page at 0x%08lX\r\n", g_currentFlashAddr);
#endif
            return false;
        }
        
        sourceOffset += NVMCTRL_FLASH_PAGESIZE;
        remainingBytes -= NVMCTRL_FLASH_PAGESIZE;
        g_currentFlashAddr += NVMCTRL_FLASH_PAGESIZE;
        g_totalBytesWritten += NVMCTRL_FLASH_PAGESIZE;
    }
    
    // Handle partial page (pad with 0xFF)
    if (remainingBytes > 0) {
        memcpy(pageBuffer, chunkData + sourceOffset, remainingBytes);
        memset(pageBuffer + remainingBytes, 0xFF, NVMCTRL_FLASH_PAGESIZE - remainingBytes);
        
#ifdef HTTP_DOWNLOADER_DEBUG
        printf("Writing partial page: %d bytes + %d padding bytes\r\n", 
               remainingBytes, NVMCTRL_FLASH_PAGESIZE - remainingBytes);
#endif
        
        if (!httpWritePageToFlash(pageBuffer, g_currentFlashAddr)) {
#ifdef HTTP_DOWNLOADER_DEBUG
            printf("Failed to write partial page at 0x%08lX\r\n", g_currentFlashAddr);
#endif
            return false;
        }
        
        g_currentFlashAddr += NVMCTRL_FLASH_PAGESIZE;
        g_totalBytesWritten += remainingBytes; // Only count actual data bytes
    }
    
    return true;
}

static bool httpWritePageToFlash(const uint8_t* pageData, uint32_t flashAddr)
{
    // Ensure the page data is 32-bit aligned
    static uint32_t alignedBuffer[NVMCTRL_FLASH_PAGESIZE / 4] __attribute__((aligned(4)));
    
    // Copy data to aligned buffer
    memcpy(alignedBuffer, pageData, NVMCTRL_FLASH_PAGESIZE);
    
#ifdef HTTP_DOWNLOADER_DEBUG
    printf("Writing page to flash at 0x%08lX\r\n", flashAddr);
#endif
    
    // Validate flash address alignment (must be page-aligned)
    if (flashAddr % NVMCTRL_FLASH_PAGESIZE != 0) {
#ifdef HTTP_DOWNLOADER_DEBUG
        printf("Error: Flash address 0x%08lX is not page-aligned\r\n", flashAddr);
#endif
        return false;
    }
    
    // Unlock the flash region for writing
    NVMCTRL_RegionUnlock(flashAddr);
    
    // Set manual write mode
    NVMCTRL_SetWriteMode(NVMCTRL_WMODE_MAN);
    
    // Wait for any previous operation to complete
    while(NVMCTRL_IsBusy());

#ifdef HTTP_DOWNLOADER_TEST        
    // Clear any previous errors
    uint16_t prevError = NVMCTRL_ErrorGet();
    if (prevError != 0) {
#ifdef HTTP_DOWNLOADER_DEBUG
        printf("Clearing previous NVMCTRL error: 0x%04X\r\n", prevError);
#endif
    }
#endif
    
    // Program 512 byte page
    bool result = NVMCTRL_PageWrite((const uint32_t*)alignedBuffer, flashAddr);
    
    // Wait for write to complete
    while(NVMCTRL_IsBusy());

#ifdef HTTP_DOWNLOADER_TEST    
    // Check for errors
    uint16_t error = NVMCTRL_ErrorGet();
    if (error != 0) {
#ifdef HTTP_DOWNLOADER_DEBUG
        printf("NVMCTRL error: 0x%04X at address 0x%08lX\r\n", error, flashAddr);
#endif
        return false;
    }
#endif
    
    // Check if the function itself reported success
    if (!result) {
#ifdef HTTP_DOWNLOADER_DEBUG
        printf("NVMCTRL_PageWrite returned false for address 0x%08lX\r\n", flashAddr);
#endif
        return false;
    }
    
    // Verify the programmed content
    if (memcmp(pageData, (void *)flashAddr, NVMCTRL_FLASH_PAGESIZE) == 0) {
#ifdef HTTP_DOWNLOADER_DEBUG
        printf("Page write verified at 0x%08lX\r\n", flashAddr);
#endif
        return true;
    } else {
#ifdef HTTP_DOWNLOADER_DEBUG
        printf("Page write verification failed at 0x%08lX\r\n", flashAddr);
        // Print first few bytes for debugging
        printf("Expected: %02X %02X %02X %02X\r\n", 
               pageData[0], pageData[1], pageData[2], pageData[3]);
        printf("Actual:   %02X %02X %02X %02X\r\n", 
               ((uint8_t*)flashAddr)[0], ((uint8_t*)flashAddr)[1], 
               ((uint8_t*)flashAddr)[2], ((uint8_t*)flashAddr)[3]);
#endif
        return false;
    }
}

static void httpShowFlashProgress(uint32_t bytesWritten, uint32_t totalBytes)
{
    if (totalBytes == 0) return;
    
    uint8_t percent = (bytesWritten * 100) / totalBytes;
    printf("\rFlash Write: [%-50.*s] %3d%% (%lu/%lu bytes)", 
           percent / 2, "**************************************************", 
           percent, bytesWritten, totalBytes);
    fflush(stdout);
}   

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS                                                           */
/* -------------------------------------------------------------------------- */

THttpStatus httpParseUrl(const char* url, THttpUrl* parsedUrl)
{
    if (!url || !parsedUrl)
        return HTTP_STATUS_ERROR;
    
    memset(parsedUrl, 0, sizeof(THttpUrl));
    
    const char* hostStart;
    if (strncmp(url, "https://", 8) == 0) {
        hostStart = url + 8;
    } else if (strncmp(url, "http://", 7) == 0) {
        hostStart = url + 7;
    } else {
#ifdef HTTP_DOWNLOADER_DEBUG
        printf("Error: URL must start with http:// or https://\r\n");
#endif
        return HTTP_STATUS_INVALID_URL;
    }
    
    const char* pathStart = strchr(hostStart, '/');
    if (!pathStart) {
        strncpy(parsedUrl->hostname, hostStart, HTTP_MAX_HOSTNAME_LENGTH - 1);
        strcpy(parsedUrl->path, "/");
    } else {
        size_t hostLen = pathStart - hostStart;
        if (hostLen >= HTTP_MAX_HOSTNAME_LENGTH)
            return HTTP_STATUS_INVALID_URL;
        
        strncpy(parsedUrl->hostname, hostStart, hostLen);
        parsedUrl->hostname[hostLen] = '\0';
        strncpy(parsedUrl->path, pathStart, HTTP_MAX_PATH_LENGTH - 1);
    }
    
    char* portStart = strchr(parsedUrl->hostname, ':');
    if (portStart) {
        *portStart = '\0';
        parsedUrl->port = (uint16_t)atoi(portStart + 1);
    } else {
        parsedUrl->port = HTTP_DEFAULT_PORT;
    }
    
    httpTrimWhitespace(parsedUrl->hostname);
    httpTrimWhitespace(parsedUrl->path);
    
    return HTTP_STATUS_OK;
}

THttpStatus httpResolveHostname(const char* hostname, char* ipAddress)
{
    if (!hostname || !ipAddress)
        return HTTP_STATUS_ERROR;

#ifdef HTTP_DOWNLOADER_DEBUG
    printf("Resolving hostname: %s\r\n", hostname);
#endif

    uint32_t timeout = HTTP_DNS_TIMEOUT_MS;
    while (timeout > 0) {
        WDRV_WINC_Tasks(sysObj.drvWifiWinc);
        
        if (strlen(ipAddress) > 0) {
#ifdef HTTP_DOWNLOADER_DEBUG
            printf("Resolved IP: %s\r\n", ipAddress);
#endif
            return HTTP_STATUS_OK;
        }
        
        for (volatile int i = 0; i < 1000; i++); // 1ms delay
        timeout--;
    }

#ifdef HTTP_DOWNLOADER_DEBUG
    printf("Error: DNS resolution timeout\r\n");
#endif
    return HTTP_STATUS_DNS_FAILED;
}

THttpStatus httpDownloadFile(const THttpUrl* parsedUrl, const char* ipAddress)
{
    uint32_t socketId = 0;
    int fileSize = 0;
    int totalBytesDownloaded = 0;
    uint8_t chunkBuffer[HTTP_RANGE_CHUNK_SIZE];
    int connectRetries = 3;
    THttpStatus finalStatus = HTTP_STATUS_ERROR;
    
    // Validate inputs
    if (!parsedUrl || !ipAddress || strlen(ipAddress) == 0) {
        SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] Error: Invalid parameters - parsedUrl=%p, ipAddress=%s\r\n", 
                          parsedUrl, ipAddress ? ipAddress : "NULL");
        return HTTP_STATUS_ERROR;
    }
    
    // Initialize flash writing
    g_currentFlashAddr = FLASH_WRITE_START_ADDR;
    g_totalBytesWritten = 0;
    
#ifdef HTTP_DOWNLOADER_DEBUG
    printf("Starting HTTP range download with flash writing: %s\r\n", parsedUrl->hostname);
    printf("Flash write starting at: 0x%08lX\r\n", FLASH_WRITE_START_ADDR);
    printf("Connecting to IP: %s, Port: %d\r\n", ipAddress, parsedUrl->port);
#endif
    
    // Connect to server with retry logic
    while (connectRetries > 0) {
        SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] Connection attempt %d of 3...\r\n", 4 - connectRetries);
        
        if (httpConnect(ipAddress, parsedUrl->port, &socketId)) {
            SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] Successfully connected to server\r\n");
            break;
        }
        
        connectRetries--;
        if (connectRetries > 0) {
            SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] Connection failed, retrying in 2 seconds...\r\n");
            for (volatile int i = 0; i < 2000000; i++); // 2 second delay
        }
    }
    
    if (connectRetries == 0) {
        SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] ERROR: Failed to connect after 3 attempts\r\n");
        return HTTP_STATUS_CONNECT_FAILED;
    }
    
#ifdef HTTP_DOWNLOADER_DEBUG
    printf("Connected to server, socket ID: %d\r\n", (int)socketId);
#endif
    
    // Get first chunk to determine file size
    SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] Requesting first chunk to determine file size...\r\n");
    
    if (!httpSendRangeRequest(socketId, parsedUrl->hostname, parsedUrl->path, 0, HTTP_RANGE_CHUNK_SIZE - 1)) {
        SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] ERROR: Failed to send initial range request\r\n");
        httpDisconnect(socketId);
        return HTTP_STATUS_REQUEST_FAILED;
    }
    
    int bytesRead = httpReadChunk(socketId, chunkBuffer, HTTP_RANGE_CHUNK_SIZE);
    if (bytesRead <= 0) {
        SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] ERROR: Failed to read first chunk (bytes read: %d)\r\n", bytesRead);
        httpDisconnect(socketId);
        return HTTP_STATUS_ERROR;
    }
    
    SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] First chunk received: %d bytes\r\n", bytesRead);
    
#ifdef HTTP_DOWNLOADER_DEBUG
    printf("First chunk: %d bytes received\r\n", bytesRead);
    printf("=== FIRST CHUNK DATA ===\r\n");
    httpPrintHex(chunkBuffer, bytesRead);
    printf("=== END FIRST CHUNK ===\r\n");
#endif
    
    // Write first chunk to flash
    if (!httpWriteChunkToFlash(chunkBuffer, bytesRead)) {
        SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] ERROR: Failed to write first chunk to flash\r\n");
        httpDisconnect(socketId);
        return HTTP_STATUS_ERROR;
    }
    
    // Get file size from the static variable in httpReadChunk
    extern int g_detectedFileSize;
    fileSize = g_detectedFileSize;
    
    if (fileSize <= 0) {
        SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] ERROR: Could not determine file size (detected: %d)\r\n", fileSize);
        httpDisconnect(socketId);
        return HTTP_STATUS_ERROR;
    }
    
    SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] File size: %d bytes, chunks needed: %d\r\n", 
                      fileSize, (fileSize + HTTP_RANGE_CHUNK_SIZE - 1) / HTTP_RANGE_CHUNK_SIZE);
    
    totalBytesDownloaded = bytesRead;
    int chunkNumber = 2;
    int consecutiveFailures = 0;
    const int maxConsecutiveFailures = 3;
    
    // Show initial progress
    httpShowFlashProgress(g_totalBytesWritten, fileSize);
    
    // Download remaining chunks
    while (totalBytesDownloaded < fileSize) {
        int startByte = totalBytesDownloaded;
        int remainingBytes = fileSize - totalBytesDownloaded;
        int chunkSize = remainingBytes < HTTP_RANGE_CHUNK_SIZE ? remainingBytes : HTTP_RANGE_CHUNK_SIZE;
        int endByte = startByte + chunkSize - 1;
        
#ifdef HTTP_DOWNLOADER_DEBUG
        printf("\rChunk %d: bytes %d-%d (%d bytes)", chunkNumber, startByte, endByte, chunkSize);
#endif
        
        // Reconnect if connection was lost
        if (!g_socketConnected) {
            SYS_CONSOLE_PRINT("\n[HTTP_DOWNLOADER] Connection lost, attempting to reconnect...\r\n");
            httpDisconnect(socketId);
            
            int reconnectRetries = 3;
            bool reconnected = false;
            
            while (reconnectRetries > 0) {
                if (httpConnect(ipAddress, parsedUrl->port, &socketId)) {
                    SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] Reconnected successfully\r\n");
                    reconnected = true;
                    break;
                }
                reconnectRetries--;
                if (reconnectRetries > 0) {
                    SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] Reconnection failed, retrying...\r\n");
                    for (volatile int i = 0; i < 1000000; i++); // 1 second delay
                }
            }
            
            if (!reconnected) {
                SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] ERROR: Failed to reconnect\r\n");
                return HTTP_STATUS_CONNECT_FAILED;
            }
        }
        
        if (!httpSendRangeRequest(socketId, parsedUrl->hostname, parsedUrl->path, startByte, endByte)) {
            SYS_CONSOLE_PRINT("\n[HTTP_DOWNLOADER] Failed to send request for chunk %d\r\n", chunkNumber);
            g_socketConnected = false;
            consecutiveFailures++;
            
            if (consecutiveFailures >= maxConsecutiveFailures) {
                SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] ERROR: Too many consecutive failures\r\n");
                httpDisconnect(socketId);
                return HTTP_STATUS_ERROR;
            }
            continue; // Retry
        }
        
        bytesRead = httpReadChunk(socketId, chunkBuffer, chunkSize);
        if (bytesRead != chunkSize) {
            SYS_CONSOLE_PRINT("\n[HTTP_DOWNLOADER] Incomplete chunk %d: got %d, expected %d\r\n", 
                            chunkNumber, bytesRead, chunkSize);
            g_socketConnected = false;
            consecutiveFailures++;
            
            if (consecutiveFailures >= maxConsecutiveFailures) {
                SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] ERROR: Too many consecutive failures\r\n");
                httpDisconnect(socketId);
                return HTTP_STATUS_ERROR;
            }
            continue; // Retry
        }
        
        // Reset consecutive failures on success
        consecutiveFailures = 0;
        
        // Write chunk to flash
        if (!httpWriteChunkToFlash(chunkBuffer, bytesRead)) {
            SYS_CONSOLE_PRINT("\n[HTTP_DOWNLOADER] ERROR: Failed to write chunk %d to flash\r\n", chunkNumber);
            httpDisconnect(socketId);
            return HTTP_STATUS_ERROR;
        }
        
        // Check if this is the last chunk
        bool isLastChunk = (totalBytesDownloaded + bytesRead >= fileSize);
        if (isLastChunk) {
#ifdef HTTP_DOWNLOADER_DEBUG
            printf("\n=== LAST CHUNK DATA ===\r\n");
            printf("Chunk %d: %d bytes\r\n", chunkNumber, bytesRead);
            httpPrintHex(chunkBuffer, bytesRead);
            printf("=== END LAST CHUNK ===\r\n");
#endif
        }
        
        totalBytesDownloaded += bytesRead;
        chunkNumber++;
        
        // Update progress
        httpShowFlashProgress(g_totalBytesWritten, fileSize);
        
        // Small delay to be gentle on server
        for (volatile int i = 0; i < 10000; i++)
        {
            WDRV_WINC_Tasks(sysObj.drvWifiWinc);
            USB_DEVICE_Tasks(sysObj.usbDevObject0);
            DRV_USBFSV1_Tasks(sysObj.drvUSBFSV1Object);
            SYS_CONSOLE_Tasks(SYS_CONSOLE_INDEX_0);
        }
    }
    
    SYS_CONSOLE_PRINT("\n\n[HTTP_DOWNLOADER] Download complete: %d bytes downloaded, %lu bytes written to flash\r\n", 
                      totalBytesDownloaded, g_totalBytesWritten);
    
#ifdef HTTP_DOWNLOADER_DEBUG
    printf("Flash write completed. Final address: 0x%08lX\r\n", g_currentFlashAddr);
#endif
    
    httpDisconnect(socketId);
    
    // Verify download completion
    if (totalBytesDownloaded == fileSize) {
        SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] SUCCESS: File download verified complete\r\n");
        finalStatus = HTTP_STATUS_OK;
    } else {
        SYS_CONSOLE_PRINT("[HTTP_DOWNLOADER] ERROR: Download incomplete (%d of %d bytes)\r\n", 
                          totalBytesDownloaded, fileSize);
        finalStatus = HTTP_STATUS_ERROR;
    }
    
    return finalStatus;
}
THttpStatus httpSendGetRequest(uint32_t socketId, const char* hostname, const char* path)
{
    return httpSendRangeRequest(socketId, hostname, path, 0, HTTP_RANGE_CHUNK_SIZE - 1) ? 
           HTTP_STATUS_OK : HTTP_STATUS_REQUEST_FAILED;
}

THttpStatus httpReadResponse(uint32_t socketId)
{
    uint8_t buffer[HTTP_RANGE_CHUNK_SIZE];
    int bytesRead = httpReadChunk(socketId, buffer, HTTP_RANGE_CHUNK_SIZE);
    return (bytesRead > 0) ? HTTP_STATUS_OK : HTTP_STATUS_ERROR;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS                                                            */
/* -------------------------------------------------------------------------- */

static bool httpConnect(const char* ipAddress, uint16_t port, uint32_t* socketId)
{
    if (!ipAddress || strlen(ipAddress) == 0 || !socketId) {
        SYS_CONSOLE_PRINT("[HTTP_CONNECT] Error: Invalid parameters\r\n");
        return false;
    }
    
    SYS_CONSOLE_PRINT("[HTTP_CONNECT] Connecting to %s:%d\r\n", ipAddress, port);
    
    SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_SET_CALLBACK, httpSocketCallback);
    
    // Create socket configuration
    SYS_WINCS_NET_SOCKET_t tcpSocket = {
        .bindType = SYS_WINCS_NET_BIND_REMOTE,
        .sockAddr = ipAddress,
        .sockType = SYS_WINCS_NET_SOCK_TYPE0,
        .sockPort = port,
        .tlsEnable = 0,
        .ipType = SYS_WINCS_NET_SOCK_TYPE_IPv4_0,
    };
    
    g_socketConnected = false;
    g_socketClosed = false;
    
    SYS_WINCS_RESULT_t result = SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_TCP_OPEN, &tcpSocket);
    if (result == SYS_WINCS_FAIL) {
        SYS_CONSOLE_PRINT("[HTTP_CONNECT] Error: Failed to open TCP socket\r\n");
        return false;
    }
    
    // Wait for connection with timeout
    uint32_t timeout = HTTP_CONNECT_TIMEOUT_MS;
    uint32_t elapsed = 0;
    
    while (!g_socketConnected && !g_socketClosed && timeout > 0) {
        WDRV_WINC_Tasks(sysObj.drvWifiWinc);
        USB_DEVICE_Tasks(sysObj.usbDevObject0);
        DRV_USBFSV1_Tasks(sysObj.drvUSBFSV1Object);
        SYS_CONSOLE_Tasks(SYS_CONSOLE_INDEX_0);
        
        for (volatile int i = 0; i < 100; i++); // 0.1ms delay
        timeout--;
        elapsed++;
        
        // Print progress every second
        if (elapsed % 10000 == 0) {
            SYS_CONSOLE_PRINT("[HTTP_CONNECT] Waiting for connection... %d \r\n", elapsed / 10000);
        }
    }
    
    if (g_socketConnected) {
        *socketId = tcpSocket.sockID;
        SYS_CONSOLE_PRINT("[HTTP_CONNECT] Successfully connected, socket ID: %lu\r\n", *socketId);
        return true;
    }
    
    if (g_socketClosed) {
        SYS_CONSOLE_PRINT("[HTTP_CONNECT] Error: Socket closed during connection\r\n");
    } else {
        SYS_CONSOLE_PRINT("[HTTP_CONNECT] Error: Connection timeout after %d ms\r\n", HTTP_CONNECT_TIMEOUT_MS);
    }
    
    return false;
}

static void httpDisconnect(uint32_t socketId)
{
    g_socketClosed = false;
    SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_CLOSE, &socketId);
    
    uint32_t timeout = 1000;
    while (!g_socketClosed && timeout > 0) {
        WDRV_WINC_Tasks(sysObj.drvWifiWinc);
        for (volatile int i = 0; i < 100; i++);
        timeout--;
    }
}

static bool httpSendRangeRequest(uint32_t socketId, const char* hostname, const char* path, int startByte, int endByte)
{
    char request[HTTP_MAX_HEADER_LENGTH];
    
    int len = snprintf(request, sizeof(request),
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Range: bytes=%d-%d\r\n"
        "Connection: keep-alive\r\n"
        "User-Agent: WINCS02-HTTP/1.0\r\n"
        "\r\n",
        path, hostname, startByte, endByte);
    
    SYS_CONSOLE_PRINT("[HTTP_SEND] Sending HTTP request (socket %lu):\r\n", socketId);
    SYS_CONSOLE_PRINT("[HTTP_SEND] Host: %s, Path: %s\r\n", hostname, path);
    SYS_CONSOLE_PRINT("[HTTP_SEND] Range: bytes=%d-%d\r\n", startByte, endByte);
    SYS_CONSOLE_PRINT("[HTTP_SEND] Request length: %d bytes\r\n", len);
    
    g_socketSendComplete = false;
    
    // Add delay before sending
    for (volatile int i = 0; i < 50000; i++);
    
    SYS_WINCS_RESULT_t writeResult = SYS_WINCS_NET_TcpSockWrite(socketId, len, (uint8_t*)request);
    if (writeResult == SYS_WINCS_FAIL) {
        SYS_CONSOLE_PRINT("[HTTP_SEND] ERROR: TcpSockWrite failed\r\n");
        return false;
    }
    
    SYS_CONSOLE_PRINT("[HTTP_SEND] TcpSockWrite initiated successfully\r\n");
    
    // Wait for send completion
    uint32_t timeout = HTTP_READ_TIMEOUT_MS;
    uint32_t elapsed = 0;
    
    while (!g_socketSendComplete && g_socketConnected && timeout > 0) {
        WDRV_WINC_Tasks(sysObj.drvWifiWinc);
        USB_DEVICE_Tasks(sysObj.usbDevObject0);
        DRV_USBFSV1_Tasks(sysObj.drvUSBFSV1Object);
        SYS_CONSOLE_Tasks(SYS_CONSOLE_INDEX_0);
        for (volatile int i = 0; i < 100; i++);
        timeout--;
        elapsed++;
        
        // Print status every 20000
        if (elapsed % 20000 == 0) {
            SYS_CONSOLE_PRINT("[HTTP_SEND] Waiting for send completion... %d \r\n", elapsed / 10000);
        }
    }
    
    if (g_socketSendComplete) {
        SYS_CONSOLE_PRINT("[HTTP_SEND] Request sent successfully after %d \r\n", elapsed / 10);
        // Add delay after send
        for (volatile int i = 0; i < 200000; i++);
        return true;
    } else {
        SYS_CONSOLE_PRINT("[HTTP_SEND] ERROR: Send timeout or disconnection\r\n");
        SYS_CONSOLE_PRINT("[HTTP_SEND] g_socketConnected: %d, elapsed: %d ms\r\n", g_socketConnected, elapsed / 10);
        return false;
    }
}

static int httpReadChunk(uint32_t socketId, uint8_t* buffer, int expectedBytes)
{
    static uint8_t headerBuffer[MAX_HEADER_SIZE];
    static int headerBufferLen = 0;
    
    uint8_t readBuffer[READ_BUFFER_SIZE];
    int totalBytesRead = 0;
    int totalDataBytes = 0;  // Track only actual file data bytes
    bool headersParsed = false;
    int headerLength = 0;
    
    // Reset static variables for new chunk
    headerBufferLen = 0;
    memset(headerBuffer, 0, MAX_HEADER_SIZE);
    
    SYS_CONSOLE_PRINT("[HTTP_READ] Starting read for %d bytes of data (socket %lu)\r\n", expectedBytes, socketId);
    
    while (totalDataBytes < expectedBytes && g_socketConnected) {
        // Wait for data
        g_socketReadReady = false;
        uint32_t timeout = HTTP_READ_TIMEOUT_MS;
        uint32_t waitTime = 0;
        
        while (!g_socketReadReady && g_socketConnected && timeout > 0) {
            WDRV_WINC_Tasks(sysObj.drvWifiWinc);
            USB_DEVICE_Tasks(sysObj.usbDevObject0);
            DRV_USBFSV1_Tasks(sysObj.drvUSBFSV1Object);
            SYS_CONSOLE_Tasks(SYS_CONSOLE_INDEX_0);
            for (volatile int i = 0; i < 100; i++);
            timeout--;
            waitTime++;
        }
        
        if (!g_socketReadReady || !g_socketConnected) {
            SYS_CONSOLE_PRINT("[HTTP_READ] Timeout or disconnection after %d ms\r\n", waitTime / 10);
            break;
        }
        
        int bytesRead = SYS_WINCS_NET_TcpSockRead(socketId, READ_BUFFER_SIZE, readBuffer);
        if (bytesRead <= 0) {
            SYS_CONSOLE_PRINT("[HTTP_READ] No data read\r\n");
            continue;
        }
        
        totalBytesRead += bytesRead;
        SYS_CONSOLE_PRINT("[HTTP_READ] Read %d bytes (total: %d)\r\n", bytesRead, totalBytesRead);
        
        if (!headersParsed) {
            // Accumulate data in header buffer until we find end of headers
            int copyLen = bytesRead;
            if (headerBufferLen + copyLen > MAX_HEADER_SIZE) {
                copyLen = MAX_HEADER_SIZE - headerBufferLen;
            }
            
            memcpy(headerBuffer + headerBufferLen, readBuffer, copyLen);
            headerBufferLen += copyLen;
            
            // Look for end of headers
            char* headerEnd = strstr((char*)headerBuffer, "\r\n\r\n");
            if (headerEnd) {
                headerLength = (headerEnd - (char*)headerBuffer) + 4;
                headersParsed = true;
                
                SYS_CONSOLE_PRINT("[HTTP_READ] Headers found, length: %d bytes\r\n", headerLength);
                
                // Extract file size from Content-Range header
                headerBuffer[headerBufferLen] = '\0';
                g_detectedFileSize = httpExtractFileSize((char*)headerBuffer);
                if (g_detectedFileSize > 0) {
                    SYS_CONSOLE_PRINT("[HTTP_READ] File size from Content-Range: %d bytes\r\n", g_detectedFileSize);
                }
                
                // Check for HTTP errors
                if (strstr((char*)headerBuffer, "HTTP/1.1 4") || strstr((char*)headerBuffer, "HTTP/1.1 5")) {
                    SYS_CONSOLE_PRINT("[HTTP_READ] HTTP error response detected\r\n");
                    return -1;
                }
                
                // Calculate how much data we have after headers in the current read
                int dataStartInCurrentRead = 0;
                if (bytesRead > copyLen) {
                    // We read more than what we copied to header buffer
                    dataStartInCurrentRead = bytesRead - copyLen + (headerBufferLen - headerLength);
                } else if (headerBufferLen > headerLength) {
                    // We have some data after headers in the accumulated buffer
                    dataStartInCurrentRead = headerBufferLen - headerLength;
                }
                
                if (dataStartInCurrentRead > 0) {
                    // Copy data portion to output buffer
                    int dataToCopy = dataStartInCurrentRead;
                    if (dataToCopy > expectedBytes) {
                        dataToCopy = expectedBytes;
                    }
                    
                    // Figure out where the data starts
                    if (bytesRead > copyLen) {
                        // Data is in readBuffer
                        memcpy(buffer, readBuffer + (bytesRead - dataStartInCurrentRead), dataToCopy);
                    } else {
                        // Data is in headerBuffer
                        memcpy(buffer, headerBuffer + headerLength, dataToCopy);
                    }
                    
                    totalDataBytes += dataToCopy;
                    SYS_CONSOLE_PRINT("[HTTP_READ] Copied %d data bytes from first read\r\n", dataToCopy);
                }
            }
        } else {
            // Headers already parsed, this is pure data
            int copyBytes = bytesRead;
            if (totalDataBytes + copyBytes > expectedBytes) {
                copyBytes = expectedBytes - totalDataBytes;
            }
            
            memcpy(buffer + totalDataBytes, readBuffer, copyBytes);
            totalDataBytes += copyBytes;
            
            SYS_CONSOLE_PRINT("[HTTP_READ] Copied %d data bytes (total data: %d/%d)\r\n", 
                            copyBytes, totalDataBytes, expectedBytes);
        }
        
        // Check if we have all the data we need
        if (totalDataBytes >= expectedBytes) {
            SYS_CONSOLE_PRINT("[HTTP_READ] Got all requested data\r\n");
            break;
        }
    }
    
    SYS_CONSOLE_PRINT("[HTTP_READ] Read complete - got %d data bytes (expected %d)\r\n", 
                    totalDataBytes, expectedBytes);
    
    return totalDataBytes;
}
static int httpExtractFileSize(const char* headers)
{
    const char* contentRange = strstr(headers, "Content-Range: bytes ");
    if (contentRange) {
        const char* slash = strchr(contentRange + 21, '/');
        if (slash) {
            int size = atoi(slash + 1);
#ifdef HTTP_DOWNLOADER_DEBUG
            printf("File size: %d bytes\r\n", size);
#endif
            return size;
        }
    }
    return 0;
}

void httpSocketCallback(uint32_t socket, SYS_WINCS_NET_SOCK_EVENT_t event, SYS_WINCS_NET_HANDLE_t netHandle)
{
    
    // Always print callback events for debugging
    SYS_CONSOLE_PRINT("[HTTP_SOCKET_CB] Event %d on socket %lu\r\n", event, socket);
     
    switch(event) {
        case SYS_WINCS_NET_SOCK_EVENT_CONNECTED:
            g_socketConnected = true;
#ifdef HTTP_DOWNLOADER_DEBUG
            SYS_CONSOLE_PRINT("[HTTP_SOCKET] Connected (socket %lu)\r\n", socket);
#endif
            break;
            
        case SYS_WINCS_NET_SOCK_EVENT_DISCONNECTED:
            g_socketConnected = false;
#ifdef HTTP_DOWNLOADER_DEBUG
            SYS_CONSOLE_PRINT("[HTTP_SOCKET] Disconnected (socket %lu)\r\n", socket);
#endif
            break;
            
        case SYS_WINCS_NET_SOCK_EVENT_READ:
            g_socketReadReady = true;
            break;
            
        case SYS_WINCS_NET_SOCK_EVENT_SEND_COMPLETE:
            g_socketSendComplete = true;
            break;
            
        case SYS_WINCS_NET_SOCK_EVENT_CLOSED:
            g_socketClosed = true;
            g_socketConnected = false;
#ifdef HTTP_DOWNLOADER_DEBUG
            SYS_CONSOLE_PRINT("[HTTP_SOCKET] Socket closed (socket %lu)\r\n", socket);
#endif
            break;
            
        case SYS_WINCS_NET_SOCK_EVENT_ERROR:
            g_socketConnected = false;
#ifdef HTTP_DOWNLOADER_DEBUG
            SYS_CONSOLE_PRINT("[HTTP_SOCKET] Socket error (socket %lu)\r\n", socket);
#endif
            break;
            
        default:
#ifdef HTTP_DOWNLOADER_DEBUG
            SYS_CONSOLE_PRINT("[HTTP_SOCKET] Unknown event %d (socket %lu)\r\n", event, socket);
#endif
            break;
    }
}

static void httpPrintHex(const uint8_t* data, size_t length)
{
#ifdef HTTP_DOWNLOADER_DEBUG
    for (size_t i = 0; i < length; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) printf("\r\n");
    }
    if (length % 16 != 0) printf("\r\n");
#endif
}

static void httpTrimWhitespace(char* str)
{
    if (!str) return;
    
    // Trim leading whitespace
    char* start = str;
    while (isspace(*start)) start++;
    
    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > start && isspace(*end)) end--;
    
    // Copy trimmed string back
    size_t len = end - start + 1;
    memmove(str, start, len);
    str[len] = '\0';
}
/*******************************************************************************
* Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries.
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

#include "app_dldr.h"
#include "metadata.h"
#include "configuration.h"
#include "system/wifi/sys_wincs_wifi_service.h"
#include "system/sys_wincs_system_service.h"
#include "system/net/sys_wincs_net_service.h"
#include "app_wincs02.h"
#include "config/default/library/kta_lib/SOURCE/include/ktaConfig.h"
#include "fotaplatform.h"

/* -------------------------------------------------------------------------- */
/* DEBUG CONFIGURATION                                                        */
/* -------------------------------------------------------------------------- */
#define APP_DLDR_DEBUG_ENABLED
#ifdef APP_DLDR_DEBUG_ENABLED
/** @brief Enable downloader debug logs. */
#define APP_DLDR_DEBUG(fmt, ...)            printf("[APP_DLDR]: " fmt "\r\n", ##__VA_ARGS__)
#define APP_DLDR_DEBUG_GREEN(fmt, ...)      printf("\x1B[32m[APP_DLDR]: " fmt "\r\n\x1B[0m", ##__VA_ARGS__)
#define APP_DLDR_DEBUG_ERROR(fmt, ...)      printf("\x1B[31m[APP_DLDR <%d>]: " fmt "\r\n\x1B[0m", __LINE__, ##__VA_ARGS__)
#define APP_DLDR_DEBUG_PROGRESS(fmt, ...)   printf("[APP_DLDR]: " fmt "\r\n", ##__VA_ARGS__)
#else
#define APP_DLDR_DEBUG(fmt, ...)
#define APP_DLDR_DEBUG_PROGRESS(fmt, ...)
#define APP_DLDR_DEBUG_GREEN(...)
#define APP_DLDR_DEBUG_ERROR(fmt, ...)
#endif /* DEBUG. */

DLDR_APP_DATA dldr_app_data;

SYS_TIME_HANDLE dldr_timer_handle;
bool is_dldr_state_timedout;
DLDR_APP_STATES dldr_target_state;

static uint8_t dldr_retry_cnt;

static char dldr_sock_ip_addr[25];
static char dldr_tls_domain_name[64];
static char dldr_tls_server_name[128];
static uint8_t dldr_file_request[500];

// Define and initialize a TCP client socket configuration
static SYS_WINCS_NET_SOCKET_t dldr_tcp_client = {
    .bindType   = SYS_WINCS_NET_BIND_TYPE0,
    .sockType   = SYS_WINCS_NET_SOCK_TYPE0,
    .sockPort   = 443,
    .sockAddr   = dldr_sock_ip_addr,
    .tlsEnable  = 1,
    .ipType     = SYS_WINCS_NET_SOCK_TYPE_IPv4_0,
};

/** TLS configuration */
static SYS_WINCS_NET_TLS_SOC_PARAMS dldr_tls_config =
{
    .tlsPeerAuth          = true,
    .tlsCACertificate     = "AmazonRootCA1",
    .tlsCertificate       = NULL,
    .tlsKeyName           = NULL,
    .tlsKeyPassword       = NULL,
    .tlsServerName        = dldr_tls_server_name,
    .tlsDomainName        = dldr_tls_domain_name,
    .tlsDomainNameVerify  = true
};

static uint8_t dldr_tcp_data[MAX_TCP_SOCK_PAYLOAD_SZ];
static uint8_t nvm_data[NVMCTRL_FLASH_PAGESIZE] __attribute__((aligned(4)));
static uint16_t nvm_data_cnt;
static uint32_t nvm_address;
static uint8_t nvm_block_cnt;
static uint32_t rcvd_bytes;
static uint32_t dldr_file_length;

extern FOTA_Status gCurrentFOTAStatus;
extern char gaFotaImageUrl[2512];

// Global flags for busy-wait synchronization
volatile bool g_fota_download_complete = false;
volatile bool g_fota_update_complete = false;

static void build_dldr_file_request(char *fota_url);
static void dldr_timer_callback(uintptr_t context);
static void set_dldr_state_timeout(uint32_t timeout_ms, DLDR_APP_STATES target_state);
static void SYS_WINCS_NET_SockCallbackHandler(uint32_t socket,
    SYS_WINCS_NET_SOCK_EVENT_t event, SYS_WINCS_NET_HANDLE_t netHandle);
static void erase_flash_block(uint32_t start_addr);
static bool write_flash_page(uint8_t* data, uint32_t addr);


void APP_DLDR_Initialize(void)
{
    dldr_app_data.state = DLDR_APP_STATE_INIT;
}

void APP_DLDR_Tasks(void)
{
    switch(dldr_app_data.state)
    {
        case DLDR_APP_STATE_INIT:
        {
            /* Check for interrupted FOTA and auto-resume if SmartEEPROM available */
            /* Read SmartEEPROM configuration from USER_PAGE fuses */
            uint32_t sblk = ((*(uint32_t *)(USER_PAGE_ADDR + 4)) >> 0) & 0xF;
            uint32_t psz = ((*(uint32_t *)(USER_PAGE_ADDR + 4)) >> 4) & 0x7;
            bool has_smarteeprom = (sblk != 0 || psz != 0);
            
            if (has_smarteeprom)
            {
                TKFotaStatus fota_status;
                TFotaComponentRecord target_components;
                uint8_t nmComponents = 0;
                
                fota_status = fotaGetTargetComponents(&target_components, &nmComponents);
                
                if (fota_status == E_K_FOTA_SUCCESS && target_components.state == E_FOTA_STATE_IN_PROGRESS)
                {
                    APP_DLDR_DEBUG_GREEN("Resuming interrupted FOTA from SmartEEPROM");
                    APP_KTA_PauseForDLDR();
                    APP_SENSORS_SetState(SENSORS_APP_STATE_IDLE);
                    size_t urlLen = target_components.componentUrlLen;
                    memcpy(gaFotaImageUrl, target_components.componentUrl, urlLen);
                    gaFotaImageUrl[urlLen] = '\0';
                    build_dldr_file_request(gaFotaImageUrl);
                    dldr_app_data.state = DLDR_APP_STATE_DNS_REQUEST;
                    break;
                }
            }
            
            dldr_app_data.state = DLDR_APP_STATE_WAIT_FOR_DOWNLOAD;
            break;
        }

        case DLDR_APP_STATE_WAIT_FOR_DOWNLOAD:
        {
            dldr_retry_cnt = 0;
            // Check if FOTA download should start
            if (gCurrentFOTAStatus == APP_FOTA_START)
            {
                APP_DLDR_DEBUG_GREEN("FOTA download triggered - Starting download");
                APP_KTA_PauseForDLDR();
                APP_SENSORS_SetState(SENSORS_APP_STATE_IDLE);
                build_dldr_file_request(gaFotaImageUrl);
                dldr_app_data.state = DLDR_APP_STATE_DNS_REQUEST;
            }
            break;
        }

        case DLDR_APP_STATE_DNS_REQUEST:
        {
            if (!is_winc_ready_for_app()){
                break;
            }

            APP_DLDR_DEBUG("Set to resolve DLDR endpoint address");
            clear_last_dns_ip();
            if (SYS_WINCS_FAIL == SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_DNS_RESOLVE,
                (void *)KEYSTREAM_AWS_S3_SOCK_SERVER_ADDR0))
            {
                APP_DLDR_DEBUG_ERROR("DLDR DNA resolve init/fail");
                break;
            }
            set_dldr_state_timeout(10000, DLDR_APP_STATE_DNS_REQUEST);
            dldr_app_data.state = DLDR_APP_STATE_DNS_WAIT;
            break;
        }

        case DLDR_APP_STATE_DNS_WAIT:
        {
            char *dns_ip = get_last_dns_ip();
            if (strlen(dns_ip)) {
                APP_DLDR_DEBUG("DLDR DNS Resolved, Copy IP to TCP Client buffer");
                SYS_TIME_TimerStop(dldr_timer_handle);
                memcpy(dldr_sock_ip_addr, (char *)dns_ip, strlen((char *)dns_ip));
                dldr_app_data.state = DLDR_APP_STATE_DOWNLOAD_INIT;
            } else if (is_dldr_state_timedout) {
                is_dldr_state_timedout = false;
                APP_DLDR_DEBUG_ERROR("DLDR DNS Resolve timed out... Retry");
                dldr_app_data.state = dldr_target_state;
            }
            break;
        }

        case DLDR_APP_STATE_DOWNLOAD_INIT:
        {
            APP_DLDR_DEBUG_GREEN("IMAGE IS AVAILABLE - Erase/Prepare flash bank");
            nvm_address = get_passivebank_component_start_address();
            dldr_app_data.state = DLDR_APP_STATE_DOWNLOAD_ERASE;
            break;
        }

        case DLDR_APP_STATE_DOWNLOAD_ERASE:
        {
            // Erase/Prepare flash bank for new image
            erase_flash_block(nvm_address);
            APP_DLDR_DEBUG_PROGRESS("Erased block of 0x%08lx - %0.2f %% ", nvm_address,
                    (((float)++nvm_block_cnt/get_passivebank_nvm_blocks_count_to_erase()))*100);
            nvm_address += NVMCTRL_FLASH_BLOCKSIZE; // Increment to next block
            if (nvm_address < get_passivebank_component_end_address()) {
                dldr_app_data.state = DLDR_APP_STATE_DOWNLOAD_ERASE; // Erase next block
            } else {
                APP_DLDR_DEBUG("");
                APP_DLDR_DEBUG_GREEN("Flash Erase complete");
                APP_DLDR_DEBUG("Starting TCP Client to download image");
                dldr_file_length = 0;
                SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_SET_CALLBACK,
                    SYS_WINCS_NET_SockCallbackHandler);
                SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_OPEN_TLS_CTX, NULL);
                SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_TLS_CONFIG, &dldr_tls_config);
                SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_TCP_OPEN, &dldr_tcp_client);
                dldr_app_data.state = DLDR_APP_STATE_DOWNLOAD_INPROGRESS;
            }
            break;
        }

        case DLDR_APP_STATE_DOWNLOAD_INPROGRESS:
        {
            // Downloader application idle state waiting for events
            if(is_dldr_state_timedout)
            {
                is_dldr_state_timedout = false;
                dldr_app_data.state = dldr_target_state;
            }
            break;
        }

        case DLDR_APP_STATE_DOWNLOAD_COMPLETE:
        {
            // Download complete state
            APP_DLDR_DEBUG_GREEN("Download complete - Validating firmware metadata");

            // Extract information from Passive bank's metadata
            firmware_metadata_t* app_meta = (firmware_metadata_t*)get_passivebank_metadata_address();
            uint32_t* pdata = (uint32_t*)app_meta;      // Converting to uint32_t pointer for word-wise access
            for (uint8_t loop_index = 0; loop_index < sizeof(firmware_metadata_t)/4; loop_index++)
            {
                if(pdata[loop_index] == 0xFFFFFFFF)
                {
                    APP_DLDR_DEBUG_ERROR("One of the FW metadata field is invalid ie 0xFFFFFFFF");
                    dldr_app_data.state = DLDR_APP_STATE_DOWNLOAD_ERROR;
                    g_fota_download_complete = true;
                    g_fota_update_complete = false;
                    break;
                }
            }

            /* Update persistent FOTA state
             * 
             * Always call fotaUpdateComponent() regardless of SmartEEPROM availability.
             * The storage layer (k_sal_fotastorage_eeprom.c) automatically selects:
             *   - SmartEEPROM if hardware is enabled
             *   - Flash fallback if SmartEEPROM is disabled
             * 
             * This updates the component name, version, and state to E_FOTA_STATE_SUCCESS,
             * ensuring version information is persisted for display after bank swap.
             */
            TKFotaStatus status;
            APP_DLDR_DEBUG("Calling fotaUpdateComponent to set E_FOTA_STATE_SUCCESS");
            status = fotaUpdateComponent((const uint8_t*)app_meta->image_name, strlen((const char*)app_meta->image_name),
                (const uint8_t*)app_meta->image_version, strlen((const char*)app_meta->image_version), E_K_FOTA_SUCCESS);
            APP_DLDR_DEBUG("fotaUpdateComponent status: %d", status);
            
            gCurrentFOTAStatus = APP_FOTA_SUCCESS;
            
            // Set busy-wait success flag
            g_fota_download_complete = true;
            g_fota_update_complete = true;
            
            APP_KTA_Resume();
            APP_SENSORS_SetState(SENSORS_APP_STATE_RUN);
            
            // Perform bank swap and reset to boot new firmware
            APP_DLDR_DEBUG_GREEN("SWAP BANK and RESET");
            NVMCTRL_BankSwap();
            dldr_app_data.state = DLDR_APP_STATE_WAIT_FOR_DOWNLOAD;
            break;
        }

        case DLDR_APP_STATE_DOWNLOAD_ERROR:
        {
            // Download error state - Increment retry counter and retry download
            if (dldr_retry_cnt++ > DLDR_MAX_RETRY_CNT) {
                APP_DLDR_DEBUG_ERROR("Max retries reached. Stopping download attempts.");
                dldr_app_data.state = DLDR_APP_STATE_WAIT_FOR_DOWNLOAD;
                // Set busy-wait failure flag
                g_fota_download_complete = true;
                g_fota_update_complete = false;
            } else {
                APP_DLDR_DEBUG_ERROR("Retrying download... Attempt %d of %d", dldr_retry_cnt,
                    DLDR_MAX_RETRY_CNT);
                dldr_app_data.state = DLDR_APP_STATE_DOWNLOAD_INIT;
            }
            break;
        }

        default:
        {
            // Handle unexpected states
            break;
        }
    }
}

static void build_dldr_file_request(char *fota_url)
{
    char server_info[128];
    const char *server_name;
    const char *domain_name;
    const char *path;


    memset(server_info, 0, sizeof(server_info));
    memset(dldr_file_request, 0, sizeof(dldr_file_request));
    memset(dldr_tls_domain_name, 0, sizeof(dldr_tls_domain_name));
    memset(dldr_tls_server_name, 0, sizeof(dldr_tls_server_name));

    // Copy everything up to the first '/' after scheme
    const char *url_start = strstr(fota_url, "://");
    if (!url_start) {
        return; // invalid URL
    }
    url_start += 3; // skip "://"

    // Find path start (first '/')
    path = strchr(url_start, '/');
    if (path) {
        strncpy(server_info, fota_url, path - fota_url); // up to host
    } else {
        strcpy(server_info, fota_url); // no path, just host
        path = "/"; // default root
    }

    // Extract server name (without scheme)
    server_name = url_start;
    strncpy(dldr_tls_server_name, server_name, strlen(server_name) - strlen(path));

    // Extract domain name (everything after first '.')
    domain_name = strchr(server_name, '.');
    if (domain_name) {
        strncpy(dldr_tls_domain_name, domain_name + 1,
                strlen(dldr_tls_server_name) - (domain_name + 1 - server_name));
    }

    sprintf((char *)dldr_file_request,
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        path,
        dldr_tls_server_name
    );
}

static void dldr_timer_callback(uintptr_t context)
{
    // context is the state to retry on timeout
    dldr_target_state = context;
    is_dldr_state_timedout = true;
}

static void set_dldr_state_timeout(uint32_t timeout_ms, DLDR_APP_STATES target_state)
{
    is_dldr_state_timedout = false;
    SYS_TIME_TimerStop(dldr_timer_handle);
    dldr_timer_handle = SYS_TIME_CallbackRegisterMS(dldr_timer_callback,
        (uintptr_t)target_state, timeout_ms, SYS_TIME_SINGLE);
    APP_DLDR_DEBUG("Set DLDR state timeout for %ld ms with %d", timeout_ms, dldr_timer_handle);
}

static void SYS_WINCS_NET_SockCallbackHandler
(
    uint32_t socket,                    // The socket identifier
    SYS_WINCS_NET_SOCK_EVENT_t event,   // The type of socket event
    SYS_WINCS_NET_HANDLE_t netHandle    // Additional data or message associated with the event
)
{
    uint8_t *tmpPtr;

    switch(event)
    {
        case SYS_WINCS_NET_SOCK_EVENT_CONNECTED:
        {
            APP_DLDR_DEBUG("Socket(%ld) Connected", socket);
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_DISCONNECTED:
        {
            APP_DLDR_DEBUG_ERROR("Socket(%ld) Disconnected", socket);
            SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_CLOSE, &socket);
            dldr_app_data.state = DLDR_APP_STATE_DOWNLOAD_ERROR;
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_ERROR:
        {
            APP_DLDR_DEBUG_ERROR("Socket(%ld) Error", socket);
            dldr_app_data.state = DLDR_APP_STATE_DOWNLOAD_ERROR;
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_READ:
        {
            volatile int ret_val;

            // Read data from the TCP socket
            if(((ret_val = SYS_WINCS_NET_TcpSockRead(socket, sizeof(dldr_tcp_data), dldr_tcp_data)) > 0))
            {
                // If the file length is not yet determined
                if(!dldr_file_length)
                {
                    // Check if the HTTP content length is present in the received data
                    if((tmpPtr = (uint8_t *)strstr((const char *)dldr_tcp_data,
                    (const char *)SYS_WINCS_HTTP_CONTENT_LEN)) != NULL)
                    {
                        // Extract the content length from the received data
                        volatile char *token = strtok((char *)tmpPtr, "\r\n");
                        dldr_file_length = strtol((const char *)(token+sizeof(SYS_WINCS_HTTP_CONTENT_LEN)), NULL, 10);
                        APP_DLDR_DEBUG("File Size = %lu", dldr_file_length);
                        rcvd_bytes = 0;
                        nvm_data_cnt = 0;
                        nvm_address = get_passivebank_component_start_address();
                    }
                    break;
                }

                // Update the received bytes count and remaining length to read
                rcvd_bytes += ret_val;
                APP_DLDR_DEBUG_PROGRESS("Downloaded %lu bytes - %0.2f %% ", rcvd_bytes,
                    (((float)rcvd_bytes/dldr_file_length))*100 );

                //Run loop to write into flash
                uint8_t* sock_data = dldr_tcp_data;
                while(ret_val)
                {
                    uint16_t fetch_data = ((NVMCTRL_FLASH_PAGESIZE - nvm_data_cnt) <  ret_val) ? \
                        NVMCTRL_FLASH_PAGESIZE - nvm_data_cnt : ret_val;
                    memcpy(&nvm_data[nvm_data_cnt], sock_data, fetch_data);
                    sock_data += fetch_data;
                    ret_val -= fetch_data;
                    nvm_data_cnt += fetch_data;
                    if((nvm_data_cnt >= NVMCTRL_FLASH_PAGESIZE) || (rcvd_bytes >= dldr_file_length))
                    {
                        // Fill any left over bytes in the buffer with 0xFF
                        memset(&nvm_data[nvm_data_cnt], 0xFF, (NVMCTRL_FLASH_PAGESIZE - nvm_data_cnt));
                        //APP_DLDR_DEBUG("NVM Address = 0x%08lX", nvm_address);
                        write_flash_page(nvm_data, nvm_address);
                        nvm_address += NVMCTRL_FLASH_PAGESIZE;
                        nvm_data_cnt = 0;
                    }
                }
                if(rcvd_bytes >= dldr_file_length)
                {
                    APP_DLDR_DEBUG("");
                    APP_DLDR_DEBUG_GREEN("Download Completed!");
                    SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_CLOSE, (void *)&socket);
                    set_dldr_state_timeout(1000, DLDR_APP_STATE_DOWNLOAD_COMPLETE);
                }
            }
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_CLOSED:
        {
            APP_DLDR_DEBUG("Socket(%ld) Closed", socket);
            break;
        }

        case SYS_WINCS_NET_SOCK_EVENT_TLS_DONE:
        {
            APP_DLDR_DEBUG("Socket(%ld) TLS done", socket);
            // Send the AWS file request over the established TCP socket
            SYS_WINCS_NET_TcpSockWrite(socket, strlen((char *)dldr_file_request), dldr_file_request);
            break;
        }

        default:
            APP_DLDR_DEBUG("Socket(%ld) Unhandled event(%d)", socket, event);
            break;
    }
}

static void erase_flash_block(uint32_t start_addr)
{
    while (NVMCTRL_IsBusy());
    NVMCTRL_BlockErase(start_addr);
    while (NVMCTRL_IsBusy());
}

static bool write_flash_page(uint8_t* data, uint32_t addr)
{
     // Ensure page alignment
    if (addr % NVMCTRL_FLASH_PAGESIZE != 0) {
        APP_DLDR_DEBUG_ERROR("Address is not page aligned: 0x%08lX", addr);
        return false;
    }

    NVMCTRL_RegionUnlock(addr);
    NVMCTRL_SetWriteMode(NVMCTRL_WMODE_MAN);
    while(NVMCTRL_IsBusy());
    bool result = NVMCTRL_PageWrite((const uint32_t*)data, addr);
    while(NVMCTRL_IsBusy());

    // Verify write
    if (result && memcmp(data, (void*)addr, NVMCTRL_FLASH_PAGESIZE) == 0) {
        return true;
    }

    APP_DLDR_DEBUG_ERROR("Flash write verification failed: 0x%08lX", addr);
    return false;
}

/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_wincs02.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "configuration.h"
#include "definitions.h"
#include "KTA_version.h"
#include "cryptoauthlib.h"
#include "atca_version.h"
#include "driver/driver_common.h"
#include "system/system_module.h"
#include "system/wifi/sys_wincs_wifi_service.h"
#include "system/sys_wincs_system_service.h"
#include "system/net/sys_wincs_net_service.h"
#include "system/mqtt/sys_wincs_mqtt_service.h"


#define APP_WINC_DEBUG_ENABLED
#ifdef APP_WINC_DEBUG_ENABLED
/** @brief Enable downloader debug logs. */
#define APP_WINC_DEBUG(fmt, ...)          printf("[APP_WINC]: " fmt "\r\n", ##__VA_ARGS__)
#define APP_WINC_DEBUG_GREEN(fmt, ...)    printf("\x1B[32m[APP_WINC]: " fmt "\r\n\x1B[0m", ##__VA_ARGS__)
#define APP_WINC_DEBUG_ERROR(fmt, ...)    printf("\x1B[31m[APP_WINC <%d>]: " fmt "\r\n\x1B[0m", __LINE__, ##__VA_ARGS__)
#else
#define APP_WINC_DEBUG(fmt, ...)
#define APP_WINC_DEBUG_GREEN(fmt, ...)
#define APP_WINC_DEBUG_ERROR(fmt, ...)
#endif /* DEBUG. */

/* Define the NVMCTRL_SEESBLK_MASK_BITS to extract the NVMCTRL_SEESBLK bits(35:32) from NVM User Page Mapping(0x00804000) */
#define NVMCTRL_SEESBLK_MASK_BITS   0x0F
/* Define the NVMCTRL_SEEPSZ_MASK_BITS to extract the NVMCTRL_SEEPSZ bits(38:36) from NVM User Page Mapping(0x00804000) */
#define NVMCTRL_SEEPSZ_MASK_BITS    0x07

WINC_APP_DATA winc_app_data;
bool b_winc_ready_status;
static bool domainFlag;

//Global timer to track timeouts
SYS_TIME_HANDLE app_tick_handler;
volatile uint32_t app_tick_ms;

extern ATCAIfaceCfg atecc608_0_init_data;

SYS_TIME_HANDLE winc_timer_handle;
bool is_winc_state_timedout;
WINC_APP_STATES winc_state_to_retry;

static char dns_ip[25];
static bool winc_versions_printed;

static void SYS_WINCS_WIFI_CallbackHandler(SYS_WINCS_WIFI_EVENT_t event,
    SYS_WINCS_WIFI_HANDLE_t wifiHandle);
static bool NVMCTRL_IsAFIRSTSet(void);
static void winc_timer_callback(uintptr_t context);
static void set_winc_state_timeout(uint32_t timeout_ms, WINC_APP_STATES retry_from);
static void app_tick_callback(uintptr_t context);
static void start_app_tick_timer(void);

void APP_WINCS02_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    winc_app_data.state = WINC_APP_STATE_WAIT_FOR_USB;

    // Initialize your application's state machine and other parameters
    start_app_tick_timer();
}

void APP_WINCS02_Tasks ( void )
{
    /* Check the application's current state. */
    switch (winc_app_data.state)
    {
        // State to print Message
        case WINC_APP_STATE_WAIT_FOR_USB:
        {
            winc_app_data.state = WINC_APP_STATE_PRINT_APP_BANNER;
            break;
        }

        // State to print Message
        case WINC_APP_STATE_PRINT_APP_BANNER:
        {
            printf(TERM_YELLOW"\r\n########################################\r\n");
            printf(TERM_CYAN"keySTREAM FoTA Usecase - Application\r\n");
            printf("   Built: %s %s\r\n",  __DATE__, __TIME__);
            printf("   KTA Version: %s\r\n", ktaGetVersion());
            printf("   Application Version: %s\r\n", APP_COMPONENT_VERSION);
            printf("   CAL Version: %d.%d.%d\r\n", ATCA_LIBRARY_VERSION_MAJOR, ATCA_LIBRARY_VERSION_MINOR, ATCA_LIBRARY_VERSION_BUILD);
            printf("   Active Flash - BANK %s\r\n", NVMCTRL_IsAFIRSTSet() ? (char*)"A" : (char*)"B");
            uint32_t NVMCTRL_SEESBLK_FuseConfig = ((*(uint32_t *)(USER_PAGE_ADDR + 4)) >> 0) & NVMCTRL_SEESBLK_MASK_BITS;
            uint32_t NVMCTRL_SEEPSZ_FuseConfig = ((*(uint32_t *)(USER_PAGE_ADDR + 4)) >> 4) & NVMCTRL_SEEPSZ_MASK_BITS;
            printf("   SMARTEEPROM Size (SBLK = %d, PSZ = %d) - %ld \r\n"TERM_RESET,
                (int)NVMCTRL_SEESBLK_FuseConfig, (int)NVMCTRL_SEEPSZ_FuseConfig,
                (long int)(pow(2, NVMCTRL_SEEPSZ_FuseConfig)*512));
            if ((NVMCTRL_SEESBLK_FuseConfig == 0) || (NVMCTRL_SEEPSZ_FuseConfig == 0))
            {
                printf(TERM_RED"   SMARTEEPROM Disabled in Fuses, which is required to be enabled.\r\n"TERM_RESET);
            }
            printf(TERM_YELLOW"########################################\r\n"TERM_RESET);
            winc_app_data.state = WINC_APP_STATE_CRYPTO_INIT;
            break;
        }

        case WINC_APP_STATE_CRYPTO_INIT:
        {
            ATCA_STATUS status = ATCA_NOT_INITIALIZED;
            uint8_t sn[ATCA_SERIAL_NUM_SIZE];
            char displaystr[400];
            size_t displaylen;
            displaylen = sizeof(displaystr);
            APP_WINC_DEBUG("Starting Cryptoauthlib");
            // Initialize Cryptoauthlib library
            if ((status = atcab_init(&atecc608_0_init_data)) != ATCA_SUCCESS)
            {
                APP_WINC_DEBUG_ERROR("atcab_init has failed with %d", status);
                winc_app_data.state = WINC_APP_STATE_ERROR;
                break;
            }
            else
            {
                APP_WINC_DEBUG("atcab_init is successful");
            }
            // Read device serial number and print it
            APP_WINC_DEBUG("Reading device serial number");
            if (ATCA_SUCCESS != (status = atcab_read_serial_number(sn)))
            {
                APP_WINC_DEBUG_ERROR("atcab_read_serial_number has failed with %d", status);
                winc_app_data.state = WINC_APP_STATE_ERROR;
                break;
            }
            atcab_bin2hex(sn, ATCA_SERIAL_NUM_SIZE, displaystr, &displaylen);
            APP_WINC_DEBUG_GREEN("Device serial number: %s", displaystr);

            APP_WINC_DEBUG("Read KTA Fleet Profile ID");
            uint8_t data[ATCA_BLOCK_SIZE+1];
            data[ATCA_BLOCK_SIZE] = '\0';
            status = atcab_read_zone(ATCA_ZONE_DATA, 2, 0, 0, data, ATCA_BLOCK_SIZE);
            if (status != ATCA_SUCCESS) {
                APP_WINC_DEBUG_ERROR("atcab_read_zone has failed with %d", status);
                winc_app_data.state = WINC_APP_STATE_ERROR;
                break;
            }
            APP_WINC_DEBUG_GREEN("Sealed Fleet Profile ID in Slot2: %s", (char*)data);
            if (strstr((const char *)data, C_KTA_APP__DEVICE_PUBLIC_UID) == NULL) {
                APP_WINC_DEBUG_ERROR("Sealed Fleet Profile (%s) does not match with App Fleet Profile (%s)",
                        (char*)data, (char*)C_KTA_APP__DEVICE_PUBLIC_UID);
            }

            winc_app_data.state = WINC_APP_STATE_WINC_INIT;
            break;
        }

        /* Application's initial state. */
        case WINC_APP_STATE_WINC_INIT:
        {
            SYS_STATUS status;
            b_winc_ready_status = false;
            // Get the driver status
            SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_GET_DRV_STATUS, &status);

            // If the driver is ready, move to the next state
            if (SYS_STATUS_READY == status)
            {
                winc_app_data.state = WINC_APP_STATE_OPEN_DRIVER;
            }
            break;
        }

        case WINC_APP_STATE_OPEN_DRIVER:
        {
            DRV_HANDLE wdrvHandle = DRV_HANDLE_INVALID;
            // Open the Wi-Fi driver
            if (SYS_WINCS_FAIL == SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_OPEN_DRIVER, &wdrvHandle))
            {
                winc_app_data.state = WINC_APP_STATE_ERROR;
                break;
            }

            // Get the driver handle
            SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_GET_DRV_HANDLE, &wdrvHandle);
            winc_app_data.state = WINC_APP_STATE_DEVICE_INFO;
            break;
        }

        case WINC_APP_STATE_DEVICE_INFO:
        {
            APP_DRIVER_VERSION_INFO drvVersion;
            APP_FIRMWARE_VERSION_INFO fwVersion;
            APP_DEVICE_INFO devInfo;
            SYS_WINCS_RESULT_t status = SYS_WINCS_BUSY;

            if (winc_versions_printed == true) {
                // Versions already printed, skip to next state
                winc_app_data.state = WINC_APP_STATE_SET_REG_DOMAIN;
                break;
            }

            // Get the firmware version
            status = SYS_WINCS_SYSTEM_SrvCtrl(SYS_WINCS_SYSTEM_SW_REV, &fwVersion);

            if(status == SYS_WINCS_PASS)
            {
                // Get the device information
                status = SYS_WINCS_SYSTEM_SrvCtrl(SYS_WINCS_SYSTEM_DEV_INFO, &devInfo);
            }

            if(status == SYS_WINCS_PASS)
            {
                // Get the driver version
                status = SYS_WINCS_SYSTEM_SrvCtrl(SYS_WINCS_SYSTEM_DRIVER_VER, &drvVersion);
            }

            if(status == SYS_WINCS_PASS)
            {
                char buff[30];

                // Print device information
                printf(TERM_CYAN"[WiFi]: WINC: Device ID = %08lx\r\n", devInfo.id);
                for (int i = 0; i < devInfo.numImages; i++)
                {
                    printf("[WiFi]: %d: Seq No = %08lx, Version = %08lx, Source Address = %08lx\r\n",
                            i, devInfo.image[i].seqNum, devInfo.image[i].version, devInfo.image[i].srcAddr);
                }

                // Print firmware version
                printf("[WiFi]: Firmware Version: %d.%d.%d ", fwVersion.version.major,
                        fwVersion.version.minor, fwVersion.version.patch);
                strftime(buff, sizeof(buff), "%X %b %d %Y", localtime((time_t*)&fwVersion.build.timeUTC));
                printf(" [%s]\r\n", buff);

                // Print driver version
                printf("[WiFi]: Driver Version: %d.%d.%d\r\n"TERM_RESET, drvVersion.version.major,
                        drvVersion.version.minor, drvVersion.version.patch);
                if (fwVersion.version.major < 3)
                    APP_WINC_DEBUG_ERROR("WINC firmware version is less than 3.x.x, please upgrade to 3.x.x or higher");

                winc_versions_printed = true;
                winc_app_data.state = WINC_APP_STATE_SET_REG_DOMAIN;
            }
            break;
        }

        case WINC_APP_STATE_SET_REG_DOMAIN:
        {
            APP_WINC_DEBUG("Setting REG domain to %s", SYS_WINCS_WIFI_COUNTRYCODE);
            // Set the callback handler for Wi-Fi events
            SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_SET_CALLBACK, SYS_WINCS_WIFI_CallbackHandler);
            // Set the regulatory domain
            domainFlag = false;     //Reset the flag to process it once again
            if (SYS_WINCS_FAIL == SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_SET_REG_DOMAIN, SYS_WINCS_WIFI_COUNTRYCODE))
            {
                winc_app_data.state = WINC_APP_STATE_ERROR;
                break;
            }

            set_winc_state_timeout(20000, WINC_APP_STATE_WINC_INIT);
            winc_app_data.state = WINC_APP_STATE_IDLE;
            break;
        }

        case WINC_APP_STATE_SET_WIFI_PARAMS:
        {
            APP_WINC_DEBUG("Initiate SNTP and WiFi set parameters...");
            char sntp_url[] =  SYS_WINCS_WIFI_SNTP_ADDRESS;
            if (SYS_WINCS_FAIL == SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_SET_SNTP, sntp_url))
            {
                winc_app_data.state = WINC_APP_STATE_ERROR;
                break;
            }

            // Configuration parameters for Wi-Fi station mode
            SYS_WINCS_WIFI_PARAM_t wifi_sta_cfg = {
                .mode        = SYS_WINCS_WIFI_MODE_STA,        // Set Wi-Fi mode to Station (STA)
                .ssid        = SYS_WINCS_WIFI_STA_SSID,        // Set the SSID (network name) for the Wi-Fi connection
                .passphrase  = SYS_WINCS_WIFI_STA_PWD,         // Set the passphrase (password) for the Wi-Fi connection
                .security    = SYS_WINCS_WIFI_STA_SECURITY,    // Set the security type (e.g., WPA2) for the Wi-Fi connection
                .autoConnect = SYS_WINCS_WIFI_STA_AUTOCONNECT  // Enable or disable auto-connect to the Wi-Fi network
            };

            // Set the Wi-Fi parameters
            if (SYS_WINCS_FAIL == SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_SET_PARAMS, &wifi_sta_cfg))
            {
                winc_app_data.state = WINC_APP_STATE_ERROR;
                break;
            }

            set_winc_state_timeout(15000, WINC_APP_STATE_WINC_INIT);
            winc_app_data.state = WINC_APP_STATE_IDLE;
            break;
        }

        case WINC_APP_STATE_ERROR:
        {
            APP_WINC_DEBUG_ERROR("WINC_APP_ERROR... Reinitialize");
            set_winc_state_timeout(3000, WINC_APP_STATE_WINC_INIT);
            winc_app_data.state = WINC_APP_STATE_IDLE;
            break;
        }

        case WINC_APP_STATE_IDLE:
        {
            if (is_winc_state_timedout) {
                is_winc_state_timedout = false;
                winc_app_data.state = winc_state_to_retry;
            }

            //Wait on other events to trigger actions
            break;
        }

        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

static void SYS_WINCS_WIFI_CallbackHandler
(
    SYS_WINCS_WIFI_EVENT_t event,         // The type of Wi-Fi event
    SYS_WINCS_WIFI_HANDLE_t wifiHandle    // Handle to the Wi-Fi event data
)
{
    switch(event)
    {
        /* Set regulatory domain Acknowledgment */
        case SYS_WINCS_WIFI_REG_DOMAIN_SET_ACK:
        {
            // The driver generates this event callback twice, hence the if condition
            // to ignore one more callback. This will be resolved in the next release.
            if( domainFlag == false)
            {
                APP_WINC_DEBUG("Set Reg Domain -> SUCCESS");
                winc_app_data.state = WINC_APP_STATE_SET_WIFI_PARAMS;
                domainFlag = true;
            }
            break;
        }

        /* SNTP UP event code*/
        case SYS_WINCS_WIFI_SNTP_UP:
        {
            APP_WINC_DEBUG_GREEN("SNTP Time Received");
            SYS_TIME_TimerStop(winc_timer_handle);
            winc_app_data.state = WINC_APP_STATE_IDLE;
            b_winc_ready_status = true;
            break;
        }
        break;

        /* Wi-Fi DHCP complete event code*/
        case SYS_WINCS_WIFI_DHCP_IPV4_COMPLETE:
        {
            APP_WINC_DEBUG("DHCP IPv4 : %s", (uint8_t *)wifiHandle);
            // Retrieve the current time from the Wi-Fi service
            APP_WINC_DEBUG("Request current time from SNTP server...");
            SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_GET_TIME, NULL);
            break;
        }

        case SYS_WINCS_WIFI_DNS_RESOLVED:
        {
            if (wifiHandle) {
                APP_WINC_DEBUG("DNS IP - %s", (char *)wifiHandle);
                strncpy(dns_ip, (char *)wifiHandle, sizeof(dns_ip) - 1);
            } else {
                APP_WINC_DEBUG_ERROR("DNS Resolve failed");
                dns_ip[0] = '\0';
            }
            break;
        }

        case SYS_WINCS_WIFI_DISCONNECTED:
        case SYS_WINCS_WIFI_CONNECT_FAILED:
        {
            if (event == SYS_WINCS_WIFI_DISCONNECTED){
                APP_WINC_DEBUG_ERROR("WiFi Disconnected... Reconnecting... ");
            } else{
                APP_WINC_DEBUG_ERROR("WiFi Connect failed");
            }
            winc_app_data.state = WINC_APP_STATE_WINC_INIT;
            SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_STA_CONNECT, NULL);
            break;
        }

        case SYS_WINCS_WIFI_CONNECTED:
        {
            APP_WINC_DEBUG_GREEN("WiFi Connected ");
            break;
        }

        case SYS_WINCS_WIFI_DHCP_IPV6_LOCAL_COMPLETE:
        {
            APP_WINC_DEBUG("DHCP IPv6 Local : %s", (uint8_t *)wifiHandle);
            break;
        }

        case SYS_WINCS_WIFI_DHCP_IPV6_GLOBAL_COMPLETE:
        {
            APP_WINC_DEBUG("DHCP IPv6 Global: %s", (uint8_t *)wifiHandle);
            break;
        }

        default:
        {
            break;
        }
    }
}

static bool NVMCTRL_IsAFIRSTSet(void)
{
    // If AFIRST is SET then CODE is running from BANK A (0x0000 0000 is mapped to BANK A)
    // If AFIRST is CLEARED then CODE is running from BANK B (0x0000 0000 is mapped to BANK B)
    return (NVMCTRL_StatusGet() & NVMCTRL_STATUS_AFIRST_Msk) != 0;
}

static void app_tick_callback(uintptr_t context)
{
    // context is unused
    app_tick_ms++;
}

uint32_t get_app_tick_ms(void)
{
    return app_tick_ms;
}

static void start_app_tick_timer(void)
{
    app_tick_ms = 0;
    app_tick_handler = SYS_TIME_CallbackRegisterUS(app_tick_callback, (uintptr_t)NULL, 1000, SYS_TIME_PERIODIC);
}

static void winc_timer_callback(uintptr_t context)
{
    // context is the state to retry on timeout
    winc_state_to_retry = context;
    is_winc_state_timedout = true;
}

static void set_winc_state_timeout(uint32_t timeout_ms, WINC_APP_STATES retry_from)
{
    is_winc_state_timedout = false;
    SYS_TIME_TimerStop(winc_timer_handle);
    winc_timer_handle = SYS_TIME_CallbackRegisterMS(winc_timer_callback, (uintptr_t)retry_from, timeout_ms, SYS_TIME_SINGLE);
}

bool is_winc_ready_for_app(void)
{
    return b_winc_ready_status;
}

char* get_last_dns_ip(void)
{
    return dns_ip;
}

void clear_last_dns_ip(void)
{
    dns_ip[0] = '\0';
}


/*******************************************************************************
 End of File
 */

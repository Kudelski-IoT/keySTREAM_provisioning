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
#include "configuration.h"
#include "driver/driver_common.h"

#include "ktaFieldMgntHook.h"
#include "ktaConfig.h"
#include "app_wincs02.h"
#include "cryptoauthlib.h"
#include "system/system_module.h"
#include "system/console/sys_console.h"
#include "system/console/src/sys_console_usb_cdc.h"
#include "system/wifi/sys_wincs_wifi_service.h"
#include "system/sys_wincs_system_service.h"
#include "system/net/sys_wincs_net_service.h"
#include "system/mqtt/sys_wincs_mqtt_service.h"
// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/
APP_DATA            g_appData;
extern ATCAIfaceCfg atecc608_0_init_data;

char                g_KTA_socketAddressIp[50];
TKktaKeyStreamStatus ktaKSCmdStatus =   E_K_KTA_KS_STATUS_NONE;
int delay_counter = 0;

#define KEYSTREAM_NO_OPS_INTERVAL       (1000)
#define KEYSTREAM_NO_OPS_DELAY_COUNTER  (10)
#define KEYSTREAM_TEN_MINUTES_INTERVAL  (120)
#define KEYSTREAM_ONE_HOUR_INTERVAL     (720)

// MQTT configuration settings for connecting to Broker
SYS_WINCS_MQTT_CFG_t    g_mqttCfg = {
    .url                    = SYS_WINCS_MQTT_CLOUD_URL,
    .username               = SYS_WINCS_MQTT_CLOUD_USER_NAME,
    .clientId               = SYS_WINCS_MQTT_CLIENT_ID,
    .password               = SYS_WINCS_MQTT_PASSWORD,
    .port                   = SYS_WINCS_MQTT_CLOUD_PORT,
    .tlsIdx                 = SYS_WINCS_MQTT_TLS_ENABLE,
    .protoVer               = SYS_WINCS_MQTT_PROTO_VERSION,
    .keepAliveTime          = SYS_WINCS_MQTT_KEEP_ALIVE_TIME,
    .cleanSession           = SYS_WINCS_MQTT_CLEAN_SESSION,
    .sessionExpiryInterval  = SYS_WINCS_MQTT_KEEP_ALIVE_TIME,
};

// MQTT frame settings for subscribing to a topic
SYS_WINCS_MQTT_FRAME_t  g_mqttSubsframe = {
    .qos                    = SYS_WINCS_MQTT_SUB_TOPIC_0_QOS,
    .topic                  = SYS_WINCS_MQTT_SUB_TOPIC_0,
    .protoVer               = SYS_WINCS_MQTT_PROTO_VERSION
};

// System console USB status
SYS_CONSOLE_STATUS usb_status = SYS_CONSOLE_STATUS_NOT_CONFIGURED;
// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// Application NET Socket Callback Handler
//
// Summary:
//    Handles NET socket events.
//
// Description:
//    This function handles various NET socket events and performs appropriate actions.
//
// Parameters:
//    socket - The socket identifier
//    event - The type of socket event
//    netHandle - Additional data or message associated with the event
//
// Returns:
//    None.
//
// Remarks:
//    None.
// *****************************************************************************
void SYS_WINCS_NET_SockCallbackHandler
(
    uint32_t socket,                    // The socket identifier
    SYS_WINCS_NET_SOCK_EVENT_t event,   // The type of socket event
    SYS_WINCS_NET_HANDLE_t netHandle    // Additional data or message associated with the event
) 
{
    switch(event)
    {
        /* Net socket connected event code*/
        case SYS_WINCS_NET_SOCK_EVENT_CONNECTED:    
        {
            SYS_CONSOLE_PRINT("[APP] : Connected to Server!\r\n" );
            break;
        }
          
        /* Net socket disconnected event code*/
        case SYS_WINCS_NET_SOCK_EVENT_DISCONNECTED:
        {
            SYS_CONSOLE_PRINT("[APP] : Socket - %d DisConnected!\r\n",socket);
            SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_SOCK_CLOSE, &socket);
            break;
        }
         
        /* Net socket error event code*/
        case SYS_WINCS_NET_SOCK_EVENT_ERROR:
        {
            SYS_CONSOLE_PRINT("ERROR : Socket\r\n");
            break;
        }
            
        /* Net socket read event code*/
        case SYS_WINCS_NET_SOCK_EVENT_READ:
        {         
            uint8_t rx_data[64];
            int16_t rcvd_len = 64;
            memset(rx_data,0,64);
            
            // Read data from the TCP socket
            if((rcvd_len = SYS_WINCS_NET_TcpSockRead(socket, SYS_WINCS_NET_SOCK_RCV_BUF_SIZE, rx_data)) > 0)
            {
                rcvd_len = strlen((char *)rx_data);
                rx_data[rcvd_len] = '\n';
                SYS_CONSOLE_PRINT("Received ->%s\r\n", rx_data);
                
                // Write the received data back to the TCP socket
                SYS_WINCS_NET_TcpSockWrite(socket, rcvd_len, rx_data); 
            }    
            break; 
        }
        
        case SYS_WINCS_NET_SOCK_EVENT_CLOSED:
        {
            SYS_CONSOLE_PRINT("[APP] : Socket CLOSED -> socketID: %d\r\n",socket);
            break;
        }
        
        case SYS_WINCS_NET_SOCK_EVENT_TLS_DONE:    
        {
            SYS_CONSOLE_PRINT("[APP] : TLS ->Connected to Server!\r\n" );
            break;
        }
        
        default:
            break;                  
    }    
    
}

/* This function is called after period expires */
void TC2_Callback_InterruptHandler(TC_TIMER_STATUS status, uintptr_t context)
{
    /* Handle KTA events */
    /* This function is called periodically to run field management routine */
    g_appData.state = APP_STATE_KTA_HANDLER;
}


// *****************************************************************************
// Application MQTT Callback Handler
//
// Summary:
//    Handles MQTT events.
//
// Description:
//    This function handles various MQTT events and performs appropriate actions.
//
// Parameters:
//    event - The type of MQTT event
//    mqttHandle - The MQTT handle associated with the event
//
// Returns:
//    SYS_WINCS_RESULT_t - The result of the callback handling
//
// Remarks:
//    None.
// *****************************************************************************

SYS_WINCS_RESULT_t APP_MQTT_Callback
(
    SYS_WINCS_MQTT_EVENT_t event,
    SYS_WINCS_MQTT_HANDLE_t mqttHandle
)
{
    switch(event)
    {
        case SYS_WINCS_MQTT_CONNECTED:
        {    
            SYS_CONSOLE_PRINT(TERM_GREEN"\r\n[APP] : MQTT : Connected to broker\r\n"TERM_RESET);
            SYS_CONSOLE_PRINT("[APP] : Subscribing to %s\r\n",SYS_WINCS_MQTT_SUB_TOPIC_0);
            
            //Subscribe to topic 
            SYS_WINCS_MQTT_SrvCtrl(SYS_WINCS_MQTT_SUBS_TOPIC, (SYS_WINCS_MQTT_HANDLE_t)&g_mqttSubsframe);
            break;
        }
        
        
        case SYS_WINCS_MQTT_SUBCRIBE_ACK:
        {
            SYS_CONSOLE_PRINT(TERM_GREEN"[APP] : MQTT Subscription has been acknowledged. \r\n"TERM_RESET);
            break;
        }
        
        case SYS_WINCS_MQTT_SUBCRIBE_MSG:
        {   
            SYS_WINCS_MQTT_FRAME_t *mqttRxFrame = (SYS_WINCS_MQTT_FRAME_t *)mqttHandle;
            SYS_CONSOLE_PRINT(TERM_YELLOW"[APP] : MQTT RX: From Topic : %s ; Msg -> %s\r\n"TERM_RESET,
                    mqttRxFrame->topic, mqttRxFrame->message);
            break;
        }
        
        case SYS_WINCS_MQTT_UNSUBSCRIBED:
        {
            SYS_CONSOLE_PRINT("[APP] : MQTT- A topic has been un-subscribed. \r\n");
            break;
        }
        
        case SYS_WINCS_MQTT_PUBLISH_ACK:
        {
            SYS_CONSOLE_PRINT("[APP] : MQTT- Publish has been sent. \r\n");
            break;
        }
        
        case SYS_WINCS_MQTT_DISCONNECTED:
        {            
            SYS_CONSOLE_PRINT("[APP] :MQTT-  Reconnecting...\r\n");
            SYS_WINCS_MQTT_SrvCtrl(SYS_WINCS_MQTT_CONNECT, NULL);
            break;            
        }
        
        case SYS_WINCS_MQTT_ERROR:
        {
            SYS_CONSOLE_PRINT("[APP] : MQTT - ERROR\r\n");
            break;
        }
        
        default:
        break;
    }
    return SYS_WINCS_PASS;
}



// *****************************************************************************
// Application Wi-Fi Callback Handler
//
// Summary:
//    Handles Wi-Fi events.
//
// Description:
//    This function handles various Wi-Fi events and performs appropriate actions.
//
// Parameters:
//    event - The type of Wi-Fi event
//    wifiHandle - Handle to the Wi-Fi event data
//
// Returns:
//    None.
//
// Remarks:
//    None.
// *****************************************************************************
void SYS_WINCS_WIFI_CallbackHandler
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
            static bool domainFlag = false;
            if( domainFlag == false)
            {
                SYS_CONSOLE_PRINT("Set Reg Domain -> SUCCESS\r\n");
                g_appData.state = APP_STATE_WINCS_SET_WIFI_PARAMS;
                domainFlag = true;
            }
            
            break;
        }  
        
        /* SNTP UP event code*/
        case SYS_WINCS_WIFI_SNTP_UP:
        {            
            uint32_t *timeUTC = (uint32_t *)&wifiHandle;
            SYS_CONSOLE_PRINT(TERM_YELLOW"[APP] : SNTP UP - Time UTC : %d\r\n"TERM_RESET,*timeUTC); 

            // Resolve KTA endpoint address to IP
            if (SYS_WINCS_FAIL == SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_SET_REG_DOMAIN, SYS_WINCS_WIFI_COUNTRYCODE))
            {
                SYS_CONSOLE_PRINT("[APP] : KTA DNA resolve init/fail \r\n");
                g_appData.state = APP_STATE_WINCS_ERROR;
                break;
            }            
            
            SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_DNS_RESOLVE, (void *)C_KTA_APP__KEYSTREAM_HOST_HTTP_URL);
            SYS_CONSOLE_PRINT("Set Reg Domain -> SUCCESS\r\n");
            
            // Set the callback function for MQTT events
            //SYS_WINCS_MQTT_SrvCtrl(SYS_WINCS_MQTT_SET_CALLBACK, APP_MQTT_Callback);

            // Configure the MQTT service with the provided configuration
            //SYS_WINCS_MQTT_SrvCtrl(SYS_WINCS_MQTT_CONFIG, (SYS_WINCS_MQTT_HANDLE_t)&g_mqttCfg);

            // Connect to the MQTT broker using the specified configuration
            //SYS_WINCS_MQTT_SrvCtrl(SYS_WINCS_MQTT_CONNECT, &g_mqttCfg);

            break;
        }
        break;

        /* Wi-Fi connected event code*/
        case SYS_WINCS_WIFI_CONNECTED:
        {
            SYS_CONSOLE_PRINT(TERM_GREEN"[APP] : Wi-Fi Connected    \r\n"TERM_RESET);
            break;
        }
        
        /* Wi-Fi disconnected event code*/
        case SYS_WINCS_WIFI_DISCONNECTED:
        {
            SYS_CONSOLE_PRINT(TERM_RED"[APP] : Wi-Fi Disconnected\nReconnecting... \r\n"TERM_RESET);
            SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_STA_CONNECT, NULL);
            break;
        }
        
        /* Wi-Fi DHCP complete event code*/
        case SYS_WINCS_WIFI_DHCP_IPV4_COMPLETE:
        {         
            SYS_CONSOLE_PRINT("[APP] : DHCP IPv4 : %s\r\n", (uint8_t *)wifiHandle);
            break;
        }
        
        case SYS_WINCS_WIFI_DHCP_IPV6_LOCAL_COMPLETE:
        {
            SYS_CONSOLE_PRINT("[APP] : DHCP IPv6 Local : %s\r\n", (uint8_t *)wifiHandle);
            break;
        }
        
        case SYS_WINCS_WIFI_DHCP_IPV6_GLOBAL_COMPLETE:
        {
            SYS_CONSOLE_PRINT("[APP] : DHCP IPv6 Global: %s\r\n", (uint8_t *)wifiHandle);
            
            // Retrieve the current time from the Wi-Fi service
            SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_GET_TIME, NULL);
            break;
        }
        case SYS_WINCS_WIFI_DNS_RESOLVED:
        {
            /* DNS resolved event */
            static bool dnsResolved = false;
            // Copy the first resolved IP address from multiple DNS resolutions
            if( dnsResolved == false)
            {
                SYS_CONSOLE_PRINT("[APP] : DNS Resolved : IP - %s\r\n",(char *)wifiHandle);
                
                // Copy the resolved IP address from wifiHandle to g_socketAddressIp
                memcpy(&g_KTA_socketAddressIp[0],(char *) wifiHandle, (size_t)strlen((char *)wifiHandle));
                
                dnsResolved = true;
                g_appData.state = APP_STATE_INIT_CRYPTO;
            }
            break;
        }            
        default:
        {
            break;
        }
    }    
}


// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
// Application Initialization Function
//
// Summary:
//    Initializes the application.
//
// Description:
//    This function initializes the application's state machine and other
//    parameters.
//
// Parameters:
//    None.
//
// Returns:
//    None.
//
// Remarks:
//    None.
// *****************************************************************************
void APP_WINCS02_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    g_appData.state = APP_STATE_WAIT_USB_ENUMERATION;

    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}



// *****************************************************************************
// Application Tasks Function
//
// Summary:
//    Executes the application's tasks.
//
// Description:
//    This function implements the application's state machine and performs
//    the necessary actions based on the current state.
//
// Parameters:
//    None.
//
// Returns:
//    None.
//
// Remarks:
//    None.
// *****************************************************************************
void APP_WINCS02_Tasks ( void )
{
    /* Check the application's current state. */
    switch ( g_appData.state )
    {
        // State to print Message 
        case APP_STATE_WAIT_USB_ENUMERATION:
        {
            usb_status = Console_USB_CDC_Status(SYS_CONSOLE_DEFAULT_INSTANCE);
            if (usb_status == SYS_CONSOLE_STATUS_CONFIGURED){
                g_appData.state = APP_STATE_WINCS_PRINT;    
            }
            break;
        }        
        // State to print Message 
        case APP_STATE_WINCS_PRINT:
        {
            SYS_CONSOLE_PRINT(TERM_YELLOW"########################################\r\n"TERM_RESET);
            SYS_CONSOLE_PRINT(TERM_CYAN"        WINCS02 Keystream Cloud demo\r\n"TERM_RESET);
            SYS_CONSOLE_PRINT(TERM_YELLOW"########################################\r\n"TERM_RESET);


            printf(TERM_YELLOW"########################################\r\n"TERM_RESET);
            printf(TERM_CYAN"        Keystream Cloud - debug log\r\n"TERM_RESET);
            printf(TERM_YELLOW"########################################\r\n"TERM_RESET);
            
            
            g_appData.state = APP_STATE_WINCS_INIT;
            break;
        }
        
        /* Application's initial state. */
        case APP_STATE_WINCS_INIT:
        {
            SYS_STATUS status;
            // Get the driver status
            SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_GET_DRV_STATUS, &status);

            // If the driver is ready, move to the next state
            if (SYS_STATUS_READY == status)
            {
                g_appData.state = APP_STATE_WINCS_OPEN_DRIVER;
            }

            break;
        }

        case APP_STATE_WINCS_OPEN_DRIVER:
        {
            DRV_HANDLE wdrvHandle = DRV_HANDLE_INVALID;
            // Open the Wi-Fi driver
            if (SYS_WINCS_FAIL == SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_OPEN_DRIVER, &wdrvHandle))
            {
                g_appData.state = APP_STATE_WINCS_ERROR;
                break;
            }

            // Get the driver handle
            SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_GET_DRV_HANDLE, &wdrvHandle);
            g_appData.state = APP_STATE_WINCS_DEVICE_INFO;
            break;
        }

        case APP_STATE_WINCS_DEVICE_INFO:
        {
            APP_DRIVER_VERSION_INFO drvVersion;
            APP_FIRMWARE_VERSION_INFO fwVersion;
            APP_DEVICE_INFO devInfo;
            SYS_WINCS_RESULT_t status = SYS_WINCS_BUSY;

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
                SYS_CONSOLE_PRINT("WINC: Device ID = %08x\r\n", devInfo.id);
                for (int i = 0; i < devInfo.numImages; i++)
                {
                    SYS_CONSOLE_PRINT("%d: Seq No = %08x, Version = %08x, Source Address = %08x\r\n", 
                            i, devInfo.image[i].seqNum, devInfo.image[i].version, devInfo.image[i].srcAddr);
                }

                // Print firmware version
                SYS_CONSOLE_PRINT(TERM_CYAN "Firmware Version: %d.%d.%d ", fwVersion.version.major,
                        fwVersion.version.minor, fwVersion.version.patch);
                strftime(buff, sizeof(buff), "%X %b %d %Y", localtime((time_t*)&fwVersion.build.timeUTC));
                SYS_CONSOLE_PRINT(" [%s]\r\n", buff);

                // Print driver version
                SYS_CONSOLE_PRINT("Driver Version: %d.%d.%d\r\n"TERM_RESET, drvVersion.version.major, 
                        drvVersion.version.minor, drvVersion.version.patch);

                g_appData.state = APP_STATE_WINCS_SET_REG_DOMAIN;
            }
            break;
        }

        case APP_STATE_WINCS_SET_REG_DOMAIN:
        {
            // Set the callback handler for Wi-Fi events
            SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_SET_CALLBACK, SYS_WINCS_WIFI_CallbackHandler);

            SYS_CONSOLE_PRINT(TERM_YELLOW"Setting REG domain to " TERM_UL "%s\r\n"TERM_RESET ,SYS_WINCS_WIFI_COUNTRYCODE);
            // Set the regulatory domain
            if (SYS_WINCS_FAIL == SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_SET_REG_DOMAIN, SYS_WINCS_WIFI_COUNTRYCODE))
            {
                g_appData.state = APP_STATE_WINCS_ERROR;
                break;
            }

            g_appData.state = APP_STATE_WINCS_SERVICE_TASKS;
            break;
        }

        case APP_STATE_WINCS_SET_WIFI_PARAMS:
        {
            char sntp_url[] =  SYS_WINCS_WIFI_SNTP_ADDRESS;
            if (SYS_WINCS_FAIL == SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_SET_SNTP, sntp_url))
            {
                g_appData.state = APP_STATE_WINCS_ERROR;
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
                g_appData.state = APP_STATE_WINCS_ERROR;
                break;
            }
            SYS_CONSOLE_PRINT("\r\n\r\n[APP] : Wi-Fi Connecting to : %s\r\n", SYS_WINCS_WIFI_STA_SSID);
            g_appData.state = APP_STATE_WINCS_SERVICE_TASKS;
            break;
        }
  
        case APP_STATE_INIT_CRYPTO:
        {
            ATCA_STATUS status = ATCA_NOT_INITIALIZED;
            uint8_t sn[ATCA_SERIAL_NUM_SIZE];
            char displaystr[400];
            size_t displaylen;
            displaylen = sizeof(displaystr);
            SYS_CONSOLE_PRINT("\r\n\r\n[APP] : STARTING Cryptoauthlib");
            // Initialize Cryptoauthlib library
            if ((status = atcab_init(&atecc608_0_init_data)) != ATCA_SUCCESS)
            {
                SYS_CONSOLE_PRINT("\r\n[APP] : ATCAB INIT FAIL \r\n");
                g_appData.state = APP_STATE_WINCS_ERROR;
                break;
            }
            else
            {
                SYS_CONSOLE_PRINT("\r\n[APP] : ATCAB INIT PASS");
            }            
            // Read device serial number and print it 
            SYS_CONSOLE_PRINT("\r\n[APP] : Reading device serial number");
            if (ATCA_SUCCESS == (status = atcab_read_serial_number(sn)))
            {
                SYS_CONSOLE_PRINT("\r\n[APP] : ATCAB READ SERIAL NUM PASS");
            }
            else
            {
                SYS_CONSOLE_PRINT("\r\n[APP] : ATCAB READ SERIAL NUM FAIL");
                g_appData.state = APP_STATE_WINCS_ERROR;
                break;                
            }
            atcab_bin2hex(sn, ATCA_SERIAL_NUM_SIZE, displaystr, &displaylen);
            SYS_CONSOLE_PRINT(TERM_YELLOW"\r\n[APP] : Device serial number: %s\r\n"TERM_RESET, displaystr);
            
            g_appData.state = APP_STATE_PRE_KTA;
            break;
        }        
        
        case APP_STATE_PRE_KTA:
        {
            ATCA_STATUS status = ATCA_NOT_INITIALIZED;
            uint8_t data[ATCA_BLOCK_SIZE+1];
            data[ATCA_BLOCK_SIZE] = '\0';
            // Read slot 2
            status = atcab_read_zone(ATCA_ZONE_DATA, 2, 0, 0, data, ATCA_BLOCK_SIZE);
            if (status != ATCA_SUCCESS) {
                SYS_CONSOLE_PRINT("Read failed: %d\n", status);
                g_appData.state = APP_STATE_WINCS_ERROR;
                break;  
            }
            printf(TERM_YELLOW"\r\n[APP] : Slot 2 Data: %s\r\n\r\n"TERM_RESET, (char*)data);
            
            if (strstr((const char *)data, C_KTA_APP__DEVICE_PUBLIC_UID) != NULL) {
                printf(TERM_YELLOW"Sealed fleet profile matches with current app fleet profile \r\n\r\n"TERM_RESET);
            } else {
                printf(TERM_YELLOW"Sealed fleet profile does not match with current app fleet profile \r\n\r\n"TERM_RESET);
            }
            
            g_appData.state = APP_STATE_KTA_INIT;
            break;
        }
        
        case APP_STATE_KTA_INIT:
        {
            TKStatus ktastatus = E_K_STATUS_ERROR;
            
            SYS_CONSOLE_PRINT("\r\n\r\n[APP] : Running KTA Keystream INIT\r\n");
            ktastatus = ktaKeyStreamInit();
            if (ktastatus != E_K_STATUS_OK)
            {
                SYS_CONSOLE_PRINT("[APP] :ktaKeyStreamInit failed\r\n");
                g_appData.state = APP_STATE_WINCS_ERROR;
                break;
            }            
            
            SYS_CONSOLE_PRINT("\r\n\r\n[APP] : KTA Keystream INIT - Successful\r\n");
            g_appData.state = APP_STATE_SETUP_TIMER;
            break;
        }            

        case APP_STATE_SETUP_TIMER:
        {
            /* Register callback function for TC0 period interrupt */
            TC2_TimerCallbackRegister(TC2_Callback_InterruptHandler, (uintptr_t)NULL);
            g_appData.state = APP_STATE_KTA_HANDLER;
            // Start timer
            TC2_TimerStart();
            break;
        }
        
       case APP_STATE_KTA_HANDLER:
        {
            TKStatus ktastatus = E_K_STATUS_ERROR;
            SYS_CONSOLE_PRINT("\r\n\r\n[APP] : Running KTA Keystream Field Mgmt\r\n");

            if (ktaKSCmdStatus == E_K_KTA_KS_STATUS_REFURBISH){
                ktastatus = ktaKeyStreamInit();
                if (ktastatus != E_K_STATUS_OK)
                {
                    SYS_CONSOLE_PRINT("ktaKeyStreamInit failed during Refurb\r\n");
                    g_appData.state = APP_STATE_WINCS_ERROR;
                    break;
                }  
            }
            
            /* Calling KTA keySTREAM field management. */
            ktastatus = ktaKeyStreamFieldMgmt(true , &ktaKSCmdStatus);
            if (ktastatus != E_K_STATUS_OK)
            {
                SYS_CONSOLE_PRINT("ktaKeyStreamFieldMgmt failed\r\n");
                g_appData.state = APP_STATE_WINCS_SERVICE_TASKS;
                //Over all delay of 30 seconds in case of error.
                while (delay_counter < KEYSTREAM_NO_OPS_DELAY_COUNTER) {
                    atca_delay_ms(KEYSTREAM_NO_OPS_INTERVAL); // Sleep for 1 second
                    delay_counter++;
                }
                break;
            }

            if (ktaKSCmdStatus == E_K_KTA_KS_STATUS_NO_OPERATION)
            {
                SYS_CONSOLE_PRINT("Device On-boarding in keySTREAM Done.Device is in ACTIVATED State.\r\n");
                g_appData.state = APP_STATE_WINCS_SERVICE_TASKS;
                break;
            }

            /* Checking KTA status for Renewed or Refurbished. */
            if (ktaKSCmdStatus == E_K_KTA_KS_STATUS_RENEW || ktaKSCmdStatus == E_K_KTA_KS_STATUS_REFURBISH)
            {
                SYS_CONSOLE_PRINT("KTA in RENEW/REFURBISH state - Running KTA init and FieldManagement again\r\n");
                g_appData.state = APP_STATE_KTA_HANDLER;
                break;
            }
            /* Uninitialized */
            if (ktaKSCmdStatus == E_K_KTA_KS_STATUS_NONE)
            {
                SYS_CONSOLE_PRINT("KTA in E_K_KTA_KS_STATUS_NONE\r\n");
                g_appData.state = APP_STATE_KTA_HANDLER;
                break;
            }
            atca_delay_ms(1);
            g_appData.state = APP_STATE_KTA_HANDLER;
            break;
        }

        case APP_STATE_WINCS_SERVICE_TASKS:
        {
            break;
        }

        case APP_STATE_WINCS_ERROR:
        {
            SYS_CONSOLE_PRINT(TERM_RED"[APP_ERROR] : ERROR in Application "TERM_RESET);
            g_appData.state = APP_STATE_WINCS_SERVICE_TASKS;
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


/*******************************************************************************
 End of File
 */

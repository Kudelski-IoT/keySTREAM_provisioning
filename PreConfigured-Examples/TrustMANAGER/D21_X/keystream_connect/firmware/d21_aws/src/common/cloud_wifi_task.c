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

#include "app.h"
#include "wdrv_winc_client_api.h"
#include "cryptoauthlib.h"
#include "MQTTClient.h"
#include "parson.h"
#include "hex_dump.h"
#include "cloud_status.h"
#include "cloud_wifi_task.h"
#include "network_interface.h"
#include "cloud_wifi_config.h"
#include "timer_interface.h"
#include "cloud_wifi_ecc_process.h"
#include <limits.h>
#include <stdint.h>
#include "ecc_types.h"
#include "kta_handler.h"

#include "k_sal_fota.h"
#include "Fota_Agent.h"
#include "Fota_Process.h"

#define MQTT_BUFFER_SIZE            (1024)
#define MQTT_COMMAND_TIMEOUT_MS     (4000)
#define MQTT_YEILD_TIMEOUT_MS       (500)
#define MQTT_KEEP_ALIVE_INTERVAL_S  (900)

#define WIFI_PACKET_TIMEOUT             (5000)
#define KEYSTREAM_NO_OPS_INTERVAL       (1000)
#define KEYSTREAM_NO_OPS_DELAY_COUNTER  (30)
#define KEYSTREAM_TEN_MINUTES_INTERVAL  (120)
#define KEYSTREAM_ONE_HOUR_INTERVAL     (720)

#define IDENTIFIER_MANAGED_SLOT_14 (14u)
#define IDENTIFIER_MANAGED_SLOT_5  (5u)
#define IDENTIFIER_MANAGED_SLOT_8  (8u)

#define MAX_DATA_SIZE              (64u)
#define MAX_METADATA_SIZE          (32u)
#define MAX_UID_SIZE               (16u)

struct socket_connection kta_gSocketConnection;
static uint8_t g_host_ip_address[4];
static bool g_is_connected = false;

static MQTTClient g_mqtt_client;
static Network g_mqtt_network;

extern volatile uint32_t g_timer_val;
static uint32_t g_tx_size = 0;


static uint8_t g_rx_buffer[MQTT_BUFFER_SIZE];
static int32_t g_rx_buffer_length = 0;
static uint32_t g_rx_buffer_location = 0;

static uint8_t g_mqtt_rx_buffer[MQTT_BUFFER_SIZE];
static uint8_t g_mqtt_tx_buffer[MQTT_BUFFER_SIZE];

TKktaKeyStreamStatus ktaKSCmdStatus = E_K_KTA_KS_STATUS_NONE;
static uint32_t g_five_seconds_counter = 0;
enum cloud_iot_state kta_gCloudWifiState = CLOUD_STATE_WINC1500_INIT;
static enum wifi_status g_wifi_status = WIFI_STATUS_UNKNOWN;
static bool g_demo_led_state;

static char g_mqtt_update_topic_name[257];
static char g_mqtt_update_delta_topic_name[257];

static WDRV_WINC_AUTH_CONTEXT authCtx;
static WDRV_WINC_BSS_CONTEXT bssCtx;
extern SYSTEM_OBJECTS sysObj;

void console_print_message(const char *message)
{
    APP_DebugPrintf("%s\r\n", message);
}
void console_print_success_message(const char *message)
{
    APP_DebugPrintf("SUCCESS:  %s\r\n", message);
}
void console_print_error_message(const char *message)
{
    APP_DebugPrintf("ERROR:    %s\r\n", message);
}
void console_print_hex_dump(const void *buffer, size_t length)
{
    print_hex_dump(buffer, length, true, true, 16);
}

void connect_to_keystream()
{
    TKStatus ktastatus = E_K_STATUS_ERROR;
    ktastatus = ktaKeyStreamFieldMgmt(true , &ktaKSCmdStatus);
    if (ktastatus != E_K_STATUS_OK)
    {
        console_print_error_message("ktaKeyStreamFieldMgmt failed");
        kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;
    }
    if (ktaKSCmdStatus == E_K_KTA_KS_STATUS_RENEW || ktaKSCmdStatus == E_K_KTA_KS_STATUS_REFURBISH)
    {
        APP_DebugPrintf("++++++++++Reset the state to CLOUD_STATE_CLOUD_DISCONNECT\r\n");
        kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;
    }

}
typedef struct
{
    int         code;
    const char* name;
} ErrorInfo;

#define NEW_SOCKET_ERROR(err) { err, #err }
static const ErrorInfo g_socket_error_info[] =
{
    NEW_SOCKET_ERROR(SOCK_ERR_NO_ERROR),
    NEW_SOCKET_ERROR(SOCK_ERR_INVALID_ADDRESS),
    NEW_SOCKET_ERROR(SOCK_ERR_ADDR_ALREADY_IN_USE),
    NEW_SOCKET_ERROR(SOCK_ERR_MAX_TCP_SOCK),
    NEW_SOCKET_ERROR(SOCK_ERR_MAX_UDP_SOCK),
    NEW_SOCKET_ERROR(SOCK_ERR_INVALID_ARG),
    NEW_SOCKET_ERROR(SOCK_ERR_MAX_LISTEN_SOCK),
    NEW_SOCKET_ERROR(SOCK_ERR_INVALID),
    NEW_SOCKET_ERROR(SOCK_ERR_ADDR_IS_REQUIRED),
    NEW_SOCKET_ERROR(SOCK_ERR_CONN_ABORTED),
    NEW_SOCKET_ERROR(SOCK_ERR_TIMEOUT),
    NEW_SOCKET_ERROR(SOCK_ERR_BUFFER_FULL),
};


static const char* get_socket_error_name(int error_code)
{
    for (size_t i = 0; i < sizeof(g_socket_error_info) / sizeof(g_socket_error_info[0]); i++)
    {
        if (error_code == g_socket_error_info[i].code)
        {
            return g_socket_error_info[i].name;
        }
    }
    return "UNKNOWN";
}



/* This function is called after period expires */
void TC5_Callback_InterruptHandler(TC_TIMER_STATUS status, uintptr_t context)
{

    g_timer_val += 100;
    client_timer_update();

}

static void socket_callback_handler(SOCKET socket, uint8_t messageType, void *pMessage)
{
    tstrSocketConnectMsg *socket_connect_message = NULL;
    tstrSocketRecvMsg *socket_receive_message = NULL;
    int16_t *bytes_sent = NULL;

    switch (messageType)
    {
    case SOCKET_MSG_CONNECT:
    {
        socket_connect_message = (tstrSocketConnectMsg*)pMessage;

        if (NULL != socket_connect_message)
        {
            if (socket_connect_message->s8Error == SOCK_ERR_NO_ERROR)
            {
                // Set the state to connected to the cloud IoT
                kta_gCloudWifiState = CLOUD_STATE_CLOUD_CONNECTED;
            }
            else
            {
                // An error has occurred
                APP_DebugPrintf("SOCKET_MSG_CONNECT error %s(%d)\r\n", get_socket_error_name(socket_connect_message->s8Error), socket_connect_message->s8Error);

                // Set the state to disconnect from the cloud_wifi_socket_handler IoT
                kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;
            }
        }
        break;
    }

    case SOCKET_MSG_RECV:
    case SOCKET_MSG_RECVFROM:
    {
        socket_receive_message = (tstrSocketRecvMsg*)pMessage;
        if (socket_receive_message != NULL)
        {
            if (socket_receive_message->s16BufferSize >= 0)
            {
                g_rx_buffer_length += socket_receive_message->s16BufferSize;

                // The message was received
                if (socket_receive_message->u16RemainingSize == 0)
                {
                    g_wifi_status = WIFI_STATUS_MESSAGE_RECEIVED;
                }
            }
            else
            {
                if (socket_receive_message->s16BufferSize == SOCK_ERR_TIMEOUT)
                {
                    // A timeout has occurred
                    g_wifi_status = WIFI_STATUS_TIMEOUT;
                }
                else
                {
                    // An error has occurred
                    g_wifi_status = WIFI_STATUS_ERROR;

                    // Set the state to disconnect from the cloud IoT
                    kta_gCloudWifiState = CLOUD_STATE_WIFI_DISCONNECT;
                }
            }
        }
        break;
    }

    case SOCKET_MSG_SEND:

        bytes_sent = (int16_t*)pMessage;

        if (*bytes_sent <= 0 || *bytes_sent > (int32_t)g_tx_size)
        {
            // Seen an odd instance where bytes_sent is way more than the requested bytes sent.
            // This happens when we're expecting an error, so were assuming this is an error
            // condition.
            g_wifi_status = WIFI_STATUS_ERROR;

            // Set the state to disconnect from the cloud IoT
            kta_gCloudWifiState = CLOUD_STATE_WIFI_DISCONNECT;
        }
        else if (*bytes_sent > 0)
        {
            // The message was sent
            g_wifi_status = WIFI_STATUS_MESSAGE_SENT;
        }
        break;
    default:
        APP_DebugPrintf("%s: unhandled message %d\r\n", __FUNCTION__, (int)messageType);
        // Do nothing
        break;

    }
}

static void cloud_dns_resolve_handler(uint8_t *pu8DomainName, uint32_t u32ServerIP)
{

    int8_t status = SOCK_ERR_INVALID_ARG;
    SOCKET new_socket = SOCK_ERR_INVALID;
    struct sockaddr_in socket_address;
    char message[128];

    if (u32ServerIP != 0)
    {
        // Save the Host IP Address
        g_host_ip_address[0] = u32ServerIP & 0xFF;
        g_host_ip_address[1] = (u32ServerIP >> 8) & 0xFF;
        g_host_ip_address[2] = (u32ServerIP >> 16) & 0xFF;
        g_host_ip_address[3] = (u32ServerIP >> 24) & 0xFF;

        sprintf(&message[0], "WINC1500 WIFI: DNS lookup:\r\n  Host:       %s\r\n  IP Address: %u.%u.%u.%u",
                (char*)pu8DomainName, g_host_ip_address[0], g_host_ip_address[1],
                g_host_ip_address[2], g_host_ip_address[3]);
        console_print_message(message);

        do
        {
            // Create the socket
            new_socket = socket(AF_INET, SOCK_STREAM, 1);
            if (new_socket < 0)
            {
                console_print_error_message("Failed to create the socket.");

                // Set the state to disconnect from the cloud IoT
                kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;

                // Break the do/while loop
                break;
            }

            // Set the socket address information
            socket_address.sin_family      = AF_INET;
            socket_address.sin_addr.s_addr = _htonl((uint32_t)((g_host_ip_address[0] << 24) |
                                                               (g_host_ip_address[1] << 16) |
                                                               (g_host_ip_address[2] << 8)  |
                                                               g_host_ip_address[3]));
            socket_address.sin_port        = _htons(CLOUD_PORT);

        #ifdef CLOUD_CONFIG_GCP
            int ssl_caching_enabled = 1;
            setsockopt(new_socket, SOL_SSL_SOCKET, SO_SSL_ENABLE_SESSION_CACHING, &ssl_caching_enabled, sizeof(ssl_caching_enabled));
        #else
            setsockopt(new_socket, SOL_SSL_SOCKET, SO_SSL_SNI,
                       CLOUD_ENDPOINT, sizeof(CLOUD_PORT) + 1);
        #endif

            // Connect to the cloud IoT server
            status = connect(new_socket, (struct sockaddr*)&socket_address,
                             sizeof(socket_address));
            if (status != SOCK_ERR_NO_ERROR)
            {
                memset(&message[0], 0, sizeof(message));
                sprintf(&message[0], "WINC1500 WIFI: Failed to connect to cloud Iot.");
                console_print_error_message(message);

                // Close the socket
                shutdown(new_socket);

                // Set the state to disconnect from the cloud IoT
                kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;

                // Break the do/while loop
                break;
            }

            // Save the new socket connection information
            APP_DebugPrintf("socket %d \r\n", new_socket);
            kta_gSocketConnection.socket    = new_socket;
            kta_gSocketConnection.address   = socket_address.sin_addr.s_addr;
            kta_gSocketConnection.port      = CLOUD_PORT;
        }
        while (false);
    }
    else
    {
        // An error has occurred
        console_print_error_message("WINC1500 DNS lookup failed.");

        // Set the state to disconnect from the cloud IoT
        kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;
    }

}
static void APP_ExampleDHCPAddressEventCallback(DRV_HANDLE handle, uint32_t ipAddress)
{
    char s[20];

    APP_DebugPrintf("IP address is %s\r\n", inet_ntop(AF_INET, &ipAddress, s, sizeof(s)));
    gethostbyname((char*)CLOUD_ENDPOINT);

}


static void APP_ExampleGetSystemTimeEventCallback(DRV_HANDLE handle, uint32_t time)
{

    if (true == WDRV_WINC_IPLinkActive(handle))
    {
        APP_DebugPrintf("Time %u \r\n", time);
        RTC_Timer32CounterSet(time);
        RTC_Timer32Start();
    }

}

static void wifi_callback_handler(DRV_HANDLE handle, WDRV_WINC_ASSOC_HANDLE assocHandle, WDRV_WINC_CONN_STATE currentState, WDRV_WINC_CONN_ERROR errorCode)
{
    char message[256];

    if (WDRV_WINC_CONN_STATE_CONNECTED == currentState)
    {
        APP_DebugPrintf("Wifi Connected\r\n");

    }
    else if (WDRV_WINC_CONN_STATE_DISCONNECTED == currentState)
    {
        if (CLOUD_STATE_CLOUD_CONNECTED == kta_gCloudWifiState)
        {
            APP_DebugPrintf("Failed to connect\r\n");
            kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;
        }

        else
        {
            APP_DebugPrintf("Wifi Disconnected\r\n");
            kta_gCloudWifiState = CLOUD_STATE_WIFI_CONFIGURE;
            if (ktaKSCmdStatus == E_K_KTA_KS_STATUS_RENEW || ktaKSCmdStatus == E_K_KTA_KS_STATUS_REFURBISH)
            {
              APP_DebugPrintf("++++++++++Reset CLOUD_STATE_WINC1500_INIT\r\n");
              kta_gCloudWifiState = CLOUD_STATE_WINC1500_INIT;
            }
        }
    }
    else
    {
        memset(&message[0], 0, sizeof(message));
        sprintf(&message[0], "WINC1500 WIFI: Unknown connection status: %d",
                currentState);
        console_print_error_message(message);
    }
}

int cloud_wifi_read_data(uint8_t *read_buffer, uint32_t read_length, uint32_t timeout_ms)
{
    int status = SUCCESS;

    if (g_is_connected == false || kta_gCloudWifiState <= CLOUD_STATE_WIFI_DISCONNECT)
    {
        return FAILURE;
    }

    if (g_rx_buffer_length >= read_length)
    {
        status = SUCCESS;

        // Get the data from the existing received buffer
        memcpy(&read_buffer[0], &g_rx_buffer[g_rx_buffer_location], read_length);

        g_rx_buffer_location += read_length;
        g_rx_buffer_length -= read_length;


    }
    else
    {
        // Reset the message buffer information
        g_wifi_status = WIFI_STATUS_UNKNOWN;
        g_rx_buffer_location = 0;
        g_rx_buffer_length = 0;

        memset(&g_rx_buffer[0], 0, sizeof(g_rx_buffer));

        // Receive the incoming message
        g_wifi_status = recv(kta_gSocketConnection.socket, g_rx_buffer, sizeof(g_rx_buffer), timeout_ms);
        atca_delay_ms(5);
        do
        {
            // Wait until the incoming message or error was received
            m2m_wifi_handle_events();

            if (g_wifi_status == WIFI_STATUS_TIMEOUT)
            {
                status = FAILURE;
                // Break the do/while loop
                break;
            }
            else if (g_wifi_status == WIFI_STATUS_ERROR)
            {
                status = FAILURE;

                // Break the do/while loop
                break;
            }

            if (g_wifi_status == WIFI_STATUS_MESSAGE_RECEIVED)
            {
                status = SUCCESS;

                memcpy(&read_buffer[0], &g_rx_buffer[0], read_length);

                g_rx_buffer_location += read_length;
                g_rx_buffer_length -= read_length;

            }


        }
        while ((g_wifi_status != WIFI_STATUS_MESSAGE_RECEIVED));  // && (--timeout > 0));
    }

    return (status == SUCCESS) ? (int)read_length : status;
}

int cloud_wifi_send_data(uint8_t *send_buffer, uint32_t send_length, uint32_t timeout_ms)
{
    int status = SUCCESS;


    if (g_is_connected == false)
    {
        return FAILURE;
    }


    g_wifi_status = send(kta_gSocketConnection.socket, send_buffer, send_length, 0);
    g_tx_size = send_length;

    do
    {
        // Wait until the outgoing message was sent
        m2m_wifi_handle_events();

        if (g_wifi_status == WIFI_STATUS_ERROR)
        {
            status = FAILURE;

            // Break the do/while loop
            break;
        }
        atca_delay_ms(1);
    }
    while ((g_wifi_status != WIFI_STATUS_MESSAGE_SENT));




    return (status == SUCCESS) ? (int)send_length : status;
}

static uint32_t publish_update_message;

/* Helper functions */
static bool client_counter_finished()
{
    return 0 == publish_update_message;
}

static void client_counter_set(uint32_t val)
{
    //Convert to loop time
    publish_update_message = val / CLIENT_UPDATE_PERIOD;
}

/* Must be called on the CLIENT_UPDATE_PERIOD */
void client_timer_update(void)
{
    if (publish_update_message)
    {
        publish_update_message--;

    }
}

static uint16_t cloud_get_message_id(void)
{
    static uint16_t message_id = 0;

    message_id++;

    if (message_id == (USHRT_MAX - 1))
    {
        message_id = 1;
    }

    return message_id;
}

static void cloud_subscribe_callback(MessageData *data)
{
    JSON_Value *delta_message_value = NULL;
    JSON_Object *delta_message_object = NULL;
    const char *led_status = NULL;

    console_print_message("\r\n");
    console_print_message("Received MQTT Shadow Update Delta Message:");
    console_print_hex_dump(data->message->payload, data->message->payloadlen);
    console_print_message("\r\n");
    do
    {
        // Parse the LED update message
        delta_message_value   = json_parse_string((char*)data->message->payload);
        delta_message_object  = json_value_get_object(delta_message_value);

        #if defined(CLOUD_CONFIG_AWS)
        JSON_Object *led_state_object = NULL;
        led_state_object = json_object_get_object(delta_message_object, "state");
        if (led_state_object == NULL)
        {
            // Break the do/while loop
            break;
        }

        led_status = json_object_get_string(led_state_object, "led1");

        #else

        led_status = json_object_get_string(delta_message_object, "led1");

        #endif

        if (led_status != NULL)
        {
            g_demo_led_state = (strcmp(led_status, "on") == 0) ? 0 : 1;
            if (g_demo_led_state)
            {
                STATUS_LED_Set();
            }
            else
            {
                STATUS_LED_Clear();
            }
        }
    }
    while (false);

    // Free allocated memory
    json_value_free(delta_message_value);

#if defined(CLOUD_CONFIG_AWS)
    // Report the new LED states
    cloud_publish_message();
#endif

}

void cloud_publish_message()
{
    int mqtt_status = FAILURE;
    MQTTMessage message;
    char json_message[256];
    JSON_Value *update_message_value = NULL;
    JSON_Object *update_message_object = NULL;

    memset(json_message, 0x00, sizeof(json_message));

    // Create the update message
    update_message_value   = json_value_init_object();
    update_message_object  = json_value_get_object(update_message_value);

    do
    {
        // Only publish message when in the reporting state
        if (g_mqtt_client.isconnected != 1)
        {
            break;
        }
        
        STATUS_LED_Toggle();                
#if defined(CLOUD_CONFIG_GCP)
        uint32_t ts = RTC_Timer32CounterGet();
        json_object_dotset_number(update_message_object, "timestamp", ts);
        json_object_dotset_string(update_message_object, "Led_Status", ((STATUS_LED_Get()) ? "off" : "on"));
#else
        json_object_dotset_string(update_message_object, "state.reported.led1", ((STATUS_LED_Get()) ? "off" : "on"));
#endif

        json_serialize_to_buffer(update_message_value, json_message, sizeof(json_message));
        message.qos      = QOS0;
        message.retained = 0;
        message.dup      = 0;
        message.id       = cloud_get_message_id();

        message.payload = (void*)json_message;
        message.payloadlen = strlen(json_message);

        console_print_message("Publishing MQTT Shadow Update Message:");
        console_print_hex_dump(message.payload, message.payloadlen);

        config_get_client_pub_topic(g_mqtt_update_topic_name, sizeof(g_mqtt_update_topic_name));
        mqtt_status = MQTTPublish(&g_mqtt_client, g_mqtt_update_topic_name, &message);
        if (mqtt_status != SUCCESS)
        {
            // The cloud IoT Demo failed to publish the MQTT LED update message
            cloud_iot_set_status(CLOUD_STATE_CLOUD_REPORTING,
                                 CLOUD_STATUS_CLOUD_REPORT_FAILURE,
                                 "The cloud IoT Demo failed to publish the MQTT shadow update message.");

            console_print_message("\r\n");
            console_print_error_message("The cloud IoT Demo failed to publish the MQTT shadow update message.");
        }
    }
    while (false);

    // Free allocated memory
    json_value_free(update_message_value);
}

int cloud_wifi_init(DRV_HANDLE handle)
{
    int8_t status = !0;
    tstrM2mRev strtmp;
    int8_t ret;

    APP_DebugPrintf("\r\n");
    APP_DebugPrintf("===========================\r\n");
    APP_DebugPrintf("Trust Platform Cloud Example\r\n");
    APP_DebugPrintf("===========================\r\n");

    /* Register callback function for TC5 period interrupt */
    TC5_TimerCallbackRegister(TC5_Callback_InterruptHandler, (uintptr_t)NULL);
    memset(&kta_gSocketConnection, 0, sizeof(kta_gSocketConnection));
    extern ATCAIfaceCfg atecc608_0_init_data;
    /* Start the timer*/
    TC5_TimerStart();

    do
    {
        // Initialize the cryptoauthlib stack
        atecc608_0_init_data.atcai2c.address = SECURE_ELEMENT_ADDRESS;
        if ((status = atcab_init(&atecc608_0_init_data)) != ATCA_SUCCESS)
        {
            break;
        }

        /* Enable use of DHCP for network configuration, DHCP is the default
           but this also registers the callback for notifications. */
        if ((status = WDRV_WINC_IPUseDHCPSet(handle, &APP_ExampleDHCPAddressEventCallback))  != WDRV_WINC_STATUS_OK)
        {
            break;
        }

        /* Initialize the BSS context to use default values. */
        if ((status = WDRV_WINC_BSSCtxSetDefaults(&bssCtx)) != WDRV_WINC_STATUS_OK)
        {
            break;
        }

        /* Update BSS context with target SSID for connection. */
        if ((status = WDRV_WINC_BSSCtxSetSSID(&bssCtx, (uint8_t*)WLAN_SSID, strlen(WLAN_SSID))) != WDRV_WINC_STATUS_OK)
        {
            break;
        }

        /*if (WDRV_WINC_STATUS_OK != WDRV_WINC_AuthCtxSetOpen(&authCtx))
           {
            break;
           }*/

        /*Initialize the authentication context for WPA. */
        if ((status = WDRV_WINC_AuthCtxSetWPA(&authCtx, (uint8_t*)WLAN_PSK, strlen(WLAN_PSK))) != WDRV_WINC_STATUS_OK)
        {
            break;
        }

        /* Initialize the system time callback handler. */
        if ((status = WDRV_WINC_SystemTimeGetCurrent(handle, &APP_ExampleGetSystemTimeEventCallback)) != WDRV_WINC_STATUS_OK)
        {
            break;
        }

        /* Register callback handler for DNS resolver . */
        if ((status = WDRV_WINC_SocketRegisterResolverCallback(handle, &cloud_dns_resolve_handler)) != WDRV_WINC_STATUS_OK)
        {
            break;
        }

        /* Register callback handler for socket events. */
        if ((status = WDRV_WINC_SocketRegisterEventCallback(handle, &socket_callback_handler)) != WDRV_WINC_STATUS_OK)
        {
            break;
        }

        ret = nm_get_firmware_full_info(&strtmp);
        if(M2M_ERR_FW_VER_MISMATCH == ret)
        {
            status = !0;
            break;
        }

        APP_DebugPrintf("\nWINC1500 Firmware Data:\r\n");
        APP_DebugPrintf("Firmware Ver: %u.%u.%u SVN Rev %u\r\n", strtmp.u8FirmwareMajor, strtmp.u8FirmwareMinor, strtmp.u8FirmwarePatch, strtmp.u16FirmwareSvnNum);
        APP_DebugPrintf("Firmware Built at %s Time %s\r\n", strtmp.BuildDate, strtmp.BuildTime);
        APP_DebugPrintf("Firmware Min Driver Ver: %u.%u.%u\r\n", strtmp.u8DriverMajor, strtmp.u8DriverMinor, strtmp.u8DriverPatch);
        APP_DebugPrintf("Driver Ver: %u.%u.%u\r\n", M2M_RELEASE_VERSION_MAJOR_NO, M2M_RELEASE_VERSION_MINOR_NO, M2M_RELEASE_VERSION_PATCH_NO);
        APP_DebugPrintf("Driver Built at %s Time %s\r\n\r\n", __DATE__, __TIME__);

        APP_DebugPrintf("Secure Element Address: 0x%02X \r\n", SECURE_ELEMENT_ADDRESS);
        APP_DebugPrintf("Cloud Endpoint: %s \r\n", CLOUD_ENDPOINT);
    #ifdef CLOUD_CONNECT_WITH_CUSTOM_CERTS
        APP_DebugPrintf("Connecting with CUSTOM certs\r\n");
    #else
        APP_DebugPrintf("Connecting with MCHP certs\r\n");
    #endif

    #ifndef CLOUD_CONFIG_GCP
        //transfer_ecc_certs_to_winc();
    #endif

        if ((status = m2m_ssl_set_active_ciphersuites(SSL_CIPHER_SUITE_SELECTION)) != M2M_SUCCESS)
        {
            break;
        }

        // Initialize the MQTT library
        g_mqtt_network.mqttread  = &mqtt_packet_read;
        g_mqtt_network.mqttwrite = &mqtt_packet_write;

        MQTTClientInit(&g_mqtt_client, &g_mqtt_network, MQTT_COMMAND_TIMEOUT_MS,
                       g_mqtt_tx_buffer, sizeof(g_mqtt_tx_buffer),
                       g_mqtt_rx_buffer, sizeof(g_mqtt_rx_buffer));

    }
    while (0);


    return status;
}

void printFotaStatus(TKFotaStatus status)
{
    switch (status)
    {
        case E_K_FOTA_SUCCESS:
            APP_DebugPrintf("FOTA status E_K_FOTA_SUCCESS.\r\n");
            break;
        case E_K_FOTA_ERROR:
            APP_DebugPrintf("FOTA status E_K_FOTA_ERROR. \r\n");
            break;
        case E_K_FOTA_IN_PROGRESS:
            APP_DebugPrintf("FOTA status E_K_FOTA_IN_PROGRESS.\r\n");
            break;
        default:
            break;
    }
}
void Fota_Test()
{

    APP_DebugPrintf("\r\nFota_Test start \r\n\r\n");

    uint8_t ComponentName1[] = {0x63, 0x61, 0x6C};
    uint8_t ComponentName2[] = {0x4b, 0x54, 0x41};
    uint8_t ComponentVersion1[] ={0x33, 0x2E, 0x32, 0x2E, 0x30};
    uint8_t ComponentVersion2[] ={0x33, 0x2E, 0x32, 0x2E, 0x30};

    uint8_t Url[] = {0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x65, \
                     0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, \
                     0x6f, 0x6d, 0x2f, 0x63, 0x61, 0x6c};
    uint8_t FOTA_NAME[] = {0x46, 0x4F, 0x54, 0x41, 0x5F, 0x55, 0x50, 0x44, 0x41, 0x54, 0x45}; // "FOTA_UPDATE" in HEX
    uint8_t FOTA_METADATA[] = {0x4D, 0x45, 0x54, 0x41, 0x44, 0x41, 0x54, 0x41}; // "METADATA" in HEX
    uint8_t errorCode = 0;
    uint8_t FOTA_Error_Cause[16] = {0};
    TTargetComponent targetComponents[COMPONENTS_MAX] = {0};
 
    targetComponents[0].componentTargetName = (uint8_t *)&ComponentName1;
    targetComponents[0].componentTargetNameLen = sizeof(ComponentName1);
    targetComponents[0].componentTargetVersion = (uint8_t *)&ComponentVersion1;
    targetComponents[0].componentTargetVersionLen = sizeof(ComponentVersion1);
    targetComponents[0].componentUrl = (uint8_t *)&Url;
    targetComponents[0].componentUrlLen = sizeof(Url);

    targetComponents[1].componentTargetName = (uint8_t *)&ComponentName2;
    targetComponents[1].componentTargetNameLen = sizeof(ComponentName2);
    targetComponents[1].componentTargetVersion = (uint8_t *)&ComponentVersion2;
    targetComponents[1].componentTargetVersionLen = sizeof(ComponentVersion2);
    targetComponents[1].componentUrl = (uint8_t *)&Url;
    targetComponents[1].componentUrlLen = sizeof(Url);
    
    TFotaError fotaError = {0};
    fotaError.fotaErrorCause = (uint8_t *)&FOTA_Error_Cause;
    fotaError.fotaErrorCauseLen = sizeof(FOTA_Error_Cause);
    
    fotaError.fotaErrorCode = (uint8_t *)&errorCode;
    fotaError.fotaErrorCodeLen = 1;
    TComponent components[COMPONENTS_MAX] = {0};
    TKFotaStatus status = E_K_FOTA_ERROR;
    
    fotaAgentInit();
   
    APP_DebugPrintf("\r\nCalling salFotaInstall \r\n");
    status = salFotaInstall((uint8_t *)&FOTA_NAME,
                                         sizeof(FOTA_NAME),
                                         (uint8_t *)&FOTA_METADATA,
                                         sizeof(FOTA_METADATA),
                                         targetComponents,
                                         &fotaError,
                                         components);
    APP_DebugPrintf("salFotaInstall returned %d\r\n", status);
    printFotaStatus(status);

    /* Updating Component status to SUCCESS for testing */
    APP_DebugPrintf("\r\nCalling fotaUpdateComponent to set E_FOTA_STATE_SUCCESS \r\n");
    fotaUpdateComponent(ComponentName1, 3, ComponentVersion1, 5, E_FOTA_STATE_SUCCESS);
    fotaUpdateComponent(ComponentName2, 3, ComponentVersion2, 5, E_FOTA_STATE_SUCCESS);
    
    APP_DebugPrintf("\r\nCalling salFotaGetStatus \r\n");
    status = salFotaGetStatus((uint8_t*)&FOTA_NAME, sizeof(FOTA_NAME), &fotaError, components);
    APP_DebugPrintf("salFotaGetStatus returned %d\r\n", status);
    printFotaStatus(status);


    APP_DebugPrintf("\r\nCalling salDeviceGetInfo \r\n");
    status = salDeviceGetInfo(components);
    APP_DebugPrintf("SalDeviceGetInfo returned %d\r\n", status);
    printFotaStatus(status);
    if(E_K_FOTA_SUCCESS == status)
    {
        for (size_t i = 0; i < COMPONENTS_MAX; i++) {
            if (components[i].componentNameLen > 0) {
                APP_DebugPrintf("Component: %s \t Version: %s\r\n",
                                 components[i].componentName,
                                 components[i].componentVersion);
            }
        }
    }

    APP_DebugPrintf("\r\nFota Test End.\r\n\r\n");

}
void APP_ExampleTasks(DRV_HANDLE handle)
{
    MQTTPacket_connectData mqtt_options = MQTTPacket_connectData_initializer;
    int mqtt_status = FAILURE;
    int wifi_status = M2M_SUCCESS;
    ATCA_STATUS status = !ATCA_SUCCESS;
    static bool publish = true;
    TKStatus ktastatus = E_K_STATUS_ERROR;
    int delay_counter = 0u;

    switch (kta_gCloudWifiState)
    {
    case CLOUD_STATE_WINC1500_INIT:
        wifi_status  = cloud_wifi_init(handle);
        if (wifi_status == M2M_SUCCESS)
        {
            // Set the current state
            cloud_iot_set_status(CLOUD_STATE_WIFI_CONFIGURE, CLOUD_STATUS_SUCCESS,
                                 "The cloud IoT Demo WINC1500 WIFI Init was successful.");
            ktastatus = ktaKeyStreamInit();
            if (ktastatus != E_K_STATUS_OK)
            {
                console_print_error_message("ktaKeyStreamInit failed");
                kta_gCloudWifiState = CLOUD_STATE_UNKNOWN;
                break;
            }
            // Set the next cloud WIFI state
            kta_gCloudWifiState = CLOUD_STATE_WIFI_CONFIGURE;
        }
        else
        {
            // Set the current state
            cloud_iot_set_status(CLOUD_STATE_ATECCx08A_INIT, CLOUD_STATUS_ATECCx08A_INIT_FAILURE,
                                 "The cloud IoT Demo WINC1500 WIFI init was not successful.");

            console_print_error_message("An WINC1500 WIFI initialization error has occurred.");
            console_print_error_message("Stopping the cloud IoT demo.");

            // An error has occurred during initialization.  Stop the demo.
            kta_gCloudWifiState = CLOUD_STATE_UNKNOWN;
        }
        break;

    case CLOUD_STATE_WIFI_CONFIGURE:
        // Set the current state
        cloud_iot_set_status(CLOUD_STATE_CLOUD_CONNECT, CLOUD_STATUS_SUCCESS,
                             "The cloud IoT Demo WINC1500 WIFI connect was successful.");

        // Set the next cloud WIFI state
        kta_gCloudWifiState = CLOUD_STATE_CLOUD_CONNECT;
        break;

    case CLOUD_STATE_CLOUD_CONNECT:
        /* Connect to the target BSS with the chosen authentication. */
        if (WDRV_WINC_STATUS_OK == WDRV_WINC_BSSConnect(handle, &bssCtx, &authCtx, &wifi_callback_handler))
        {
            // Set the next cloud WIFI state
            kta_gCloudWifiState = CLOUD_STATE_CLOUD_CONNECTING;
            console_print_message("Connecting to Cloud...");
        }
        break;
    case CLOUD_STATE_WIFI_CONNECTED:
        if (ktaKSCmdStatus == E_K_KTA_KS_STATUS_REFURBISH)
        {
          ktastatus = ktaKeyStreamInit();
        /*
         * TODO: Apply patch for soft reset in case of -- REFURBISH
         *  if (ktastatus != E_K_STATUS_OK)
         *  {
         *       console_print_error_message("ktaKeyStreamInit failed");
         *       kta_gCloudWifiState = CLOUD_STATE_UNKNOWN;
         *       break;
         *  }
         */
        }
        /* Calling KTA keySTREAM field management. */
        ktastatus = ktaKeyStreamFieldMgmt(true , &ktaKSCmdStatus);
        if (ktastatus != E_K_STATUS_OK)
        {
            console_print_error_message("ktaKeyStreamFieldMgmt failed");
            kta_gCloudWifiState = CLOUD_STATE_WIFI_DISCONNECT;
            //Over all delay of 30 seconds in case of error.
            while (delay_counter < KEYSTREAM_NO_OPS_DELAY_COUNTER) {
                nm_sleep(KEYSTREAM_NO_OPS_INTERVAL); // Sleep for 1 second
                delay_counter++;
            }
            break;
        }

        static bool isReadOnce = false;

        if (ktaKSCmdStatus == E_K_KTA_KS_STATUS_NO_OPERATION)
        {
            console_print_message("Device Onboarding in keySTREAM Done.Device is in ACTIVATED State.\r\n");

            if (isReadOnce)
                return;  // Exit if already executed.

            // Test Fota usecase
            Fota_Test();

            // Local buffers for data, metadata, and UID
            uint8_t dataBuffer[MAX_DATA_SIZE];
            uint8_t metadataBuffer[MAX_METADATA_SIZE];
            uint8_t uidBuffer[MAX_UID_SIZE];

            // Local structure with allocated buffers
            TKktaDataObject slotObject = {
                .data = dataBuffer,
                .dataLen = MAX_DATA_SIZE,
                .customerMetadata = metadataBuffer,
                .customerMetadataLen = MAX_METADATA_SIZE,
                .objectUid = uidBuffer,
                .objectUidLen = MAX_UID_SIZE
            };

            // Fetch object data
            ktastatus = ktaGetObject(IDENTIFIER_MANAGED_SLOT_14, &slotObject);

            if (E_K_STATUS_OK == ktastatus)
            {
                APP_DebugPrintf("Slot 14 Data:\r\n");
                for (int i = 0; i < slotObject.dataLen; i++)
                {
                    APP_DebugPrintf("%02x", slotObject.data[i]);
                }
                APP_DebugPrintf("\r\n");

                APP_DebugPrintf("Slot 14 MetaData:\r\n");
                for (int i = 0; i < slotObject.customerMetadataLen; i++)
                {
                    APP_DebugPrintf("%02x", slotObject.customerMetadata[i]);
                }
                APP_DebugPrintf("\r\n");

                APP_DebugPrintf("Slot 14 ObjectUid:\r\n");
                for (int i = 0; i < slotObject.objectUidLen; i++)
                {
                    APP_DebugPrintf("%02x", slotObject.objectUid[i]);
                }
                APP_DebugPrintf("\r\n");
            }

            isReadOnce = true;
        }

        /* Checking KTA status for Renewed or Refurbished. */
        if (ktaKSCmdStatus == E_K_KTA_KS_STATUS_RENEW || ktaKSCmdStatus == E_K_KTA_KS_STATUS_REFURBISH)
        {
            console_print_message("++++++++++Reset the state to CLOUD_STATE_WIFI_DISCONNECT");
            kta_gCloudWifiState = CLOUD_STATE_WIFI_DISCONNECT;
        }
        else if (kta_gCloudWifiState != CLOUD_STATE_CLOUD_CONNECTED)
        {
            /* Transferring Certificates received from keySTREAM to winc for AWS connection. */
            transfer_ecc_certs_to_winc();
            if (kta_gCloudWifiState != CLOUD_STATE_CLOUD_DISCONNECT)
            {
                kta_gCloudWifiState = CLOUD_STATE_CLOUD_CONNECTING;
                console_print_message("++++++++++CLOUD_STATE_CLOUD_CONNECTING");
            }
        }

        break;
    case CLOUD_STATE_CLOUD_CONNECTING:
        // Waiting for the cloud IoT connection to complete
        kta_gCloudWifiState = CLOUD_STATE_CLOUD_CONNECTING;
        break;

    case CLOUD_STATE_CLOUD_CONNECTED:
        // The cloud Demo is connect to cloud IoT
        console_print_success_message("cloud Demo: Connected to cloud IoT.");
        g_is_connected = true;
        uint8_t buf[1024];
        size_t buf_bytes_remaining = 1024;

        do
        {
            // Send the MQTT Connect message
            mqtt_options.keepAliveInterval = MQTT_KEEP_ALIVE_INTERVAL_S;
            mqtt_options.cleansession = 1;

            status = config_set_thing_id();
            if (status != ATCA_SUCCESS)
            {
                kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;
                console_print_error_message("The cloud IoT config failed.");
                break;
            }
            
            /* Client ID String */
            mqtt_options.clientID.cstring = (char*)&buf[0];
            status = config_get_client_id(mqtt_options.clientID.cstring, buf_bytes_remaining);
            if (status != ATCA_SUCCESS)
            {
                kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;
                console_print_error_message("The cloud IoT config failed.");
                break;
            }

            /* Username String */
            mqtt_options.username.cstring = mqtt_options.clientID.cstring + strlen(mqtt_options.clientID.cstring) + 1;
            buf_bytes_remaining -= (mqtt_options.username.cstring - mqtt_options.clientID.cstring);
            status = config_get_client_username(mqtt_options.username.cstring, buf_bytes_remaining);
            if (status != ATCA_SUCCESS)
            {
                kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;
                console_print_error_message("The cloud IoT config failed.");
                break;
            }

            /* Password String */
            mqtt_options.password.cstring = mqtt_options.username.cstring + strlen(mqtt_options.username.cstring) + 1;
            buf_bytes_remaining -= (mqtt_options.password.cstring - mqtt_options.username.cstring);
            status = config_get_client_password(mqtt_options.password.cstring, buf_bytes_remaining);
            if (status != ATCA_SUCCESS)
            {
                kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;
                console_print_error_message("The cloud IoT config failed.");
                break;
            }

            mqtt_status = MQTTConnect(&g_mqtt_client, &mqtt_options);
            if (mqtt_status != SUCCESS)
            {
                // The cloud IoT Demo failed to retrieve the device serial number
                cloud_iot_set_status(CLOUD_STATE_CLOUD_SUBSCRIPTION, CLOUD_STATUS_CLOUD_SUBSCRIPTION_FAILURE,
                                     "The cloud IoT Demo failed to connect with the MQTT connect message.");
                console_print_error_message("The cloud IoT Demo failed to connect with the MQTT connect message.");

                // Set the state to start the cloud WIFI Disconnect process
                if (kta_gCloudWifiState > CLOUD_STATE_WIFI_DISCONNECT)
                {
                    kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;
                }

                // Break the do/while loop
                break;
            }
            console_print_success_message("MQTT Connection");
            console_print_message("Device Provisioning with AWS Done.Device is in PROVISIONED State");
            // Set the state to cloud WIFI Reporting process
            kta_gCloudWifiState = CLOUD_STATE_CLOUD_SUBSCRIPTION;
        }
        while (false);
        break;

    case CLOUD_STATE_CLOUD_SUBSCRIPTION:
        config_get_client_sub_topic(g_mqtt_update_delta_topic_name, sizeof(g_mqtt_update_delta_topic_name));
        // Subscribe to the cloud IoT update delta topic message
        mqtt_status = MQTTSubscribe(&g_mqtt_client, g_mqtt_update_delta_topic_name, QOS0, &cloud_subscribe_callback);
        if (mqtt_status != SUCCESS)
        {
            // The cloud IoT Demo failed to retrieve the device serial number
            cloud_iot_set_status(CLOUD_STATE_CLOUD_SUBSCRIPTION, CLOUD_STATUS_CLOUD_SUBSCRIPTION_FAILURE,
                                 "The cloud IoT Demo failed to subscribe to the MQTT update topic subscription.");

            console_print_message("\r\n");
            console_print_error_message("The cloud IoT Demo failed to subscribe to the MQTT update topic subscription.");

            // Set the state to start the cloud WIFI Disconnect process
            if (kta_gCloudWifiState > CLOUD_STATE_WIFI_DISCONNECT)
            {
                kta_gCloudWifiState = CLOUD_STATE_CLOUD_DISCONNECT;
            }

            // Break the do/while loop
            break;
        }

        console_print_message("\r\n");
        console_print_success_message("Subscribed to the MQTT update topic subscription:");
        console_print_success_message(g_mqtt_update_delta_topic_name);
        console_print_message("\r\n");
        kta_gCloudWifiState = CLOUD_STATE_CLOUD_REPORTING;

        break;

    case CLOUD_STATE_CLOUD_REPORTING:
        // Sending/receiving topic update messages to/from cloud IoT

        if (client_counter_finished())
        {
            client_counter_set(PUBLISH_INTERVAL);
            // Publish the button update message
            publish = true;

            if (g_five_seconds_counter > (KEYSTREAM_TEN_MINUTES_INTERVAL +
                KEYSTREAM_ONE_HOUR_INTERVAL))
            {
                g_five_seconds_counter = KEYSTREAM_TEN_MINUTES_INTERVAL;
            }
            g_five_seconds_counter++;
        }

        if (publish)
        {
            cloud_publish_message();
            publish = false;
            if (g_five_seconds_counter < KEYSTREAM_TEN_MINUTES_INTERVAL)
            {
                connect_to_keystream();
            }
            else if ((g_five_seconds_counter - KEYSTREAM_TEN_MINUTES_INTERVAL) %
                    KEYSTREAM_ONE_HOUR_INTERVAL == 0)
            {
                connect_to_keystream();
            }
        }
        else
        {
            // Wait for incoming update messages
            mqtt_status = MQTTYield(&g_mqtt_client, MQTT_YEILD_TIMEOUT_MS);
            if (mqtt_status != SUCCESS)
            {
                // The cloud IoT Demo failed to retrieve the device serial number
                cloud_iot_set_status(CLOUD_STATE_CLOUD_REPORTING, CLOUD_STATUS_CLOUD_REPORT_FAILURE,
                                     "The cloud IoT Demo failed to publish the MQTT LED update message.");
                console_print_message("\r\n");
                console_print_error_message("The cloud IoT Demo failed to publish the MQTT LED update message.");
            }

            // If an error occurred in the WIFI connection, make sure to disconnect properly
            if (g_wifi_status == WIFI_STATUS_ERROR)
            {
                g_is_connected = false;
            }
        }
        break;

    case CLOUD_STATE_WIFI_DISCONNECT:
        // The cloud Demo is disconnected from access point
        g_is_connected = false;

        g_rx_buffer_length = 0;
        g_rx_buffer_location = 0;

        // Disconnect from the WINC1500 WIFI
        m2m_wifi_disconnect();

        // Close the socket
        shutdown(kta_gSocketConnection.socket);

        console_print_success_message("cloud Demo: Disconnected from WIFI access point.");

        // Set the state to start the cloud WIFI Configure process
        kta_gCloudWifiState = CLOUD_STATE_WIFI_CONFIGURE;

    case CLOUD_STATE_CLOUD_DISCONNECT:
        // The cloud Demo is disconnected from cloud IoT

        // Disconnect from cloud IoT
        if (g_is_connected == true)
        {
            // Unsubscribe to the cloud IoT update delta topic message
            mqtt_status = MQTTUnsubscribe(&g_mqtt_client, g_mqtt_update_delta_topic_name);
            if (mqtt_status != SUCCESS)
            {
                // The cloud IoT Demo failed to unsubscribe from the MQTT subscription
                cloud_iot_set_status(CLOUD_STATE_CLOUD_DISCONNECT, CLOUD_STATUS_CLOUD_SUBSCRIPTION_FAILURE,
                                     "The cloud IoT Demo failed to unsubscribe to the MQTT update topic subscription.");

                console_print_message("\r\n");
                console_print_error_message("The cloud IoT Demo failed to unsubscribe to the MQTT update topic subscription.");
            }

            // Disconnect from cloud IoT
            mqtt_status = MQTTDisconnect(&g_mqtt_client);
            if (mqtt_status != SUCCESS)
            {
                // The cloud IoT Demo failed to disconnect from cloud IoT
                cloud_iot_set_status(CLOUD_STATE_CLOUD_DISCONNECT, CLOUD_STATUS_CLOUD_SUBSCRIPTION_FAILURE,
                                     "The cloud IoT Demo failed to disconnect with the MQTT disconnect message.");

                console_print_message("\r\n");
                console_print_error_message("The cloud IoT Demo failed to disconnect with the MQTT disconnect message.");
            }
        }

        // Set the state to start the cloud WIFI Disconnect process
        kta_gCloudWifiState = CLOUD_STATE_WIFI_DISCONNECT;
        break;

    case CLOUD_STATE_UNKNOWN:
    default:
        break;
    }
    atca_delay_us(500);
}
/*******************************************************************************
  WINCS Host Assisted Wifi Service Header file 

  File Name:
    sys_wincs_wifi_service.h

  Summary:
    Header file for the WINCS Host Assisted Wifi Service implementation.

  Description:
    This file contains the header file for the WINCS Host Assisted Wifi Service
    implementation.
 *******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (C) 2020 released Microchip Technology Inc.  All rights reserved.

Microchip Technology Inc. and its subsidiaries.  You may use this software 
and any derivatives exclusively with Microchip products. 

THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 

IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
TERMS. 
 *******************************************************************************/
//DOM-IGNORE-END

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef SYS_WINCS_WIFI_SERVICE_H
#define    SYS_WINCS_WIFI_SERVICE_H

#include <xc.h> // include processor files - each processor file is guarded.
#include <stdbool.h>
#include "driver/driver_common.h"
#include "driver/wifi/wincs02/include/wdrv_winc_common.h"

// *****************************************************************************
// Handle for Wifi Configurations
//
// Summary:
//    Type definition for Wi-Fi handle.
//
// Description:
//    This type is used to represent a handle for Wi-Fi configurations.
//
// Remarks:
//    None.
// *****************************************************************************
typedef void * SYS_WINCS_WIFI_HANDLE_t;

#define SYS_WINCS_WIFI_DBG_MSG(args, ...)      SYS_CONSOLE_PRINT("[WIFI]:"args, ##__VA_ARGS__)

#define SYS_WINCS_WIFI_SSID_LEN_MAX       33
#define SYS_WINCS_WIFI_BSSID_LEN_MAX      32
#define SYS_WINCS_WIFI_PWD_LEN_MAX        128

// *****************************************************************************
// Wi-Fi Service Callback Maximum
//
// Summary:
//    Maximum number of Wi-Fi service callbacks.
//
// Description:
//    This macro defines the maximum number of Wi-Fi service callbacks.
//
// Remarks:
//    None.
// *****************************************************************************
#define SYS_WINCS_WIFI_SERVICE_CB_MAX     2

// *****************************************************************************
// WINCS Result Enumeration
//
// Summary:
//    Enumeration of possible WINCS results.
//
// Description:
//    This enumeration defines the possible results for WINCS operations.
//
// Remarks:
//    None.
// *****************************************************************************
typedef enum
{      
    /**<Success*/
    SYS_WINCS_PASS =  0x0000, 

    /**<Failure*/
    SYS_WINCS_FAIL =  -1, 

    /**<RAW mode*/
    SYS_WINCS_RAW = -2,  

    /**<Retry*/ 
    SYS_WINCS_COTN =  -3, 

    /**<Busy*/
    SYS_WINCS_BUSY = -4, 

    /**<Timeout*/
    SYS_WINCS_TIMEOUT = -5,

} SYS_WINCS_RESULT_t;

// *****************************************************************************
// Wi-Fi Service List Enumeration
//
// Summary:
//    Enumeration of Wi-Fi service requests.
//
// Description:
//    This enumeration defines the possible service requests for Wi-Fi operations.
//
// Remarks:
//    None.
// *****************************************************************************
typedef enum 
{
    /**<Request Driver version */ 
    SYS_WINCS_WIFI_GET_DRV_STATUS,

    /**<Request Open Driver */ 
    SYS_WINCS_WIFI_OPEN_DRIVER,

    /**<Request Driver Handle */ 
    SYS_WINCS_WIFI_GET_DRV_HANDLE,

    /* Get Time */
    SYS_WINCS_WIFI_GET_TIME,

    /* Set SNTP Conf */
    SYS_WINCS_WIFI_SET_SNTP,

    /**<Configure the Wi-Fi parameters */          
    SYS_WINCS_WIFI_SET_PARAMS, 

    /**<Request/Trigger Wi-Fi connect */
    SYS_WINCS_WIFI_STA_CONNECT,          

    /**<Request/Trigger Wi-Fi disconnect */         
    SYS_WINCS_WIFI_STA_DISCONNECT, 

    /**<Register the callback for async events */
    SYS_WINCS_WIFI_AP_ENABLE,

    /**<Request/Trigger SoftAP disable */        
    SYS_WINCS_WIFI_AP_DISABLE,       

    /**<Configure the Wi-Fi channel */        
    SYS_WINCS_WIFI_SET_CHANNEL_AP,   

    /**<Configure the Access point's BSSID */        
    SYS_WINCS_WIFI_SET_BSSID,          

    /**<Configure Hidden mode SSID in SoftAP mode*/        
    SYS_WINCS_WIFI_SET_HIDDEN,       

    /**<Request/Trigger Wi-Fi passive scan */       
    SYS_WINCS_WIFI_PASSIVE_SCAN,     

    /**<Request/Trigger Wi-Fi active scan */
    SYS_WINCS_WIFI_ACTIVE_SCAN, 

     /* DNS Resolve */
    SYS_WINCS_WIFI_DNS_RESOLVE,

    /* Set Reg Domain */
    SYS_WINCS_WIFI_SET_REG_DOMAIN,

    /* Get Reg Domain */
    SYS_WINCS_WIFI_GET_REG_DOMAIN,

    /**<Register the callback for async events */        
    SYS_WINCS_WIFI_SET_CALLBACK,     

    /**<Get Callback functions data*/        
    SYS_WINCS_WIFI_GET_CALLBACK,

    /**<Register the callback for async events */
    SYS_WINCS_WIFI_SET_SRVC_CALLBACK, 


} SYS_WINCS_WIFI_SERVICE_t;

// *****************************************************************************
// Wi-Fi Event Code Enumeration
//
// Summary:
//    Enumeration of Wi-Fi event codes.
//
// Description:
//    This enumeration defines the possible event codes for Wi-Fi operations.
//
// Remarks:
//    None.
// *****************************************************************************
typedef enum 
{
    /**<Wi-Fi connected event code*/
    SYS_WINCS_WIFI_CONNECTED,

    /**<Wi-Fi disconnected event code*/
    SYS_WINCS_WIFI_DISCONNECTED,          

    /**<Wi-Fi connection failure event code*/       
    SYS_WINCS_WIFI_CONNECT_FAILED,
     
    /* Wifi Association state is connecting. */        
    SYS_WINCS_WIFI_CONN_STATE_ROAMED,
            
    /* Wifi Association state is roamed. */        
    SYS_WINCS_WIFI_CONN_STATE_CONNECTING,

    /**<Wi-Fi DHCP complete event code*/
    SYS_WINCS_WIFI_DHCP_IPV4_COMPLETE, 

    /**<Wi-Fi DHCP complete event code*/
    SYS_WINCS_WIFI_DHCP_IPV6_LOCAL_COMPLETE,

    /**<Wi-Fi DHCP complete event code*/
    SYS_WINCS_WIFI_DHCP_IPV6_GLOBAL_COMPLETE,

    /**<SNTP up event*/
    SYS_WINCS_WIFI_SNTP_UP,

    /* Set regulatory domain Ack */
    SYS_WINCS_WIFI_REG_DOMAIN_SET_ACK,

    /* DNS resolved event */
    SYS_WINCS_WIFI_DNS_RESOLVED,

    /* WIFI Service Error*/
    SYS_WINCS_WIFI_ERROR,

} SYS_WINCS_WIFI_EVENT_t;

// *****************************************************************************
// Wi-Fi Security Modes Enumeration
//
// Summary:
//    Enumeration of Wi-Fi security modes.
//
// Description:
//    This enumeration defines the possible security modes for Wi-Fi connections.
//
// Remarks:
//    None.
// *****************************************************************************
typedef enum 
{
    /**<OPEN mode, no security*/
    SYS_WINCS_WIFI_SECURITY_OPEN = 0,              

    /* WPA2 mixed mode (AP) / compatibility mode (STA) with PSK.
     * (As an AP GTK is TKIP, as a STA GTK is chosen by AP;
     * PTK can be CCMP or TKIP) */
    SYS_WINCS_WIFI_SECURITY_WPA2_MIXED = 2,            

    /* WPA2-only authentication with PSK. (PTK and GTK are both CCMP).       */
    /* Note that a WPA2-only STA will not connect to a WPA2 mixed mode AP.   */
    SYS_WINCS_WIFI_SECURITY_WPA2 = 3,                  

    /* WPA3 SAE transition mode. (CCMP, IGTK can be BIP or none) */
    SYS_WINCS_WIFI_SECURITY_WPA3_TRANS = 4,            

    /* WPA3 SAE authentication. (CCMP, IGTK is BIP) */
    SYS_WINCS_WIFI_SECURITY_WPA3 = 5,   

} SYS_WINCS_WIFI_SECURITY_t;

// *****************************************************************************
// Wi-Fi Operation Modes Enumeration
//
// Summary:
//    Enumeration of Wi-Fi operation modes.
//
// Description:
//    This enumeration defines the possible operation modes for Wi-Fi.
//
// Remarks:
//    None.
// *****************************************************************************
typedef enum 
{
    /**<Station (STA) mode of WiFi operation*/
    SYS_WINCS_WIFI_MODE_STA, 

    /**<Software Access Point (SoftAP) mode of WiFi operation*/
    SYS_WINCS_WIFI_MODE_AP,

} SYS_WINCS_WIFI_MODE_t;

// *****************************************************************************
// Wi-Fi Parameters Structure
//
// Summary:
//    Structure to hold Wi-Fi parameters.
//
// Description:
//    This structure holds the parameters for Wi-Fi configuration.
//
// Remarks:
//    None.
// *****************************************************************************
typedef struct
{
    /**< Wi-Fi operation mode ::SYS_WINCS_WIFI_MODE_t either STA (Station) or SoftAP (Software Access Point) */
    SYS_WINCS_WIFI_MODE_t     mode;      

    /**< Wi-Fi SSID (Service Set Identifier) of Home AP (Access Point) or SoftAP */
    const char                *ssid;              

    /**< Wi-Fi Passphrase of Home AP or SoftAP */
    const char                *passphrase;   

    /**< Wi-Fi Security mode ::SYS_WINCS_WIFI_SECURITY_t */
    SYS_WINCS_WIFI_SECURITY_t security;  

    /**< Wi-Fi autoconnect flag, indicates whether to automatically connect to the network */
    uint8_t                   autoConnect;   

    /**< Wi-Fi channel number */
    uint8_t                   channel;

    /**< SSID visibility flag, indicates whether the SSID is hidden (true) or visible (false) */
    bool                      ssidVisibility;

} SYS_WINCS_WIFI_PARAM_t;

// *****************************************************************************
// Wi-Fi Scan Parameters Structure
//
// Summary:
//    Structure to hold Wi-Fi scan parameters.
//
// Description:
//    This structure holds the parameters for Wi-Fi scanning.
//
// Remarks:
//    None.
// *****************************************************************************
typedef struct 
{
    /**< Wi-Fi channel number to scan */
    uint8_t  channel;

    /**< Duration of the scan time in milliseconds */
    uint16_t scanTime;

} SYS_WINCS_WIFI_SCAN_PARAM_t;



// *****************************************************************************
// Wi-Fi Callback Function Type
//
// Summary:
//    Type definition for Wi-Fi callback function.
//
// Description:
//    This type defines the callback function for Wi-Fi events.
//
// Parameters:
//    event - One of the ::SYS_WINCS_WIFI_EVENT_t events
//    wifiHandle - Handle to the Wi-Fi service
//
// Remarks:
//    None.
// *****************************************************************************
typedef void (*SYS_WINCS_WIFI_CALLBACK_t)(SYS_WINCS_WIFI_EVENT_t event, SYS_WINCS_WIFI_HANDLE_t wifiHandle);

// *****************************************************************************
// Wi-Fi Service Layer API
//
// Summary:
//    API to handle STA and SoftAP mode operations.
//
// Description:
//    This function handles various Wi-Fi service requests for STA and SoftAP mode operations.
//
// Parameters:
//    request - Service request ::SYS_WINCS_WIFI_SERVICE_t
//    wifiHandle - Handle to the Wi-Fi service
//
// Returns:
//    ::SYS_WINCS_PASS - Requested service is handled successfully
//    ::SYS_WINCS_FAIL - Requested service has failed
//
// Remarks:
//    The asynchronous events are reported through callback, make sure that
//    the application registers the callback using the ::SYS_WINCS_WIFI_SET_CALLBACK
//    service request.
// *****************************************************************************
SYS_WINCS_RESULT_t SYS_WINCS_WIFI_SrvCtrl( SYS_WINCS_WIFI_SERVICE_t request, SYS_WINCS_WIFI_HANDLE_t wifiHandle);

// *****************************************************************************
// Get WINC Status
//
// Summary:
//    Retrieves the status of the WINC module.
//
// Description:
//    This function retrieves the status of the WINC module and provides a corresponding result.
//
// Parameters:
//    status - The current status of the WINC module, represented by ::WDRV_WINC_STATUS
//    service - A pointer to a character array where the service status will be stored
//
// Returns:
//    ::SYS_WINCS_PASS - Status retrieved successfully
//    ::SYS_WINCS_FAIL - Failed to retrieve status
//
// Remarks:
//    None.
// *****************************************************************************
SYS_WINCS_RESULT_t SYS_WINCS_WIFI_GetWincsStatus( WDRV_WINC_STATUS status, const char *functionName, int lineNo);

#endif    /* WINCS_WIFI_SERVICE_H */

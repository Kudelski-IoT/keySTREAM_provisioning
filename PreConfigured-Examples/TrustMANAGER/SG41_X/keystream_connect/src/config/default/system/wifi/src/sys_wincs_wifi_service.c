/*******************************************************************************
  WINCS Host Assisted Wifi Service Implementation

  File Name:
    sys_wincs_wifi_service.c

  Summary:
    Source code for the WINCS Host Assisted wifi Service implementation.

  Description:
    This file contains the source code for the WINCS Host Assisted wifiService
    implementation.
 ******************************************************************************/

/*******************************************************************************
Copyright (C) 2020 released Microchip Technology Inc.  All rights reserved.

 
Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
//DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdio.h>
#include <string.h>
#include <time.h>

/* This section lists the other files that are included in this file.
 */
#include "system/wifi/sys_wincs_wifi_service.h"
#include "driver/wifi/wincs02/include/wdrv_winc.h"
#include "driver/wifi/wincs02/include/wdrv_winc_client_api.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

// Handle for the Wi-Fi driver, initialized to an invalid handle
static DRV_HANDLE             g_wdrvHandle = DRV_HANDLE_INVALID;

// Authentication context for the Wi-Fi driver
static WDRV_WINC_AUTH_CONTEXT g_authContext;

// Basic Service Set (BSS) context for the Wi-Fi driver
static WDRV_WINC_BSS_CONTEXT  g_bssContext;

// Index for finding BSS results
static uint8_t                g_bssFindIdx;

// Semaphore handle for BSS find result synchronization
static OSAL_SEM_HANDLE_TYPE   g_bssFindResult;

// Wi-Fi connection configuration structure
static WDRV_WINC_CONN_CFG     g_wifiCfg;

// Array of Wi-Fi callback handlers, with a size defined by SYS_WINCS_WIFI_SERVICE_CB_MAX
SYS_WINCS_WIFI_CALLBACK_t     g_wifiCallBackHandler[SYS_WINCS_WIFI_SERVICE_CB_MAX];


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

// *****************************************************************************
// AP Notify Callback
//
// Summary:
//    Callback function for AP notifications.
//
// Description:
//    This function is called to notify the application of changes in the connection
//    state of an associated client.
//
// Parameters:
//    handle - The handle to the WINC driver instance
//    assocHandle - The handle to the associated client
//    currentState - The current connection state of the associated client, represented by ::WDRV_WINC_CONN_STATE
//
// Returns:
//    None.
//
// Remarks:
//    This function is typically registered with the WINC driver to handle AP notifications.
// *****************************************************************************

static void SYS_WINCS_WIFI_ApNotifyCallback
(
    DRV_HANDLE handle, 
    WDRV_WINC_ASSOC_HANDLE assocHandle, 
    WDRV_WINC_CONN_STATE currentState
)
{
    switch (currentState)
    {
        case WDRV_WINC_CONN_STATE_CONNECTED:
        {
            WDRV_WINC_STATUS status;
            WDRV_WINC_MAC_ADDR macAddr;
            
            status = WDRV_WINC_AssocPeerAddressGet(assocHandle, &macAddr);
            if (WDRV_WINC_STATUS_OK == status)
            {
                #ifdef SYS_WINCS_WIFI_DEBUG_LOGS
                SYS_WINCS_WIFI_DBG_MSG("Association state is connected\r\n");
                SYS_WINCS_WIFI_DBG_MSG("Wifi State :: CONNECTED :: MAC : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                                        macAddr.addr[0], macAddr.addr[1], macAddr.addr[2],
                                        macAddr.addr[3], macAddr.addr[4], macAddr.addr[5]);
                #endif
            }
            
            break;
        }
        
        case WDRV_WINC_CONN_STATE_CONNECTING:
        {
            #ifdef SYS_WINCS_WIFI_DEBUG_LOGS
            SYS_WINCS_WIFI_DBG_MSG("Association state is connecting......\r\n");
            #endif
            break;
        }
        
        case WDRV_WINC_CONN_STATE_FAILED:
        {
            #ifdef SYS_WINCS_WIFI_DEBUG_LOGS
            SYS_WINCS_WIFI_DBG_MSG("Association state is connection failed. \r\n");
            #endif
            break;
        }
        
        case WDRV_WINC_CONN_STATE_DISCONNECTED:
        {
            #ifdef SYS_WINCS_WIFI_DEBUG_LOGS
            SYS_WINCS_WIFI_DBG_MSG("Association state is disconnected. \r\n");
            #endif
            break;
        }
        
        case WDRV_WINC_CONN_STATE_ROAMED:
        {
            #ifdef SYS_WINCS_WIFI_DEBUG_LOGS
            SYS_WINCS_WIFI_DBG_MSG("Association state is roamed. \r\n");
            #endif
            break;
        }
    }
}


// *****************************************************************************
// BSS Find Notify Callback
//
// Summary:
//    Callback function for BSS find notifications.
//
// Description:
//    This function is called to notify the application of the results of a BSS
//    (Basic Service Set) find operation. It provides information about each BSS
//    found during the scan.
//
// Parameters:
//    handle - The handle to the WINC driver instance
//    index - The index of the current BSS in the list of found BSSs
//    ofTotal - The total number of BSSs found during the scan
//    pBSSInfo - A pointer to a structure containing information about the current BSS
//
// Returns:
//    true - Continue to receive notifications for subsequent BSSs
//    false - Stop receiving notifications for subsequent BSSs
//
// Remarks:
//    This function is typically registered with the WINC driver to handle BSS find notifications.
// *****************************************************************************

static bool SYS_WINCS_WIFI_BssFindNotifyCallback
(
    DRV_HANDLE handle, 
    uint8_t index, 
    uint8_t ofTotal, 
    WDRV_WINC_BSS_INFO *pBSSInfo
)
{
    
    if (0 == ofTotal)
    {
        SYS_WINCS_WIFI_DBG_MSG("No AP Found Rescan\r\n");
    }
    else
    {
        if (1 == index)
        {
            SYS_WINCS_WIFI_DBG_MSG("#%02d\r\n", ofTotal);
            SYS_WINCS_WIFI_DBG_MSG(">>#  RI  Recommend CH BSSID             SSID\r\n");
            SYS_WINCS_WIFI_DBG_MSG(">>#      Auth Type\r\n>>#\r\n");
        }

        g_bssFindIdx = index;

        OSAL_SEM_Post(&g_bssFindResult);
    }

    return false;
}


// *****************************************************************************
// Connect Notify Callback
//
// Summary:
//    Callback function for connection state notifications.
//
// Description:
//    This function is called to notify the application of changes in the connection
//    state of the WINC module. It provides information about the current connection
//    state.
//
// Parameters:
//    handle - The handle to the WINC driver instance
//    assocHandle - The handle to the associated connection
//    currentState - The current connection state, represented by ::WDRV_WINC_CONN_STATE
//
// Returns:
//    None.
//
// Remarks:
//    This function is typically registered with the WINC driver to handle connection
//    state change notifications.
// *****************************************************************************

static void SYS_WINCS_WIFI_ConnectNotifyCallback 
( 
    DRV_HANDLE handle, 
    WDRV_WINC_ASSOC_HANDLE assocHandle, 
    WDRV_WINC_CONN_STATE currentState
)
{
    SYS_WINCS_WIFI_CALLBACK_t wifi_cb_func = g_wifiCallBackHandler[1];
    
    #if SYS_WINCS_WIFI_DEBUG_LOGS
    SYS_WINCS_WIFI_DBG_MSG("SYS_WINCS_WIFI_ConnectNotifyCallback : currentState %d\r\n",currentState);
    #endif   
    switch(currentState)
    {
        /* Association state is disconnected. */
        case WDRV_WINC_CONN_STATE_DISCONNECTED:
        {
            wifi_cb_func(SYS_WINCS_WIFI_DISCONNECTED, (SYS_WINCS_WIFI_HANDLE_t) assocHandle);
            break;
        }

        /* Association state is connecting. */
        case WDRV_WINC_CONN_STATE_CONNECTING:
        {
            wifi_cb_func(SYS_WINCS_WIFI_CONN_STATE_CONNECTING, (SYS_WINCS_WIFI_HANDLE_t) assocHandle);
            break;
        }

        /* Association state is connected. */
        case WDRV_WINC_CONN_STATE_CONNECTED:
        {
            wifi_cb_func(SYS_WINCS_WIFI_CONNECTED, (SYS_WINCS_WIFI_HANDLE_t) assocHandle);
            break;
        }

        /* Association state is connection failed. */
        case WDRV_WINC_CONN_STATE_FAILED:
        {
            wifi_cb_func(SYS_WINCS_WIFI_CONNECT_FAILED, (SYS_WINCS_WIFI_HANDLE_t) assocHandle);
            break;
        }

        /* Association state is roamed. */
        case WDRV_WINC_CONN_STATE_ROAMED:
        {
            wifi_cb_func(SYS_WINCS_WIFI_CONN_STATE_ROAMED,(SYS_WINCS_WIFI_HANDLE_t) assocHandle);
            break;
        }
    }
}


// *****************************************************************************
// DHCP Event Callback
//
// Summary:
//    Callback function for DHCP events.
//
// Description:
//    This function is called to notify the application of DHCP events related to
//    the WINC module. It provides information about the type of DHCP event and
//    any associated event data.
//
// Parameters:
//    handle - The handle to the WINC driver instance
//    ifIdx - The index of the network interface on which the event occurred
//    eventType - The type of DHCP event, represented by ::WDRV_WINC_NETIF_EVENT_TYPE
//    pEventInfo - A pointer to a structure containing information about the event
//
// Returns:
//    None.
//
// Remarks:
//    This function is typically registered with the WINC driver to handle DHCP events.
// *****************************************************************************

static void SYS_WINCS_WIFI_DhcpEventCallback
(
    DRV_HANDLE handle, 
    WDRV_WINC_NETIF_IDX ifIdx, 
    WDRV_WINC_NETIF_EVENT_TYPE eventType, 
    void *pEventInfo
)
{
    SYS_WINCS_WIFI_CALLBACK_t wifi_cb_func = g_wifiCallBackHandler[1];
    
    switch (eventType)
    {
        case WDRV_WINC_NETIF_EVENT_ADDR_UPDATE:
        {
            WDRV_WINC_NETIF_ADDR_UPDATE_TYPE *pAddrUpdateInfo = pEventInfo;

            if (NULL != pAddrUpdateInfo)
            {
                if (WDRV_WINC_IP_ADDRESS_TYPE_IPV4 == pAddrUpdateInfo->type)
                {
                    char s[20];
                    WDRV_WINC_UtilsIPAddressToString(&pAddrUpdateInfo->addr.v4, s, sizeof(s));
                    wifi_cb_func(SYS_WINCS_WIFI_DHCP_IPV4_COMPLETE,(uint8_t*) s);
                }
                else if (WDRV_WINC_IP_ADDRESS_TYPE_IPV6 == pAddrUpdateInfo->type)
                {
                    char s[64];
                    WDRV_WINC_UtilsIPv6AddressToString(&pAddrUpdateInfo->addr.v6, s, sizeof(s));
                    if(strstr(s,"fe80"))
                    {
                        wifi_cb_func(SYS_WINCS_WIFI_DHCP_IPV6_LOCAL_COMPLETE,(uint8_t*) s);
                    }
                    else
                    {
                        wifi_cb_func(SYS_WINCS_WIFI_DHCP_IPV6_GLOBAL_COMPLETE,(uint8_t*) s);
                    }
                }
            }
            break;
        }

         /* Network interface up. */
        case WDRV_WINC_NETIF_EVENT_INTF_UP:
        {
            #ifdef SYS_WINCS_WIFI_DEBUG_LOGS
            SYS_WINCS_WIFI_DBG_MSG(" Network interface up. \r\n" );
            #endif
            break;
        }
        
        default:
        {
            break;
        }
    }
}


// *****************************************************************************
// DNS Resolve Callback
//
// Summary:
//    Callback function for DNS resolution events.
//
// Description:
//    This function is called to notify the application of the result of a DNS
//    resolution request. It provides information about the status of the DNS
//    resolution, the type of DNS record, the domain name, and the resolved IP address.
//
// Parameters:
//    handle - The handle to the WINC driver instance
//    status - The status of the DNS resolution, represented by ::WDRV_WINC_DNS_STATUS_TYPE
//    recordType - The type of DNS record (e.g., A, AAAA)
//    pDomainName - A pointer to the domain name that was resolved
//    pIPAddr - A pointer to a structure containing the resolved IP address
//
// Returns:
//    None.
//
// Remarks:
//    This function is typically registered with the WINC driver to handle DNS resolution events.
// *****************************************************************************

static void SYS_WINCS_WIFI_ResolveCallback
(
    DRV_HANDLE handle, 
    WDRV_WINC_DNS_STATUS_TYPE status, 
    uint8_t recordType, 
    const char *pDomainName, 
    WDRV_WINC_IP_MULTI_TYPE_ADDRESS *pIPAddr
)
{
    char addrStr[64] = {""};
    if ((WDRV_WINC_DNS_STATUS_OK != status) || (NULL == pIPAddr))
    {
        SYS_WINCS_WIFI_DBG_MSG("DNS resolve failed (%d)\r\n", status);
        return;
    }

    if (pIPAddr->type == WDRV_WINC_IP_ADDRESS_TYPE_IPV4)
    {
        inet_ntop(AF_INET, &pIPAddr->addr, addrStr, sizeof(addrStr));
    }
    else if (pIPAddr->type == WDRV_WINC_IP_ADDRESS_TYPE_IPV6)
    {
        inet_ntop(AF_INET6, &pIPAddr->addr, addrStr, sizeof(addrStr));
    }
    else
    {
        SYS_WINCS_WIFI_DBG_MSG("DNS resolve type error (%d)\r\n", pIPAddr->type);
    }

    SYS_WINCS_WIFI_CALLBACK_t wifi_cb_func = g_wifiCallBackHandler[1];
    wifi_cb_func(SYS_WINCS_WIFI_DNS_RESOLVED, (void *)addrStr);
}

// *****************************************************************************
// System Time Get Current Callback
//
// Summary:
//    Callback function for retrieving the current system time.
//
// Description:
//    This function is called to notify the application of the current system time
//    in UTC format. It provides the current time as a 32-bit unsigned integer
//    representing the number of seconds since the Unix epoch (January 1, 1970).
//
// Parameters:
//    handle - The handle to the WINC driver instance
//    timeUTC - The current system time in UTC, represented as a 32-bit unsigned integer
//
// Returns:
//    None.
//
// Remarks:
//    This function is typically registered with the WINC driver to handle system
//    time retrieval events.
// *****************************************************************************

static void SYS_WINCS_SYSTEM_TimeGetCurrentCallback
(
    DRV_HANDLE handle, 
    uint32_t timeUTC
)
{
    struct tm *ptm;
    
    ptm = gmtime(&timeUTC);
    static bool print = true;

    if(print == true)
    {
        if ((ptm->tm_year+1900) > 2000)
        {
            #ifdef SYS_WINCS_WIFI_DEBUG_LOGS
            SYS_WINCS_WIFI_DBG_MSG("Time UTC : %d\r\n",timeUTC);
            SYS_WINCS_WIFI_DBG_MSG("Time: %02d:%02d:%02d of %02d/%02d/%02d\r\n",
                    ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
                    ptm->tm_mday, ptm->tm_mon+1, ptm->tm_year+1900);
            #endif
            
            SYS_WINCS_WIFI_CALLBACK_t wifi_cb_func = g_wifiCallBackHandler[1];
            wifi_cb_func(SYS_WINCS_WIFI_SNTP_UP, (void *)&timeUTC);
            
            print = false;
        }
    }
}



// *****************************************************************************
// Get Regulatory Domain Callback
//
// Summary:
//    Callback function for retrieving regulatory domain information.
//
// Description:
//    This function is called to notify the application of the regulatory domain
//    information for the WINC module. It provides details about the current
//    regulatory domain and other available domains.
//
// Parameters:
//    handle - The handle to the WINC driver instance
//    index - The index of the current regulatory domain information being provided
//    ofTotal - The total number of regulatory domains available
//    isCurrent - A boolean indicating if the provided regulatory domain is the current one
//    pRegDomInfo - A pointer to a structure containing the regulatory domain information
//
// Returns:
//    None.
//
// Remarks:
//    This function is typically registered with the WINC driver to handle regulatory
//    domain information retrieval events.
// *****************************************************************************


static void SYS_WINCS_WIFI_getRegDomainCallback
(
    DRV_HANDLE handle, 
    uint8_t index, 
    uint8_t ofTotal, 
    bool isCurrent, 
    const WDRV_WINC_REGDOMAIN_INFO *const pRegDomInfo
)
{
    if (0 == ofTotal)
    {
        if ((NULL == pRegDomInfo) || (0 == pRegDomInfo->regDomainLen))
        {
            SYS_WINCS_WIFI_DBG_MSG("No Regulatory Domains Defined\r\n");
        }
        else
        {
            SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_INVALID_ARG, __FUNCTION__, __LINE__);
        }
    }
    else if (NULL != pRegDomInfo)
    {
        if (1 == index)
        {
            SYS_WINCS_WIFI_DBG_MSG("#.   CC      Channels Ver Status\r\n");
        }
        SYS_WINCS_WIFI_DBG_MSG("%02d: [%-6s] 0x%0-6x %d.%d %s\r\n", 
            index, pRegDomInfo->regDomain, 0, 0, 0, isCurrent ? "Active" : "");
        if (index == ofTotal)
        {
            SYS_WINCS_WIFI_DBG_MSG("DONE \r\n");
        }
    }
    else
    {
        SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_INVALID_ARG, __FUNCTION__, __LINE__);
    }
}

// *****************************************************************************
// Set Regulatory Domain Callback
//
// Summary:
//    Callback function for setting regulatory domain information.
//
// Description:
//    This function is called to notify the application of the result of setting
//    the regulatory domain for the WINC module. It provides details about the
//    regulatory domain that was set and whether it is the current one.
//
// Parameters:
//    handle - The handle to the WINC driver instance
//    index - The index of the regulatory domain information being provided
//    ofTotal - The total number of regulatory domains available
//    isCurrent - A boolean indicating if the provided regulatory domain is the current one
//    pRegDomInfo - A pointer to a structure containing the regulatory domain information
//
// Returns:
//    None.
//
// Remarks:
//    This function is typically registered with the WINC driver to handle regulatory
//    domain setting events.
// *****************************************************************************

static void SYS_WINCS_WIFI_setRegDomainCallback
(
    DRV_HANDLE handle, 
    uint8_t index, 
    uint8_t ofTotal, 
    bool isCurrent, 
    const WDRV_WINC_REGDOMAIN_INFO *const pRegDomInfo
)
{
    SYS_WINCS_WIFI_CALLBACK_t wifi_cb_func = g_wifiCallBackHandler[1];
    
    if ((1 != index) || (1 != ofTotal) || (false == isCurrent) || 
        (NULL == pRegDomInfo) || (0 == pRegDomInfo->regDomainLen))
    {
        wifi_cb_func(SYS_WINCS_WIFI_ERROR, NULL);
    }
    else
    {
        wifi_cb_func(SYS_WINCS_WIFI_REG_DOMAIN_SET_ACK, NULL);
    }
}

// *****************************************************************************
// Get WINC Status
//
// Summary:
//    Function to retrieve and log the status of the WINC module.
//
// Description:
//    This function is called to retrieve the current status of the WINC module
//    and log it along with the function name and line number where the status
//    was checked. It returns a result indicating the success or failure of the
//    status retrieval.
//
// Parameters:
//    status - The current status of the WINC module, represented by WDRV_WINC_STATUS
//    functionName - A string containing the name of the function where the status was checked
//    lineNo - The line number in the source code where the status was checked
//
// Returns:
//    SYS_WINCS_RESULT_t - A result indicating the success or failure of the status retrieval.
//
// Remarks:
//    This function can be used for debugging and logging purposes to track the
//    status of the WINC module throughout the application.
// *****************************************************************************

SYS_WINCS_RESULT_t SYS_WINCS_WIFI_GetWincsStatus
(
    WDRV_WINC_STATUS status,
    const char *functionName,
    int lineNo
)
{
    if( WDRV_WINC_STATUS_OK == status)
    {
        return SYS_WINCS_PASS;
    }
    else if((WDRV_WINC_STATUS_RETRY_REQUEST == status) || (WDRV_WINC_STATUS_BUSY == status))
    {
        return SYS_WINCS_BUSY;
    }
    else
    {
        SYS_CONSOLE_PRINT("ERROR!!! : Function %s, Line No : %d, Status : %d\r\n",
                functionName, lineNo, status);
    }
    return SYS_WINCS_FAIL;
}

// *****************************************************************************
// Set Authentication Personal Type
//
// Summary:
//    Function to set the authentication type for a personal Wi-Fi network.
//
// Description:
//    This function sets the authentication type for a personal Wi-Fi network
//    in the provided Wi-Fi configuration structure. It updates the configuration
//    with the specified authentication type.
//
// Parameters:
//    wifi_config - A pointer to the Wi-Fi configuration structure to be updated
//    authType - The authentication type to be set, represented by WDRV_WINC_AUTH_TYPE
//
// Returns:
//    WDRV_WINC_STATUS - The status of the operation, indicating success or failure.
//
// Remarks:
//    This function is used to configure the authentication type for personal
//    Wi-Fi networks such as WPA2-Personal.
// *****************************************************************************

static WDRV_WINC_STATUS SYS_WINCS_WIFI_SetAuthPersonalType
(
    SYS_WINCS_WIFI_PARAM_t *wifi_config,
    WDRV_WINC_AUTH_TYPE authType
)
{
    return WDRV_WINC_AuthCtxSetPersonal(&g_authContext, (uint8_t*)wifi_config ->passphrase 
                            , strlen(wifi_config ->passphrase), authType);
}


// *****************************************************************************
// Set Wi-Fi Parameters
//
// Summary:
//    Function to set the Wi-Fi parameters for the WINC module.
//
// Description:
//    This function sets the Wi-Fi parameters in the provided Wi-Fi configuration
//    structure. It updates the configuration with the specified parameters and
//    applies them to the WINC module.
//
// Parameters:
//    wifi_config - A pointer to the Wi-Fi configuration structure to be updated
//
// Returns:
//    WDRV_WINC_STATUS - The status of the operation, indicating success or failure.
//
// Remarks:
//    This function is used to configure various Wi-Fi parameters such as SSID,
//    authentication type, and other settings for the WINC module.
// *****************************************************************************

static WDRV_WINC_STATUS SYS_WINCS_WIFI_SetWifiParams
(
    SYS_WINCS_WIFI_PARAM_t *wifi_config
)
{
    WDRV_WINC_STATUS status = WDRV_WINC_STATUS_NOT_CONNECTED;
    uint8_t ssidLength;    

    ssidLength = strlen(wifi_config -> ssid);

    status = WDRV_WINC_BSSCtxSetDefaults(&g_bssContext);
    if(status != WDRV_WINC_STATUS_OK)
    {
        return status;
    }

    status = WDRV_WINC_AuthCtxSetDefaults(&g_authContext);
    if(status != WDRV_WINC_STATUS_OK)
    {
        return status;
    }

    status =  WDRV_WINC_BSSCtxSetSSID(&g_bssContext, (uint8_t*)wifi_config -> ssid, ssidLength);
    if(status != WDRV_WINC_STATUS_OK)
    {
        return status;
    }
                
    /* Security Configuration*/
    switch (wifi_config -> security)
    {
        case SYS_WINCS_WIFI_SECURITY_OPEN:
        {
            /* Initialize the authentication context for open mode. */
            status =  WDRV_WINC_AuthCtxSetOpen(&g_authContext);
            break;
        }
        case SYS_WINCS_WIFI_SECURITY_WPA2:
        {
            status =  SYS_WINCS_WIFI_SetAuthPersonalType(wifi_config, WDRV_WINC_AUTH_TYPE_WPA2_PERSONAL);
            break;
        }

        case SYS_WINCS_WIFI_SECURITY_WPA2_MIXED:
        {
            status =  SYS_WINCS_WIFI_SetAuthPersonalType(wifi_config, WDRV_WINC_AUTH_TYPE_WPAWPA2_PERSONAL);
            break;
        }

        case SYS_WINCS_WIFI_SECURITY_WPA3_TRANS:
        {
            status =  SYS_WINCS_WIFI_SetAuthPersonalType(wifi_config, WDRV_WINC_AUTH_TYPE_WPA2WPA3_PERSONAL);
            break;
        }

        case SYS_WINCS_WIFI_SECURITY_WPA3:
        {
            status =  SYS_WINCS_WIFI_SetAuthPersonalType(wifi_config, WDRV_WINC_AUTH_TYPE_WPA3_PERSONAL);
            break;
        }

        default:
        {
            SYS_WINCS_WIFI_DBG_MSG("[ERROR] : INVALID_PARAMETER for AUTHENTICATION\r\n");
            return status;
        }
    }

    if(wifi_config->mode == SYS_WINCS_WIFI_MODE_STA)
    {
        if(wifi_config->autoConnect)
        {
            WDRV_WINC_NetIfRegisterEventCallback(g_wdrvHandle, SYS_WINCS_WIFI_DhcpEventCallback);
            #ifdef SYS_WINCS_WIFI_DEBUG_LOGS
            SYS_WINCS_WIFI_DBG_MSG("[WIFI] : Connecting to AP : %s\r\n",wifi_config->ssid);
            #endif
            status = WDRV_WINC_BSSConnect(g_wdrvHandle, &g_bssContext, &g_authContext,
                &g_wifiCfg,SYS_WINCS_WIFI_ConnectNotifyCallback);
        }
    }
    else if(wifi_config->mode == SYS_WINCS_WIFI_MODE_AP)                
    {        
        WDRV_WINC_CHANNEL_ID channel;
        channel = wifi_config->channel;

        status =  WDRV_WINC_BSSCtxSetChannel(&g_bssContext, channel);
        if(status != WDRV_WINC_STATUS_OK)
        {
            return status;
        }
        WDRV_WINC_APDefaultWiFiCfg(&g_wifiCfg);
        g_wifiCfg.ap.cloaked = wifi_config ->ssidVisibility;
    }
    
    return status;
}



// *****************************************************************************
// Wi-Fi Service Control
//
// Summary:
//    Function to control Wi-Fi services for the WINC module.
//
// Description:
//    This function handles various Wi-Fi service control requests for the WINC
//    module. It processes the specified request and performs the corresponding
//    action using the provided Wi-Fi handle.
//
// Parameters:
//    request - The Wi-Fi service request to be processed, represented by SYS_WINCS_WIFI_SERVICE_t
//    wifiHandle - The handle to the Wi-Fi service instance
//
// Returns:
//    SYS_WINCS_RESULT_t - The result of the service control operation, indicating success or failure.
//
// Remarks:
//    This function is used to manage different Wi-Fi services such as starting
//    or stopping the Wi-Fi module, scanning for networks, and other control operations.
// *****************************************************************************

SYS_WINCS_RESULT_t SYS_WINCS_WIFI_SrvCtrl
( 
    SYS_WINCS_WIFI_SERVICE_t request, 
    SYS_WINCS_WIFI_HANDLE_t wifiHandle
)
{
    WDRV_WINC_STATUS status = WDRV_WINC_STATUS_OK;
    
    switch (request)
    {
        case SYS_WINCS_WIFI_GET_DRV_STATUS:
        {
            *(int8_t*)wifiHandle = '\0';
            *(int8_t*)wifiHandle = WDRV_WINC_Status(sysObj.drvWifiWinc);
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__); 
        }
        
        case SYS_WINCS_WIFI_OPEN_DRIVER:
        {
            if (DRV_HANDLE_INVALID == g_wdrvHandle)
            {
                g_wdrvHandle = WDRV_WINC_Open(0, 0);
                if (DRV_HANDLE_INVALID == g_wdrvHandle)
                {
                    SYS_WINCS_WIFI_DBG_MSG("ERROR : Driver Handle Invalid\r\n");
                    return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
                }
            }
            *(DRV_HANDLE *)wifiHandle = g_wdrvHandle;
            return SYS_WINCS_PASS;
        }
        
        case SYS_WINCS_WIFI_GET_DRV_HANDLE:
        {
            *(DRV_HANDLE *)wifiHandle = g_wdrvHandle;
            return SYS_WINCS_PASS;
        }
        
        /* WINCS Get Time */
        case SYS_WINCS_WIFI_GET_TIME:
        {
            status = WDRV_WINC_SystemTimeGetCurrent(g_wdrvHandle, SYS_WINCS_SYSTEM_TimeGetCurrentCallback);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
        
        /* WINCS Set SNTP */
        case SYS_WINCS_WIFI_SET_SNTP:
        {            
            status =  WDRV_WINC_SNTPServerAddressSet(g_wdrvHandle, (char *)wifiHandle);
            if (WDRV_WINC_STATUS_OK != status )
            {
                return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
            }
            
            status =   WDRV_WINC_SNTPStaticSet(g_wdrvHandle, true);
            if (WDRV_WINC_STATUS_OK != status )
            {
                return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
            }
            
            status =  WDRV_WINC_SNTPEnableSet(g_wdrvHandle, true);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }


        /**<Request/Trigger Wi-Fi connect */
        case SYS_WINCS_WIFI_STA_CONNECT:
        {
            status = WDRV_WINC_NetIfRegisterEventCallback(g_wdrvHandle, SYS_WINCS_WIFI_DhcpEventCallback);
            if(status != WDRV_WINC_STATUS_OK)
            {
                return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
            }
            status = WDRV_WINC_BSSConnect(g_wdrvHandle, &g_bssContext, &g_authContext,
                    &g_wifiCfg,SYS_WINCS_WIFI_ConnectNotifyCallback);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }

         /**<Request/Trigger Wi-Fi disconnect */    
        case SYS_WINCS_WIFI_STA_DISCONNECT:
        {
            status = WDRV_WINC_BSSDisconnect(g_wdrvHandle);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }

        /**<Request/Trigger SoftAP disable */       
        case SYS_WINCS_WIFI_AP_DISABLE:
        {
            status = WDRV_WINC_APStop(g_wdrvHandle);
            if(status != WDRV_WINC_STATUS_OK)
            {
                return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
            }

            status =  WDRV_WINC_DHCPSEnableSet(g_wdrvHandle, WDRV_WINC_DHCPS_IDX_0, false, NULL);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }

         /**<Configure the Wi-Fi channel */    
        case SYS_WINCS_WIFI_SET_CHANNEL_AP:
        {      
            uint8_t channel = *(uint8_t *)wifiHandle;
            status =  WDRV_WINC_BSSCtxSetChannel(&g_bssContext, channel);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);            
        }   
        
        /**<Configure the Access point's BSSID */ 
        case SYS_WINCS_WIFI_SET_BSSID:
        {  
            uint8_t *newBssid = (uint8_t *)wifiHandle;
            WDRV_WINC_BSSCtxSetBSSID(&g_bssContext, newBssid);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        } 
        
        /**<Configure Hidden mode SSID in SoftAP mode*/     
        case SYS_WINCS_WIFI_SET_HIDDEN:
        {     
            g_wifiCfg.ap.cloaked = (bool)wifiHandle;
            status =  WDRV_WINC_STATUS_OK;
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }  

        /**<Configure the Wi-Fi parameters */ 
        case SYS_WINCS_WIFI_SET_PARAMS:  
        {
            status = SYS_WINCS_WIFI_SetWifiParams((SYS_WINCS_WIFI_PARAM_t *)wifiHandle);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);           
        }
        
        case SYS_WINCS_WIFI_AP_ENABLE:
        {
            status = WDRV_WINC_APStart(g_wdrvHandle, &g_bssContext, &g_authContext
                    ,&g_wifiCfg, SYS_WINCS_WIFI_ApNotifyCallback);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }

        /**<Request/Trigger Wi-Fi passive scan */ 
        case SYS_WINCS_WIFI_PASSIVE_SCAN:
        {
            SYS_WINCS_WIFI_SCAN_PARAM_t *scanParams = (SYS_WINCS_WIFI_SCAN_PARAM_t *)wifiHandle;
            
            status = WDRV_WINC_BSSFindSetScanParameters(g_wdrvHandle, 0, 0, scanParams->scanTime, 0);
            if(status != WDRV_WINC_STATUS_OK)
            {
                return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
            }

            status =  WDRV_WINC_BSSFindFirst(g_wdrvHandle, scanParams->channel, 
                false, NULL, SYS_WINCS_WIFI_BssFindNotifyCallback);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }

        /**<Request/Trigger Wi-Fi active scan */    
        case SYS_WINCS_WIFI_ACTIVE_SCAN:
        {
            SYS_WINCS_WIFI_SCAN_PARAM_t *scanParams = (SYS_WINCS_WIFI_SCAN_PARAM_t *)wifiHandle;
            
            OSAL_SEM_Create(&g_bssFindResult, OSAL_SEM_TYPE_COUNTING, 255, 0);
            status =  WDRV_WINC_BSSFindFirst(g_wdrvHandle,(WDRV_WINC_CHANNEL_ID ) scanParams->channel,
                 true, NULL, SYS_WINCS_WIFI_BssFindNotifyCallback);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
        
        case SYS_WINCS_WIFI_SET_REG_DOMAIN:
        {
            uint8_t *regDomainName = (uint8_t *)wifiHandle;
            if (regDomainName == NULL)
            {
                return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_INVALID_ARG, __FUNCTION__, __LINE__);
            }
            
            status = WDRV_WINC_WifiRegDomainSet(g_wdrvHandle,regDomainName, 
                strlen((const char *)regDomainName),SYS_WINCS_WIFI_setRegDomainCallback );
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
        
        case SYS_WINCS_WIFI_GET_REG_DOMAIN:
        {
            status = WDRV_WINC_WifiRegDomainGet(g_wdrvHandle, WDRV_WINC_REGDOMAIN_SELECT_ALL,
                 SYS_WINCS_WIFI_getRegDomainCallback );
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
        

        /**<Regester the call back for async events */    
        case SYS_WINCS_WIFI_SET_CALLBACK:  
        {
            g_wifiCallBackHandler[1] = (SYS_WINCS_WIFI_CALLBACK_t)wifiHandle;
            return SYS_WINCS_PASS;
        }

        /**<Regester the call back for async events */
        case SYS_WINCS_WIFI_SET_SRVC_CALLBACK:                        
        {
            g_wifiCallBackHandler[0] = (SYS_WINCS_WIFI_CALLBACK_t)wifiHandle;  
            return SYS_WINCS_PASS;
        }
        
        case SYS_WINCS_WIFI_DNS_RESOLVE:
        {
            char *domainName = (char *)wifiHandle;
            uint16_t timeoutMs = 100;
            status = WDRV_WINC_DNSResolveDomain(g_wdrvHandle, WINC_CONST_DNS_TYPE_A,
                    domainName, timeoutMs, SYS_WINCS_WIFI_ResolveCallback);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
        
        case SYS_WINCS_WIFI_GET_CALLBACK:
        {
            SYS_WINCS_WIFI_CALLBACK_t *callBackHandler;
            callBackHandler = (SYS_WINCS_WIFI_CALLBACK_t *)wifiHandle;
            
            callBackHandler[0] = g_wifiCallBackHandler[0];
            callBackHandler[1] = g_wifiCallBackHandler[1];
            return SYS_WINCS_PASS;
        }
        
        default:
        {
            SYS_WINCS_WIFI_DBG_MSG("ERROR : Unknown Wifi Service Request\r\n");
        }
    }  
    
    
    return SYS_WINCS_FAIL;
}


/* *****************************************************************************
 End of File
 */

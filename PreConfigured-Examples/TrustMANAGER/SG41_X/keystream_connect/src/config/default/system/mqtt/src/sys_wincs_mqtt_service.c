/*******************************************************************************
  WINCS Host Assisted MQTT Service Implementation(WINCS02)

  File Name:
    sys_wincs_mqtt_service.c

  Summary:
    Source code for the WINCS Host Assisted MQTT Service implementation.

  Description:
    This file contains the source code for the WINCS Host Assisted MQTT Service
    implementation.
 *******************************************************************************/

//DOM-IGNORE-BEGIN
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
/* This section lists the other files that are included in this file.
 */
#include <stdio.h>
#include <string.h>

/* This section lists the other files that are included in this file.
 */
#include "system/sys_wincs_system_service.h"
#include "system/net/sys_wincs_net_service.h"
#include "system/mqtt/sys_wincs_mqtt_service.h"



/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

static SYS_WINCS_MQTT_CALLBACK_t g_MqttCallBackHandler[SYS_WINCS_MQTT_SERVICE_CB_MAX] = {NULL, NULL};
    

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

// *****************************************************************************
// Function: SYS_WINCS_MQTT_ConnCallback
//
// Summary:
//    MQTT connection callback function.
//
// Description:
//    This function is called when there is a change in the MQTT connection status.
//    It handles the connection status and performs necessary actions based on the
//    connection state.
//
// Parameters:
//    handle      - The driver handle
//    userCtx     - User context
//    state       - The connection status type
//    pConnInfo   - Pointer to the connection information
//
// Returns:
//    None
//
// Remarks:
//    None
// *****************************************************************************
static void SYS_WINCS_MQTT_ConnCallback
(
    DRV_HANDLE handle, 
    uintptr_t userCtx, 
    WDRV_WINC_MQTT_CONN_STATUS_TYPE state,
    WDRV_WINC_MQTT_CONN_INFO *pConnInfo
)
{
    SYS_WINCS_MQTT_CALLBACK_t mqtt_cb_func = g_MqttCallBackHandler[1];
    switch(state)
    {
        case WDRV_WINC_MQTT_CONN_STATUS_CONNECTED:
        {
            SYS_WINC_MQTT_CONN_ACK_PROP *mqttConnAckProp = 
                    (SYS_WINC_MQTT_CONN_ACK_PROP *)pConnInfo;
            mqtt_cb_func(SYS_WINCS_MQTT_CONNECTED, mqttConnAckProp);
            break;
        }
        
        case WDRV_WINC_MQTT_CONN_STATUS_CONNECTING:
        {
            SYS_WINCS_MQTT_DBG_MSG( "MQTT connecting.....\r\n");
            break;
        }
        
        case WDRV_WINC_MQTT_CONN_STATUS_DISCONNECTED:
        {
            mqtt_cb_func(SYS_WINCS_MQTT_DISCONNECTED, NULL);
            break;
        }
        
        case WDRV_WINC_MQTT_CONN_STATUS_DISCONNECTING:
        {
            SYS_WINCS_MQTT_DBG_MSG( "MQTT disconnecting.....\r\n");
            break;
        }
        
         case WDRV_WINC_MQTT_CONN_STATUS_UNKNOWN:
        {
            SYS_WINCS_MQTT_DBG_MSG( "MQTT connect status unknown.\r\n");
            break;
        }
    }
}




// *****************************************************************************
// Function: SYS_WINCS_MQTT_SubscribeCallback
//
// Summary:
//    MQTT subscription callback function.
//
// Description:
//    This function is called when there is a change in the MQTT subscription status
//    or when a message is received on a subscribed topic. It handles the subscription
//    status and processes the received message.
//
// Parameters:
//    handle        - The driver handle
//    userCtx       - User context
//    pMsgInfo      - Pointer to the message information
//    pTopicName    - Pointer to the topic name
//    pTopicData    - Pointer to the topic data
//    topicDataLen  - Length of the topic data
//    status        - The subscription status type
//
// Returns:
//    None
//
// Remarks:
//    None
// *****************************************************************************
void SYS_WINCS_MQTT_SubscribeCallback
(
    DRV_HANDLE handle, 
    uintptr_t userCtx, 
    const WDRV_WINC_MQTT_MSG_INFO *pMsgInfo, 
    const char *pTopicName, 
    const uint8_t *pTopicData, 
    size_t topicDataLen, 
    WDRV_WINC_MQTT_SUB_STATUS_TYPE status
)
{
    SYS_WINCS_MQTT_CALLBACK_t mqtt_cb_func = g_MqttCallBackHandler[1];
    switch(status)
    {
        case WDRV_WINC_MQTT_SUB_STATUS_ACKED:
        {
            mqtt_cb_func(SYS_WINCS_MQTT_SUBCRIBE_ACK,NULL);
            break;
        }
        
        case WDRV_WINC_MQTT_SUB_STATUS_RXDATA:
        {
            // Create an MQTT frame to hold the received message details
            SYS_WINCS_MQTT_FRAME_t mqttRxFrame;

            // Populate the MQTT frame with the topic name and message data
            mqttRxFrame.topic = (char *)pTopicName;
            mqttRxFrame.message = (char *)pTopicData;
            // Call the callback function with the received message
            mqtt_cb_func(SYS_WINCS_MQTT_SUBCRIBE_MSG, (void *)&mqttRxFrame);

            break;
        }

        
        case WDRV_WINC_MQTT_SUB_STATUS_ERROR:
        {
            mqtt_cb_func(SYS_WINCS_MQTT_ERROR, NULL );
            break;
        }
        
        case WDRV_WINC_MQTT_SUB_STATUS_END:
        {
            mqtt_cb_func(SYS_WINCS_MQTT_UNSUBSCRIBED, NULL );
            break;
        }
        
        default:
            break;
    }
}

// *****************************************************************************
// Function: SYS_WINCS_MQTT_SrvCtrl
//
// Summary:
//    MQTT service control function.
//
// Description:
//    This function handles various MQTT service control requests. It processes
//    the specified request and performs the corresponding action on the given
//    MQTT handle.
//
// Parameters:
//    request     - The MQTT service request type
//    mqttHandle  - The MQTT handle
//
// Returns:
//    SYS_WINCS_RESULT_t - Result of the service control request
//
// Remarks:
//    None
// *****************************************************************************
SYS_WINCS_RESULT_t SYS_WINCS_MQTT_SrvCtrl
(
    SYS_WINCS_MQTT_SERVICE_t request, 
    SYS_WINCS_MQTT_HANDLE_t mqttHandle
)
{
    DRV_HANDLE wdrvHandle = DRV_HANDLE_INVALID;
    
    SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_GET_DRV_HANDLE, &wdrvHandle);
    WDRV_WINC_STATUS status = WDRV_WINC_STATUS_OK;
    
    switch(request)
    {
        /**<Configure the MQTT Broker parameters*/
        case SYS_WINCS_MQTT_CONFIG:
        {
            SYS_WINCS_MQTT_CFG_t *mqtt_cfg = (SYS_WINCS_MQTT_CFG_t *)mqttHandle;  
            
            if(mqtt_cfg->tlsIdx == true)
            {
                mqtt_cfg->tlsHandle = 0;
                SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_OPEN_TLS_CTX,NULL);
                SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_GET_TLS_CTX_HANDLE,(void *)&mqtt_cfg->tlsHandle);
                if( mqtt_cfg->tlsHandle == WDRV_WINC_TLS_INVALID_HANDLE)
                {
                    status = WDRV_WINC_STATUS_INVALID_CONTEXT;
                    return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
                }
                
                if (SYS_WINCS_FAIL == SYS_WINCS_NET_SockSrvCtrl(SYS_WINCS_NET_TLS_CONFIG, mqtt_cfg->tlsConf))
                {
                    status = WDRV_WINC_STATUS_INVALID_CONTEXT;
                    return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
                }
            }
            else
            {
                mqtt_cfg->tlsHandle = 0;
            }
            
            status = WDRV_WINC_MQTTBrokerSet(wdrvHandle,mqtt_cfg->url,
                        mqtt_cfg->port, mqtt_cfg->tlsHandle );
            if (WDRV_WINC_STATUS_OK != status)
            {
                return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
            }
            
            status = WDRV_WINC_MQTTClientCfgSet(wdrvHandle,mqtt_cfg->clientId,
                        mqtt_cfg->username, mqtt_cfg->password );
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }  
		/**<Connect to the MQTT Broker */  
        case SYS_WINCS_MQTT_CONNECT:
        {
            SYS_WINCS_MQTT_CFG_t *mqttConfig = (SYS_WINCS_MQTT_CFG_t *)mqttHandle; 
            
            if(mqttConfig->protoVer == SYS_WINCS_MQTT_PROTO_VER_3)
            {
                status =  WDRV_WINC_MQTTConnect(wdrvHandle, mqttConfig->cleanSession,
                        mqttConfig->keepAliveTime, mqttConfig->protoVer,
                        NULL, SYS_WINCS_MQTT_ConnCallback, 0);
            }
            else if(mqttConfig->protoVer == SYS_WINCS_MQTT_PROTO_VER_5)
            {
                WDRV_WINC_MQTT_CONN_PROP mqttConnProp = {
                    .sessionExpiryInterval = mqttConfig->sessionExpiryInterval
                };
            
                status =  WDRV_WINC_MQTTConnect(wdrvHandle, mqttConfig->cleanSession,
                            mqttConfig->keepAliveTime, mqttConfig->protoVer,
                            &mqttConnProp, SYS_WINCS_MQTT_ConnCallback, 0);
            }
			return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
		
        /**<Trigger Disconnect from MQTT Broker*/        
        case SYS_WINCS_MQTT_DISCONNECT:
	    {	
            status = WDRV_WINC_MQTTDisconnect(wdrvHandle, WDRV_WINC_MQTT_DISCONN_REASON_CODE_NORMAL);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
	

        /**<Subscribe to Topics */
        case SYS_WINCS_MQTT_SUBS_TOPIC:
        {
            SYS_WINCS_MQTT_FRAME_t *mqttFrame = (SYS_WINCS_MQTT_FRAME_t *)mqttHandle;
            
            if(mqttFrame->protoVer == SYS_WINCS_MQTT_PROTO_VER_3)
            {
                status =  WDRV_WINC_MQTTSubscribe(wdrvHandle, mqttFrame->qos,
                        mqttFrame->topic ,        NULL,
                        SYS_WINCS_MQTT_SubscribeCallback, 0);
            }
            else if(mqttFrame->protoVer == SYS_WINCS_MQTT_PROTO_VER_5)
            {
                WDRV_WINC_MQTT_SUB_PROP *mqtt_sub_prop =
                    (WDRV_WINC_MQTT_SUB_PROP *)&mqttFrame->subscribeProp;
                
                status =  WDRV_WINC_MQTTSubscribe(wdrvHandle, mqttFrame->qos,
                        mqttFrame->topic ,        mqtt_sub_prop,
                        SYS_WINCS_MQTT_SubscribeCallback, 0);
            }
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
	
        /* Un-subscribe from a topic */
        case SYS_WINCS_MQTT_UNSUBSCRIBE:
        {
            char *topicName = (char *)mqttHandle;
            status =  WDRV_WINC_MQTTUnsubscribe(wdrvHandle, topicName);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
        /**<Configure the MQTT Application Callback*/ 
        case SYS_WINCS_MQTT_SET_CALLBACK:
        {
            g_MqttCallBackHandler[1] = (SYS_WINCS_MQTT_CALLBACK_t)(mqttHandle);
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__); 
        }

        /**<Configure the MQTT Application Callback*/     
        case SYS_WINCS_MQTT_SET_SRVC_CALLBACK:        
        {
            g_MqttCallBackHandler[0] = (SYS_WINCS_MQTT_CALLBACK_t)(mqttHandle);   
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__); 
        }
        
        case SYS_WINCS_MQTT_GET_CALLBACK:
        {
            SYS_WINCS_MQTT_CALLBACK_t *mqttCallBackHandler;
            mqttCallBackHandler = (SYS_WINCS_MQTT_CALLBACK_t *)mqttHandle;
            
            mqttCallBackHandler[0] = g_MqttCallBackHandler[0];
            mqttCallBackHandler[1] = g_MqttCallBackHandler[1];
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__); 
        }
            
        default:
        {
            SYS_WINCS_MQTT_DBG_MSG("ERROR : Unknown MQTT Service Request\r\n");
        }   
    }
    
    return SYS_WINCS_FAIL;
}


/* *****************************************************************************
 End of File
 */

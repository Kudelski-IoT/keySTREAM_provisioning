/*******************************************************************************
  WINCS Host Assisted MQTT Service Header file 

  File Name:
    sys_wincs_mqtt_service.h

  Summary:
    Header file for the WINCS Host Assisted MQTT Service implementation.

  Description:
    This file contains the header file for the WINCS Host Assisted MQTT Service
    implementation.
 *******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (C) 2020 released Microchip Technology Inc.  All rights reserved.


 * Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */


// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef SYS_WINCS_MQTT_SERVICE_H
#define	SYS_WINCS_MQTT_SERVICE_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include "system/wifi/sys_wincs_wifi_service.h"
#include "system/net/sys_wincs_net_service.h"
#include "driver/wifi/wincs02/include/wdrv_winc_client_api.h"


/* Handle for MQTT Configurations */
typedef void * SYS_WINCS_MQTT_HANDLE_t;

/* MQTT Service Debug print*/
#define SYS_WINCS_MQTT_DBG_MSG(args, ...)      SYS_CONSOLE_PRINT("[MQTT]:"args, ##__VA_ARGS__)


/*MQTT Service max callback service*/
#define SYS_WINCS_MQTT_SERVICE_CB_MAX                   2

#define SYS_WINCS_MQTT_PUB_MAX_CONTENT_TYPE_LEN         32
#define SYS_WINCS_MQTT_CONN_MAX_ASSIGNED_CLIENT_ID      48


/* *****************************************************************************
 * Enumeration of MQTT network and socket services.
 *
 * Summary:
 *    Enumeration of MQTT network and socket services.
 *
 * Description:
 *    This enumeration defines various services related to MQTT network and socket
 *    operations, such as MQTT configuration, connection management, subscription,
 *    publication, and callback registration.
 *
 * Remarks:
 *    None.
****************************************************************************** */
typedef enum 
{
    /**<Configure the MQTT Broker parameters*/
    SYS_WINCS_MQTT_CONFIG,


    /**<Connect to the MQTT Broker */        
    SYS_WINCS_MQTT_CONNECT,              
            
    /**<Request reconnect to the MQTT Cloud*/        
    SYS_WINCS_MQTT_RECONNECT,        
            
    /**<Trigger Disconnect from MQTT Broker*/        
    SYS_WINCS_MQTT_DISCONNECT, 

    /**<Subscribe to QoS0 Topics */        
    SYS_WINCS_MQTT_SUBS_TOPIC,  
                         
    /**<UNSubscribe to Topic */        
    SYS_WINCS_MQTT_UNSUBSCRIBE, 

    /**<Configure the MQTT Application Callback*/            
    SYS_WINCS_MQTT_SET_CALLBACK,                
            
    /**<Configure the MQTT Application Callback*/
    SYS_WINCS_MQTT_SET_SRVC_CALLBACK,   
            
    /*< Get Callback Function data*/
    SYS_WINCS_MQTT_GET_CALLBACK,
            
}SYS_WINCS_MQTT_SERVICE_t;


/* *****************************************************************************
 * MQTT Event List
 *
 * Summary:
 *    Enumeration of MQTT events.
 *
 * Description:
 *    This enumeration defines various events related to MQTT operations, such as
 *    connection and disconnection events, message receipt, acknowledgments, and errors.
 *
 * Remarks:
 *    None.
****************************************************************************** */
typedef enum
{
    /**<Connected to MQTT broker event */
    SYS_WINCS_MQTT_CONNECTED,    
            
    /**<Disconnected from MQTT broker event*/   
    SYS_WINCS_MQTT_DISCONNECTED, 
            
    /**<Event to report received MQTT message*/   
    SYS_WINCS_MQTT_SUBCRIBE_MSG,  
            
    /*Subscribe MQTT ACK*/        
    SYS_WINCS_MQTT_SUBCRIBE_ACK,
            
    /*MQTT Publish ACK*/
    SYS_WINCS_MQTT_PUBLISH_ACK,  
       
    /*MQTT Publish acknowledgement and completion received. */
    SYS_WINCS_MQTT_PUBLISH_MSG_ACK,
            
    /*MQTT A topic has been un-subscribed.*/
    SYS_WINCS_MQTT_UNSUBSCRIBED,        
            
    /*MQTT DPS Status*/
    SYS_WINCS_MQTT_DPS_STATUS,
            
    /*MQTT ERROR*/
    SYS_WINCS_MQTT_ERROR,
	   
}SYS_WINCS_MQTT_EVENT_t;


// *****************************************************************************
/**
 * @brief MQTT Quality of Service (QoS) Levels
 *
 * Summary:
 *    Enumeration of MQTT Quality of Service levels.
 *
 * Description:
 *    This enumeration defines the different levels of Quality of Service (QoS) for MQTT
 *    message delivery, ranging from best effort delivery to guaranteed delivery without duplicates.
 *
 * Remarks:
 *    None.
 ***************************************************************************** */
typedef enum
{
    /**< No-Ack, Best effort delivery (No Guarantee) */          
    SYS_WINCS_MQTT_QOS0 = WDRV_WINC_MQTT_QOS_0,      

    /**< Pub-Ack, sent until PUBACK from broker (possible duplicates) */
    SYS_WINCS_MQTT_QOS1 = WDRV_WINC_MQTT_QOS_1,      

    /**< Highest service, no duplicate with guarantee */            
    SYS_WINCS_MQTT_QOS2 = WDRV_WINC_MQTT_QOS_2,   

} SYS_WINCS_MQTT_QOS_t;

/* *****************************************************************************
 * @brief MQTT Payload Format Indicator
 *
 * Summary:
 *    Enumeration of MQTT payload format indicators.
 *
 * Description:
 *    This enumeration defines the format of the MQTT payload, indicating whether the payload
 *    is an unspecified byte stream or a UTF-8 encoded string.
 *
 * Remarks:
 *    None.
****************************************************************************** */
typedef enum
{
    /**< Unspecified byte stream */          
    SYS_WINCS_MQTT_UNSPECIFIED,

    /**< UTF-8 encoded payload */
    SYS_WINCS_MQTT_UTF8,

} SYS_WINCS_MQTT_PAYLOAD_FORMAT_INDI;


// *****************************************************************************
/* MQTT Protocol Version

  Summary:
    MQTT protocol versions.

  Description:
    Possible MQTT protocol versions supported.

  Remarks:
    None.
****************************************************************************** */

typedef enum
{
    /* MQTT protocol version 3.1.1. */
    SYS_WINCS_MQTT_PROTO_VER_3       = 3,

    /* MQTT protocol version 5. */
    SYS_WINCS_MQTT_PROTO_VER_5       = 5

} SYS_WINCS_MQTT_PROTO_VER;



// *****************************************************************************
/**
 @brief Network and Socket service List

 */
/**
 * @brief MQTT Configuration Structure
 *
 * Summary:
 *    Defines the structure for MQTT configuration settings.
 *
 * Description:
 *    This structure contains various configuration settings required to establish
 *    and maintain an MQTT connection, including broker URL, client credentials,
 *    port, TLS settings, protocol version, and other MQTT-specific parameters.
 */
typedef struct 
{    
    /**< MQTT Broker/Server URL */    
    const char                     *url;          

    /**< MQTT Service client ID */
    const char                     *clientId;

    /**< MQTT User Name Credential */
    const char                     *username;       

    /**< MQTT Password Credential */ 
    const char                     *password;       

    /**< MQTT Broker/Server Port */
    uint16_t                        port;          

    /**< MQTT TLS Enable */
    bool                            tlsIdx;

    /**< Azure DPS */
    uint8_t                         azureDps;

    /**< TLS Configuration */
    SYS_WINCS_NET_TLS_SOC_PARAMS   *tlsConf; 

    /**< Protocol version */
    SYS_WINCS_MQTT_PROTO_VER        protoVer;

    /**< MQTT Clean session flag */
    bool                            cleanSession;

    /**< MQTT keep alive time */
    uint16_t                        keepAliveTime;

    /**< MQTT connection session expiry interval */
    uint16_t                        sessionExpiryInterval;

    /**< TLS handle */
    uint8_t                         tlsHandle;

} SYS_WINCS_MQTT_CFG_t;


// *****************************************************************************
/* MQTT Subscribe Properties

  Summary:
    MQTT subscription properties.

  Description:
    Properties to be applied to MQTT subscription requests.

  Remarks:
    None.
*/

typedef struct
{
    /* Subscription Identifier. */
    uint32_t                        subscriptionIdentifer;

} SYS_WINCS_MQTT_SUB_PROP;

// *****************************************************************************
/**
 * @brief MQTT Frame Format
 *
 * Summary:
 *    Defines the structure for an MQTT frame.
 *
 * Description:
 *    This structure contains the properties and metadata associated with an MQTT frame,
 *    including flags, QoS level, topic, message content, protocol version, subscription
 *    properties, and publish properties.
 */

typedef struct
{
    /**< Indicates if the message is new or a duplicate */
    bool                            isDuplicate;          

    /**< QoS type for the message */
    SYS_WINCS_MQTT_QOS_t            qos;         

    /**< Retain flag for the publish message */
    bool                            retain;    

    /**< Publish topic for the message */
    char                           *topic;           

    /**< Content of the message */
    const char                     *message;

    /**< Protocol version */
    SYS_WINCS_MQTT_PROTO_VER        protoVer;

    /**< Subscription Identifier */
    SYS_WINCS_MQTT_SUB_PROP         subscribeProp;


} SYS_WINCS_MQTT_FRAME_t;


// *****************************************************************************
/* MQTT Connection Acknowledge Properties

  Summary:
    MQTT connection acknowledge properties.

  Description:
    Properties associated with an MQTT connection acknowledgement.

  Remarks:
    None.
*/

typedef struct
{
    /* Session expiry interval. */
    uint32_t                        sessionExpiryInterval;

    /* Maximum packet size. */
    uint32_t                        maxPacketSize;

    /* Receive maximum. */
    uint16_t                        receiveMax;

    /* Topic alias max. */
    uint16_t                        topicAliasMax;

    /* Flag indicating retain available property. */
    bool                            retainAvailable;

    /* Flag indicating if wildcard subscriptions are supported. */
    bool                            wildcardSubsAvailable;

    /* Flag indicating if subscription IDs are available. */
    bool                            subIDsAvailable;

    /* Flag indicating if shared subscriptions are supported. */
    bool                            sharedSubsAvailable;

    /* Max QoS settings. */
    uint8_t                         maxQoS;

    /* Assigned client ID. */
    uint8_t                         assignedClientId[SYS_WINCS_MQTT_CONN_MAX_ASSIGNED_CLIENT_ID+1];

} SYS_WINC_MQTT_CONN_ACK_PROP;



// *****************************************************************************
/**
 * @brief MQTT Callback Function Definition
 *
 * Summary:
 *    Defines the prototype for the MQTT callback function.
 *
 * Description:
 *    This typedef defines the prototype for the callback function that will be
 *    called to handle MQTT events. The callback function takes an MQTT event
 *    and an MQTT handle as parameters and returns a result of type SYS_WINCS_RESULT_t.
 *
 * Parameters:
 *    @param event        The MQTT event that triggered the callback.
 *    @param mqttHandle   The handle to the MQTT instance.
 *
 * Returns:
 *    SYS_WINCS_RESULT_t  The result of handling the MQTT event.
 */
typedef SYS_WINCS_RESULT_t (*SYS_WINCS_MQTT_CALLBACK_t)(SYS_WINCS_MQTT_EVENT_t event, SYS_WINCS_MQTT_HANDLE_t mqttHandle);


// *****************************************************************************
/**
 * @brief Control MQTT Service
 *
 * Summary:
 *    This function controls various MQTT service operations.
 *
 * Description:
 *    This function is used to send control requests to the MQTT service.
 *    It takes a request type and an MQTT handle as parameters.
 *
 * Parameters:
 *    @param request      The type of MQTT service request.
 *    @param mqttHandle   The handle to the MQTT instance.
 *
 * Returns:
 *    SYS_WINCS_RESULT_t  The result of the MQTT service control operation.
 */
SYS_WINCS_RESULT_t SYS_WINCS_MQTT_SrvCtrl
( 
    SYS_WINCS_MQTT_SERVICE_t   request,      /**< The type of MQTT service request. */
    SYS_WINCS_MQTT_HANDLE_t    mqttHandle    /**< The handle to the MQTT instance. */
);


#endif	/* XC_HEADER_TEMPLATE_H */


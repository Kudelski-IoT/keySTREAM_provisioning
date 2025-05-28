/*******************************************************************************
  WINCS Host Assisted System service Header file

  File Name:
    sys_wincs_system_service.h

  Summary:
    Header file for the WINCS Host Assisted System service implementation.

  Description:
    This header file provides a simple APIs to enable System service with WINCS device 
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
#ifndef SYS_WINCS_SYSTEM_SERVICE_H
#define	SYS_WINCS_SYSTEM_SERVICE_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <xc.h>
#include "system/wifi/sys_wincs_wifi_service.h" 

/* Handle for System service Configurations */
typedef void * SYS_WINCS_SYSTEM_HANDLE_t;

#define SYS_WINCS_SYS_DBG_MSG(args, ...)      SYS_CONSOLE_PRINT("[SYS]: "args, ##__VA_ARGS__)


// *****************************************************************************
// SYS_WINCS_SYSTEM_SERVICE_t
//
// Summary:
//    Enumeration of control requests for the WINC system service.
//
// Description:
//    This enumeration defines the various control requests that can be made
//    to the WINC system service. Each enumerator represents a specific request
//    or action to be performed by the system service.
//
// Remarks:
//    None.
// *****************************************************************************

typedef enum
{
    /**<Request/Trigger reset the system */
    SYS_WINCS_SYSTEM_RESET,             
            
    /**<Request Software Revision */
    SYS_WINCS_SYSTEM_SW_REV,            
        
    /**<Request Software Revision */            
    SYS_WINCS_SYSTEM_DEV_INFO,  


    /**<Request Driver version */          
    SYS_WINCS_SYSTEM_DRIVER_VER,
            
    /**<Set Driver system event callback */          
    SYS_WINCS_SYSTEM_SET_SYS_EVENT_CALLBACK, 
          
    /* Debug UART Set*/
    SYS_WINCS_SYSTEM_DEBUG_UART_SET,
    
    /* Debug UART Register callback*/
    SYS_WINCS_SYSTEM_SET_DEBUG_REG_CLBK,
            
}SYS_WINCS_SYSTEM_SERVICE_t;

// *****************************************************************************
// SYS_WINCS_SYSTEM_getTaskStatus
//
// Summary:
//    Retrieves the current status of the WINC system task.
//
// Description:
//    This function returns the current status of the WINC system task. It can
//    be used to check if the task is running or if it has encountered any issues.
//
// Parameters:
//    None.
//
// Returns:
//    bool - True if the task is running, false otherwise.
//
// Remarks:
//    None.
// *****************************************************************************
bool SYS_WINCS_SYSTEM_getTaskStatus(void);

// *****************************************************************************
// SYS_WINCS_SYSTEM_SrvCtrl
//
// Summary:
//    Controls the WINC system service based on the specified request.
//
// Description:
//    This function handles various control requests for the WINC system service.
//    It processes the request and performs the necessary actions based on the
//    type of request and the provided system handle.
//
// Parameters:
//    request      - The control request for the WINC system service
//    systemHandle - Handle to the WINC system
//
// Returns:
//    SYS_WINCS_RESULT_t - Result of the control request
//
// Remarks:
//    None.
// *****************************************************************************
SYS_WINCS_RESULT_t SYS_WINCS_SYSTEM_SrvCtrl(SYS_WINCS_SYSTEM_SERVICE_t request, SYS_WINCS_SYSTEM_HANDLE_t systemHandle);


#endif	/* SYS_WINCS_SYSTEM_SERVICE_H */

/** @}*/

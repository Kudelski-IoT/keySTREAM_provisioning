/*******************************************************************************
  WINCS Host Assisted System Service Implementation

  File Name:
    sys_wincs_system_service.c

  Summary:
    Source code for the WINCS Host Assisted System Service implementation.

  Description:
    This file contains the source code for the WINCS Host Assisted System Service
    implementation.
 *******************************************************************************/

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
#include <stdbool.h>


/* This section lists the other files that are included in this file.
 */
#include "wdrv_winc.h"
#include "wdrv_winc_common.h"
#include "system/sys_wincs_system_service.h"


// Variable to track the completion status of the WINC system task
static bool wincsSysTaskCompleted = false;

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

// Function to get the current status of the system task
bool SYS_WINCS_SYSTEM_getTaskStatus()
{
    return wincsSysTaskCompleted;
}

// Function to set the status of the system task
bool SYS_WINCS_SYSTEM_setTaskStatus(bool taskStatus)
{
    wincsSysTaskCompleted = taskStatus;
    return true;
}

// *****************************************************************************
// SYS_WINCS_SYSTEM_EventCallback
//
// Summary:
//    Callback function to handle system events in the WINC system.
//
// Description:
//    This function is called when a system event occurs. It processes the
//    event based on its type and performs the necessary actions.
//
// Parameters:
//    object - System module object
//    event  - Type of the system event
//
// Returns:
//    None.
//
// Remarks:
//    None.
// *****************************************************************************


static void SYS_WINCS_SYSTEM_EventCallback
(
    SYS_MODULE_OBJ object, 
    WDRV_WINC_SYSTEM_EVENT_TYPE event
)
{
    SYS_WINCS_SYS_DBG_MSG("System Event: ");

    switch (event)
    {
        case WDRV_WINC_SYSTEM_EVENT_DEVICE_HARD_RESET:
        {
            SYS_WINCS_SYS_DBG_MSG("Hard Reset\r\n");
            break;
        }

        case WDRV_WINC_SYSTEM_EVENT_DEVICE_INIT_BEGIN:
        {
            SYS_WINCS_SYS_DBG_MSG("Init Begin\r\n");
            break;
        }

        case WDRV_WINC_SYSTEM_EVENT_DEVICE_INIT_COMPLETE:
        {
            SYS_WINCS_SYS_DBG_MSG("Init Complete\r\n");
            break;
        }

        case WDRV_WINC_SYSTEM_EVENT_DEVICE_READY:
        {
            SYS_WINCS_SYS_DBG_MSG("Device Ready\r\n");
            break;
        }

        case WDRV_WINC_SYSTEM_EVENT_DEVICE_RESET_FAILED:
        {
            SYS_WINCS_SYS_DBG_MSG("Reset Failed\r\n");
            break;
        }

        case WDRV_WINC_SYSTEM_EVENT_DEVICE_RESET_TIMEOUT:
        {
            SYS_WINCS_SYS_DBG_MSG("Reset Timeout\r\n");
            break;
        }

        case WDRV_WINC_SYSTEM_EVENT_DEVICE_RESET_RETRY:
        {
            SYS_WINCS_SYS_DBG_MSG("Reset Retry\r\n");
            break;
        }

        case WDRV_WINC_SYSTEM_EVENT_DEVICE_COMMS_ERROR:
        {
            SYS_WINCS_SYS_DBG_MSG("Comms Error\r\n");
            break;
        }

        default:
        {
            SYS_WINCS_SYS_DBG_MSG("Unknown System event%d\r\n", event);
            break;
        }
    }
}



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
SYS_WINCS_RESULT_t SYS_WINCS_SYSTEM_SrvCtrl
(
    SYS_WINCS_SYSTEM_SERVICE_t request,
    SYS_WINCS_SYSTEM_HANDLE_t systemHandle
)
{
    WDRV_WINC_STATUS status = WDRV_WINC_STATUS_OK;
    DRV_HANDLE wdrvHandle = DRV_HANDLE_INVALID;
    
    SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_GET_DRV_HANDLE, &wdrvHandle);
    
    switch(request)
    {
        /* WINCS Reset */
        case SYS_WINCS_SYSTEM_RESET:
        {
            WDRV_WINC_RESETN_Set();
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__); 
        }
        
        /* WINCS Software Revision */
        case SYS_WINCS_SYSTEM_SW_REV:
        {
            *(uint8_t*)systemHandle = '\0';
            status = WDRV_WINC_InfoDeviceFirmwareVersionGet(wdrvHandle, true,(WDRV_WINC_FIRMWARE_VERSION_INFO *) systemHandle);
            
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
        
        /* WINCS Device Info  */
        case SYS_WINCS_SYSTEM_DEV_INFO:
        {
            *(uint8_t*)systemHandle = '\0';
            status = WDRV_WINC_InfoDeviceGet(wdrvHandle, (WDRV_WINC_DEVICE_INFO *)systemHandle);
            
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
        
        /* WINCS Driver version  */
        case SYS_WINCS_SYSTEM_DRIVER_VER:
        {
            *(uint8_t*)systemHandle = '\0';
            status = WDRV_WINC_InfoDriverVersionGet(wdrvHandle, (WDRV_WINC_DRIVER_VERSION_INFO *)systemHandle);
            
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
    

		//Registers a system event callback to be notified of system events.
        case SYS_WINCS_SYSTEM_SET_SYS_EVENT_CALLBACK:
        {
            WDRV_WINC_RegisterSystemEventCallback(sysObj.drvWifiWinc, SYS_WINCS_SYSTEM_EventCallback);
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__); 
        }
        
        //Set the debug printf function pointer.
        case SYS_WINCS_SYSTEM_SET_DEBUG_REG_CLBK:
        {
            WDRV_WINC_DebugRegisterCallback((WDRV_WINC_DEBUG_PRINT_CALLBACK)systemHandle);
            WINC_DevSetDebugPrintf((WINC_DEBUG_PRINTF_FP)systemHandle);
            return SYS_WINCS_WIFI_GetWincsStatus(WDRV_WINC_STATUS_OK, __FUNCTION__, __LINE__); 
        }
        
        //Configures the debug UART on the WINC device.
        case SYS_WINCS_SYSTEM_DEBUG_UART_SET:
        {
            status = WDRV_WINC_DebugUARTSet(wdrvHandle, WDRV_WINC_DEBUG_UART_2, 0);
            return SYS_WINCS_WIFI_GetWincsStatus(status, __FUNCTION__, __LINE__);
        }
        
        default:
        {
            SYS_WINCS_SYS_DBG_MSG("ERROR : Unknown System Service Request\r\n");
        }
    }
    
    return SYS_WINCS_FAIL;
}

/* *****************************************************************************
 End of File
 */

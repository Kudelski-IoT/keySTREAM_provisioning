/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes
#include "cryptoauthlib.h"
#include "ktaFieldMgntHook.h"
#include "k_kta.h"

extern ATCAIfaceCfg atecc608_0_init_data;
ATCAIfaceCfg* device_cfg = &atecc608_0_init_data;
TKktaKeyStreamStatus ktaKSCmdStatus = E_K_KTA_KS_STATUS_NO_OPERATION;
uint8_t sernum[9];
char displayStr[30];
size_t displen = sizeof(displayStr);
// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main ( void )
{
    ATCA_STATUS status;
    /* Initialize all modules */
    SYS_Initialize ( NULL );

    printf("\r\n");
    printf("Starting keySTREAM late provisioning example\r\n");
    
    atecc608_0_init_data.atcai2c.address = 0x70;
    status = atcab_init(device_cfg);
    printf("atcab_init: %d\r\n", status);
    if (ATCA_SUCCESS == status)
    {
        TKStatus ktastatus = E_K_STATUS_ERROR;
        ktastatus = ktaKeyStreamInit();
        if (ktastatus != E_K_STATUS_OK)
        {
            printf("ktaKeyStreamInit failed");
            return ATCA_STATUS_UNKNOWN;
        }
    
        ktastatus = ktaKeyStreamFieldMgmt(true , &ktaKSCmdStatus);
        if (ktastatus != E_K_STATUS_OK)
        {
            printf("ktaKeyStreamFieldMgmt failed");
            return ATCA_STATUS_UNKNOWN;
        }
        status = atcab_read_serial_number(sernum);
        atcab_bin2hex(sernum, 9, displayStr, &displen);
        printf("Serial Number of the Device: %s\r\n\n", displayStr);
    }
    
    atcab_release();
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/


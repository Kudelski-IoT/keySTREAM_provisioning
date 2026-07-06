/*******************************************************************************
  MPLAB Harmony Application Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app.h

  Summary:
    This header file provides prototypes and definitions for the application.

  Description:
    This header file provides function prototypes and data type definitions for
    the application.  Some of these are required by the system (such as the
    "APP_Initialize" and "APP_Tasks" prototypes) and some of them are only used
    internally by the application (such as the "APP_STATES" definition).  Both
    are defined here for convenience.
*******************************************************************************/

#ifndef _APP_H
#define _APP_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "configuration.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************
#define BOOT_COMPONENT_NAME      "FOTA-BOOT"
#define BOOT_COMPONENT_VERSION   "1.2.0"

#define TERM_RED        "\x1B[31m"
#define TERM_GREEN      "\x1B[32m"
#define TERM_YELLOW     "\x1B[33m"
#define TERM_CYAN       "\x1B[36m"
#define TERM_RESET      "\x1B[0m"

// *****************************************************************************
/* Application states

  Summary:
    Application states enumeration

  Description:
    This enumeration defines the valid application states.  These states
    determine the behavior of the application at various times.
*/

typedef enum
{
    /* Application's state machine's initial state. */
    APP_STATE_INIT=0,
    APP_STATE_READ_SER_NUM,
    APP_STATE_CHECK_LIFECYCLE,
    APP_STATE_CALCULATE_APP_DIGEST,
    APP_STATE_VERIFY_APP_SIGNATURE,
    APP_STATE_JUMP_TO_APPLICATION,
    APP_STATE_BANK_SWAP_ON_FAIL,
    APP_STATE_FINISH,
    /* TODO: Define states used by the application state machine. */

} APP_STATES;

/*
 * Life cycle state location in the ATECC608 (mirrors the KTA configuration
 * defined in slotConfig.h / k_sal_storage.c, kept local here so the
 * bootloader does not depend on KTA / k_sal_storage sources).
 *   - Slot   : 0x08 (C_KTA__STORAGE_SLOT_LIFE_CYCLE_STATE)
 *   - Block  : 0    (C_KTA__SLOT_BLOCK_0)
 *   - Offset : 0    (C_KTA__BLOCK_OFFSET_0)
 *   - Length : 4 bytes (C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE)
 */
#define LIFECYCLE_STATE_SLOT        (0x08u)
#define LIFECYCLE_STATE_BLOCK       (0u)
#define LIFECYCLE_STATE_OFFSET      (0u)
#define LIFECYCLE_STATE_SIZE        (4u)

typedef enum
{
    BL_LIFE_CYCLE_STATE_INIT        = 0,
    BL_LIFE_CYCLE_STATE_SEALED      = 1,
    BL_LIFE_CYCLE_STATE_ACTIVATED   = 2,
    BL_LIFE_CYCLE_STATE_PROVISIONED = 3,
    BL_LIFE_CYCLE_STATE_CON_REQ     = 4,
    BL_LIFE_CYCLE_STATE_INVALID     = 0xFF
} BL_LifeCycleState;


// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    Application strings and buffers are be defined outside this structure.
 */

typedef struct
{
    /* The application's current state */
    APP_STATES state;

    /* TODO: Define any additional data used by the application. */

} APP_DATA;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Routines
// *****************************************************************************
// *****************************************************************************
/* These routines are called by drivers when certain events occur.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Summary:
     MPLAB Harmony application initialization routine.

  Description:
    This function initializes the Harmony application.  It places the
    application in its initial state and prepares it to run so that its
    APP_Tasks function can be called.

  Precondition:
    All other system initialization routines should be called before calling
    this routine (in "SYS_Initialize").

  Parameters:
    None.

  Returns:
    None.

  Example:
    <code>
    APP_Initialize();
    </code>

  Remarks:
    This routine must be called from the SYS_Initialize function.
*/

void APP_Initialize ( void );


/*******************************************************************************
  Function:
    void APP_Tasks ( void )

  Summary:
    MPLAB Harmony Demo application tasks function

  Description:
    This routine is the Harmony Demo application's tasks function.  It
    defines the application's state machine and core logic.

  Precondition:
    The system and application initialization ("SYS_Initialize") should be
    called before calling this.

  Parameters:
    None.

  Returns:
    None.

  Example:
    <code>
    APP_Tasks();
    </code>

  Remarks:
    This routine must be called from SYS_Tasks() routine.
 */

void APP_Tasks( void );

//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

/*******************************************************************************
  Function:
    bool VerifySignature ( uint32_t signature_address )

  Summary:
    MPLAB Harmony Demo application tasks function

  Description:
    This routine is the Harmony Demo application's tasks function.  It
    defines the application's state machine and core logic.

  Precondition:
    The system and application initialization ("SYS_Initialize") should be
    called before calling this.

  Parameters:
    None.

  Returns:
    None.

  Example:
    <code>
    APP_Tasks();
    </code>

  Remarks:
    This routine must be called from SYS_Tasks() routine.
 */

bool VerifySignature ( uint32_t signature_address );


#endif /* _APP_H */

/*******************************************************************************
 End of File
 */

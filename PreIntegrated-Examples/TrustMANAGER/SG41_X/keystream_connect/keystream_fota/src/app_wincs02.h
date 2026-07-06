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
#include "k_sal_fota.h"
#include "metadata.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility
extern "C" {
#endif
// DOM-IGNORE-END

#define APP_COMPONENT_NAME      "MCHP-CAL"
#define APP_COMPONENT_VERSION   "1.2.0"

#define TERM_RED                "\x1B[31m"
#define TERM_GREEN              "\x1B[32m"
#define TERM_YELLOW             "\x1B[33m"
#define TERM_BLUE               "\x1B[34m"
#define TERM_MAGENTA            "\x1B[35m"
#define TERM_CYAN               "\x1B[36m"
#define TERM_WHITE              "\x1B[47m"

// Bright colors
#define TERM_RED_B              "\x1B[91m"
#define TERM_GREEN_B            "\x1B[92m"
#define TERM_YELLOW_B           "\x1B[93m"
#define TERM_BLUE_B             "\x1B[94m"
#define TERM_MAGENTA_B          "\x1B[95m"
#define TERM_CYAN_B             "\x1B[96m"
#define TERM_WHITE_B            "\x1B[97m"

#define TERM_RESET              "\x1B[0m"
#define TERM_BOLD               "\x1B[1m"
#define TERM_UL                 "\x1B[4m"

#define APP_BUILD_HASH_SZ       5
#define APP_DI_IMAGE_INFO_NUM   2


// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

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
    // State to wait for USB enumeration
    WINC_APP_STATE_WAIT_FOR_USB,

    // State to print Message
    WINC_APP_STATE_PRINT_APP_BANNER,

    // State to initialize CryptoAuthLib
    WINC_APP_STATE_CRYPTO_INIT,

    // WINC States
    WINC_APP_STATE_WINC_INIT,
    WINC_APP_STATE_OPEN_DRIVER,
    WINC_APP_STATE_DEVICE_INFO,
    WINC_APP_STATE_SET_REG_DOMAIN,
    WINC_APP_STATE_SET_WIFI_PARAMS,
    WINC_APP_STATE_ERROR,

    //Wait state
    WINC_APP_STATE_IDLE
} WINC_APP_STATES;

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
    WINC_APP_STATES state;

    /* TODO: Define any additional data used by the application. */

} WINC_APP_DATA;



typedef struct
{
    /* Version number structure. */
    struct
    {
        /* Major version number. */
        uint16_t major;

        /* Major minor number. */
        uint16_t minor;

        /* Major patch number. */
        uint16_t patch;
    } version;
} APP_DRIVER_VERSION_INFO;

typedef struct
{
    /* Flag indicating if this information is valid. */
    bool isValid;

    /* Version number structure. */
    struct
    {
        /* Major version number. */
        uint16_t major;

        /* Major minor number. */
        uint16_t minor;

        /* Major patch number. */
        uint16_t patch;
    } version;

    /* Build date/time structure. */
    struct
    {
        uint8_t hash[APP_BUILD_HASH_SZ];

        uint32_t timeUTC;
    } build;
} APP_FIRMWARE_VERSION_INFO;


// *****************************************************************************
/*  Device Information

  Summary:
    Defines the device information.

  Description:
    This data type defines the device information of the WINC.

  Remarks:
    None.
*/

typedef struct
{
    /* Flag indicating if this information is valid. */
    bool isValid;

    /* WINC device ID. */
    uint32_t id;

    /* Number of flash images. */
    uint8_t numImages;

    /* Information for each device image. */
    struct
    {
        /* Sequence number. */
        uint32_t seqNum;

        /* Version information. */
        uint32_t version;

        /* Source address. */
        uint32_t srcAddr;
    } image[APP_DI_IMAGE_INFO_NUM];
} APP_DEVICE_INFO;


typedef enum {
    APP_FOTA_NONE = 0,
    APP_FOTA_START,
    APP_FOTA_INPROGRESS,
    APP_FOTA_SUCCESS
} FOTA_Status;


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

void APP_WINCS02_Initialize ( void );


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

void APP_WINCS02_Tasks( void );

bool is_winc_ready_for_app(void);
char* get_last_dns_ip(void);
void clear_last_dns_ip(void);
uint32_t get_app_tick_ms(void);


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _APP_H */

/*******************************************************************************
 End of File
 */

/*******************************************************************************
* Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries.
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

#ifndef _APP_DLDR_H
#define _APP_DLDR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "definitions.h"

typedef enum
{
    // FoTA States
    DLDR_APP_STATE_INIT,
    DLDR_APP_STATE_WAIT_FOR_DOWNLOAD,
    DLDR_APP_STATE_DNS_REQUEST,
    DLDR_APP_STATE_DNS_WAIT,
    DLDR_APP_STATE_DOWNLOAD_INIT,
    DLDR_APP_STATE_DOWNLOAD_ERASE,
    DLDR_APP_STATE_DOWNLOAD_INPROGRESS,
    DLDR_APP_STATE_DOWNLOAD_COMPLETE,
    DLDR_APP_STATE_DOWNLOAD_ERROR,
    DLDR_APP_STATE_IDLE,

} DLDR_APP_STATES;

typedef struct
{
    /* The application's current state */
    DLDR_APP_STATES state;

    /* TODO: Define any additional data used by the application. */

} DLDR_APP_DATA;


#define KEYSTREAM_AWS_S3_SOCK_SERVER_ADDR0      "s3.eu-west-1.amazonaws.com"

#define SYS_WINCS_HTTP_CONTENT_LEN              "Content-Length:"   /*TCP buffer HTTP Cnntent length */
#define DLDR_MAX_RETRY_CNT                      3                   /* Max retries for download */

// Global flags for busy-wait synchronization (for fotaDownloadAndInstall busy-wait)
extern volatile bool g_fota_download_complete;
extern volatile bool g_fota_update_complete;

/**
 * @brief Downloader application initialization.
 *
 */
void APP_DLDR_Initialize(void);

/**
 * @brief Executes the Downloader application tasks.
 *
 */
void APP_DLDR_Tasks(void);

#ifdef __cplusplus
}
#endif

#endif // _APP_TIMER_H

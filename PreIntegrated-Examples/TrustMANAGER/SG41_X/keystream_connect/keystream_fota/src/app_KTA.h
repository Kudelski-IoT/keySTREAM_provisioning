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

#ifndef _APP_KTA_H
#define _APP_KTA_H

#ifdef __cplusplus
extern "C" {
#endif

#define KTA_DNS_MAX_RETRY_CNT              3                   /* Max retries for KTA DNS */

typedef enum
{
    /* Application's state machine's initial state. */
    KTA_APP_STATE_IDLE,
    KTA_APP_STATE_WAIT_FOR_WINC_READY,
    KTA_APP_STATE_DNS_REQUEST,
    KTA_APP_STATE_DNS_WAIT,
    KTA_APP_STATE_INIT,
    KTA_APP_STATE_RUN,
    KTA_APP_STATE_MAX_STATES
    /* TODO: Define states used by the application state machine. */

} KTA_APP_STATES;

typedef struct
{
    /* The application's current state */
    KTA_APP_STATES state;
    /* TODO: Define any additional data used by the application. */

} KTA_APP_DATA;


/**
 * @brief KTA application initialization.
 *
 */
void APP_KTA_Initialize(void);

/**
 * @brief Executes the KTA application tasks.
 *
 */
void APP_KTA_Tasks(void);

/**
 * @brief Control KTA state to pause for Downloader
 *
 * @param[in] state Target state
 */
void APP_KTA_PauseForDLDR(void);

/**
 * @brief Control KTA state to resume the state machine
 *
 * @param[in] state Target state
 */
void APP_KTA_Resume(void);

#ifdef __cplusplus
}
#endif

#endif // _APP_KTA_H

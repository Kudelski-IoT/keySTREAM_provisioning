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

#include "app_kta.h"
#include <stdio.h>
#include "definitions.h"
#include "cryptoauthlib.h"
#include "ktaFieldMgntHook.h"
#include "ktaConfig.h"
#include "system/time/sys_time.h"
#include "system/wifi/sys_wincs_wifi_service.h"

#define APP_KTA_DEBUG_ENABLED
#ifdef APP_KTA_DEBUG_ENABLED
/** @brief Enable downloader debug logs. */
#define APP_KTA_DEBUG(fmt, ...)          printf("[APP_KTA]: " fmt "\r\n", ##__VA_ARGS__)
#define APP_KTA_DEBUG_ERROR(fmt, ...)    printf("\x1B[31m[APP_KTA <%d>]: " fmt "\r\n\x1B[0m", __LINE__, ##__VA_ARGS__)
#else
#define APP_KTA_DEBUG(fmt, ...)
#define APP_KTA_DEBUG_ERROR(fmt, ...)
#endif /* DEBUG. */


static KTA_APP_DATA kta_app_data;
char g_KTA_sock_ip_addr[25];

SYS_TIME_HANDLE kta_timer_handle;
bool is_kta_state_timedout;
KTA_APP_STATES kta_target_state;

static void kta_timer_callback(uintptr_t context);
static void set_kta_state_timeout(uint32_t timeout_ms, KTA_APP_STATES target_state);

void APP_KTA_Initialize(void)
{
    kta_app_data.state = KTA_APP_STATE_WAIT_FOR_WINC_READY;
}

void APP_KTA_Tasks(void)
{
    switch(kta_app_data.state)
    {
        case KTA_APP_STATE_WAIT_FOR_WINC_READY:
        {
            if (is_winc_ready_for_app()){
                APP_KTA_DEBUG("WINC is ready for KTA app");
                kta_app_data.state = KTA_APP_STATE_DNS_REQUEST;
            }
            break;
        }

        case KTA_APP_STATE_DNS_REQUEST:
        {
            APP_KTA_DEBUG("Set to resolve KTA endpoint address ");
            clear_last_dns_ip();
            if (SYS_WINCS_FAIL == SYS_WINCS_WIFI_SrvCtrl(SYS_WINCS_WIFI_DNS_RESOLVE,
                (void *)C_KTA_APP__KEYSTREAM_HOST_HTTP_URL))
            {
                APP_KTA_DEBUG_ERROR("KTA DNA resolve init/fail ");
                break;
            }
            set_kta_state_timeout(10000, KTA_APP_STATE_DNS_REQUEST);
            kta_app_data.state = KTA_APP_STATE_DNS_WAIT;
            break;
        }

        case KTA_APP_STATE_DNS_WAIT:
        {
            char *dns_ip = get_last_dns_ip();
            if (strlen(dns_ip)) {
                APP_KTA_DEBUG("KTA DNS Resolved, Copy IP to Keystream buffer");
                SYS_TIME_TimerStop(kta_timer_handle);
                memcpy(&g_KTA_sock_ip_addr[0], (char *)dns_ip, strlen((char *)dns_ip));
                kta_app_data.state = KTA_APP_STATE_INIT;
            } else if (is_kta_state_timedout) {
                is_kta_state_timedout = false;
                APP_KTA_DEBUG_ERROR("KTA DNS Resolve timed out... Retry");
                kta_app_data.state = kta_target_state;
                kta_target_state = KTA_APP_STATE_MAX_STATES;
            }
            break;
        }

        case KTA_APP_STATE_INIT:
        {
            if (!is_winc_ready_for_app()){
                break;
            }

            APP_KTA_DEBUG("Running KTA Keystream INIT");
            if (E_K_STATUS_OK != ktaKeyStreamInit())
            {
                APP_KTA_DEBUG_ERROR("KTA Keystream INIT failed");
                kta_app_data.state = KTA_APP_STATE_IDLE;
                break;
            }
            APP_KTA_DEBUG("KTA Keystream INIT - Successful");
            kta_app_data.state = KTA_APP_STATE_RUN;
            break;
        }

        case KTA_APP_STATE_RUN:
        {
            if (!is_winc_ready_for_app()){
                break;
            }

            /* Calling KTA keySTREAM field management. */
            TKktaKeyStreamStatus ktaKSCmdStatus;
            if (E_K_STATUS_OK != ktaKeyStreamFieldMgmt(true , &ktaKSCmdStatus))
            {
                APP_KTA_DEBUG_ERROR("ktaKeyStreamFieldMgmt failed");
            }

            kta_app_data.state = KTA_APP_STATE_IDLE;
            set_kta_state_timeout(5000, KTA_APP_STATE_RUN);

            if (ktaKSCmdStatus == E_K_KTA_KS_STATUS_RENEW || ktaKSCmdStatus == E_K_KTA_KS_STATUS_REFURBISH)
            {
                APP_KTA_DEBUG("KTA in RENEW/REFURBISH state");
                if (ktaKSCmdStatus == E_K_KTA_KS_STATUS_REFURBISH){
                    kta_app_data.state = KTA_APP_STATE_INIT;
                }
            }
            break;
        }

        case KTA_APP_STATE_IDLE:
        {
            // Idle state; Wait for an event to trigger next action
            if (is_kta_state_timedout) {
                is_kta_state_timedout = false;
                kta_app_data.state = kta_target_state;
                kta_target_state = KTA_APP_STATE_MAX_STATES;
            }
            break;
        }

        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

void APP_KTA_PauseForDLDR(void)
{
    is_kta_state_timedout = false;
    SYS_TIME_TimerStop(kta_timer_handle);
    kta_app_data.state = KTA_APP_STATE_IDLE;
}

void APP_KTA_Resume(void)
{
    kta_app_data.state = KTA_APP_STATE_INIT;
}

static void kta_timer_callback(uintptr_t context)
{
    // context is the state to retry on timeout
    kta_target_state = context;
    is_kta_state_timedout = true;
}

static void set_kta_state_timeout(uint32_t timeout_ms, KTA_APP_STATES target_state)
{
    is_kta_state_timedout = false;
    SYS_TIME_TimerStop(kta_timer_handle);
    kta_timer_handle = SYS_TIME_CallbackRegisterMS(kta_timer_callback,
        (uintptr_t)target_state, timeout_ms, SYS_TIME_SINGLE);
}

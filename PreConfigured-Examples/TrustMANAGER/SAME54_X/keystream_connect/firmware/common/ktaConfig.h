/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2024 Nagravision Sàrl

* Subject to your compliance with these terms, you may use the Nagravision Sàrl
* Software and any derivatives exclusively with Nagravision's products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may accompany
* Nagravision Software.

* Redistribution of this Nagravision Software in source or binary form is allowed
* and must include the above terms of use and the following disclaimer with the
* distribution and accompanying materials.

* THIS SOFTWARE IS SUPPLIED BY NAGRAVISION "AS IS". NO WARRANTIES, WHETHER EXPRESS,
* IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF
* NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE. IN NO
* EVENT WILL NAGRAVISION BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL
* OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO
* THE SOFTWARE, HOWEVER CAUSED, EVEN IF NAGRAVISION HAS BEEN ADVISED OF THE
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW,
* NAGRAVISION 'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS
* SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY
* TO NAGRAVISION FOR THIS SOFTWARE.
********************************************************************************/
/** \brief Configuration file for keySTREAM Trusted Agent.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file ktaConfig.h
 ******************************************************************************/

/**
 * @brief Configuration file for keySTREAM Trusted Agent.
 */

/** @addtogroup g_kta_hook
 * @{
 */
/** @defgroup g_kta_hook */
/** @} g_kta_hook */

#ifndef K_KTA_CONFIG_H
#define K_KTA_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "app.h"
#include "tmg_conf.h"

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/** @brief keySTREAM CIE CoAP Url. */
#define C_KTA_APP__KEYSTREAM_HOST_COAP_URL       KEYSTREAM_COAP_URL

/** @brief keySTREAM CIE Url. */
#define C_KTA_APP__KEYSTREAM_HOST_HTTP_URL       KEYSTREAM_HTTP_URL

/** @brief L1 Segmentation Seed of CIE. */
#define C_KTA_APP__L1_SEG_SEED_CIE          {0x2b, 0x2b, 0x42, 0x6e, 0x10, 0x35, 0xad, 0x6b,\
                                             0x73, 0xf0, 0x56, 0x1d, 0xc4, 0xe0, 0x54, 0x72}

/** @brief L1 Segmentation Seed. */
#define C_KTA_APP__L1_SEG_SEED              C_KTA_APP__L1_SEG_SEED_CIE

/** @brief Device profile public uid of keySTREAM. */
#define C_KTA_APP__DEVICE_PUBLIC_UID        KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID

/** @brief Application log */
/** 
 * SUPPRESS: MISRA_DEV_KTA_003 : misra_c2012_rule_21.6_violation
 * SUPPRESS: MISRA_DEV_KTA_001 : misra_c2012_rule_17.1_violation
 * Using printf for logging.
 * Not checking the return status of printf, since not required.
 **/
#ifdef __free_rtos__
#define C_KTA_APP__LOG                      printf
#else
#define C_KTA_APP__LOG                      APP_DebugPrintf
#endif

/**
 * @brief Enable FOTA services.
 * Define this macro to enable FOTA-Services.
 */
// #define FOTA_ENABLE

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_KTA_CONFIG_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */


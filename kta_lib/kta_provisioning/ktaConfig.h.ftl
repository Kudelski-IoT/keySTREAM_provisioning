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

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */
/** @brief keySTREAM PROD CoAP Url. */
#define C_KTA_APP__KEYSTREAM_HOST_COAP_URL       (const uint8_t*)"icpp.mss.iot.kudelski.com"

/** @brief keySTREAM PROD http Url. */
#define C_KTA_APP__KEYSTREAM_HOST_HTTP_URL       (const uint8_t*)"icph.mss.iot.kudelski.com"

/** @brief L1 Segmentation Seed of CIE. */
#define C_KTA_APP__L1_SEG_SEED_CIE          {0x2b, 0x2b, 0x42, 0x6e, 0x10, 0x35, 0xad, 0x6b,\
                                             0x73, 0xf0, 0x56, 0x1d, 0xc4, 0xe0, 0x54, 0x72}

/** @brief L1 Segmentation Seed. */
#define C_KTA_APP__L1_SEG_SEED              C_KTA_APP__L1_SEG_SEED_CIE

/** @brief Device profile public uid of keySTREAM. */
#define C_KTA_APP__DEVICE_PUBLIC_UID        ("${KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID}")

/** @brief Application log */
#define C_KTA_APP__LOG                      printf
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

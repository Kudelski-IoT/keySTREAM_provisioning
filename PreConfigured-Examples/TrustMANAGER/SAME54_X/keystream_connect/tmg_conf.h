/******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")***********************
* (c) 2023-2024 Nagravision Sarl

* Subject to your compliance with these terms, you may use the Nagravision Sarl
* Software and any derivatives exclusively with Nagravision�s products. It is your
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
* NAGRAVISION S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS
* SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY
* TO NAGRAVISION FOR THIS SOFTWARE. 
******************************************************************************/
/** \brief  Configuration file for environment setup.
*
*  \author Kudelski IoT
*
*  \date 2023/06/12
*
*  \file tmg_conf.h
******************************************************************************/

/**
 * @brief Configuration file for environment setup.
 */

#ifndef TMG_CONF_H
#define TMG_CONF_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* This header file generated from one of the TPDS steps */
/* Please do NOT make any changes to this file */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/** @brief Wifi SSID for TrustManaged device */
#define WIFI_SSID                           ""

/** @brief Wifi password for TrustManaged device */
#define WIFI_PWD                            ""

/** @brief TrustManaged Device Public UID */
#define KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID ""

/** @brief keySTREAM COAP Endpoint */
#define KEYSTREAM_COAP_URL                  (const uint8_t*)"icpp.mss.iot.kudelski.com"

/** @brief keySTREAM HTTP Endpoint */
#define KEYSTREAM_HTTP_URL                  (const uint8_t*)"icph.mss.iot.kudelski.com"

/** @brief AWS Endpoint */
#define AWS_ENDPOINT                        "a3k3bohxtu19qi-ats.iot.us-east-2.amazonaws.com"

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif /* C++ */

#endif // TMG_CONF_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

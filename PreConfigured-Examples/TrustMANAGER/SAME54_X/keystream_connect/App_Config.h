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
*  \file APP_Config.h
******************************************************************************/

/**
 * @brief Configuration file for environment setup.
 */

#ifndef APP_CONFIG
#define APP_CONFIG

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



/**
 * @brief Fleet Profile UID created in keySTREAM.
 * Copy the Fleet Profile UID from keySTREAM and uncomment the following line.
 * Replace xxxxxxxxxxx with the Fleet Profile UID.
 */
// #define KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID             ""
#if !defined(KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID)
    #error KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID not defined Uncomment define and replace xxxxxxxxxx with Fleet Profile UID!
#endif

/**
 * @brief AWS endpoint URL.
 * Define this macro to set the AWS endpoint URL.
 * This is the URL of the AWS IoT Core service.
 * The URL should be in the format of "xxxxxxxxxxxxxx.iot.region.amazonaws.com".
 * This is used to connect to the AWS IoT Core service.
 * Replace xxxxxxxxxxx with your AWS IoT Core endpoint.
 * Example: "a1b2c3d4e5f6g7h8i9j0.iot.us-west-2.amazonaws.com"
 */
// #define AWS_ENDPOINT      "xxxxxxxxxxx"
#if !defined(AWS_ENDPOINT)
    #error AWS_ENDPOINT not defined! \
            Uncomment define and replace xxxxxxxxxx with AWS endpoint!
#endif


/**
 * @brief AWS IoT Core Thing name.
 * Define AWS_THING_NAME macro to set the AWS IoT Core Thing name.
 * This is the name of the Thing in AWS IoT Core.
 * The Thing name should be unique within your AWS account.
 * Replace xxxxxxxxxxx with your Thing name.
 * Example: "MyIoTDevice"
 */
// #define AWS_THING_NAME      "xxxxxxxxxxx"
#if !defined(AWS_THING_NAME)
    #error AWS_THING_NAME not defined! \
            Uncomment define and replace xxxxxxxxxx with AWS Thing name!
#endif
/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif /* C++ */

#endif // APP_CONFIG

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

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
/** \brief  Common sal constants.
 *
 * \author Kudelski IoT
 *
 * \date    2023/06/02
 *
 * \file k_sal.h
 *
 ******************************************************************************/

/**
 * @brief Common sal constants.
 */

#ifndef K_SAL_H
#define K_SAL_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/** @defgroup g_sal_api SAL Interface */

/** @addtogroup g_sal_api
 * @{
*/

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "cryptoConfig.h"

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Crypto Object Ids.
 */
/** @brief Chip Secret Key Id(Persistent). */
#define C_K_KTA__CHIP_SK_ID          (0x0001u)
/** @brief L1 Filed Key Id(Persistent). */
#define C_K_KTA__L1_FIELD_KEY_ID     (0x0002u)
/** @brief Volatile Id. */
#define C_K_KTA__VOLATILE_ID         (0x8000u)
/** @brief Volatile 2 Id. */
#define C_K_KTA__VOLATILE_2_ID       (0x8001u)
/** @brief Volatile 3 Id. */
#define C_K_KTA__VOLATILE_3_ID       (0x8002u)

/**
 * @brief Persistent Storage_Ids.
 */
/** @brief Life cycle data ID. */
#define C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID  (0x4000u)
/** @brief Rot public UID ID. */
#define C_K_KTA__ROT_PUBLIC_UID_STORAGE_ID    (0x4001u)
/** @brief Sealed data ID. */
#define C_K_KTA__SEALED_DATA_STORAGE_ID       (0x4002u)
/** @brief L1 key material data ID. */
#define C_K_KTA__L1_KEY_MATERIAL_DATA_ID      (0x4003u)
/** @brief Device certificate ID. */
#define C_K_KTA__DEVICE_CERTIFICATE_ID        (0x4004u)
/** @brief Signer certificate ID. */
#define C_K_KTA__SIGNER_CERTIFICATE_ID        (0x4005u)
/** @brief Signer public key. */
#define C_K_KTA__SIGNER_PUB_KEY_ID            (0x4006u)
/** @brief KTA Version ID*/
#define C_K_KTA__VERSION_SLOT_ID              (0x4007u)

/**
 * @brief HKDF Extract Mode.
 */
/** @brief Activation Mode. */
#define C_K_KTA__HKDF_ACT_MODE                (0x0000u)

/** @brief Generic Mode. */
#define C_K_KTA__HKDF_GEN_MODE                (0x0001u)

/** @brief Maximal size of chipset id field, in bytes. */
#define C_K_KTA__CHIPSET_UID_MAX_SIZE         (32u)

/** @brief Maximal size of chipset cert field, in bytes. */
#define C_K_KTA__CHIP_CERT_MAX_SIZE           C_K__CHIP_CERT_MAX_SIZE_VENDOR_SPECIFIC

/** @brief Maximal size of random field, in bytes. */
#define C_K_KTA__RANDOM_MAX_SIZE              (256u)

/** @brief Maximal size of secret key field, in bytes. */
#define C_K_KTA__SHARED_SECRET_KEY_MAX_SIZE   (32u)

/** @brief Maximal size of Public key field, in bytes. */
#define C_K_KTA__PUBLIC_KEY_MAX_SIZE          (64u)

/** @brief Maximal size of Mac key field, in bytes. */
#define C_K_KTA__HMAC_MAX_SIZE                (16u)

/** @brief Maximal size of salt for  activation mode key field, in bytes. */
#define C_K_KTA__HKDF_ACT_SALT_MAX_SIZE       (64u)

/** @brief Maximal size of salt for generic mode key field, in bytes. */
#define C_K_KTA__HKDF_GEN_SALT_MAX_SIZE       (16u)

/** @brief Maximal size of Rot public uid field, in bytes. */
#define C_K_KTA__ROT_PUBLIC_UID_MAX_SIZE      (8u)

/** @brief Maximal size of command field , in bytes. */
#define C_K_KTA__CMD_FIELD_MAX_SIZE           (1024u)

/** @brief Maximal size of sealed information, in bytes. */
#define C_K_KTA__SEALED_INFORMATION_MAX_SIZE  (133u)

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/** @} g_sal_api */

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */


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
/** \brief  Interface for object operation.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_object.h
 ******************************************************************************/

/**
 * @brief Interface for object operation.
 */

#ifndef K_SAL_OBJECT_H
#define K_SAL_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup g_sal_api SAL Interface */

/** @addtogroup g_sal_api
 * @{
*/

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_defs.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef K_SAL_API
/** @brief K Sal Api. */
#define K_SAL_API
#endif /* K_SAL_API */

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/** @brief Sal object type data. */
#define C_SAL_OBJECT__TYPE_DATA                           (0u)
/** @brief Sal object type key. */
#define C_SAL_OBJECT__TYPE_KEY                            (1u)
/** @brief Sal object type certificate. */
#define C_SAL_OBJECT__TYPE_CERTIFICATE                    (2u)
/** @brief Sal object type sealed data. */
#define C_SAL_OBJECT__TYPE_SEALED_DATA                    (3u)

/** @brief Tags used in the command fields. */
enum
{
  /**
   * Object type data.
   */
  E_K_SAL_OBJECT_TYPE_DATA,
  /**
   * Object type rfu.
   */
  E_K_SAL_OBJECT_TYPE_RFU_V2,
  /**
   * Object type cert.
   */
  E_K_SAL_OBJECT_TYPE_CERTFICATE,
  /**
   * Object type sealed data.
   */
  E_K_SAL_OBJECT_TYPE_SEALED_DATA,
  /**
   * Object type max num.
   */
  E_K_SAL_OBJECT_TYPE_MAX_NUM
};
typedef uint32_t TKSalObjectType;

/** @brief Structure holds the associated Objects. */
typedef struct
{
  uint32_t associatedKeyId;
  /* KeyId for device cert. */
  uint32_t associatedKeyIdDeprecated;
  /* Depricated keyId of device cert. */
  uint32_t associatedObjectId;
  /* Object ID of Signer cert. */
  uint32_t associatedObjectIdDeprecated;
  /* Depricated Object ID of Signer cert. */
  uint8_t  associatedObjectType;
  /* Either key/data. */
} TKSalObjAssociationInfo;

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Generates a key pair and persists private key in a platform and returns
 *   the public key.
 *
 * @param[in] xKeyId
 *   Persistent object identifier.
 *   Size is fixed to 32-bits.
 *   Should not be NULL.
 * @param[in] xpKeyAttributes
 *   Address of buffer containing the key attributes.
 *   Should not be NULL.
 * @param[in] xKeyAttributesLen
 *   Length of the buffer containing the key attributes.
 * @param[out] xpPublicKey
 *   Address of buffer containing the public key.
 * @param[in,out] xpPublicKeyLen
 *   Length of public key buffer (in Bytes).
 * @param[out] xpPlatformStatus
 *   Status with Platform format.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salObjectKeyGen
(
  uint32_t        xKeyId,
  const uint8_t*  xpKeyAttributes,
  size_t          xKeyAttributesLen,
  uint8_t*        xpPublicKey,
  size_t*         xpPublicKeyLen,
  uint8_t*        xpPlatformStatus
);

/**
 * @brief
 *   Sets a persistent object in the Platform.
 *
 * @param[in] xObjectType
 *   Type of the object.
 * @param[in] xObjectId
 *   Persistent object identifier.
 *   Size is fixed to 32-bits. Should not be NULL.
 * @param[in] xpDataAttributes
 *   Address of buffer containing the object data_attributes.
 *   Should not be NULL.
 * @param[in] xDataAttributesLen
 *   Length of the buffer containing the object data_attributes.
 * @param[in] xpData
 *   Address of buffer containing the object input data.
 * @param[in] xDataLen
 *   Length of object input data buffer (in Bytes).
 * @param[out] xpPlatformStatus
 *   Status with Platform format.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salObjectSet
(
  TKSalObjectType  xObjectType,
  uint32_t         xObjectId,
  const uint8_t*   xpDataAttributes,
  size_t           xDataAttributesLen,
  const uint8_t*   xpData,
  size_t           xDataLen,
  uint8_t*         xpPlatformStatus
);

/**
 * @brief
 *   This function sets a persistent key object in the Platform.
 *
 * @param[in] xKeyId
 *   Persistent key object identifier. Size is fixed to 32-bits.
 * @param[in] xpDataAttributes
 *   Address of buffer containing the object key_attributes.
 * @param[in] xDataAttributesLen
 *   Length of the buffer containing the object key_attributes.
 * @param[in] xpKey
 *   Address of buffer containing the object input key.
 * @param[in] xKeyLen
 *   Length of object input key buffer (in Bytes).
 * @param[in,out] xpPlatformStatus
 *   platform_status out Status with Platform format.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */

K_SAL_API TKStatus salObjectKeySet
(
  uint32_t        xKeyId,
  const uint8_t*  xpDataAttributes,
  size_t          xDataAttributesLen,
  const uint8_t*  xpKey,
  size_t          xKeyLen,
  uint8_t*        xpPlatformStatus
);

/**
 * @brief
 *   Gets the persistent object from the Platform.
 *
 * @param[in] xObjectType
 *   Type of the object.
 * @param[in] xObjectId
 *   Persistent object identifier.
 *   Size is fixed to 32-bits.
 *   Should not be NULL.
 * @param[out] xpData
 *   Address of buffer where the device platform will return the output data.
 *   Should not be NULL.
 * @param[in,out] xpDataLen
 *   Address of output buffer length (in Bytes). Caller set the
 *   maximum output buffer length expected.
 *   Then, the function set the actual length of the output buffer.
 * @param[out] xpPlatformStatus
 *   Status with Platform format.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salObjectGet
(
  TKSalObjectType  xObjectType,
  uint32_t         xObjectId,
  uint8_t*         xpData,
  size_t*          xpDataLen,
  uint8_t*         xpPlatformStatus
);

/**
 * @brief
 *   Deletes the persistent object from the Platform.
 *
 * @param[in] xObjectType
 *   Type of the object.
 * @param[in] xObjectId
 *   Persistent object identifier.
 *   Size is fixed to 32-bits.
 *   Should not be NULL.
 * @param[out] xpPlatformStatus
 *   Status with Platform format.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salObjectDelete
(
  TKSalObjectType  xObjectType,
  uint32_t         xObjectId,
  uint8_t*         xpPlatformStatus
);

/**
 * @brief
 *   Deletes the persistent key object from the Platform.
 *
 * @param[in] xKeyId
 *   Persistent object identifier.
 *   Size is fixed to 32-bits.
 *   Should not be NULL.
 * @param[out] xpPlatformStatus
 *   Status with Platform format.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus  salObjectKeyDelete
(
  uint32_t  xKeyId,
  uint8_t*  xpPlatformStatus
);

/**
 * @brief
 *   Set in the Platform a persistent object which includes association information.
 *
 * @param[in] xObjectType
 *   Type of the object.
 * @param[in] xobjectWithAssociationId
 *   Persistent object identifier.
 *   Size is fixed to 32-bits. Should not be NULL.
 * @param[in] xpDataAttributes
 *   Address of buffer containing the object data_attributes.
 *   Should not be NULL.
 * @param[in] xDataAttributesLen
 *   Length of the buffer containing the object data_attributes.
 * @param[in] xpData
 *   Address of buffer containing the object input data.
 * @param[in] xDataLen
 *   Length of object input data buffer (in Bytes).
 * @param[in] xpAssociationInfo
 *   Information associated to the Object which are used by internally
 *   by keySTREAM Trusted Agent to manage the associated objects and
 *   partly by the Application to use the associated objects.
 * @param[out] xpPlatformStatus
 *   Status with Platform format.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salObjectSetWithAssociation
(
  uint32_t                  xObjectType,
  uint32_t                  xObjectWithAssociationId,
  const uint8_t*            xpDataAttributes,
  size_t                    xDataAttributesLen,
  const uint8_t*            xpData,
  size_t                    xDataLen,
  TKSalObjAssociationInfo*  xpAssociationInfo,
  uint8_t*                  xpPlatformStatus
);

/**
 * @brief
 *    Get from the Platform a persistent object which includes association information.
 *
 * @param[in] xobjectWithAssociationId
 *   Persistent object identifier.
 *   Size is fixed to 32-bits. Should not be NULL.
 * @param[out] xpData
 *   Address of buffer where the device platform will return the output data.
 * @param[in,out] xpDataLen
 *   Address of output buffer length (in Bytes).
 *   Caller set the maximum output buffer length expected.
 *   Then, the function set the actual length of the output buffer.
 * @param[in] xpAssociationInfo
 *   Information associated to the Object which are used by internally by KTA
 *   to manage the associated objects.
 *   and partly by the Application to use the associated objects.
 * @param[out] xpPlatformStatus
 *   Status with Platform format.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter(s).
 * - E_K_STATUS_ERROR for other errors.
 */
K_SAL_API TKStatus salObjectGetWithAssociation
(
  uint32_t                  xObjectWithAssociationId,
  const uint8_t*            xpData,
  size_t*                   xpDataLen,
  TKSalObjAssociationInfo*  xpAssociationInfo,
  uint8_t*                  xpPlatformStatus
);

/** @} g_sal_api */

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_OBJECT_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */


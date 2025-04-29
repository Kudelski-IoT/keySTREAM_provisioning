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
/** \brief  SAL Object for Mircrochip.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_object.c
 ******************************************************************************/

/**
 * @brief SAL Object for Mircrochip.
 */

#include "k_sal_object.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_defs.h"
#include "k_sal.h"
#include "slotConfig.h"
#include "KTALog.h"
#include "k_sal_storage.h"
#include "cryptoConfig.h"
#include "cust_def_device.h"
#include "cust_def_signer.h"
#include "atca_basic.h"
#include "atcacert.h"
#include "atcacert_client.h"
#include "host/atca_host.h"

#include <string.h>


/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Key of 64 bytes. */
#define C_SAL_OBJ_64_BYTE_KEY_SIZE                    (64u)

/** @brief Key of 32 bytes. */
#define C_SAL_OBJ_32_BYTE_KEY_SIZE                    (32u)

/** @brief Random num in size for random nonce creation. */
#define C_SAL_OBJ_RANDOM_NUM_IN_SIZE                  (20u)

/** @brief Default crypto key size. */
#define C_SAL_CRYPTO_KEY_SIZE_32_BYTE                 (32u)

/** @brief Life cycle data id length. */
#define C_SAL_LIFE_CYCLE_STATE_STORAGE_ID_LENGTH      (4u)

/** @brief Device certificate length. */
#define C_SAL_DEVICE_CERTIFICATE_ID_LENGTH            (72u)

/** @brief Signer certificate length. */
#define C_SAL_SIGNER_CERTIFICATE_ID_LENGTH            (72u)

/** @brief L1 key material data id length. */
#define C_SAL_L1_KEY_MATERIAL_DATA_ID_ACTUAL_LENGTH   (17u)

/** @brief Slot 8 identifier. */
#define C_SAL_SLOT8_IDENTIFIER                        (0x1000008u)

/** @brief keySTREAM Trusted Agent onboarding key - L1 field key. */
#define C_SAL_L1_FIELD_KEY_OBJECT_ID                  (0x00000004u)

/** @brief Customer device secret key. */
#define C_SAL_SECRET_KEY_OBJECT_ID                    (0x00000000u)

/** @brief Local buffer size. */
#define C_SAL_MAX_LOCAL_BUFZERO_SIZE                  (72u)

/** @brief Signer Id data id length. */
#define C_SAL_SIGNER_ID_STORAGE_ID_LENGTH             (4u)

/** @brief Signer Id slot ID. */
#define C_SAL_SIGNER_ID_STORAGE_ID                    (0x5000u)

/** @brief Slot ID Defines */
#define C_SAL_SLOT_6_KEY_ID                           (0x06u)
#define MAX_MANAGED_SLOT_DATA_SIZE                    (32u)
#define MAX_MANAGED_SLOT_MAC_SIZE                     (32u)
#define MANAGED_SLOT_14_MAX_DATA_SIZE                 (128u)
#define MANAGED_SLOT_5_MAX_DATA_SIZE                  (64u)
#define C_KTA_MANAGED_SLOT_8                          (8U)

/** @brief Slot Identifier */
#define SERVER_IDENTIFIER                             (0x0100000Eu)
#define SLOT_IDENTIFIER                               (14u)

/** @brief Zone for doing an Encrypt Write */
#define C_SAL_ENCRYPT_WRITE_ZONE (ATCA_ZONE_DATA|ATCA_ZONE_READWRITE_32|ATCA_ZONE_ENCRYPTED)

/******************************************************************************/
/** \brief  Set an argument/return value as unused.
*
*  \param[in]  xArg
*
******************************************************************************/
#define M_UNUSED(xArg)            (void)(xArg)
/* Sal ID Map Object. */

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
static const char* gpModuleName = "SALOBJECT";
static atca_temp_key_t gTempKey;
static bool globalEncWriteFlag = 0;

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
TKStatus lsalEncryptWrite(uint16_t xSlot, uint8_t* xpData, size_t xDataLen, uint8_t xblock);
/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief   implement salObjectKeyGen
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salObjectKeyGen
(
  uint32_t        xKeyId,
  const uint8_t*  xpKeyAttributes,
  size_t          xKeyAttributesLen,
  uint8_t*        xpPublicKey,
  size_t*         xpPublicKeyLen,
  uint8_t*        xpPlatformStatus
)
{
  TKStatus             status = E_K_STATUS_ERROR;
  ATCA_STATUS          cryptoStatus = ATCA_STATUS_UNKNOWN;
  uint8_t              aPublicKey[C_SAL_OBJ_64_BYTE_KEY_SIZE];
  uint8_t              aSignature[C_SAL_OBJ_64_BYTE_KEY_SIZE];
  uint8_t              aNumIn[C_SAL_OBJ_RANDOM_NUM_IN_SIZE];
  uint16_t             slotSign = C_SAL__CRYPTO_SLOT_FOR_ATTESTAION;
  uint8_t              aRandOut[C_SAL_OBJ_32_BYTE_KEY_SIZE];
  atca_temp_key_t      tempKey;
  atca_nonce_in_out_t  nonceParams;

  M_UNUSED(xpKeyAttributes);
  M_UNUSED(xKeyAttributesLen);
  M_UNUSED(xpPlatformStatus);

  M_KTALOG__START("Start");

  if ((NULL == xpPublicKey)   ||
      ((NULL == xpPublicKeyLen)  ||
      (C_K__ICPP_CMD_RESPONSE_SIZE_VENDOR_SPECIFIC != *xpPublicKeyLen)))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter");
  }
  else
  {
    (void)memset(aNumIn, 0x00, C_SAL_OBJ_RANDOM_NUM_IN_SIZE);
    cryptoStatus = atcab_nonce_rand(aNumIn, aRandOut);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_nonce_rand() failed with ret=0x%08X", cryptoStatus);
      goto end;
    }

    cryptoStatus = atcab_genkey((uint16_t)xKeyId, aPublicKey);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_genkey() failed with ret=0x%08X", cryptoStatus);
      goto end;
    }

    cryptoStatus = atcab_genkey_base(GENKEY_MODE_DIGEST, (uint16_t)xKeyId, NULL, NULL);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_genkey_base() failed with ret=0x%08X", cryptoStatus);
      goto end;
    }

    cryptoStatus = atcab_sign_internal(slotSign, false, false, aSignature);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_sign_internal() failed with ret=0x%08X", cryptoStatus);
      goto end;
    }

    (void)memcpy(xpPublicKey, aPublicKey, C_SAL_OBJ_64_BYTE_KEY_SIZE);
    (void)memcpy(&xpPublicKey[C_SAL_OBJ_64_BYTE_KEY_SIZE], aSignature, C_SAL_OBJ_64_BYTE_KEY_SIZE);

    (void)memset(&tempKey, 0, sizeof(tempKey));
    (void)memset(&nonceParams, 0, sizeof(nonceParams));
    nonceParams.mode = NONCE_MODE_SEED_UPDATE;
    nonceParams.zero = 0;
    nonceParams.num_in = aNumIn;
    nonceParams.rand_out = aRandOut;
    nonceParams.temp_key = &tempKey;
    cryptoStatus = atcah_nonce(&nonceParams);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcah_nonce() failed with ret=0x%08X", cryptoStatus);
      goto end;
    }

    (void)memcpy(&xpPublicKey[C_SAL_OBJ_64_BYTE_KEY_SIZE * 2u], nonceParams.temp_key->value,
                 C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

    *xpPublicKeyLen = sizeof(aPublicKey) + sizeof(aSignature) + C_SAL_CRYPTO_KEY_SIZE_32_BYTE;

    status = E_K_STATUS_OK;
  }

end:
  M_KTALOG__END("End, status : %d", status);

  return status;
}

/**
 * @brief   implement salObjectSet
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salObjectSet
(
  TKSalObjectType xObjectType,
  uint32_t        xIdentifier,
  const uint8_t*  xpDataAttributes,
  size_t          xDataAttributesLen,
  object_t*       xpObject,
  uint8_t*        xpPlatformStatus
)
{
  TKStatus     status = E_K_STATUS_ERROR;
  ATCA_STATUS  cryptoStatus = ATCA_STATUS_UNKNOWN;

  // Convert KeySTREAM identifier to SLOT identifier
  if (xIdentifier == SERVER_IDENTIFIER) {
      xIdentifier = SLOT_IDENTIFIER;
  }

  M_UNUSED(xpDataAttributes);
  M_UNUSED(xDataAttributesLen);

  M_KTALOG__START("Start");

  if ((xObjectType > C_SAL_OBJECT__TYPE_TRUST_ANCHOR) ||
      (NULL == xpObject) ||
      (NULL == xpPlatformStatus))
  {
    M_KTALOG__ERR("Invalid parameter");
  }
  else
  {
    if (E_K_SAL_OBJECT_TYPE_MANAGED_SLOT_14 == xIdentifier)
    {
      status = lsalEncryptWrite(E_K_SAL_OBJECT_TYPE_MANAGED_SLOT_14, xpObject->data, xpObject->dataLen, 0);
      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("Encrypt Write to slot 14 failed.");
        goto end;
      }

      cryptoStatus = salStorageSetValue(C_K_KTA__CUSTOMER_TRUST_ANCHOR_METADATA_ID,
                                        xpObject->customerMetadata,
                                        xpObject->customerMetadataLen);

      if (cryptoStatus != ATCACERT_E_SUCCESS)
      {
        M_KTALOG__ERR("Trust Anchor Meta data set failed with error, 0x%02x", cryptoStatus);
        goto end;
      }

      cryptoStatus = salStorageSetValue(C_K_KTA__CUSTOMER_TRUST_ANCHOR_OBJECT_UID_ID,
                                        xpObject->objectUid,
                                        xpObject->objectUidLen);

      if (cryptoStatus != ATCACERT_E_SUCCESS)
      {
        M_KTALOG__ERR("Trust Anchor Object UID set failed with error, 0x%02x", cryptoStatus);
        goto end;
      }

      status = E_K_STATUS_OK;
    }
    else if (E_K_SAL_OBJECT_TYPE_MANAGED_SLOT_5 == xIdentifier)
    {
      status = lsalEncryptWrite(E_K_SAL_OBJECT_TYPE_MANAGED_SLOT_5, xpObject->data, xpObject->dataLen, 0);
      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("Encrypt Write to slot 5 failed.");
        goto end;
      }

      cryptoStatus = salStorageSetValue(C_K_KTA__CUSTOMER_SYM_KEY_METADATA_ID,
                                        xpObject->customerMetadata,
                                        xpObject->customerMetadataLen);

      if (cryptoStatus != ATCACERT_E_SUCCESS)
      {
        M_KTALOG__ERR("Symmetric Key Meta Data set failed with error, 0x%02x", cryptoStatus);
        goto end;
      }

      cryptoStatus = salStorageSetValue(C_K_KTA__CUSTOMER_SYM_KEY_OBJECT_UID_ID,
                                        xpObject->objectUid,
                                        xpObject->objectUidLen);

      if (cryptoStatus != ATCACERT_E_SUCCESS)
      {
        M_KTALOG__ERR("Symmetric Key Object UID set failed with error, 0x%02x", cryptoStatus);
        goto end;
      }

      status = E_K_STATUS_OK;
    }
    else if (E_K_SAL_OBJECT_TYPE_MANAGED_SLOT_8 == xIdentifier)
    {
      cryptoStatus = salStorageSetValue(C_K_KTA__CUSTOMER_DATA_ID,
                                        xpObject->data,
                                        xpObject->dataLen);

      cryptoStatus = salStorageSetValue(C_K_KTA__CUSTOMER_DATA_METADATA_ID,
                                        xpObject->customerMetadata,
                                        xpObject->customerMetadataLen);

      cryptoStatus = salStorageSetValue(C_K_KTA__CUSTOMER_DATA_OBJECT_UID_ID,
                                        xpObject->objectUid,
                                        xpObject->objectUidLen);
      if (cryptoStatus != ATCACERT_E_SUCCESS)
      {
        M_KTALOG__ERR("Customer data set failed with error, 0x%02x", cryptoStatus);
        goto end;
      }

      status = E_K_STATUS_OK;
    }
    else if (xObjectType == C_SAL_OBJECT__TYPE_CERTIFICATE)
    {
      if (C_KTA__DEVICE_CERT_STORAGE_SLOT == xIdentifier)
      {
        M_KTALOG__INFO("Device cert len, %d", xpObject->dataLen);
        cryptoStatus = atcacert_write_cert(&g_cert_def_3_device,
                                           xpObject->data,
                                           xpObject->dataLen);
      }
      else if (C_KTA__SIGNER_CERT_STORAGE_SLOT == xIdentifier)
      {
        M_KTALOG__INFO("Signer cert len, %d", xpObject->dataLen);
        cryptoStatus = atcacert_write_cert(&g_cert_def_1_signer,
                                           xpObject->data,
                                           xpObject->dataLen);
      }
      else
      {
        M_KTALOG__ERR("Invalid Object Id");
        goto end;
      }

      if (cryptoStatus != ATCACERT_E_SUCCESS)
      {
        M_KTALOG__ERR("Write cert failed with error, 0x%02x", cryptoStatus);
        goto end;
      }

      status = E_K_STATUS_OK;
    }
    else
    {
      status = E_K_STATUS_OK;
    }
  }

end:
  M_KTALOG__END("End, status : %d", status);

  //*xpPlatformStatus = cryptoStatus;
  (void)memcpy((void *)xpPlatformStatus, (const void *)&cryptoStatus, sizeof(cryptoStatus));
  return status;
}

/**
 * @brief   implement salObjectDelete
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salObjectDelete
(
  TKSalObjectType xObjectType,
  uint32_t        xObjectId,
  uint8_t*        xpPlatformStatus
)
{
  TKStatus     status = E_K_STATUS_ERROR;
  ATCA_STATUS  cryptoStatus = ATCA_STATUS_UNKNOWN;
  uint8_t      aTempBufZero[C_SAL_MAX_LOCAL_BUFZERO_SIZE] = {0};
  uint8_t      aFixedInfo[] = C_KTA__FIELD_KEY_FIXED_INFO;
  size_t       xFixedInfoLen = C_KTA__FIELD_KEY_FIXED_INFO_SIZE;
  uint8_t      aSalt[] = { 0xEE, 0xA7, 0x24, 0xE1, 0xA6, 0x22, 0x3D, 0x48,
                           0xD3, 0x0D, 0x86, 0xFF, 0x4E, 0xC5, 0xBE, 0x53,
                           0x81, 0xA1, 0x81, 0xE0, 0x6D, 0x98, 0x8D, 0x72,
                           0xFC, 0x92, 0xA1, 0x62, 0x75, 0xD4, 0xB0, 0x5A,
                         };

  M_KTALOG__START("Start");

  if ((xObjectType > C_SAL_OBJECT__TYPE_SEALED_DATA) || (NULL == xpPlatformStatus))
  {
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    M_KTALOG__INFO("xObjectType[%x] xObjectId[%x]", xObjectType, xObjectId);

    if (xObjectType == C_SAL_OBJECT__TYPE_KEY)
    {
      /* Customer device secret key. */
      if (xObjectId == C_SAL_SECRET_KEY_OBJECT_ID)
      {
        M_KTALOG__INFO("Reset device secret key");
        cryptoStatus = atcab_genkey(C_KTA__DEVICE_PRIVATE_STORAGE_SLOT, NULL);

        if (cryptoStatus != ATCA_SUCCESS)
        {
          M_KTALOG__ERR("atcab_genkey failed, 0x%2x", status);
          goto end;
        }
      }
      /* keySTREAM Trusted Agent onboarding key - L1 field key. */
      else if (xObjectId == C_SAL_L1_FIELD_KEY_OBJECT_ID)
      {
        M_KTALOG__INFO("Reset L1 Field key");
        cryptoStatus = atcab_nonce_load(NONCE_MODE_TARGET_TEMPKEY,
                                        aSalt,
                                        C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

        if (cryptoStatus != ATCA_SUCCESS)
        {
          M_KTALOG__ERR("atcab_nonce_load()  failed with ret=0x%08X", cryptoStatus);
          goto end;
        }

        /* HKDF Expand. */
        cryptoStatus = atcab_kdf(
                         KDF_MODE_ALG_HKDF | KDF_MODE_SOURCE_TEMPKEY | KDF_MODE_TARGET_SLOT,
                         C_KTA__KDF_KEY_ID,
                         KDF_DETAILS_HKDF_MSG_LOC_INPUT | (xFixedInfoLen << 24),
                         aFixedInfo,
                         NULL,
                         NULL);

        if (cryptoStatus != ATCA_SUCCESS)
        {
          M_KTALOG__ERR("atcab_kdf() for expand failed with ret=0x%08X", cryptoStatus);
          goto end;
        }
      }
      else
      {
        M_KTALOG__ERR("Invalid Value Identifier[%d]", xObjectId);
      }
    }
    else if (xObjectType == C_SAL_OBJECT__TYPE_DATA)
    {
      cryptoStatus = ATCA_SUCCESS;

      switch (xObjectId)
      {
        case C_KTA__DEVICE_CERT_STORAGE_SLOT:
        {
          M_KTALOG__INFO("Reset Device Certificate");
          status = salStorageSetValue(C_K_KTA__DEVICE_CERTIFICATE_ID,
                                      aTempBufZero,
                                      C_SAL_DEVICE_CERTIFICATE_ID_LENGTH);

          if (status != E_K_STATUS_OK)
          {
            M_KTALOG__ERR("SetVal Dev Cert failed, 0x%2x", status);
            break;
          }
        }
        break;

        case C_KTA__SIGNER_PUB_KEY_STORAGE_SLOT:
        {
          M_KTALOG__INFO("Reset Signer public key");
          status = salStorageSetValue(C_K_KTA__SIGNER_PUB_KEY_ID,
                                      aTempBufZero,
                                      C_KTA__SIGNER_PUBLIC_KEY_LENGTH);

          if (status != E_K_STATUS_OK)
          {
            M_KTALOG__ERR("SetVal Signer pubKey failed, 0x%2x", status);
            break;
          }
        }
        break;

        case C_KTA__SIGNER_CERT_STORAGE_SLOT:
        {
          M_KTALOG__INFO("Reset Signer Certificate");
          status = salStorageSetValue(C_K_KTA__SIGNER_CERTIFICATE_ID,
                                      aTempBufZero,
                                      C_SAL_SIGNER_CERTIFICATE_ID_LENGTH);

          if (status != E_K_STATUS_OK)
          {
            M_KTALOG__ERR("SetVal Signer Cert failed, 0x%2x", status);
            break;
          }
        }
        break;

        case C_SAL_SLOT8_IDENTIFIER:
        {
          M_KTALOG__INFO("Reset LifeCycle/L1 Key Material");

          status = salStorageSetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                                      aTempBufZero,
                                      C_SAL_LIFE_CYCLE_STATE_STORAGE_ID_LENGTH);

          if (status != E_K_STATUS_OK)
          {
            M_KTALOG__ERR("SetVal LCS failed, 0x%2x", status);
            break;
          }

          status = salStorageSetValue(C_K_KTA__L1_KEY_MATERIAL_DATA_ID,
                                      aTempBufZero,
                                      C_SAL_L1_KEY_MATERIAL_DATA_ID_ACTUAL_LENGTH);

          if (status != E_K_STATUS_OK)
          {
            M_KTALOG__ERR("SetVal L1_KEY material failed, 0x%2x", status);
            break;
          }

          status = salStorageSetValue(C_SAL_SIGNER_ID_STORAGE_ID,
                                      aTempBufZero,
                                      C_SAL_SIGNER_ID_STORAGE_ID_LENGTH);

          if (status != E_K_STATUS_OK)
          {
            M_KTALOG__ERR("SetVal LCS failed, 0x%2x", status);
            break;
          }
        }
        break;

        default:
        {
          M_KTALOG__ERR("Invalid Identifier %d", xObjectId);
        }
        break;
      }

      if (status != E_K_STATUS_OK)
      {
        goto end;
      }
    }
    else
    {
      M_KTALOG__ERR("Invalid object type, 0x%2x", xObjectType);
      goto end;
    }
    status = E_K_STATUS_OK;
  }

end:
  // *xpPlatformStatus = cryptoStatus;
  (void)memcpy((void *)xpPlatformStatus, (const void *)&cryptoStatus, 4);

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief   Implement salGetChallenge
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * SUPPRESS: MISRA_DEV_KTA_002 : misra_c2012_rule_17.7_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salGetChallenge
(
  uint8_t* xpChallengeKey,
  uint8_t* xpPlatformStatus
)
{
  ATCA_STATUS cryptoStatus = ATCA_STATUS_UNKNOWN;
  TKStatus    status = E_K_STATUS_ERROR;
  uint8_t     aNumIn[C_SAL_OBJ_RANDOM_NUM_IN_SIZE] = {0};
  uint8_t     aRandOut[C_SAL_OBJ_32_BYTE_KEY_SIZE] = {0};
  atca_nonce_in_out_t nonce_params;

  M_KTALOG__START("Start");
  if ((NULL == xpChallengeKey) ||
      (NULL == xpPlatformStatus))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter");
  }
  else
  {
    cryptoStatus = atcab_nonce_rand(aNumIn, aRandOut);
    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_nonce_rand() failed with ret=0x%08X", cryptoStatus);
      goto end;
    }

    (void)memset(&nonce_params, 0, sizeof(nonce_params));
    nonce_params.mode = NONCE_MODE_SEED_UPDATE;
    nonce_params.zero = 0;
    nonce_params.num_in = aNumIn;
    nonce_params.rand_out = aRandOut;
    nonce_params.temp_key = &gTempKey;
    cryptoStatus = atcah_nonce(&nonce_params);
    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcah_nonce() failed with ret=0x%08X", cryptoStatus);
      goto end;
    }

    (void)memcpy((void *)xpChallengeKey, (const void *)nonce_params.temp_key->value, MAX_MANAGED_SLOT_DATA_SIZE);

    status = E_K_STATUS_OK;
  }

end:
  M_KTALOG__END("End, status : %d", status);
  (void)memcpy((void *)xpPlatformStatus, (const void *)&cryptoStatus, sizeof(cryptoStatus));

  return status;
}

/**
 * @brief   implement salObjectKeyDelete
 *
 */
K_SAL_API TKStatus  salObjectKeyDelete
(
  uint32_t  xKeyId,
  uint8_t*  xpPlatformStatus
)
{
  M_UNUSED(xKeyId);
  M_UNUSED(xpPlatformStatus);
  return E_K_STATUS_OK;
}

/**
 * @brief   implement salObjectSetWithAssociation
 *
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
)
{
  M_UNUSED(xObjectType);
  M_UNUSED(xObjectWithAssociationId);
  M_UNUSED(xpDataAttributes);
  M_UNUSED(xDataAttributesLen);
  M_UNUSED(xpData);
  M_UNUSED(xDataLen);
  M_UNUSED(xpAssociationInfo);
  M_UNUSED(xpPlatformStatus);
  return E_K_STATUS_OK;
}

/**
 * @brief   implement salObjectGetWithAssociation
 *
 */
K_SAL_API TKStatus salObjectGetWithAssociation
(
  uint32_t                  xObjectWithAssociationId,
  const uint8_t*            xpData,
  size_t*                   xpDataLen,
  TKSalObjAssociationInfo*  xpAssociationInfo,
  uint8_t*                  xpPlatformStatus
)
{
  M_UNUSED(xObjectWithAssociationId);
  M_UNUSED(xpData);
  M_UNUSED(xpDataLen);
  M_UNUSED(xpAssociationInfo);
  M_UNUSED(xpPlatformStatus);
  return E_K_STATUS_OK;
}

/**
 * @brief   implement salObjectKeySet
 *
 */
K_SAL_API TKStatus salObjectKeySet
(
  uint32_t        xKeyId,
  const uint8_t*  xpDataAttributes,
  size_t          xDataAttributesLen,
  const uint8_t*  xpKey,
  size_t          xKeyLen,
  uint8_t*        xpPlatformStatus
)
{
  M_UNUSED(xKeyId);
  M_UNUSED(xpDataAttributes);
  M_UNUSED(xDataAttributesLen);
  M_UNUSED(xpKey);
  M_UNUSED(xKeyLen);
  M_UNUSED(xpPlatformStatus);
  return E_K_STATUS_OK;
}

/**
 * @brief   implement salObjectGet
 *
 */
K_SAL_API TKStatus salObjectGet
(
  TKSalObjectType  xObjectType,
  uint32_t         xIdentifier,
  object_t*        xpObject,
  uint8_t*         xpPlatformStatus
)
{
  TKStatus  status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  /* Ensure xpObject is not NULL */
  if (xpObject == NULL) {
    M_KTALOG__ERR("xpObject is NULL");
    return E_K_STATUS_PARAMETER;
  }

  /* Get data for managed slots */
  if (E_K_SAL_OBJECT_TYPE_MANAGED_SLOT_14 == xIdentifier)
  {
    M_KTALOG__INFO("Fetching data for MANAGED_SLOT_14");

    status = salStorageGetValue(C_K_KTA__CUSTOMER_TRUST_ANCHOR_DATA_ID,
                                xpObject->data,
                                &xpObject->dataLen);
    if (status != E_K_STATUS_OK) {
      M_KTALOG__ERR("Failed to get CUSTOMER_TRUST_ANCHOR_DATA_ID");
      goto end;
    }

    status = salStorageGetValue(C_K_KTA__CUSTOMER_TRUST_ANCHOR_METADATA_ID,
                                xpObject->customerMetadata,
                                &xpObject->customerMetadataLen);
    if (status != E_K_STATUS_OK) {
      M_KTALOG__ERR("Failed to get CUSTOMER_TRUST_ANCHOR_METADATA_ID");
      goto end;
    }

    status = salStorageGetValue(C_K_KTA__CUSTOMER_TRUST_ANCHOR_OBJECT_UID_ID,
                                xpObject->objectUid,
                                &xpObject->objectUidLen);
    if (status != E_K_STATUS_OK) {
      M_KTALOG__ERR("Failed to get CUSTOMER_TRUST_ANCHOR_OBJECT_UID_ID");
      goto end;
    }
  }
  else if (E_K_SAL_OBJECT_TYPE_MANAGED_SLOT_5 == xIdentifier)
  {
    M_KTALOG__INFO("Fetching data for MANAGED_SLOT_5");

    status = salStorageGetValue(C_K_KTA__CUSTOMER_SYM_KEY_METADATA_ID,
                                xpObject->customerMetadata,
                                &xpObject->customerMetadataLen);
    if (status != E_K_STATUS_OK) {
      M_KTALOG__ERR("Failed to get CUSTOMER_SYM_KEY_METADATA_ID");
      goto end;
    }

    status = salStorageGetValue(C_K_KTA__CUSTOMER_SYM_KEY_OBJECT_UID_ID,
                                xpObject->objectUid,
                                &xpObject->objectUidLen);
    if (status != E_K_STATUS_OK) {
      M_KTALOG__ERR("Failed to get CUSTOMER_SYM_KEY_OBJECT_UID_ID");
      goto end;
    }
  }
  else if (E_K_SAL_OBJECT_TYPE_MANAGED_SLOT_8 == xIdentifier)
  {
    M_KTALOG__INFO("Fetching data for MANAGED_SLOT_8");

    status = salStorageGetValue(C_K_KTA__CUSTOMER_DATA_ID,
                                xpObject->data,
                                &xpObject->dataLen);
    if (status != E_K_STATUS_OK) {
      M_KTALOG__ERR("Failed to get CUSTOMER_DATA_ID");
      goto end;
    }

    status = salStorageGetValue(C_K_KTA__CUSTOMER_DATA_METADATA_ID,
                                xpObject->customerMetadata,
                                &xpObject->customerMetadataLen);
    if (status != E_K_STATUS_OK) {
      M_KTALOG__ERR("Failed to get CUSTOMER_DATA_METADATA_ID");
      goto end;
    }

    status = salStorageGetValue(C_K_KTA__CUSTOMER_DATA_OBJECT_UID_ID,
                                xpObject->objectUid,
                                &xpObject->objectUidLen);
    if (status != E_K_STATUS_OK) {
      M_KTALOG__ERR("Failed to get CUSTOMER_DATA_OBJECT_UID_ID");
      goto end;
    }
  }
  else
  {
    M_KTALOG__ERR("Unsupported xIdentifier: %d", xIdentifier);
    status = E_K_STATUS_PARAMETER;
    goto end;
  }

  status = E_K_STATUS_OK;

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Function to do the encrypt write for slot 14 and 5
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus lsalEncryptWrite
(
  uint16_t xSlot,
  uint8_t* xpData,
  size_t   xDataLen,
  uint8_t  xblock
)
{
  uint16_t     slot_addr      = 0;
  TKStatus     status         = E_K_STATUS_ERROR;
  ATCA_STATUS  cryptoStatus   = ATCA_STATUS_UNKNOWN;
  uint8_t      other_data[4]  = {0};
  uint8_t      writeBuffData[MAX_MANAGED_SLOT_DATA_SIZE] = {0};
  uint8_t      writeBuffMac[MAX_MANAGED_SLOT_DATA_SIZE] = {0};
  uint8_t      slot_14_block = 0;
  uint8_t      aRandOut[32];
  uint8_t      aNumIn[20] = {0};

  M_KTALOG__START("Start");

  other_data[0] = ATCA_GENDIG;
  other_data[1] = GENDIG_ZONE_DATA;
  other_data[2] = (uint8_t)(6 & 0xFFu);
  other_data[3] = (uint8_t)(6 >> 8u);

  // Send the GenDig command
  status = atcab_gendig(GENDIG_ZONE_DATA, 6, other_data, (uint8_t)sizeof(other_data));
  if (status != ATCA_SUCCESS)
  {
      M_KTALOG__ERR("encrWrite: GenDig failed");
      goto end;
  }
  M_KTALOG__ERR("%d", xSlot);
  if (E_K_SAL_OBJECT_TYPE_MANAGED_SLOT_5 == xSlot)
  {
    cryptoStatus = atcab_get_addr(ATCA_ZONE_DATA, E_K_SAL_OBJECT_TYPE_MANAGED_SLOT_5, 0, 0, &slot_addr);
    if (ATCA_SUCCESS != cryptoStatus)
    {
      M_KTALOG__ERR("Encrypt write failed to get addr for xSlot %d", xSlot);
      goto end;
    }

    memcpy(writeBuffData, xpData, MAX_MANAGED_SLOT_DATA_SIZE);
    memcpy(writeBuffMac, xpData+MAX_MANAGED_SLOT_DATA_SIZE, MAX_MANAGED_SLOT_DATA_SIZE);

    cryptoStatus = atcab_write(C_SAL_ENCRYPT_WRITE_ZONE,
                             slot_addr,
                             writeBuffData,
                             writeBuffMac);
    if (ATCA_SUCCESS != cryptoStatus)
    {
      M_KTALOG__ERR("Encrypt write failed for xSlot %d : %d", xSlot, cryptoStatus);
      status = cryptoStatus;
      goto end;
    }

    cryptoStatus = atcab_nonce_rand(aNumIn, aRandOut);

    if (ATCA_SUCCESS != cryptoStatus)
    {
      M_KTALOG__ERR("Atcab Nonce Rand Failed for xSlot %d : %d", xSlot, cryptoStatus);
      status = cryptoStatus;
      goto end;
    }

  }
  else if (E_K_SAL_OBJECT_TYPE_MANAGED_SLOT_14 == xSlot)
  {
    M_KTALOG__INFO("Processing Slot 14 with xIdentifier: %d", xSlot);
    if (globalEncWriteFlag == 0)
    {
      slot_14_block = E_K_SAL_OBJECT_MANAGED_SLOT_14_BLOCK_0;
      M_KTALOG__INFO("globalEncWriteFlag is 0, setting slot_14_block to 0");
    }
    else
    {
      slot_14_block = E_K_SAL_OBJECT_MANAGED_SLOT_14_BLOCK_1;
      M_KTALOG__INFO("globalEncWriteFlag is 1, setting slot_14_block to 1");
    }
    M_KTALOG__INFO("Current globalEncWriteFlag value: %d", globalEncWriteFlag);
    cryptoStatus = atcab_get_addr(ATCA_ZONE_DATA, xSlot, slot_14_block, 0, &slot_addr);
    M_KTALOG__INFO("Obtained slot address: %d", slot_addr);
    if (ATCA_SUCCESS != cryptoStatus)
    {
        M_KTALOG__ERR("Failed to get address for xSlot %d, cryptoStatus: %d", xSlot, cryptoStatus);
        goto end;
    }

    memcpy(writeBuffData, xpData, MAX_MANAGED_SLOT_DATA_SIZE);
    memcpy(writeBuffMac, xpData+MAX_MANAGED_SLOT_DATA_SIZE, MAX_MANAGED_SLOT_DATA_SIZE);

    M_KTALOG__INFO("Attempting to write to slot %d with data and MAC\r\n", slot_addr);

    cryptoStatus = atcab_write(C_SAL_ENCRYPT_WRITE_ZONE,
                               slot_addr,
                               writeBuffData,
                               writeBuffMac);
    if (ATCA_SUCCESS != cryptoStatus)
    {
      M_KTALOG__ERR("Encrypt write failed for xSlot %d\r\n, cryptoStatus: %d\r\n", xSlot, cryptoStatus);
      status = cryptoStatus;
      goto end;
    }
    else
    {
      M_KTALOG__INFO("Encrypt write successful for xSlot %d", xSlot);
      if (globalEncWriteFlag == 0)
      {
        globalEncWriteFlag = 1;
        M_KTALOG__INFO("Setting globalEncWriteFlag to 1");
      }
      else
      {
        globalEncWriteFlag = 0;
        M_KTALOG__INFO("Setting globalEncWriteFlag to 0");
      }
    }

    status = E_K_STATUS_OK;
  }

end:
  M_KTALOG__INFO("End, status : %d", status);
  return status;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

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

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

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
  uint32_t        xObjectType,
  uint32_t        xObjectId,
  const uint8_t*  xpDataAttributes,
  size_t          xDataAttributesLen,
  const uint8_t*  xpData,
  size_t          xDataLen,
  uint8_t*        xpPlatformStatus
)
{
  TKStatus     status = E_K_STATUS_ERROR;
  ATCA_STATUS  cryptoStatus = ATCA_STATUS_UNKNOWN;

  M_UNUSED(xpDataAttributes);
  M_UNUSED(xDataAttributesLen);

  M_KTALOG__START("Start");

  if ((xObjectType > C_SAL_OBJECT__TYPE_SEALED_DATA) ||
      ((xObjectId != C_KTA__DEVICE_CERT_STORAGE_SLOT) &&
       (xObjectId != C_KTA__SIGNER_CERT_STORAGE_SLOT)) ||
      (xpData == NULL) ||
      (xDataLen == 0U) ||
      (xpPlatformStatus == NULL))
  {
    M_KTALOG__ERR("Invalid parameter");
  }
  else
  {
    if (xObjectType == C_SAL_OBJECT__TYPE_CERTIFICATE)
    {
      if (C_KTA__DEVICE_CERT_STORAGE_SLOT == xObjectId)
      {
        M_KTALOG__INFO("Device cert len, %d", xDataLen);
        cryptoStatus = atcacert_write_cert(&g_cert_def_3_device,
                                           xpData,
                                           xDataLen);
      }
      else if (C_KTA__SIGNER_CERT_STORAGE_SLOT == xObjectId)
      {
        M_KTALOG__INFO("Signer cert len, %d", xDataLen);
        cryptoStatus = atcacert_write_cert(&g_cert_def_1_signer,
                                           xpData,
                                           xDataLen);
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
  uint32_t         xObjectId,
  uint8_t*         xpData,
  size_t*          xpDataLen,
  uint8_t*         xpPlatformStatus
)
{
  M_UNUSED(xObjectType);
  M_UNUSED(xObjectId);
  M_UNUSED(xpData);
  M_UNUSED(xpDataLen);
  M_UNUSED(xpPlatformStatus);
  return E_K_STATUS_OK;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

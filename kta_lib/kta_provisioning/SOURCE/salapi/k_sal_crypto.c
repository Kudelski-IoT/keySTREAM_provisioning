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
/** \brief  SAL crypto for microchip.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_crypto.c
 ******************************************************************************/

/**
 * @brief SAL crypto for microchip.
 */

#include "k_sal_crypto.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */

#include "k_defs.h"
#include "k_sal.h"
#include "k_sal_storage.h"
#include "slotConfig.h"
#include "KTALog.h"
#include "atca_status.h"
#include "atca_basic.h"
#include "atcacert/atcacert_client.h"
#include "atcacert/atcacert_pem.h"
#include "host/atca_host.h"

#include <string.h>

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Mac key length. */
#define C_SAL_CRYPTO_MAC_KEY_LENGTH                   (32u)

/** @brief Activation key length. */
#define C_SAL_CRYPTO_ACT_KEY_LENGTH                   (32u)

/** @brief Shared secret bit mask. */
#define C_SAL_CRYPTO_SHARED_SECRET_BITMASK            (0x80000000u)

/** @brief Key length 64 bytes. */
#define C_SAL_CRYPTO_KEY_SIZE_64_BYTE                 (64u)

/** @brief Key length 32 bytes. */
#define C_SAL_CRYPTO_KEY_SIZE_32_BYTE                 (32u)

/** @brief Key length 16 bytes. */
#define C_SAL_CRYPTO_KEY_SIZE_16_BYTE                 (16u)

/** @brief Maximum size of fixed info in bytes. */
#define C_SAL_CRYPTO_MAX_INFO_SIZE_BYTE               (84u)

/** @brief Maximum no of bytes for random number generation. */
#define C_SAL_CRYPTO_MAX_RANDOM_GEN_SIZE_BYTES        (32u)

/** @brief Random num in size for random nonce creation. */
#define C_SAL_CRYPTO_RANDOM_NUM_IN_SIZE               (20u)

/** @brief Other data size for digest calculation. */
#define C_SAL_CRYPTO_OTHER_DATA_SIZE                  (3u)

/** @brief Chip public key length. */
#define C_SAL_CRYPTO_PUBLIC_KEY_LENGTH                (64)

/** @brief Chip signature key length. */
#define C_SAL_CRYPTO_SIGNATURE_KEY_LENGTH             (64)

/** @brief Chip certificate tag length. */
#define C_SAL_CRYPTO_CHIP_CERT_TAG_LENGTH             (1u)

/** @brief Chip certificate tag value length in bytes. */
#define C_SAL_CRYPTO_CHIP_CERT_TAG_VALUE_LENGTH       (2u)

/** @brief Chip certificate tag and tag value length. */
#define C_SAL_CRYPTO_CHIP_CERT_TAG_AND_VAL_LENGTH     (C_SAL_CRYPTO_CHIP_CERT_TAG_LENGTH + \
                                                       C_SAL_CRYPTO_CHIP_CERT_TAG_VALUE_LENGTH)

/** @brief Chip certificate signature offset. */
#define C_SAL_CRYPTO_CHIP_CERT_SIGN_OFFSET            (C_SAL_CRYPTO_PUBLIC_KEY_LENGTH + \
                                                       C_SAL_CRYPTO_CHIP_CERT_TAG_AND_VAL_LENGTH)

/** @brief Random value offset. */
#define C_SAL_CRYPTO_RANDOM_VALUE_OFFSET              (C_SAL_CRYPTO_CHIP_CERT_SIGN_OFFSET \
                                                       + C_SAL_CRYPTO_SIGNATURE_KEY_LENGTH)

/** @brief IV to perform encryption. */
#define C_SAL_CRYPTO_IV                               {0xA9, 0x32, 0x30, 0x31, \
    0x38, 0x4E, 0x61, 0x67, \
    0x72, 0x61, 0x76, 0x69, \
    0x73, 0x69, 0x6F, 0x6E }

/** @brief Get the value of bit at 31. */
#define M_SAL_CRYPTO_GET_BIT31(x_shared_secret) \
  ((uint32_t)(((uint32_t)(x_shared_secret)) & (uint32_t)C_SAL_CRYPTO_SHARED_SECRET_BITMASK) == \
   (uint32_t)C_SAL_CRYPTO_SHARED_SECRET_BITMASK)

/**
 * @brief Object Id Map.
 */
typedef struct
{
  uint16_t  virtualObjId;
  /* Virtual object ID */
  uint8_t   aObjectIdData[C_SAL_CRYPTO_KEY_SIZE_32_BYTE];
  /* Data mapped to object ID */
} TKSalObjectIdMap;

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/* Module name used for logging */
static const char* gpModuleName = "SALCRYPTO";

static TKSalObjectIdMap gaSalObjectIdMapTable[] =
{
  {
    C_K_KTA__CHIP_SK_ID, {
      /* Private secret key slot */
      C_KTA__CHIP_SK_STORAGE_SLOT, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {C_K_KTA__VOLATILE_ID, {0}},
  {C_K_KTA__VOLATILE_2_ID, {0}},
  {C_K_KTA__VOLATILE_3_ID, {0}},
};

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   Set the value using the ID
 *
 * @param[in] xObjectId
 *   Object identifier.
 * @param[in] xpValue
 *   Key data buffer. Should not be NULL.
 * @param[in] xValueLen
 *   Length of the buffer xpValue.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lSetValueById
(
  uint32_t  xObjectId,
  uint8_t*  xpValue,
  size_t    xValueLen
);

/**
 * @brief
 *   Get the value using the ID
 *
 * @param[in] xObjectId
 *   Object identifier.
 * @param[out] xpValue
 *   Buffer to load the key data. Should not be NULL.
 * @param[in] xValueLen
 *   Length of the buffer xpValue.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lGetValueById
(
  uint32_t  xObjectId,
  uint8_t*  xpValue,
  size_t    xValueLen
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief implement salRotGetChipUID
 *
 */
K_SAL_API TKStatus salRotGetChipUID
(
  uint8_t*  xpChipUid,
  size_t*   xpChipUidLen
)
{
  TKStatus  status = E_K_STATUS_ERROR;
  ATCA_STATUS atcaStatus = ATCA_SUCCESS;

  M_KTALOG__START("Start");

  if ((NULL == xpChipUid) ||
      (NULL == xpChipUidLen) ||
      (0u == *xpChipUidLen) ||
      (C_K_KTA__CHIPSET_UID_MAX_SIZE > *xpChipUidLen))
  {
    M_KTALOG__ERR("Invalid parameter");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    /* Reading serial number into the buffer */
    atcaStatus = atcab_read_serial_number(xpChipUid);

    if (ATCA_SUCCESS != atcaStatus)
    {
      M_KTALOG__ERR("atcab_read_serial_number Failed[%d]", atcaStatus);
    }
    else
    {
      *xpChipUidLen = strlen((const char*)xpChipUid);
      M_KTALOG__HEX("Chip Uid:", xpChipUid, *xpChipUidLen);
      status = E_K_STATUS_OK;
    }
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement salRotGetChipCertificate
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salRotGetChipCertificate
(
  uint8_t*  xpChipCert,
  size_t*   xpChipCertLen
)
{
  TKStatus            status = E_K_STATUS_ERROR;
  ATCA_STATUS         cryptoStatus = ATCA_STATUS_UNKNOWN;
  uint16_t            slotSign = C_SAL__CRYPTO_SLOT_FOR_ATTESTAION;
  uint16_t            slotChipSK = C_KTA__CHIP_SK_STORAGE_SLOT;
  uint8_t             aSignature[C_SAL_CRYPTO_KEY_SIZE_64_BYTE];
  uint8_t             aPublicKey[C_SAL_CRYPTO_KEY_SIZE_64_BYTE];
  uint8_t             aNumIn[C_SAL_CRYPTO_RANDOM_NUM_IN_SIZE];
  uint8_t             aRandOut[C_SAL_CRYPTO_KEY_SIZE_32_BYTE];
  size_t              tmpLen = 0;
  atca_temp_key_t     tempKey;
  atca_nonce_in_out_t nonceParams;

  M_KTALOG__START("Start");

  if ((NULL == xpChipCert) ||
      (NULL == xpChipCertLen) ||
      (0u == *xpChipCertLen) ||
      (C_K_KTA__CHIP_CERT_MAX_SIZE > *xpChipCertLen))
  {
    M_KTALOG__ERR("Invalid parameter");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    xpChipCert[0] = 0xF6;
    xpChipCert[1] = 0x00;
    xpChipCert[2] = 0xA0;
    (void)memset(aNumIn, 0x00, C_SAL_CRYPTO_RANDOM_NUM_IN_SIZE);

    cryptoStatus = atcab_nonce_rand(aNumIn, aRandOut);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_nonce_rand() failed with ret=0x%08X", cryptoStatus);
      goto end;
    }

    cryptoStatus = atcab_genkey(slotChipSK, aPublicKey);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_genkey() failed with ret=0x%08X", cryptoStatus);
      goto end;
    }

    cryptoStatus = atcab_genkey_base(GENKEY_MODE_DIGEST, slotChipSK, NULL, NULL);

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

    (void)memcpy(&xpChipCert[C_SAL_CRYPTO_CHIP_CERT_TAG_AND_VAL_LENGTH], aPublicKey,
                 C_SAL_CRYPTO_KEY_SIZE_64_BYTE);
    (void)memcpy(&xpChipCert[C_SAL_CRYPTO_CHIP_CERT_SIGN_OFFSET], aSignature,
                 C_SAL_CRYPTO_KEY_SIZE_64_BYTE);

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

    (void)memcpy(&xpChipCert[C_SAL_CRYPTO_RANDOM_VALUE_OFFSET], nonceParams.temp_key->value,
                 C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

    tmpLen = C_SAL_CRYPTO_CHIP_CERT_TAG_AND_VAL_LENGTH + sizeof(aPublicKey) + sizeof(aSignature) +
             C_SAL_CRYPTO_KEY_SIZE_32_BYTE;

    status = E_K_STATUS_OK;
  }

end:
  *xpChipCertLen = tmpLen;

  M_KTALOG__END("End, status : %d", status);

  return status;
}

/**
 * @brief implement salRotKeyPairGeneration
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salRotKeyPairGeneration
(
  uint8_t*  xpPublicKey
)
{
  TKStatus              status = E_K_STATUS_ERROR;
  ATCA_STATUS           cryptoStatus = ATCA_STATUS_UNKNOWN;
  uint8_t               aPublicKey[C_K_KTA__PUBLIC_KEY_MAX_SIZE] = {0};
  uint8_t               privateKeyId = C_KTA__GEN_KEY_PAIR_SLOT;

  M_KTALOG__START("Start");

  if (NULL == xpPublicKey)
  {
    M_KTALOG__ERR("Invalid parameter");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    cryptoStatus = atcab_genkey(privateKeyId, aPublicKey);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_genkey() failed with ret=0x%08X", cryptoStatus);
      goto end;
    }

    if (lSetValueById(C_K_KTA__VOLATILE_ID, &privateKeyId, 1) != E_K_STATUS_OK)
    {
      M_KTALOG__ERR("lSetValueById Failed");
      goto end;
    }

    (void)memcpy(xpPublicKey, aPublicKey, C_SAL_CRYPTO_KEY_SIZE_64_BYTE);
    M_KTALOG__HEX("Public Key:", xpPublicKey, C_SAL_CRYPTO_KEY_SIZE_64_BYTE);

    status = E_K_STATUS_OK;
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement salRotKeyAgreement
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salRotKeyAgreement
(
  uint32_t        xPrivateKeyId,
  const uint8_t*  xpPeerPublicKey,
  uint32_t        xSharedSecretTarget,
  uint8_t*        xpSharedSecret
)
{
  TKStatus          status = E_K_STATUS_ERROR;
  ATCA_STATUS       cryptoStatus = ATCA_STATUS_UNKNOWN;
  uint32_t          exposeSecret = (uint32_t)(M_SAL_CRYPTO_GET_BIT31(xSharedSecretTarget));
  uint8_t           aSecret[C_SAL_CRYPTO_KEY_SIZE_32_BYTE];
  uint8_t           keyID;

  M_KTALOG__START("Start");

  if (((C_K_KTA__CHIP_SK_ID != xPrivateKeyId) && (C_K_KTA__VOLATILE_ID != xPrivateKeyId)) ||
      (NULL == xpPeerPublicKey) ||
      ((1u == exposeSecret) && (NULL == xpSharedSecret)) ||
      ((0u == exposeSecret) &&
      ((xSharedSecretTarget != C_K_KTA__VOLATILE_2_ID) || (NULL != xpSharedSecret))))
  {
    M_KTALOG__ERR("Invalid parameter");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    if (lGetValueById(xPrivateKeyId, &keyID, 1) != E_K_STATUS_OK)
    {
      M_KTALOG__ERR("lGetValueById Failed");
      goto end;
    }

    /* Output to volatile key identifier */
    if (exposeSecret == 0u)
    {
      /**
       *  ECDH Key Agreement is the first phase of a common operation with HKDF Key Derivation
       *  initialize operation.
       */
      cryptoStatus = atcab_ecdh_base(ECDH_MODE_COPY_OUTPUT_BUFFER,
                                     keyID, xpPeerPublicKey, aSecret, NULL);

      if (cryptoStatus != ATCA_SUCCESS)
      {
        M_KTALOG__ERR("atcab_ecdh_base() failed with ret=0x%08X", cryptoStatus);
        goto end;
      }

      M_KTALOG__INFO("Generated shared secret for act Req");
      M_KTALOG__HEX("Secret:", aSecret, C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

      if (lSetValueById(xSharedSecretTarget,
                        aSecret, C_SAL_CRYPTO_KEY_SIZE_32_BYTE) != E_K_STATUS_OK)
      {
        M_KTALOG__ERR("lSetValueById Failed");
        goto end;
      }
    }
    else  /* Export to buffer */
    {
      cryptoStatus = atcab_ecdh_base(ECDH_MODE_COPY_OUTPUT_BUFFER,
                                     (uint16_t)keyID, xpPeerPublicKey, xpSharedSecret, NULL);

      if (cryptoStatus != ATCA_SUCCESS)
      {
        M_KTALOG__ERR("atcab_ecdh_base() failed with ret=0x%08X", cryptoStatus);
        goto end;
      }

      M_KTALOG__INFO("Generated shared secret for reg request");
      M_KTALOG__HEX("SharedSecret:", xpSharedSecret, C_SAL_CRYPTO_KEY_SIZE_32_BYTE);
    }

    status = E_K_STATUS_OK;
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement salRotHkdfExtractAndExpand
 *
 */
K_SAL_API TKStatus salRotHkdfExtractAndExpand
(
  uint32_t        xMode,
  const uint8_t*  xpSecret,
  const uint8_t*  xpSalt,
  const uint8_t*  xpInfo,
  size_t          xInfoLen
)
{
  TKStatus              status = E_K_STATUS_ERROR;
  ATCA_STATUS           cryptoStatus = ATCA_STATUS_UNKNOWN;
  uint8_t               aSecret[C_SAL_CRYPTO_KEY_SIZE_32_BYTE];
  uint8_t               aL1Key[C_SAL_CRYPTO_KEY_SIZE_32_BYTE];
  uint32_t              genMsgLen = C_SAL_CRYPTO_KEY_SIZE_64_BYTE;
  uint8_t               aSalt[C_SAL_CRYPTO_KEY_SIZE_32_BYTE];
  uint8_t               aFixedInfo[C_SAL_CRYPTO_MAX_INFO_SIZE_BYTE] = { 0 };
  size_t                l1KeySize = sizeof(aL1Key);
  #ifdef __free_rtos__
  atcac_hmac_sha256_ctx ctx = { 0 };
  #else
  atcac_hmac_ctx_t      ctx;
  atcac_sha2_256_ctx_t  sha256_ctx;
  #endif
  size_t                infoLen = xInfoLen;

  M_KTALOG__START("Start");

  if (((C_K_KTA__HKDF_ACT_MODE != xMode) && (C_K_KTA__HKDF_GEN_MODE != xMode)) ||
      ((C_K_KTA__HKDF_GEN_MODE == xMode) && (NULL == xpSecret)) ||
      (NULL == xpSalt) ||
      (0u == infoLen) ||
      (NULL == xpInfo))
  {
    M_KTALOG__ERR("Invalid parameter");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    M_KTALOG__INFO("salRotHkdfExtractAndExpand");
    M_KTALOG__HEX("Info:", xpInfo, infoLen);

    if (xpSecret != NULL)
    {
      M_KTALOG__INFO("salRotHkdfExtractAndExpand");
      M_KTALOG__HEX("Secret:", xpSecret, C_SAL_CRYPTO_KEY_SIZE_64_BYTE);
    }

    /* Appending 0x01 to the fixed info to match with the keySTREAM key generation. */
    (void)memcpy(aFixedInfo, xpInfo, infoLen);
    aFixedInfo[infoLen] = 0x01;
    infoLen++;

    switch (xMode)
    {
      case C_K_KTA__HKDF_ACT_MODE:
      {
        M_KTALOG__INFO("salRotHkdfExtractAndExpand");
        M_KTALOG__HEX("Salt:", xpSalt, C_SAL_CRYPTO_KEY_SIZE_64_BYTE);

        if (lGetValueById(C_K_KTA__VOLATILE_2_ID,
                          aSecret, C_SAL_CRYPTO_KEY_SIZE_32_BYTE) != E_K_STATUS_OK)
        {
          M_KTALOG__ERR("lGetValueById Failed");
          break;
        }
#ifdef __free_rtos__
		    cryptoStatus = atcac_sha256_hmac_init(&ctx, xpSalt, C_SAL_CRYPTO_KEY_SIZE_64_BYTE);
#else
        cryptoStatus = atcac_sha256_hmac_init(&ctx, &sha256_ctx, xpSalt, C_SAL_CRYPTO_KEY_SIZE_64_BYTE);
#endif

        if (cryptoStatus != ATCA_SUCCESS)
        {
          M_KTALOG__ERR("atcac_sha256_hmac_init failed, ret=0x%08X", cryptoStatus);
          break;
        }

        cryptoStatus = atcac_sha256_hmac_update(&ctx, aSecret, C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

        if (cryptoStatus != ATCA_SUCCESS)
        {
          M_KTALOG__ERR("atcac_sha256_hmac_update failed, ret=0x%08X", cryptoStatus);
          break;
        }

        cryptoStatus = atcac_sha256_hmac_finish(&ctx, aL1Key, &l1KeySize);

        if (cryptoStatus != ATCA_SUCCESS)
        {
          M_KTALOG__ERR("atcac_sha256_hmac_finish failed, ret=0x%08X", cryptoStatus);
          break;
        }

        cryptoStatus = atcab_nonce_load(NONCE_MODE_TARGET_TEMPKEY, aL1Key, (uint16_t)l1KeySize);

        if (cryptoStatus != ATCA_SUCCESS)
        {
          M_KTALOG__ERR("atcab_nonce_load()  failed with ret=0x%08X", cryptoStatus);
          break;
        }

        M_KTALOG__INFO("l1 key in expand for activation request:");
        M_KTALOG__HEX("L1 Key:", aL1Key, l1KeySize);
        /* HKDF Expand */
        cryptoStatus = atcab_kdf(
                         KDF_MODE_ALG_HKDF | KDF_MODE_SOURCE_TEMPKEY | KDF_MODE_TARGET_OUTPUT,
                         0x0000,
                         KDF_DETAILS_HKDF_MSG_LOC_INPUT | (infoLen << 24),
                         aFixedInfo,
                         aL1Key,
                         NULL);

        if (cryptoStatus != ATCA_SUCCESS)
        {
          M_KTALOG__ERR("atcab_kdf() for expand failed with ret=0x%08X", cryptoStatus);
          break;
        }

        M_KTALOG__INFO("L1 key in extract for activation request:");
        M_KTALOG__HEX("L1 Key:", aL1Key, l1KeySize);

        if (lSetValueById(C_K_KTA__VOLATILE_2_ID,
                          aL1Key, C_SAL_CRYPTO_KEY_SIZE_32_BYTE) != E_K_STATUS_OK)
        {
          M_KTALOG__INFO("lSetValueById Failed");
          break;
        }

        status = E_K_STATUS_OK;
      }
      break;

      case C_K_KTA__HKDF_GEN_MODE:
      {
        M_KTALOG__INFO("salRotHkdfExtractAndExpand");
        M_KTALOG__HEX("Salt:", xpSalt, C_K_KTA__HKDF_GEN_SALT_MAX_SIZE);
        (void)memset(aSalt, 0x00, 32);
        (void)memcpy(aSalt, xpSalt, 16);
        cryptoStatus = atcab_nonce_load(NONCE_MODE_TARGET_TEMPKEY,
                                        aSalt, C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

        if (cryptoStatus != ATCA_SUCCESS)
        {
          M_KTALOG__ERR("atcab_nonce_load()  failed with ret=0x%08X", cryptoStatus);
          break;
        }

        /* HKDF Extract */
        cryptoStatus = atcab_kdf(
                         KDF_MODE_ALG_HKDF | KDF_MODE_SOURCE_TEMPKEY | KDF_MODE_TARGET_TEMPKEY,
                         0x0000,
                         KDF_DETAILS_HKDF_MSG_LOC_INPUT | (genMsgLen << 24),
                         xpSecret,
                         NULL,
                         NULL);

        if (cryptoStatus != ATCA_SUCCESS)
        {
          M_KTALOG__ERR("atcab_kdf() for extract failed with ret=0x%08X", cryptoStatus);
          break;
        }

        /* HKDF Expand */
        cryptoStatus = atcab_kdf(
                         KDF_MODE_ALG_HKDF | KDF_MODE_SOURCE_TEMPKEY | KDF_MODE_TARGET_SLOT,
                         C_KTA__KDF_KEY_ID,
                         KDF_DETAILS_HKDF_MSG_LOC_INPUT | (infoLen << 24),
                         aFixedInfo,
                         NULL,
                         NULL);

        if (cryptoStatus != ATCA_SUCCESS)
        {
          M_KTALOG__ERR("atcab_kdf() for expand failed with ret=0x%08X", cryptoStatus);
          break;
        }

        M_KTALOG__INFO("L1 key in extract for registration request:");
        M_KTALOG__HEX("L1 Key:", aL1Key, l1KeySize);
        status = E_K_STATUS_OK;
      }
      break;

      default:
      {
        M_KTALOG__ERR("Invalid mode %d", xMode);
      }
      break;
    }
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement salRotKeyDerivation
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salRotKeyDerivation
(
  uint32_t        xKeyId,
  const uint8_t*  xpInputData,
  size_t          xInputDataLen,
  uint32_t        xDerivedKeyId
)
{
  TKStatus      status = E_K_STATUS_ERROR;
  ATCA_STATUS   cryptoStatus = ATCA_STATUS_UNKNOWN;
  uint8_t       aKeyBuff[C_SAL_CRYPTO_KEY_SIZE_32_BYTE] = { 0 };

  M_KTALOG__START("Start");

  if (((C_K_KTA__VOLATILE_2_ID != xKeyId) && (C_K_KTA__L1_FIELD_KEY_ID != xKeyId)) ||
      (0u == xInputDataLen) ||
      (NULL == xpInputData) ||
      ((C_K_KTA__VOLATILE_2_ID != xDerivedKeyId) && (C_K_KTA__VOLATILE_3_ID != xDerivedKeyId)))
  {
    M_KTALOG__ERR("Invalid parameter");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    M_KTALOG__INFO("xpInputData for key Id: %x", xKeyId);
    M_KTALOG__HEX("InputData:", xpInputData, xInputDataLen);

    if (xKeyId == C_K_KTA__VOLATILE_2_ID)
    {
      if (lGetValueById(xKeyId, aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE) != E_K_STATUS_OK)
      {
        M_KTALOG__ERR("lGetValueById Failed");
        goto end;
      }

      M_KTALOG__INFO("l1 key in key derivation for activation request:");
      M_KTALOG__HEX("L1 Key:", aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE);
      cryptoStatus = atcab_nonce_load(NONCE_MODE_TARGET_TEMPKEY, aKeyBuff,
                                      C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

      if (cryptoStatus != ATCA_SUCCESS)
      {
        M_KTALOG__ERR("atcab_nonce_load()  failed with ret=0x%08X", cryptoStatus);
        goto end;
      }

      (void)memset(aKeyBuff, 0x00, C_SAL_CRYPTO_KEY_SIZE_32_BYTE);
      cryptoStatus = atcab_sha_hmac(xpInputData, xInputDataLen,
                                    ATCA_TEMPKEY_KEYID, aKeyBuff, SHA_MODE_TARGET_OUT_ONLY);

      if (cryptoStatus != ATCA_SUCCESS)
      {
        M_KTALOG__ERR("atcab_sha_hmac() failed with ret=0x%08X", cryptoStatus);
        goto end;
      }

      M_KTALOG__INFO("Derived L2 key for activation request:");
      M_KTALOG__HEX("L2 Key:", aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

      if (lSetValueById(xDerivedKeyId,
                        aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE) != E_K_STATUS_OK)
      {
        M_KTALOG__ERR("lSetValueById Failed");
        goto end;
      }
      status = E_K_STATUS_OK;
    }
    else if (xKeyId == C_K_KTA__L1_FIELD_KEY_ID)
    {
      cryptoStatus = atcab_sha_hmac(xpInputData, xInputDataLen, C_KTA__L1_FIELD_KEY_STORAGE_SLOT,
                                    aKeyBuff, SHA_MODE_TARGET_OUT_ONLY);

      if (cryptoStatus != ATCA_SUCCESS)
      {
        M_KTALOG__ERR("atcab_sha_hmac() failed with ret=0x%08X", cryptoStatus);
        goto end;
      }

      M_KTALOG__INFO("Derived l2 key for registration request:");
      M_KTALOG__HEX("L2 Key:", aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

      if (lSetValueById(xDerivedKeyId, aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE) != E_K_STATUS_OK)
      {
        M_KTALOG__ERR("lSetValueById Failed");
        goto end;
      }
      status = E_K_STATUS_OK;
    }
    else
    {
      /* Added for Misra */
      M_KTALOG__ERR("Invalid Id");
      status = E_K_STATUS_PARAMETER;
    }
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement salCryptoHmac
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salCryptoHmac
(
  uint32_t        xKeyId,
  const uint8_t*  xpInputData,
  size_t          xInputDataLen,
  uint8_t*        xpMac
)
{
  TKStatus          status = E_K_STATUS_ERROR;
  ATCA_STATUS       cryptoStatus = ATCA_STATUS_UNKNOWN;
  uint8_t           aKeyBuff[C_SAL_CRYPTO_KEY_SIZE_32_BYTE] = { 0 };
  uint8_t           aMac[C_SAL_CRYPTO_KEY_SIZE_32_BYTE] = { 0 };

  M_KTALOG__START("Start");

  if ((C_K_KTA__VOLATILE_2_ID != xKeyId) ||
      (0u == xInputDataLen) ||
      (NULL == xpInputData) ||
      (NULL == xpMac))
  {
    M_KTALOG__ERR("Invalid parameter");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    if (lGetValueById(xKeyId, aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE) != E_K_STATUS_OK)
    {
      M_KTALOG__ERR("lGetValueById Failed");
      goto end;
    }

    M_KTALOG__INFO("L2 key for HMAC");
    M_KTALOG__HEX("L2 Key:", aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE);
    M_KTALOG__HEX("InputData:", xpInputData, xInputDataLen);
    /* Truncate the key */
    (void)memset(&aKeyBuff[C_SAL_CRYPTO_KEY_SIZE_16_BYTE], 0x00, C_SAL_CRYPTO_KEY_SIZE_16_BYTE);

    cryptoStatus =  atcab_nonce_load(NONCE_MODE_TARGET_TEMPKEY, aKeyBuff,
                                     C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_nonce_load() with ret=0x%08X", cryptoStatus);
      goto end;
    }

    cryptoStatus = atcab_sha_hmac(xpInputData, xInputDataLen,
                                  ATCA_TEMPKEY_KEYID, aMac, SHA_MODE_TARGET_OUT_ONLY);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_sha_hmac() with ret=0x%08X", cryptoStatus);
      goto end;
    }

    (void)memcpy(xpMac, aMac, C_K_KTA__HMAC_MAX_SIZE);
    M_KTALOG__HEX("MAC:", xpMac, C_K_KTA__HMAC_MAX_SIZE);

    status = E_K_STATUS_OK;
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement salCryptoHmacVerify
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salCryptoHmacVerify
(
  uint32_t          xKeyId,
  const uint8_t*    xpInputData,
  size_t            xInputDataLen,
  const uint8_t*    xpMac
)
{
  TKStatus          status = E_K_STATUS_ERROR;
  ATCA_STATUS       cryptoStatus = ATCA_STATUS_UNKNOWN;
  uint8_t           aKeyBuff[C_SAL_CRYPTO_KEY_SIZE_32_BYTE] = { 0 };
  uint8_t           aMac[C_SAL_CRYPTO_KEY_SIZE_32_BYTE] = { 0 };

  M_KTALOG__START("Start");

  if ((C_K_KTA__VOLATILE_2_ID != xKeyId) ||
      (0u == xInputDataLen) ||
      (NULL == xpInputData) ||
      (NULL == xpMac))
  {
    M_KTALOG__ERR("Invalid parameter");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    if (lGetValueById(xKeyId, aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE) != E_K_STATUS_OK)
    {
      M_KTALOG__ERR("lGetValueById Failed");
      goto end;
    }

    M_KTALOG__INFO("L2 key for HMAC verification");
    M_KTALOG__HEX("L2 Key:", aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE);
    /* Truncate the key  to 16 bytes */
    (void)memset(&aKeyBuff[C_SAL_CRYPTO_KEY_SIZE_16_BYTE], 0x00, C_SAL_CRYPTO_KEY_SIZE_16_BYTE);

    cryptoStatus =  atcab_nonce_load(NONCE_MODE_TARGET_TEMPKEY, aKeyBuff,
                                     C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_nonce_load() with ret=0x%08X", cryptoStatus);
      goto end;
    }

    M_KTALOG__INFO("Input data for hmac verify ");
    M_KTALOG__HEX("Input Data:", xpInputData, xInputDataLen);

    cryptoStatus = atcab_sha_hmac(xpInputData, xInputDataLen,
                                  ATCA_TEMPKEY_KEYID, aMac, SHA_MODE_TARGET_OUT_ONLY);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_sha_hmac() with ret=0x%08X", cryptoStatus);
      goto end;
    }

    M_KTALOG__HEX("Mac:", aMac, C_K_KTA__HMAC_MAX_SIZE);

    if (!memcmp(aMac, xpMac, C_K_KTA__HMAC_MAX_SIZE))
    {
      status = E_K_STATUS_OK;
    }
    else
    {
      M_KTALOG__ERR("MAC not OK");
    }
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement salCryptoAesEnc
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salCryptoAesEnc
(
  uint32_t        xKeyId,
  const uint8_t*  xpInputData,
  size_t          xInputDataLen,
  uint8_t*        xpOutputData,
  size_t*         xpOutputDataLen
)
{
  TKStatus            status = E_K_STATUS_ERROR;
  ATCA_STATUS         cryptoStatus = ATCA_STATUS_UNKNOWN;
  uint8_t             aKeyBuff[C_SAL_CRYPTO_KEY_SIZE_32_BYTE] = { 0 };
  uint16_t            dataBlock;
  atca_aes_cbc_ctx_t  ctx;
  uint8_t             aIV[] = C_SAL_CRYPTO_IV;

  M_KTALOG__START("Start");

  if ((C_K_KTA__VOLATILE_3_ID != xKeyId) ||
      (0u == xInputDataLen) ||
      (NULL == xpInputData) ||
      (NULL == xpOutputData) ||
      (NULL == xpOutputDataLen))
  {
    M_KTALOG__ERR("Invalid parameter");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    if (lGetValueById(xKeyId,
                      aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE) != E_K_STATUS_OK)
    {
      M_KTALOG__ERR("lGetValueById Failed");
      goto end;
    }

    M_KTALOG__HEX("L2 key for HMAC verification:", aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE);
    M_KTALOG__HEX("Input Data for encryption :", xpInputData, xInputDataLen);
    cryptoStatus =  atcab_nonce_load(NONCE_MODE_TARGET_TEMPKEY, aKeyBuff,
                                     C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_nonce_load() with ret=0x%08X", cryptoStatus);
      goto end;
    }

    cryptoStatus = atcab_aes_cbc_init(&ctx, ATCA_TEMPKEY_KEYID, 0, aIV);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_aes_cbc_init() with ret=0x%08X", cryptoStatus);
      goto end;
    }

    /* Encrypt blocks */
    for (dataBlock = 0; dataBlock < (xInputDataLen / ATCA_AES128_BLOCK_SIZE); dataBlock++)
    {
      cryptoStatus = atcab_aes_cbc_encrypt_block(&ctx,
                                                 &xpInputData[dataBlock * ATCA_AES128_BLOCK_SIZE],
                                                 &xpOutputData[dataBlock * ATCA_AES128_BLOCK_SIZE]);

      if (cryptoStatus != ATCA_SUCCESS)
      {
        M_KTALOG__ERR("atcab_aes_cbc_encrypt_block() with ret=0x%08X", cryptoStatus);
        break;
      }
    }

    if (cryptoStatus == ATCA_SUCCESS)
    {
      status = E_K_STATUS_OK;
      *xpOutputDataLen = dataBlock * ATCA_AES128_BLOCK_SIZE;
    }
  }

end:
  M_KTALOG__HEX("Output Data:", xpOutputData, *xpOutputDataLen);

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement salCryptoAesDec
 *
 */
K_SAL_API TKStatus salCryptoAesDec
(
  uint32_t        xKeyId,
  const uint8_t*  xpInputData,
  size_t          xInputDataLen,
  uint8_t*        xpOutputData,
  size_t*         xpOutputDataLen
)
{
  TKStatus            status = E_K_STATUS_ERROR;
  ATCA_STATUS         cryptoStatus = ATCA_STATUS_UNKNOWN;
  uint8_t             aKeyBuff[C_SAL_CRYPTO_KEY_SIZE_32_BYTE] = { 0 };
  uint16_t            dataBlock;
  atca_aes_cbc_ctx_t  ctx;
  uint8_t             aIV[] = C_SAL_CRYPTO_IV;

  M_KTALOG__START("Start");

  for (;;)
  {
    if (
      (C_K_KTA__VOLATILE_3_ID != xKeyId) ||
      (0u == xInputDataLen) ||
      (NULL == xpInputData) ||
      (NULL == xpOutputData) ||
      (NULL == xpOutputDataLen)
    )
    {
      M_KTALOG__ERR("Invalid parameter");
      status = E_K_STATUS_PARAMETER;
      break;
    }

    if (lGetValueById(xKeyId, aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE) != E_K_STATUS_OK)
    {
      M_KTALOG__ERR("lGetValueById Failed");
      break;
    }

    M_KTALOG__INFO("L2 key for AES dec");
    M_KTALOG__HEX("L2 Key:", aKeyBuff, C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

    M_KTALOG__HEX("Input Data:", xpInputData, xInputDataLen);
    cryptoStatus = atcab_nonce_load(NONCE_MODE_TARGET_TEMPKEY, aKeyBuff,
                                    C_SAL_CRYPTO_KEY_SIZE_32_BYTE);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_nonce_load() with ret=0x%08X", cryptoStatus);
      break;
    }

    cryptoStatus = atcab_aes_cbc_init(&ctx, ATCA_TEMPKEY_KEYID, 0, aIV);

    if (cryptoStatus != ATCA_SUCCESS)
    {
      M_KTALOG__ERR("atcab_aes_cbc_init() with ret=0x%08X", cryptoStatus);
      break;
    }

    /* Decrypt blocks */
    for (dataBlock = 0; dataBlock < (xInputDataLen / ATCA_AES128_BLOCK_SIZE); dataBlock++)
    {
      cryptoStatus = atcab_aes_cbc_decrypt_block(&ctx,
                                                 &xpInputData[dataBlock * ATCA_AES128_BLOCK_SIZE],
                                                 &xpOutputData[dataBlock * ATCA_AES128_BLOCK_SIZE]);

      if (cryptoStatus != ATCA_SUCCESS)
      {
        M_KTALOG__ERR("atcab_aes_cbc_decrypt_block() with ret=0x%08X", cryptoStatus);
        break;
      }
    }

    if (cryptoStatus == ATCA_SUCCESS)
    {
      *xpOutputDataLen = dataBlock * ATCA_AES128_BLOCK_SIZE;
      M_KTALOG__HEX("Output Data:", xpOutputData, *xpOutputDataLen);
      status = E_K_STATUS_OK;
    }

    break;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement salCryptoGetRandom
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salCryptoGetRandom
(
  uint8_t* xpRandomData,
  size_t*  xpRandomDataLen
)
{
  TKStatus      status = E_K_STATUS_ERROR;
  ATCA_STATUS   randomStatus = ATCA_SUCCESS;
  uint8_t       aTmpRandom[C_SAL_CRYPTO_MAX_RANDOM_GEN_SIZE_BYTES] = { 0 };
  uint32_t      loopCount = 0;
  size_t        tmpRandomLen = 0;
  uint8_t*      pRandomData = xpRandomData;

  M_KTALOG__START("Start");

  if ((NULL == pRandomData) ||
      (NULL == xpRandomDataLen) ||
      (0u == *xpRandomDataLen) ||
      (C_K_KTA__RANDOM_MAX_SIZE < *xpRandomDataLen))
  {
    M_KTALOG__ERR("Invalid parameter");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    tmpRandomLen = *xpRandomDataLen;
    loopCount = *xpRandomDataLen / sizeof(aTmpRandom);

    while (loopCount > 0u)
    {
      randomStatus = atcab_random(pRandomData);

      if (randomStatus != ATCA_SUCCESS)
      {
        M_KTALOG__ERR("atcab_random() with ret=0x%08X", randomStatus);
        goto end;
      }

      pRandomData = &pRandomData[sizeof(aTmpRandom)];
      tmpRandomLen -= sizeof(aTmpRandom);
      --loopCount;
    }

    if (randomStatus != ATCA_SUCCESS)
    {
      *xpRandomDataLen = 0;
      goto end;
    }

    if (tmpRandomLen > 0u)
    {
      randomStatus = atcab_random(aTmpRandom);

      if (randomStatus != ATCA_SUCCESS)
      {
        M_KTALOG__ERR("atcab_random() with ret=0x%08X", randomStatus);
        *xpRandomDataLen = 0;
        goto end;
      }

      (void)memcpy(pRandomData, aTmpRandom, tmpRandomLen);
      // tmpRandomLen = 0;
    }

    status = E_K_STATUS_OK;
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @implements lSetValueById
 *
 */
static TKStatus lSetValueById
(
  uint32_t  xObjectId,
  uint8_t* xpValue,
  size_t    xValueLen
)
{
  TKStatus  status = E_K_STATUS_ERROR;
  uint32_t  loopCount = 0;
  uint32_t  noOfItems = sizeof(gaSalObjectIdMapTable) / sizeof(gaSalObjectIdMapTable[0]);

  M_KTALOG__START("Start");

  for (; loopCount < noOfItems; loopCount++)
  {
    if (gaSalObjectIdMapTable[loopCount].virtualObjId == xObjectId)
    {
      (void)memset(gaSalObjectIdMapTable[loopCount].aObjectIdData,
                   0,
                   C_SAL_CRYPTO_KEY_SIZE_32_BYTE);
      (void)memcpy(gaSalObjectIdMapTable[loopCount].aObjectIdData, xpValue, xValueLen);
      status = E_K_STATUS_OK;
      break;
    }
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @implements lGetValueById
 *
 */
static TKStatus lGetValueById
(
  uint32_t  xObjectId,
  uint8_t* xpValue,
  size_t    xValueLen
)
{
  TKStatus  status = E_K_STATUS_ERROR;
  uint32_t  loopCount = 0;
  uint32_t  noOfItems = sizeof(gaSalObjectIdMapTable) / sizeof(gaSalObjectIdMapTable[0]);

  M_KTALOG__START("Start");

  for (; loopCount < noOfItems; loopCount++)
  {
    if (gaSalObjectIdMapTable[loopCount].virtualObjId == xObjectId)
    {
      M_KTALOG__INFO("Found the ID %x", xObjectId);
      (void)memcpy(xpValue, gaSalObjectIdMapTable[loopCount].aObjectIdData, xValueLen);
      status = E_K_STATUS_OK;
      break;
    }
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement salSignHash
 *
 */
TKStatus salSignHash
(
  uint32_t  xKeyId,
  uint8_t*  xpMsgToHash,
  size_t    xMsgToHashLen,
  uint8_t*  xpSignedHashOutBuff,
  uint32_t  xSignedHashOutBuffLen,
  size_t*   xpActualSignedHashOutLen
)
{
  M_UNUSED(xKeyId);
  M_UNUSED(xpMsgToHash);
  M_UNUSED(xMsgToHashLen);
  M_UNUSED(xpSignedHashOutBuff);
  M_UNUSED(xSignedHashOutBuffLen);
  M_UNUSED(xpActualSignedHashOutLen);
  return E_K_STATUS_OK;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

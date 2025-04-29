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
/** \brief keySTREAM Trusted Agent - Activation handler.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file acthandler.c
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent Activation handler.
 */
#include "acthandler.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "config.h"
#include "k_crypto.h"
#include "general.h"
#include "k_sal.h"
#include "k_sal_rot.h"
#include "k_sal_storage.h"
#include "cryptoConfig.h"
#include "KTALog.h"



/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */
/** @brief Maximum Length for device profile padding. */
#define C_DEV_PROF_PAD_MAX_LEN  (C_K__DEVICE_PROFILE_PUBLIC_UID_MAX_SIZE \
                                 + C_K_CRYPTO__AES_BLOCK_SIZE)

/** @brief Expose shared secret to host. */
#define C_EXPOSE_SHARED_SECRET_TO_HOST (0x80000000u)

/** @brief To check field availability. */
#define M_ACT_RESP_IS_FIELD_AVAILABLE(x_field) (((x_field).len) > 0u)

/** @brief Activation request payload. */
typedef struct
{
  uint8_t aChipUid[C_K_KTA__CHIPSET_UID_MAX_SIZE];
  /* Chip uid value. */
  size_t  chipUidSize;
  /* Chip uid size. */
  uint8_t aChipCert[C_K_KTA__CHIP_CERT_MAX_SIZE];
  /* Chip certificate value. */
  size_t  chipCertSize;
  /* Chip certificate size. */
  uint8_t aRotEpk[C_K_KTA__PUBLIC_KEY_MAX_SIZE];
  /* Rot ephemeral key. */
} TKactreqPayload;

/** @brief Activation response fileds. */
typedef struct
{
  uint8_t*  pValue;
  /* Field Value. */
  size_t    len;
  /* Field Len. */
} TKactRespFields;

/** @brief Activation response payload. */
typedef struct
{
  TKactRespFields rotPublicUid;
  /* RotPublicUid field. */
  TKactRespFields ksEphemeralPubKey;
  /* keySTREAM ephemeral key field. */
} TKactRespPayload;

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/* Module name used for logging */
static const char* gpModuleName = "KTAACTHANDLER";

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   Derive L2 keys.
 *
 * @param[in] xKeyId
 *   Key ID to L1 keys.
 * @param[in] xpInDataEncDec
 *   Data buffer for encryption key.
 * @param[in] xInDataEncDecLen
 *   Encryption key length.
 * @param[in] xpInDataAuth
 *   Data buffer for authentication key.
 * @param[in] xInDataAuthLen
 *   Authentication key length.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lActDeriveL2Keys
(
  uint32_t        xKeyId,
  const uint8_t*  xpInDataEncDec,
  size_t          xInDataEncDecLen,
  const uint8_t*  xpInDataAuth,
  size_t          xInDataAuthLen
);

/**
 * @brief
 *   Derive L2 activation keys.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lActReqDeriveL2Keys
(
  void
);

/**
 * @brief
 *   Derive L1 activation key.
 *
 * @param[in] xpRotEpk
 *   Data buffer of rot ephemeral key.
 *   Should not be NULL.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lActReqDeriveL1Key
(
  const uint8_t* xpRotEpk
);

/**
 * @brief
 *   Fetch activation request payload.
 *
 * @param[in,out] xpActPayload
 *   Should not be NULL.
 *   [in] Pointer to the structure carrying activation request payload.
 *   [out] Buffer size filled with activation request message size.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lFetchActivationReqData
(
  TKactreqPayload* xpActPayload
);

#ifdef OBJECT_MANAGEMENT_FEATURE
/**
 * @brief
 *   Parse chip certificate.
 *
 * @param[in,out] xpProtoMessage
 *   ICPP message to parse.
 * @param[in] xCommandPos
 *   Command position.
 * @param[in,out] xpActPayload
 *   ICPP activation request payload buffer.
 * @param[in] xpFieldPos
 *   Field position.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lParseChipCert
(
  TKIcppProtocolMessage* xpProtoMessage,
  uint32_t               xCommandPos,
  TKactreqPayload*       xpActPayload,
  uint32_t*              xpFieldPos
);
#endif

/**
 * @brief
 *   Prepare ICPP activation request payload.
 *
 * @param[in] xpActPayload
 *   ICPP activation request payload.
 * @param[in] xppEncMsg
 *   Encrypted message to place in activation.
 * @param[in] xEncMsgLen
 *   Encrypted message length to place in activation.
 * @param[in,out] xpProtoMessage
 *   Should not be NULL.
 *   [in] Pointer to the buffer carrying activation request message.
 *   [out] Buffer filled with activation request message.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lPrepareActivationRequest
(
  TKactreqPayload*       xpActPayload,
  uint8_t                (*xppEncMsg)[C_DEV_PROF_PAD_MAX_LEN],
  const size_t*         xpEncMsgLen,
  TKIcppProtocolMessage* xpProtoMessage
);

/**
 * @brief
 *   Validate activation response and update payload structure.
 *
 * @param[in] xpRecvMsg
 *   Icpp Message.
 *   Should not be NULL.
 * @param[in,out] xpActRespPayload
 *   [in] Buffer to load activation response payload.
 *   [out] Buffer loaded with activation response payload.
 *   Should not be NULL.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lActRespValidateAndGetPayload
(
  TKIcppProtocolMessage*  xpRecvMsg,
  TKactRespPayload*       xpActRespPayload
);

/**
 * @brief
 *   Derive L1 field key using keySTREAM ephemeral key.
 *
  * @param[in,out]  xpKsEpk
 *   Buffer for keySTREAM ephemeral key.
 *   Should not be NULL.
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lActRespDeriveL1Key
(
  const uint8_t* xpKsEpk
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */
/**
 * @brief implement ktaActDeriveL2Keys
 *
 */
TKStatus ktaActDeriveL2Keys
(
  void
)
{
  const uint8_t aFldL2EncDec[C_KTA__FIELD_L2_ENC_INPUT_DATA_SIZE] = C_KTA__FIELD_L2_ENC_INPUT_DATA;
  const uint8_t aFldL2Auth[C_KTA__FIELD_L2_AUTH_IN_DATA_SIZE] = C_KTA__FIELD_L2_AUTH_INPUT_DATA;
  TKStatus      status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  status =  lActDeriveL2Keys(C_K_KTA__L1_FIELD_KEY_ID,
                              aFldL2EncDec,
                              C_KTA__FIELD_L2_ENC_INPUT_DATA_SIZE,
                              aFldL2Auth,
                              C_KTA__FIELD_L2_AUTH_IN_DATA_SIZE);

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaActBuildActivationRequest
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaActBuildActivationRequest
(
  uint8_t* xpMessageToSend,
  size_t*  xpMessageToSendSize
)
{
  TKIcppProtocolMessage protoMessage              = {0};
  TKactreqPayload actPayload                      = {0};
  uint8_t aPaddedMsg[C_DEV_PROF_PAD_MAX_LEN]      = {0};
  size_t  paddedMsgLen                            = C_DEV_PROF_PAD_MAX_LEN;
  uint8_t aEncMsg[C_K__MAX_DEVICE_PROFILES][C_DEV_PROF_PAD_MAX_LEN] = {0};
  size_t  aEncMsgLen[C_K__MAX_DEVICE_PROFILES]    =
                                               {C_DEV_PROF_PAD_MAX_LEN, C_DEV_PROF_PAD_MAX_LEN};
  uint8_t aSerializeBuffer[C_K__ICPP_MSG_MAX_SIZE] = {0};
  size_t serializeBufferLen                       = C_K__ICPP_MSG_MAX_SIZE;
  uint8_t aComputedMac[C_KTA_ACT__HMACSHA256_SIZE] = {0};
  size_t transactionIDLen                        = C_K_ICPP_PARSER__TRANSACTION_ID_SIZE_IN_BYTES;
  TKStatus status                                = E_K_STATUS_ERROR;
  TKParserStatus parserStatus                    = E_K_ICPP_PARSER_STATUS_ERROR;
  TKtaDeviceInfoConfig deviceInfoConfig          = {0};
  const uint8_t aRotSolID[C_KTA__ROT_SOL_ID_SIZE] = C_KTA__ROT_SOL_ID;
  size_t rotPublicUidLen                         = C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES;
  uint8_t maxDevProfiles                         = C_K__MAX_DEVICE_PROFILES;

  M_KTALOG__START("Start");

  if ((NULL == xpMessageToSend) ||
      (NULL == xpMessageToSendSize) ||
      ((C_K__ICPP_MSG_MAX_SIZE > *xpMessageToSendSize) ||
        (0u == *xpMessageToSendSize)))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter passed, status = [%d]", status);
  }
  else
  {
    status = lFetchActivationReqData(&actPayload);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("Fetching of activation request data failed, status = [%d]", status);
      goto end;
    }

    /* Generate key pair and provide public key in return. */
    // REQ RQ_M-KTA-ACTV-FN-0005_02(1) : Rot Ephemeral Public Key
    status = salRotKeyPairGeneration(actPayload.aRotEpk);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("SAL API while generation key pair failed, status = [%d]", status);
      goto end;
    }

    // REQ RQ_M-KTA-ACTV-FN-0015(1) : Generate L1 Key
    status = lActReqDeriveL1Key(actPayload.aRotEpk);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("Derivation of L1 Keys failed, status = [%d]", status);
      goto end;
    }

    // REQ RQ_M-KTA-ACTV-FN-0025(1) : Generate L2 Auth Key
    // REQ RQ_M-KTA-ACTV-FN-0026(1) : Generate L2 Encryption Key
    status = lActReqDeriveL2Keys();

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("Derivation of L2 Keys failed, status = [%d]", status);
      goto end;
    }

    /* Fetch device profile pub uid. */
    status = ktaGetDeviceInfoConfig(&deviceInfoConfig);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("Fetching device profile public UID failed, status = [%d]", status);
      goto end;
    }

   /* If mutable device profile UID not available. */
    if (deviceInfoConfig.deviceProfilePubUIDLength[1] == 0u)
    {
      aEncMsgLen[1] = 0;
      maxDevProfiles = 1;
    }
    for (uint8_t devProfile = 0; devProfile < maxDevProfiles; devProfile++)
    {
      // REQ RQ_M-KTA-ACTV-FN-0035(1) : Pad activation request device public profile uid.
      // REQ RQ_M-KTA-ACTV-FN-0030(1) : Data padding for encryption
      status = ktacipherAddPadding(deviceInfoConfig.deviceProfilePubUID[devProfile],
                                   deviceInfoConfig.deviceProfilePubUIDLength[devProfile],
                                   aPaddedMsg,
                                   &paddedMsgLen);

      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("Padding failed with status = [%d]", status);
        break;
      }
      else
      {
        // REQ RQ_M-KTA-ACTV-FN-0040(1) : Encrypt the padded activation request device public
        /* profile uid. */
        status = ktacipherEncrypt(aPaddedMsg, paddedMsgLen, aEncMsg[devProfile],
                                  &aEncMsgLen[devProfile]);

        if (E_K_STATUS_OK != status)
        {
          M_KTALOG__ERR("Encryption failed with status = [%d]", status);
          break;
        }

        /* Reusing the same buffer for next device profile. */
        memset(aPaddedMsg, 0, sizeof(aPaddedMsg));
        paddedMsgLen = C_DEV_PROF_PAD_MAX_LEN;
      }
    }
    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("Device profile padding/encryption failed, status = [%d]", status);
      goto end;
    }
    /* Fill the crypto version to use (for activation request dos based encryption is used). */
    // REQ RQ_M-KTA-ACTV-FN-0020_01(1) : Activation crypto version
    protoMessage.cryptoVersion  = E_K_ICPP_PARSER_CRYPTO_TYPE_DOS_BASED;
    /* Fill the mode of encryption (for activation request partial encryption is used). */
    // REQ RQ_M-KTA-ACTV-FN-0020_02(1) : Activation partial encryption mode
    protoMessage.encMode  = E_K_ICPP_PARSER_PARTIAL_ENC_MODE;
    /* Fill the message type with "E_K_ICCP_PARSER_MESSAGE_TYPE_NOTIFICATION" to indicate it is
       registration notification message type (client -> server). */
    // REQ RQ_M-KTA-ACTV-FN-0020_03(1) : Activation message type
    protoMessage.msgType  = E_K_ICPP_PARSER_MESSAGE_TYPE_NOTIFICATION;
    /* Fill the random transaction ID. */
    // REQ RQ_M-KTA-ACTV-FN-0020_04(1) : Activation transaction id
    status = salCryptoGetRandom(protoMessage.transactionId,
                                &transactionIDLen);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("salCryptoGetRandom failed to get transaction ID, status = [%d]", status);
      goto end;
    }

    /* Setting rotKeySetId to default value 0. */
    // REQ RQ_M-KTA-ACTV-FN-0020_05(1) : Activation rot key set id
    protoMessage.rotKeySetId = 0x00;
    // REQ RQ_M-KTA-ACTV-FN-0020_06(1) : Activation rot public uid
    status = salStorageGetValue(C_K_KTA__ROT_PUBLIC_UID_STORAGE_ID,
                                protoMessage.rotPublicUID,
                                &rotPublicUidLen);

    if ((E_K_STATUS_OK != status) && (0u == rotPublicUidLen))
    {
      M_KTALOG__ERR("SAL API failed while reading rotPublicUID, status = [%d]", status);
      goto end;
    }

    if (memcmp(protoMessage.rotPublicUID, aRotSolID, C_KTA__ROT_SOL_ID_SIZE) != 0)
    {
      (void)memset(protoMessage.rotPublicUID, 0, rotPublicUidLen);
      (void)memcpy(protoMessage.rotPublicUID, aRotSolID, C_KTA__ROT_SOL_ID_SIZE);
    }

    // REQ RQ_M-KTA-ACTV-FN-0020(1) : Activation ICPP Message
    status = lPrepareActivationRequest(&actPayload, aEncMsg, aEncMsgLen, &protoMessage);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("Prepare actRequest got failed, status = [%d]", status);
      goto end;
    }

    // REQ RQ_M-KTA-ICPP-FN-0100(1) : Serialize the message
    parserStatus = ktaIcppParserSerializeMessage(&protoMessage,
                                                 aSerializeBuffer,
                                                 &serializeBufferLen);

    if (E_K_ICPP_PARSER_STATUS_OK != parserStatus)
    {
      M_KTALOG__ERR("Serialization of message failed, parserStatus = [%d]", parserStatus);
      status = E_K_STATUS_ERROR;
      goto end;
    }

    // REQ RQ_M-KTA-ACTV-FN-0050(1) : Sign the ICPP format raw buffer
    status = ktacipherSignMsg(aSerializeBuffer, serializeBufferLen, aComputedMac);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("Signing of message failed, status = [%d]", status);
      goto end;
    }

    (void)memcpy(xpMessageToSend, aSerializeBuffer, serializeBufferLen);
    (void)memcpy(&(xpMessageToSend[serializeBufferLen]), aComputedMac, C_KTA_ACT__HMACSHA256_SIZE);
    *xpMessageToSendSize =  serializeBufferLen + C_KTA_ACT__HMACSHA256_SIZE;
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaActResponseBuildL1Keys
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaActResponseBuildL1Keys
(
  TKIcppProtocolMessage* xpRecvMsg
)
{
  TKStatus          status = E_K_STATUS_ERROR;
  TKactRespPayload  resPayload;
  size_t            rotPublicUidLen = C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES;

  M_KTALOG__START("Start");

  if (NULL == xpRecvMsg)
  {
    M_KTALOG__ERR("Invalid Parameter");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    (void)memset(&resPayload, 0, sizeof(TKactRespPayload));

    // REQ RQ_M-KTA-ACTV-FN-0080(1) : Check activation response data
    if (E_K_STATUS_OK != lActRespValidateAndGetPayload(xpRecvMsg, &resPayload))
    {
      M_KTALOG__ERR("Getting Rot Public UID / keyStream Ephemeral Public key(ks_e_pk) failed");
      goto end;
    }

    /* keySTREAM Ephemeral Public Key is Mandatory. */
    if (!M_ACT_RESP_IS_FIELD_AVAILABLE(resPayload.ksEphemeralPubKey))
    {
      M_KTALOG__ERR("ksEphemeralPubKey Not found or Invalid");
      goto end;
    }

    /* Rot public Uid is optional. */
    if (M_ACT_RESP_IS_FIELD_AVAILABLE(resPayload.rotPublicUid))
    {
      status = salStorageGetValue(C_K_KTA__ROT_PUBLIC_UID_STORAGE_ID,
                                  resPayload.rotPublicUid.pValue,
                                  &rotPublicUidLen);
      if ((E_K_STATUS_OK != status) || (0u == rotPublicUidLen))
      {
        M_KTALOG__ERR("SAL API failed while reading rotPublicUID, status = [%d]", status);
        goto end;
      }
    }

    // REQ RQ_M-KTA-ACTV-FN-0105(1) : Generate L1 Field Key
    status = lActRespDeriveL1Key(resPayload.ksEphemeralPubKey.pValue);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("Deriving L1 field key failed, status = [%d]", status);
    }
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */
/**
 * @implements lActDeriveL2Keys
 *
 */
static TKStatus lActDeriveL2Keys
(
  uint32_t        xKeyId,
  const uint8_t*  xpInDataEncDec,
  size_t          xInDataEncDecLen,
  const uint8_t*  xpInDataAuth,
  size_t          xInDataAuthLen
)
{
  TKStatus status   = E_K_STATUS_ERROR;

  // REQ RQ_M-KTA-ACTV-FN-0115(1) : Generate L2 Encryption Key
  status = salRotKeyDerivation(xKeyId, xpInDataEncDec, xInDataEncDecLen, C_K_KTA__VOLATILE_3_ID);

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("SAL API failed while deriving L2 Encryption key, status = [%d]", status);
  }
  else
  {
    // REQ RQ_M-KTA-ACTV-FN-0120(1) : Generate L2 Auth Key
    status = salRotKeyDerivation(xKeyId, xpInDataAuth, xInDataAuthLen, C_K_KTA__VOLATILE_2_ID);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("SAL API failed while deriving L2 auth key, status = [%d]", status);
    }
  }

  return status;
}

/**
 * @implements lActReqDeriveL2Keys
 *
 */
static TKStatus lActReqDeriveL2Keys
(
  void
)
{
  const uint8_t aActL2EncInputData[C_KTA__ACT_L2_ENC_INPUT_DATA_SIZE] = C_KTA__ACT_L2_ENC_INPUT_DATA;
  const uint8_t aActL2AuthInputData[C_KTA__ACT_L2_AUTH_INPUT_DATA_SIZE] = C_KTA__ACT_L2_AUTH_INPUT_DATA;

  return lActDeriveL2Keys(C_K_KTA__VOLATILE_2_ID,
                          aActL2EncInputData,
                          C_KTA__ACT_L2_ENC_INPUT_DATA_SIZE,
                          aActL2AuthInputData,
                          C_KTA__ACT_L2_AUTH_INPUT_DATA_SIZE);
}

/**
 * @implements lActReqDeriveL1Key
 *
 */
static TKStatus lActReqDeriveL1Key
(
  const uint8_t* xpRotEpk
)
{
  const uint8_t aActKeyFixInfo[C_KTA__ACT_KEY_FIXED_INFO_SIZE] = C_KTA__ACT_KEY_FIXED_INFO;
  const uint8_t aKssDosPk[C_KTA__KS_S_DOS_PK_SIZE] = C_KTA__KS_S_DOS_PK;
  TKStatus      status = E_K_STATUS_ERROR;

  status = salRotKeyAgreement(C_K_KTA__CHIP_SK_ID, aKssDosPk, C_K_KTA__VOLATILE_2_ID, NULL);

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("SAL API failed while computing shared secret key, status = [%d]", status);
  }
  else
  {
    status = salRotHkdfExtractAndExpand(C_K_KTA__HKDF_ACT_MODE, NULL, xpRotEpk, aActKeyFixInfo,
                                        C_KTA__ACT_KEY_FIXED_INFO_SIZE);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("SAL API failed while deriving HKDF HMAC-SHA256, status = [%d]", status);
    }
  }

  return status;
}

/**
 * @implements lFetchActivationReqData
 *
 */
static TKStatus lFetchActivationReqData
(
  TKactreqPayload* xpActPayload
)
{
  TKStatus status = E_K_STATUS_ERROR;

  // REQ RQ_M-KTA-ACTV-FN-0005_01(1) : Chip UID
  xpActPayload->chipUidSize = C_K_KTA__CHIPSET_UID_MAX_SIZE;
  status = salRotGetChipUID(xpActPayload->aChipUid, &xpActPayload->chipUidSize);

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("SAL API failed while reading chip UID, status = [%d]", status);
  }
  else
  {
    // REQ RQ_M-KTA-ACTV-CF-0010(1) : Certificate Buffer Size
    // REQ RQ_M-KTA-ACTV-FN-0005_03(1) : Signed Public Key
    // REQ RQ_M-KTA-ACTV-FN-0005_04(1) : X509 Certificate
    // REQ RQ_M-KTA-ACTV-FN-0005_05(1) : Attestation Certificate
#ifdef DEVICE_PROVIDES_CHIP_CERT
    xpActPayload->chipCertSize = C_K_KTA__CHIP_CERT_MAX_SIZE;
    status = salRotGetChipCertificate(xpActPayload->aChipCert, &xpActPayload->chipCertSize);

    if (E_K_STATUS_OK != status)
    {
      xpActPayload->chipCertSize = 0;
    }

#else
    xpActPayload->chipCertSize = 0;
#endif
    status = E_K_STATUS_OK;
  }

  return status;
}

#ifdef OBJECT_MANAGEMENT_FEATURE
/**
 * @implements lParseChipCert
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
static TKStatus lParseChipCert
(
  TKIcppProtocolMessage* xpProtoMessage,
  uint32_t               xCommandPos,
  TKactreqPayload*       xpActPayload,
  uint32_t*              xpFieldPos
)
{
  TKStatus status = E_K_STATUS_ERROR;
  uint32_t length = 0;
  uint32_t index  = 0;

  for (;;)
  {
    xpProtoMessage->commands[xCommandPos].data.fieldList.fields[++*xpFieldPos].fieldTag =
      xpActPayload->aChipCert[index];
    xpProtoMessage->commands[xCommandPos].data.fieldList.fields[*xpFieldPos].fieldLen =
      ((size_t)((xpActPayload->aChipCert[index + 1u]) << 8u) | xpActPayload->aChipCert[index + 2u]);
    length = xpProtoMessage->commands[xCommandPos].data.fieldList.fields[*xpFieldPos].fieldLen;
    index = index + 3u;
    xpProtoMessage->commands[xCommandPos].data.fieldList.fields[*xpFieldPos].fieldValue =
      xpActPayload->aChipCert + index;
    index += length;

    if (index >= xpActPayload->chipCertSize)
    {
      break;
    }
  }
  status = E_K_STATUS_OK;
  return status;
}
#endif

/**
 * @implements lPrepareActivationRequest
 *
 */
static TKStatus lPrepareActivationRequest
(
  TKactreqPayload*       xpActPayload,
  uint8_t                (*xppEncMsg)[C_DEV_PROF_PAD_MAX_LEN],
  const  size_t*         xpEncMsgLen,
  TKIcppProtocolMessage* xpProtoMessage
)
{
  uint32_t currentPos = xpProtoMessage->commandsCount;
  uint32_t fieldPos = 0;
  TKStatus status = E_K_STATUS_ERROR;

  xpProtoMessage->commandsCount = xpProtoMessage->commandsCount + 1u;
  /** Set the command tag with "E_K_ICPP_PARSER_COMMAND_TAG_ACTIVATION" to indicated the
      the command is for activation. */
  xpProtoMessage->commands[currentPos].commandTag = E_K_ICPP_PARSER_COMMAND_TAG_ACTIVATION;

  /**
   * Note : xpProtoMessage->commands[currentPos].data.fieldList.fieldsCount
   *         should be less then C_K_ICPP_PARSER_MAX_FIELDS_COUNT.
   */
  xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldTag =
    E_K_ICPP_PARSER_FIELD_TAG_ROT_E_PK;
  xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldLen =
    C_K_KTA__PUBLIC_KEY_MAX_SIZE;
  xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldValue =
    xpActPayload->aRotEpk;

  if (0u != xpActPayload->chipUidSize)
  {
    ++fieldPos;
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldTag =
      E_K_ICPP_PARSER_FIELD_TAG_CHIP_UID;
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldLen =
      xpActPayload->chipUidSize;
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldValue =
      xpActPayload->aChipUid;
  }

  if (0u != xpActPayload->chipCertSize)
  {
#ifdef PLATFORM_PROCESS_FEATURE
    ++fieldPos;
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldTag =
      E_K_ICPP_PARSER_FLD_TAG_CHIP_CERT;
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldLen =
      xpActPayload->chipCertSize;
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldValue =
      xpActPayload->aChipCert;
#endif
#ifdef OBJECT_MANAGEMENT_FEATURE
    /* Call tlv parser here and update fields. */
    status = lParseChipCert(xpProtoMessage, currentPos, xpActPayload, &fieldPos);

    if (status != E_K_STATUS_OK)
    {
      M_KTALOG__ERR("Parsing of certificate tlv failed");
      goto end;
    }

#endif
  }

  // REQ RQ_M-KTA-ACTV-FN-0045(1) :
  /* Convert the encrypted data into ICPP format raw buffer. */
  ++fieldPos;
  xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldTag =
    E_K_ICPP_PARSER_FIELD_TAG_DEVPROFUID;
  xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldLen = xpEncMsgLen[0];
  xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldValue = xppEncMsg[0];

  if (xpEncMsgLen[1] != 0)
  {
    // REQ RQ_M-KTA-ACTV-FN-0045(1) :
    /* Convert the encrypted data into ICPP format raw buffer. */
    ++fieldPos;
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldTag =
        E_K_ICPP_PARSER_FIELD_TAG_MUTABLE_DEVPROFUID;
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldLen =
        xpEncMsgLen[1];
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldPos].fieldValue =
        xppEncMsg[1];
  }

  /** Note : xpProtoMessage->commands[currentPos].data.fieldList.fieldsCount.
   * should be less then C_K_ICPP_PARSER_MAX_FIELDS_COUNT". */
  ++fieldPos;
  xpProtoMessage->commands[currentPos].data.fieldList.fieldsCount = fieldPos;
  status = E_K_STATUS_OK;

end:
  return status;
}

/**
 * @implements lActRespValidateAndGetPayload
 *
 */
static TKStatus lActRespValidateAndGetPayload
(
  TKIcppProtocolMessage*  xpRecvMsg,
  TKactRespPayload*       xpActRespPayload
)
{
  TKStatus        status = E_K_STATUS_ERROR;
  uint32_t        isErrorOccured = 0;
  TKIcppFieldList* pFieldList = NULL;

  if (0u == xpRecvMsg->commandsCount)
  {
    M_KTALOG__ERR("Activation Response received message command Count is 0");
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    for (size_t commandsLoop = 0; (commandsLoop < xpRecvMsg->commandsCount) &&
         (isErrorOccured == 0u); commandsLoop++)
    {
      if (E_K_ICPP_PARSER_COMMAND_TAG_ACTIVATION !=
          xpRecvMsg->commands[commandsLoop].commandTag)
      {
        M_KTALOG__ERR("Invalid command Tag 0x%02x", xpRecvMsg->commands[commandsLoop].commandTag);
        break;
      }

      pFieldList = &xpRecvMsg->commands[commandsLoop].data.fieldList;

      if (0u == pFieldList->fieldsCount)
      {
        M_KTALOG__ERR("Field Count is 0");
        break;
      }

      for (size_t fieldsLoop = 0; (fieldsLoop < pFieldList->fieldsCount) &&
           (isErrorOccured == 0u); fieldsLoop++)
      {
        if (
          (0u == pFieldList->fields[fieldsLoop].fieldLen) ||
          (NULL == pFieldList->fields[fieldsLoop].fieldValue)
        )
        {
          M_KTALOG__ERR("Invalid Field Data");
          isErrorOccured = 1u;
          break;
        }

        switch (pFieldList->fields[fieldsLoop].fieldTag)
        {
          case E_K_ICPP_PARSER_FIELD_TAG_ROT_PUBLIC_UID:
          {
            if (C_K_KTA__ROT_PUBLIC_UID_MAX_SIZE != pFieldList->fields[fieldsLoop].fieldLen)
            {
              M_KTALOG__ERR("Invalid RotPubUid len[%u]",
                            (unsigned int)xpActRespPayload->rotPublicUid.len);
              isErrorOccured = 1u;
              break;
            }

            xpActRespPayload->rotPublicUid.pValue = pFieldList->fields[fieldsLoop].fieldValue;
            xpActRespPayload->rotPublicUid.len = pFieldList->fields[fieldsLoop].fieldLen;
          }
          break;

          // REQ RQ_M-KTA-ACTV-FN-0080_01(1) : keySTREAM Ephemeral Public Key
          case E_K_ICPP_PARSER_FIELD_TAG_KS_E_PK:
          {
            if (C_K_KTA__PUBLIC_KEY_MAX_SIZE != pFieldList->fields[fieldsLoop].fieldLen)
            {
              M_KTALOG__ERR("Invalid ks_e_pk len[%u]",
                            (unsigned int)xpActRespPayload->ksEphemeralPubKey.len);
              isErrorOccured = 1u;
              break;
            }

            xpActRespPayload->ksEphemeralPubKey.pValue = pFieldList->fields[fieldsLoop].fieldValue;
            xpActRespPayload->ksEphemeralPubKey.len = pFieldList->fields[fieldsLoop].fieldLen;
          }
          break;

          default:
          {
            M_KTALOG__ERR("Unknown Field Tag %d", pFieldList->fields[fieldsLoop].fieldTag);
            isErrorOccured = 1u;
          }
          break;
        } /* switch */
      }

      if (isErrorOccured == 0)
      {
        status = E_K_STATUS_OK;
      }
    }
  }

  return status;
}

/**
 * @implements lActRespDeriveL1Key
 *
 */
static TKStatus lActRespDeriveL1Key
(
  const uint8_t* xpKsEpk
)
{
  TKStatus        status = E_K_STATUS_ERROR;
  const uint8_t   aKsSharedPubKey[C_KTA__KS_S_PK_SIZE] = C_KTA__KS_S_PK;
  const uint8_t   aSalt[C_KTA__FIELD_KEY_SALT_SIZE] = C_KTA__FIELD_KEY_SALT;
  uint8_t         aInfoHkdf[C_KTA__FIELD_KEY_FIXED_INFO_SIZE] = C_KTA__FIELD_KEY_FIXED_INFO;
  uint8_t         aSharedSecretES[2 * C_K_KTA__SHARED_SECRET_KEY_MAX_SIZE] = { 0 };

  /* Storing shared Secret E. */
  // REQ RQ_M-KTA-ACTV-FN-0090(1) : Generate Shared Secret E(**shs_e**)
  status = salRotKeyAgreement(C_K_KTA__VOLATILE_ID,
                              xpKsEpk,
                              C_EXPOSE_SHARED_SECRET_TO_HOST,
                              aSharedSecretES);

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("SAL API failed while computing secret E, status = [%d]", status);
  }
  else
  {
    /* Appending shared Secret S. */
    // REQ RQ_M-KTA-ACTV-FN-0095(1) : Generate Shared Secret S(**shs_s**)
    // REQ RQ_M-KTA-ACTV-FN-0100(1) : Generate Shared Secret ES(**shs_es**)
    status = salRotKeyAgreement(C_K_KTA__CHIP_SK_ID,
                                aKsSharedPubKey,
                                C_EXPOSE_SHARED_SECRET_TO_HOST,
                                &aSharedSecretES[C_K_KTA__SHARED_SECRET_KEY_MAX_SIZE]);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("SAL API failed while computing secret S, status = [%d]", status);
    }
    else
    {
      /* This function will update L1 segmentation seed. */
      status = ktaGetL1SegSeed(&aInfoHkdf[C_KTA__FIELD_KEY_FIXED_INFO_L1SEGSEED_POS]);
      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("Getting L1 SegSeed failed, status = [%d]", status);
      }
      else
      {
        /* Storing the L1 field key. */
        // REQ RQ_M-KTA-ACTV-FN-0110(1) : Store L1 field key
        status = salRotHkdfExtractAndExpand(C_K_KTA__HKDF_GEN_MODE,
                                            aSharedSecretES,
                                            aSalt,
                                            aInfoHkdf,
                                            C_KTA__FIELD_KEY_FIXED_INFO_SIZE);

        if (E_K_STATUS_OK != status)
        {
          M_KTALOG__ERR("SAL API failed while storing L1 field key, status = [%d]", status);
        }
      }
    }
  }

  return status;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

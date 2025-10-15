/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2025 Nagravision SÃ rl

* Subject to your compliance with these terms, you may use the Nagravision SÃ rl
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
/** \brief keySTREAM Trusted Agent - Registration module
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file reghandler.c
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - Registration module.
 */
#include "reghandler.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_storage.h"
#include "k_sal_rot.h"
#include "k_sal.h"
#include "icpp_parser.h"
#include "config.h"
#include "k_crypto.h"
#include "general.h"
#include "KTALog.h"
#include "cryptoConfig.h"

#include <string.h>

#ifdef FOTA_ENABLE
#include "k_sal_fota.h"
#endif
/* To Supress the misra-config Error*/
#include "stddef.h"
/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Sequence counter. */
#define C_ACK_SEQ_CNT_SIZE            (3u)

/** @brief Capability size. */
#define C_KTA_CAPABILITY_SIZE         (1u)

/** @brief Version size. */
#define C_KTA_VERSION_SIZE            (1u)

/** @brief Size of ICPP message header. */
#define C_K_ICPP_PARSER_HEADER_SIZE   (21u)

/** @brief Capability based on feature. */
#ifdef PLATFORM_PROCESS_FEATURE
#define C_KTA_CAPABILITY    {0x00}
#endif
#ifdef OBJECT_MANAGEMENT_FEATURE
#define C_KTA_CAPABILITY    {0x01}
#endif

/** @brief Registration info payload. */
typedef struct
{
  TKtaDeviceInfoConfig deviceInfo;
  /* Device configuration info. */
  TKtaContextInfoConfig contextInfo;
  /* Context info configuration information. */
  uint8_t aAckSeqCnt[C_ACK_SEQ_CNT_SIZE];
  /* Sequence counter data. */
  uint8_t aKtaCapability[C_KTA_CAPABILITY_SIZE];
  /* Capability with respect to feature. */
  uint8_t aKtaVersion[C_KTA__VERSION_MAX_SIZE];
  /* Version buffer. */
#ifdef FOTA_ENABLE
  TComponent xComponents[COMPONENTS_MAX];
  /* Component list. */
#endif
} TKRegInfoPayload;
/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/**
 * SUPPRESS: MISRA_DEV_KTA_009 : misra_c2012_rule_5.9_violation
 * The identifier gpModuleName is intentionally defined as a common global for logging purposes
 */
#if LOG_KTA_ENABLE != C_KTA_LOG_LEVEL_NONE
static const char* gpModuleName = "KTAREGHANDLER";
#endif

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   Collect registration information.
 *
 * @param[out] xpRegInfo
 *   [out] TKRegInfoPayload structure to fill with payload.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lGetRegistrationInfo
(
  TKRegInfoPayload* xpRegInfo
);

/**
 * @brief
 *   Prepare notification message.
 *
 * @param[in,out] xpProtoMessage
 *   [in] Structure contains notification message info.
 *   [out] Filled protocol message structure with necessary info.
 *   ICPP structure for notification message.
 *
 * @param[in] xpRegInfo
 *   Should not be NULL.
 *   Registration information payload.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lPrepareNotificationMsg
(
  TKIcppProtocolMessage* xpProtoMessage,
  TKRegInfoPayload* xpRegInfo
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */
/**
 * @brief implement ktaregBuildRegistrationRequest
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaregBuildRegistrationRequest
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  uint8_t* xpMessageToSend,
  size_t*  xpMessageToSendSize
)
{
  TKIcppProtocolMessage protoMessage = {0};
  TKStatus              status = E_K_STATUS_ERROR;
  size_t                rotPublicUidLen = C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES;
  TKRegInfoPayload      xpRegInfo = {0};

  M_KTALOG__START("Start");

  if ((NULL == xpRecvdProtoMessage) ||
      (NULL == xpMessageToSend) ||
      (NULL == xpMessageToSendSize) ||
      (C_K__ICPP_MSG_MAX_SIZE > *xpMessageToSendSize))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter passed");
  }
  else
  {
    /* keySTREAM should give activation response based on dos key. */
    if (E_K_ICPP_PARSER_CRYPTO_TYPE_DOS_BASED != xpRecvdProtoMessage->cryptoVersion)
    {
      M_KTALOG__ERR("Received wrong crypto version from the server, cryptoVersion = [%d]",
                    xpRecvdProtoMessage->cryptoVersion);
    }
    else
    {
      /* Fill the crypto version to use (for activation request dos based encryption is used). */
      // REQ RQ_M-KTA-REGT-FN-0010(1) : Check crypto version
      // REQ RQ_M-KTA-REGT-FN-0020_01(1) : Registeration Info crypto version
      protoMessage.cryptoVersion  = E_K_ICPP_PARSER_CRYPTO_TYPE_L2_BASED;
      /* Fill the mode of encryption (for activation request partial encryption is used). */
      // REQ RQ_M-KTA-REGT-FN-0020_02(1) : Registeration Info partial encryption mode
      protoMessage.encMode  = E_K_ICPP_PARSER_FULL_ENC_MODE;

      /* Fill the message type with "E_K_ICPP_PARSER_MESSAGE_TYPE_RESPONSE" to indicate it is
        registration notification response (client -> server). */
      // REQ RQ_M-KTA-REGT-FN-0020_03(1) : Registeration Info message type
      protoMessage.msgType  = E_K_ICPP_PARSER_MESSAGE_TYPE_RESPONSE;
      /* Fill transaction ID. */
      // REQ RQ_M-KTA-REGT-FN-0020_04(1) : Registeration Info transaction id
      (void)memcpy(protoMessage.transactionId,
                  xpRecvdProtoMessage->transactionId,
                  C_K_ICPP_PARSER__TRANSACTION_ID_SIZE_IN_BYTES);
      /* Fill rotKeySetId as it is which is received from keyStream. */
      // REQ RQ_M-KTA-REGT-FN-0020_05(1) : Registeration Info rot key set id
      protoMessage.rotKeySetId = xpRecvdProtoMessage->rotKeySetId;
      // REQ RQ_M-KTA-REGT-FN-0020_06(1) : Registeration Info rot public uid
      status = salStorageGetValue(C_K_KTA__ROT_PUBLIC_UID_STORAGE_ID,
                                  protoMessage.rotPublicUID,
                                  &rotPublicUidLen);

      if ((E_K_STATUS_OK != status) || (0u == rotPublicUidLen))
      {
        M_KTALOG__ERR("SAL API failed while reading rotPublicUID, status = [%d]", status);
        goto end;
      }

      /* Get registration information. */
      status = lGetRegistrationInfo(&xpRegInfo);

      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("Collecting registration info failed, status = [%d]", status);
        goto end;
      }

      /* Prepare registration response message. */
      // REQ RQ_M-KTA-REGT-FN-0020(1) : Registeration Info ICPP Message
      status = lPrepareNotificationMsg(&protoMessage, &xpRegInfo);

      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("Preparing registration response message failed, status = [%d]", status);
        goto end;
      }

      status = ktaGenerateResponse((C_GEN__SERIALIZE | C_GEN__PADDING |
                                    C_GEN__ENCRYPT | C_GEN__SIGNING),
                                  &protoMessage,
                                  xpMessageToSend,
                                  xpMessageToSendSize);

      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("ktaGenerateResponse failed, status = [%d]", status);
        goto end;
      }
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
 * @implements lGetRegistrationInfo
 *
 *//**
 * @implements lGetRegistrationInfo
 *
 */
static TKStatus lGetRegistrationInfo
(
  TKRegInfoPayload* xpRegInfo
)
{
  TKStatus status                                = E_K_STATUS_ERROR;
  uint8_t aAckSeqCnt[C_ACK_SEQ_CNT_SIZE]         = {0x00, 0x00, 0x00};
  uint8_t aKtaCapability[C_KTA_CAPABILITY_SIZE]  = C_KTA_CAPABILITY;

#ifdef FOTA_ENABLE
  TComponent xComponents[COMPONENTS_MAX]         = {0};
  uint8_t startIndex = 0;
#endif

  status = ktaGetContextInfoConfig(&(xpRegInfo->contextInfo));

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("Reading context specific data from platform failed, status = [%d]", status);
    goto end;
  }

  /* Fetch required info (Device info from config module). */
  status = ktaGetDeviceInfoConfig(&(xpRegInfo->deviceInfo));

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("Reading device info from config failed, status = [%d]", status);
    goto end;
  }

  (void)memcpy(xpRegInfo->aAckSeqCnt, aAckSeqCnt, sizeof(aAckSeqCnt));
  (void)memcpy(xpRegInfo->aKtaCapability, aKtaCapability, sizeof(aKtaCapability));
  (void)memcpy(xpRegInfo->aKtaVersion,
               xpRegInfo->contextInfo.ktaVersion,
               C_KTA__VERSION_MAX_SIZE);

#ifdef FOTA_ENABLE

  status = salDeviceGetInfo(xComponents);

  // Check if the device info was successfully retrieved
  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("Reading component failed, status = [%d]", status);
    goto end;
  }
  // Copy the xComponents array to xpRegInfo->xComponents one by one
  for (startIndex = 0; startIndex < COMPONENTS_MAX; startIndex++)
  {
    (void)memcpy(xpRegInfo->xComponents[startIndex].componentName, xComponents[startIndex].componentName, CURRENT_MAX_LENGTH);
    xpRegInfo->xComponents[startIndex].componentNameLen = xComponents[startIndex].componentNameLen;
    (void)memcpy(xpRegInfo->xComponents[startIndex].componentVersion, xComponents[startIndex].componentVersion, CURRENT_MAX_LENGTH);
    xpRegInfo->xComponents[startIndex].componentVersionLen = xComponents[startIndex].componentVersionLen;
  }

#endif // FOTA_ENABLE
  end:
  return status;
}

/**
 * @implements lPrepareNotificationMsg
 *
 */
static TKStatus lPrepareNotificationMsg
(
  TKIcppProtocolMessage* xpProtoMessage,
  TKRegInfoPayload* xpRegInfo
)
{
  uint32_t currentPos = 0;
  TKStatus status = E_K_STATUS_ERROR;

  /* This is for handling the current position. */
  currentPos = xpProtoMessage->commandsCount;
  xpProtoMessage->commandsCount = xpProtoMessage->commandsCount + 1u;

  /**
   * Set the command tag with "E_K_ICCP_PARSER_COMMAND_TAG_REGISTERATION_INFO" to indicated the
   * the command is for registration notification.
   */
  xpProtoMessage->commands[currentPos].commandTag =
    E_K_ICPP_PARSER_COMMAND_TAG_REGISTERATION_INFO;


  /* Calculate the total number of fields */
  uint32_t totalFields = 7u;

#ifdef FOTA_ENABLE

  for (uint32_t i = 0; i < COMPONENTS_MAX; i++)
  {
    if ((xpRegInfo->xComponents[i].componentNameLen > 0) && (xpRegInfo->xComponents[i].componentVersionLen > 0))
    {
      totalFields += 2u;
    }
  }

#endif // FOTA_ENABLE

  /* Two fields are available for this message type. */
  xpProtoMessage->commands[currentPos].data.fieldList.fieldsCount = totalFields;

  /**
   * Set the first field tag with "E_K_ICPP_PARSER_FIELD_TAG_ACK_SEQ_CNT" to indicate that
   * the first field contains ack seq cnt.
    */
  // REQ RQ_M-KTA-REGT-FN-0011_01(1) : Acknowledgement sequence count
  xpProtoMessage->commands[currentPos].data.fieldList.fields[0].fieldTag   =
    E_K_ICPP_PARSER_FIELD_TAG_ACK_SEQ_CNT;
  xpProtoMessage->commands[currentPos].data.fieldList.fields[0].fieldLen   =
    sizeof(xpRegInfo->aAckSeqCnt);
  xpProtoMessage->commands[currentPos].data.fieldList.fields[0].fieldValue =
    xpRegInfo->aAckSeqCnt;

  /**
   * Set the second  field tag with "E_K_ICPP_PARSER_FLD_TAG_KTA_CAPABILITY" to indicate that
   * the second field contains kta_capability.
   */
  // REQ RQ_M-KTA-REGT-FN-0011_02(1) : KTA Capability
  xpProtoMessage->commands[currentPos].data.fieldList.fields[1].fieldTag   =
    E_K_ICPP_PARSER_FLD_TAG_KTA_CAPABILITY;
  xpProtoMessage->commands[currentPos].data.fieldList.fields[1].fieldLen   =
    sizeof(xpRegInfo->aKtaCapability);
  xpProtoMessage->commands[currentPos].data.fieldList.fields[1].fieldValue =
    xpRegInfo->aKtaCapability;

  /**
   * Set the third field tag with "E_K_ICPP_PARSER_FIELD_TAG_KTA_CTX_PRO_UID" to indicate that
   * the third field contains kta_context_profile_uid.
   */
  // REQ RQ_M-KTA-REGT-FN-0011_03(1) : KTA Context Profile Uid
  xpProtoMessage->commands[currentPos].data.fieldList.fields[2].fieldTag   =
    E_K_ICPP_PARSER_FIELD_TAG_KTA_CTX_PRO_UID;
  xpProtoMessage->commands[currentPos].data.fieldList.fields[2].fieldLen   =
    sizeof(xpRegInfo->contextInfo.ktaContextProfileUid);
  xpProtoMessage->commands[currentPos].data.fieldList.fields[2].fieldValue =
    xpRegInfo->contextInfo.ktaContextProfileUid;

  /**
   * Set the fourth field tag with "E_K_ICPP_PARSER_FLD_TAG_KTA_CTX_SERIAL_NO" to indicate that
   * the fourth field contains kta_context_serial_number.
   */
  // REQ RQ_M-KTA-REGT-FN-0011_04(1) : KTA Context Serial No
  xpProtoMessage->commands[currentPos].data.fieldList.fields[3].fieldTag   =
    E_K_ICPP_PARSER_FLD_TAG_KTA_CTX_SERIAL_NO;
  xpProtoMessage->commands[currentPos].data.fieldList.fields[3].fieldLen   =
    sizeof(xpRegInfo->contextInfo.ktaContexSerialNumber);
  xpProtoMessage->commands[currentPos].data.fieldList.fields[3].fieldValue =
    xpRegInfo->contextInfo.ktaContexSerialNumber;

  /**
   * Set the fifth field tag with "E_K_ICPP_PRSR_FLD_TAG_KTA_CTX_VER" to indicate that
   * the fifth field contains kta_context_version.
   */
  // REQ RQ_M-KTA-REGT-FN-0011_05(1) : KTA Context Version
  xpProtoMessage->commands[currentPos].data.fieldList.fields[4].fieldTag   =
    E_K_ICPP_PRSR_FLD_TAG_KTA_CTX_VER;
  xpProtoMessage->commands[currentPos].data.fieldList.fields[4].fieldLen   =
    sizeof(xpRegInfo->contextInfo.ktaContextVersion);
  xpProtoMessage->commands[currentPos].data.fieldList.fields[4].fieldValue =
    xpRegInfo->contextInfo.ktaContextVersion;

  /**
   * Set the sixth field tag with "E_K_ICPP_PARSER_FIELD_TAG_KTA_VER" to indicate that
   * the sixth field contains Kta_version.
   */
  // REQ RQ_M-KTA-REGT-FN-0011_06(1) : KTA Version
  xpProtoMessage->commands[currentPos].data.fieldList.fields[5].fieldTag   =
    E_K_ICPP_PARSER_FIELD_TAG_KTA_VER;
  xpProtoMessage->commands[currentPos].data.fieldList.fields[5].fieldLen   =
    strnlen((char*)xpRegInfo->aKtaVersion, C_KTA__VERSION_MAX_SIZE);
  xpProtoMessage->commands[currentPos].data.fieldList.fields[5].fieldValue =
    xpRegInfo->aKtaVersion;

  /**
   * Set the Seventh field tag with "E_K_ICPP_PARSER_FIELD_TAG_DEV_SERIAL_NO" to indicate that
   * the Seventh field contains device_serial_number.
   */
  // REQ RQ_M-KTA-REGT-FN-0011_07(1) : Device Serial Number
  xpProtoMessage->commands[currentPos].data.fieldList.fields[6].fieldTag   =
    E_K_ICPP_PARSER_FIELD_TAG_DEV_SERIAL_NO;
  xpProtoMessage->commands[currentPos].data.fieldList.fields[6].fieldLen   =
    xpRegInfo->deviceInfo.deviceSerailNoLength;
  xpProtoMessage->commands[currentPos].data.fieldList.fields[6].fieldValue =
    xpRegInfo->deviceInfo.deviceSerailNo;

#ifdef FOTA_ENABLE

  // Initialize fieldIndex to 7, as the first 7 fields are already set
  uint32_t fieldIndex = 7;

  for (uint32_t i = 0; i < COMPONENTS_MAX; i++)
  {
    if ((xpRegInfo->xComponents[i].componentNameLen > 0) && (xpRegInfo->xComponents[i].componentVersionLen > 0))
    {
   /**
   * Set the Seventh field tag with "E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_COMPONENT_TARGET"
   */
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldIndex].fieldTag =
      E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_COMPONENT_TARGET;
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldIndex].fieldLen =
      xpRegInfo->xComponents[i].componentNameLen;
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldIndex].fieldValue =
      xpRegInfo->xComponents[i].componentName;

   /**
   * Set the Seventh field tag with "E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_COMPONENT_VERSION"
   */
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldIndex + 1].fieldTag =
      E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_COMPONENT_VERSION;
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldIndex + 1].fieldLen =
      xpRegInfo->xComponents[i].componentVersionLen;
    xpProtoMessage->commands[currentPos].data.fieldList.fields[fieldIndex + 1].fieldValue =
      xpRegInfo->xComponents[i].componentVersion;

    // Increment fieldIndex by 2 to move to the next pair of fields
    fieldIndex += 2;
    }
  }

#endif // FOTA_ENABLE

  status = E_K_STATUS_OK;

  return status;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

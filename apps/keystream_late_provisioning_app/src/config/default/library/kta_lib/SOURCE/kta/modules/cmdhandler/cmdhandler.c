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
/** \brief keySTREAM Trusted Agent - ICPP command handler.
 *
 * \author Kudelski IoT
 *
 * \date 2023/06/12
 *
 * \file cmdhandler.c
 *
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent ICPP command handler.
 */
#include "cmdhandler.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "icpp_parser.h"
#include "k_sal_platform.h"
#include "k_sal_object.h"
#include "k_sal_storage.h"
#include "k_crypto.h"
#include "k_sal.h"
#include "k_kta.h"
#include "k_defs.h"
#include "KTALog.h"
#include "general.h"
#include "cryptoConfig.h"

#include <string.h>
/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */
/** @brief Maximum size of thirdparty error code size. */
#define C_K_ICPP_MAX_THIRDPARTY_ERROR_CODE_SIZE         (2u)

/** @brief Maximum size of third party error code and maximum commands count. */
#define C_K_ICPP_PARSER_MAX_COUNT_THIRDPARTY_ERROR_SIZE \
  (C_K_ICPP_MAX_THIRDPARTY_ERROR_CODE_SIZE * C_K_ICPP_PARSER__MAX_COMMANDS_COUNT)

/** @brief Maximum size of ICPP message header. */
#define C_K_ICPP_PARSER_HEADER_SIZE                     (21u)

/** @brief Maximum length of processing status field. */
#define C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH  (0x04)

/** @brief Maximum public key length. */
#define C_K_PUBLIC_KEY_LENGTH                           (64u)

/** @brief Maximal size of command field in bytes. */
#define C_K_CMD_FIELD_MAX_SIZE                          (1024u)

#ifdef OBJECT_MANAGEMENT_FEATURE
/** @brief Mandatory generate key pair fields. */
#define C_K_CMD_GENKEYPAIR_FIELDS_MANDATORY \
  (E_K_CMD_FIELD_OBJECT_ID)

/** @brief Optional generate key pair fields. */
#define C_K_CMD_GENKEYPAIR_FIELDS_OPTIONAL \
  ((E_K_CMD_FIELD_OBJECT_ID | E_K_CMD_FIELD_DATA_ATTRIBUTES \
   | E_K_CMD_FIELD_OBJECT_OWNER))

/** @brief Mandatory set object fields. */
#define C_K_CMD_SETOBJ_FIELDS_MANDATORY \
  ((E_K_CMD_FIELD_OBJECT_TYPE | E_K_CMD_FIELD_OBJECT_ID \
   | E_K_CMD_FIELD_DATA))

/** @brief Optional set object fields. */
#define C_K_CMD_SETOBJ_FIELDS_OPTIONAL \
  ((E_K_CMD_FIELD_OBJECT_TYPE | E_K_CMD_FIELD_OBJECT_ID \
   | E_K_CMD_FIELD_DATA | E_K_CMD_FIELD_DATA_ATTRIBUTES \
   | E_K_CMD_FIELD_OBJECT_OWNER))

/** @brief Mandatory set object association fields. */
#define C_K_CMD_SETOBJASSOCIATION_FIELDS_MANDATORY \
  ((E_K_CMD_FIELD_OBJECT_TYPE | E_K_CMD_FIELD_OBJECT_ID \
   | E_K_CMD_FIELD_DATA | E_K_CMD_FIELD_ASSOCIATION_INFO))

/** @brief Mandatory delete object fields. */
#define C_K_CMD_DELETEOBJ_FIELDS_MANDATORY \
  ((E_K_CMD_FIELD_OBJECT_ID | E_K_CMD_FIELD_OBJECT_TYPE))

/** @brief Optional delete object fields. */
#define C_K_CMD_DELETEOBJ_FIELDS_OPTIONAL \
  ((E_K_CMD_FIELD_OBJECT_ID | E_K_CMD_FIELD_OBJECT_TYPE \
   | E_K_CMD_FIELD_OBJECT_OWNER))

/** @brief Mandatory delete key object fields. */
#define C_K_CMD_DELETEKEYOBJ_FIELDS_MANDATORY \
  (E_K_CMD_FIELD_OBJECT_ID)

/** @brief Optional delete key object fields. */
#define C_K_CMD_DELETEKEYOBJ_FIELDS_OPTIONAL \
  ((E_K_CMD_FIELD_OBJECT_ID | E_K_CMD_FIELD_OBJECT_TYPE))

/** @brief  Command response fields structure. */
typedef struct
{
  uint8_t*  pValue;
  /* Response field value. */
  size_t    len;
  /* Response field length. */
} TKcmdRespFields;

/** @brief  Command response payload structure. */
typedef struct
{
  uint32_t objectType;
  /* Type of the object. */
  uint32_t objectId;
  /* Persistent object identifier. */
  uint32_t objectOwner;
  /* Object owner. */
  TKcmdRespFields dataAttribtes;
  /* Response data attributes. */
  TKcmdRespFields data;
  /* Response data fields. */
  TKSalObjAssociationInfo associationInfo;
  /* Object association info. */
} TKcmdRespPayload;

/** @brief  Different commands for field tag. */
typedef enum
{
  /**
   * Object type field mask
   */
  E_K_CMD_FIELD_OBJECT_TYPE = 0x01u,
  /**
   * Object id field mask
   */
  E_K_CMD_FIELD_OBJECT_ID = 0x02u,
  /**
   * Data Attribute field mask
   */
  E_K_CMD_FIELD_DATA_ATTRIBUTES = 0x04u,
  /**
   * Data field mask
   */
  E_K_CMD_FIELD_DATA = 0x08u,
  /**
   * Association info field mask
   */
  E_K_CMD_FIELD_ASSOCIATION_INFO = 0x10u,
  /**
   * Object owner field mask
   */
  E_K_CMD_FIELD_OBJECT_OWNER = 0x20u
} TKcmdFieldTag;

#endif

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
static const char* gpModuleName = "KTACMDHANDLER";

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
#ifdef OBJECT_MANAGEMENT_FEATURE

/**
 * @brief
 *   Validate the command tag and field tag are mapped proper.
 *
 * @param[in] xCmdTag
 *   Should not be NULL.
 *   Command tag received.
 * @param[in] xFieldTagMask
 *   Should not be NULL.
 *   Field tag received.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lKtaCmdCheckFieldTag
(
  TKIcppCommandTag xCmdTag,
  uint32_t         xFieldTagMask
);

/**
 * @brief
 *   Validate the received commands from keySTREAM and prepare payload.
 *
 * @param[in] xpRecvMsg.
 *   Should not be NULL.
 *   Icpp message received from server.
 * @param[out] xpCmdRespPayload
 *   Response payload.
 * @param[in] xCmdCount
 *   Command count.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lKtaCmdValidateAndGetPayload
(
  TKIcppProtocolMessage*   xpRecvMsg,
  TKcmdRespPayload*        xpCmdRespPayload,
  uint32_t                 xCmdCount
);

/**
 * @brief
 *   Validate the generate key pair payload received from keySTREAM, generates a new key pair and
 *   persists private key in a platform and returns the public key.
 *
 * @param[in] xpData
 *   Should not be NULL.
 *   Buffer contains payload for generate key pair recevied from keySTREAM.
 * @param[out] xpOutData
 *   Should not be NULL. Buffer should be provided by caller.
 *   Buffer will be filled with generated data.
 * @param[in,out] xpDataSize
 *   Should not be NULL.
 *   [in] Size of xpOutData buffer.
 *   [out] Size of the data filled in buffer.
 * @param[out] xpPlatformStatus
 *   Platform status from SAL API.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lKtaGenerateKeyPair
(
  TKIcppProtocolMessage* xpData,
  uint8_t*               xpOutData,
  size_t*                xpDataSize,
  uint8_t*               xpPlatformStatus,
  uint8_t                xCommandCount
);

/**
 * @brief
 *   Validate the received set object command from keySTREAM and calls respective SAL API
 *   for storing the data in permanent storage.
 *
 * @param[in] xpData
 *   Should not be NULL.
 *   Icpp message received from server.
 * @param[out] xpPlatformStatus
 *   Platform status from SAL API.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lKtaSetObject
(
  TKIcppProtocolMessage* xpData,
  uint8_t*               xpPlatformStatus
);

/**
 * @brief
 *   Validate the received set object with association command from keySTREAM and calls respective
 *   SAL API for storing the data in permanent storage.
 *
 * @param[in] xpData
 *   Should not be NULL.
 *   Icpp message received from server.
 * @param[out] xpPlatformStatus
 *   Platform status from SAL API.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lKtaSetObjWithAssociation
(
  TKIcppProtocolMessage* xpData,
  uint8_t*               xpPlatformStatus
);

/**
 * @brief
 *   Validate the received commands from keySTREAM and calls respective SAL API
 *   for deleting the data from permanent storage.
 *
 * @param[in] xpData
 *   Should not be NULL.
 *   Icpp message received from server.
 * @param[out] xpPlatformStatus
 *   Platform status from SAL API.
 * @param[in] xCmdCount
 *   Command count.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lKtaDeleteObject
(
  TKIcppProtocolMessage* xpData,
  uint8_t*               xpPlatformStatus,
  uint32_t               xCmdCount
);

/**
 * @brief
 *   Validate the received delete key object commands from keySTREAM and calls respective SAL API
 *   for deleting the key data from permanent storage.
 *
 * @param[in] xpData
 *   Should not be NULL.
 *   Icpp message received from server.
 * @param[out] xpPlatformStatus
 *   Platform status from SAL API.
 * @param[in] xCmdCount
 *   Command count.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lKtaDeleteKeyObject
(
  TKIcppProtocolMessage* xpData,
  uint8_t*               xpPlatformStatus,
  uint32_t               xCmdCount
);
#endif
#ifdef PLATFORM_PROCESS_FEATURE
/**
 * @brief
 *   Set third-party payload data.
 *
 * @param[in] xpData
 *   Should not be NULL.
 *   Buffer contains payload for third-party data.
 * @param[in] xDatadSize
 *   third-party data Size.
 * @param[out] xpOutData
 *   Should not be NULL. Buffer should be provided by caller.
 *   Buffer will be filled with thirdparty data.
 * @param[in,out] xpOutDataSize
 *   Should not be NULL.
 *   [in] Size of the xpOutData buffer.
 *   [out] Size of the filled third-party data.
 *   Buffer will be filled with third-party data.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lKtaSetThirdPartyData
(
  const uint8_t* xpData,
  const size_t   xDatadSize,
  uint8_t*       xpOutData,
  size_t*        xpOutDataSize
);
#endif

/**
 * @brief
 *   Process command and prepare response message.
 *
 * @param[in] xpRecvdProtoMessage
 *   ICPP structure contains data received from server.
 * @param[in,out] xpSendProtoMessage
 *   [in] Protocol message structure to contain notification message info.
 *   [out] Filled protocol message structure with necessary info.
 *   Output structure to send to server back.
 * @param[in,out] xpCmdResponse
 *   Holds the address of buffer to store the response data.
 * @param[in] xCmdItemSize
 *   Holds the size of 2 bytes, to prepare response data for each command.
 * @param[out] xpPlatformStatus
 *   Platform status from SAL API.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lProcessCmdPrepareResponse
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  TKIcppProtocolMessage* xpSendProtoMessage,
  uint8_t*               xpCmdResponse,
  uint32_t               xCmdItemSize
#ifdef OBJECT_MANAGEMENT_FEATURE
  , uint8_t*              xpPlatformStatus
#endif
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief implement ktaCmdProcess
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaCmdProcess
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  uint8_t*               xpMessageToSend,
  size_t*                xpMessageToSendSize
)
{
  TKIcppProtocolMessage sendProtoMessage;
  TKStatus status           = E_K_STATUS_ERROR;
#ifdef OBJECT_MANAGEMENT_FEATURE
  uint8_t aPlatformStatus[4] = {0};
  uint8_t aCmdResponse[C_K__ICPP_CMD_RESPONSE_SIZE_VENDOR_SPECIFIC] = {0};
  size_t  cmdResponseSize = sizeof(aCmdResponse);
#else
  uint8_t aCmdResponse[C_K_ICPP_PARSER_MAX_COUNT_THIRDPARTY_ERROR_SIZE] = {0};
  size_t  cmdResponseSize = C_K_ICPP_PARSER_MAX_COUNT_THIRDPARTY_ERROR_SIZE;
#endif

  M_KTALOG__START("Start");

  // REQ RQ_M-KTA-STRT-FN-0300(1) : Input Parameters Check.
  if ((NULL == xpRecvdProtoMessage) || (NULL == xpMessageToSend) || (NULL == xpMessageToSendSize)
      || (0u == *xpMessageToSendSize))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter passed");
  }
  else
  {
    /* keySTREAM should give activation response based on L2 key. */
    if ((uint8_t)E_K_ICPP_PARSER_CRYPTO_TYPE_L2_BASED != xpRecvdProtoMessage->cryptoVersion)
    {
      M_KTALOG__ERR("Invalid crypto version received from the server, cryptoVersion = [%d]",
                    xpRecvdProtoMessage->cryptoVersion);
      goto end;
    }

    /* Fill the message type with "E_K_ICCP_PARSER_MESSAGE_TYPE_RESPONSE" to indicate it is
       registration notification message type (client -> server). */
    // REQ RQ_M-KTA-OBJM-FN-0100_03(1) : message type
    // REQ RQ_M-KTA-TRDP-FN-0110_03(1) : Third Party Response message type
    sendProtoMessage.msgType  = E_K_ICPP_PARSER_MESSAGE_TYPE_RESPONSE;
    // REQ RQ_M-KTA-OBJM-FN-0100_01(1) : crypto version
    // REQ RQ_M-KTA-OBJM-FN-0080(1) : Crypto version from keySTEREAM in Generate key pair.
    // REQ RQ_M-KTA-OBJM-FN-0280(1) : Crypto version from keySTEREAM in Set Object.
    // REQ RQ_M-KTA-OBJM-FN-0580(1) : Crypto version from keySTEREAM in Delete Object.
    // REQ RQ_M-KTA-OBJM-FN-0780(1) : Crypto version from keySTEREAM in
    // Set Object With Association.
    // REQ RQ_M-KTA-OBJM-FN-0980(2) : Crypto version from keySTEREAM in Delete Key Object */
    // REQ RQ_M-KTA-TRDP-FN-0080(1) : Crypto version from keySTEREAM in Third Party.
    // REQ RQ_M-KTA-TRDP-FN-0110_01(1) : Third Party Response crypto version.
    sendProtoMessage.cryptoVersion = xpRecvdProtoMessage->cryptoVersion;
    // REQ RQ_M-KTA-OBJM-FN-0100_02(1) : partial encryption mode
    // REQ RQ_M-KTA-TRDP-FN-0110_02(1) : Third Party Response partial encryption mode
    sendProtoMessage.encMode = xpRecvdProtoMessage->encMode;
    // REQ RQ_M-KTA-OBJM-FN-0100_06(1) : rot public uid
    // REQ RQ_M-KTA-TRDP-FN-0110_06(1) : Third Party Response rot public uid
    (void)memcpy(sendProtoMessage.rotPublicUID,
                 xpRecvdProtoMessage->rotPublicUID,
                 C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES);
    // REQ RQ_M-KTA-OBJM-FN-0100_04(1) : transaction id
    // REQ RQ_M-KTA-TRDP-FN-0110_04(1) : Third Party Response transaction id
    (void)memcpy(sendProtoMessage.transactionId, xpRecvdProtoMessage->transactionId,
                 C_K_ICPP_PARSER__TRANSACTION_ID_SIZE_IN_BYTES);
    // REQ RQ_M-KTA-OBJM-FN-0100_05(1) : rot key set id
    // REQ RQ_M-KTA-TRDP-FN-0110_05(1) : Third Party Response rot key set id
    sendProtoMessage.rotKeySetId = xpRecvdProtoMessage->rotKeySetId;

    status = lProcessCmdPrepareResponse(xpRecvdProtoMessage,
                                        &sendProtoMessage,
                                        aCmdResponse,
                                        cmdResponseSize
#ifdef OBJECT_MANAGEMENT_FEATURE
                                        , (uint8_t*)&aPlatformStatus
#endif
                                       );

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("Processing command or preparing response failed, status = [%d]", status);
      goto end;
    }

    status = ktaGenerateResponse((C_GEN__SERIALIZE | C_GEN__PADDING |
                                  C_GEN__ENCRYPT | C_GEN__SIGNING),
                                 &sendProtoMessage,
                                 xpMessageToSend,
                                 xpMessageToSendSize);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("ktaGenerateResponse failed, status = [%d]", status);
      goto end;
    }
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */
#ifdef OBJECT_MANAGEMENT_FEATURE
/**
 * @implements lKtaCmdCheckFieldTag
 *
 */
static TKStatus lKtaCmdCheckFieldTag
(
  TKIcppCommandTag xCmdTag,
  uint32_t         xFieldTagMask
)
{
  TKStatus  status = E_K_STATUS_ERROR;
  uint32_t  mandatorymask = 0;
  uint32_t  optionalmask = 0;

  switch (xCmdTag)
  {
    case E_K_ICPP_PARSER_COMMAND_TAG_GENERATE_KEY_PAIR:
    {
      mandatorymask = xFieldTagMask & C_K_CMD_GENKEYPAIR_FIELDS_MANDATORY;
      optionalmask = xFieldTagMask & C_K_CMD_GENKEYPAIR_FIELDS_OPTIONAL;
      if ((mandatorymask == C_K_CMD_GENKEYPAIR_FIELDS_MANDATORY)
          || (optionalmask == C_K_CMD_GENKEYPAIR_FIELDS_OPTIONAL))
      {
        status = E_K_STATUS_OK;
      }
    }
    break;

    case E_K_ICPP_PARSER_COMMAND_TAG_SET_OBJECT:
    {
      mandatorymask = xFieldTagMask & C_K_CMD_SETOBJ_FIELDS_MANDATORY;
      optionalmask = xFieldTagMask & C_K_CMD_SETOBJ_FIELDS_OPTIONAL;
      if ((mandatorymask == C_K_CMD_SETOBJ_FIELDS_MANDATORY)
          || (optionalmask == C_K_CMD_SETOBJ_FIELDS_OPTIONAL))
      {
        status = E_K_STATUS_OK;
      }
    }
    break;

    case E_K_ICPP_PARSER_CMD_TAG_SET_OBJ_WITH_ASSOCIATION:
    {
      mandatorymask = xFieldTagMask & C_K_CMD_SETOBJASSOCIATION_FIELDS_MANDATORY;
      if (mandatorymask == C_K_CMD_SETOBJASSOCIATION_FIELDS_MANDATORY)
      {
        status = E_K_STATUS_OK;
      }
    }
    break;

    case E_K_ICPP_PARSER_COMMAND_TAG_DELETE_OBJECT:
    {
      mandatorymask = xFieldTagMask & C_K_CMD_DELETEOBJ_FIELDS_MANDATORY;
      optionalmask = xFieldTagMask & C_K_CMD_DELETEOBJ_FIELDS_OPTIONAL;
      if ((mandatorymask == C_K_CMD_DELETEOBJ_FIELDS_MANDATORY)
          || (optionalmask == C_K_CMD_DELETEOBJ_FIELDS_OPTIONAL))
      {
        status = E_K_STATUS_OK;
      }
    }
    break;

    case E_K_ICPP_PARSER_CMD_TAG_DELETE_KEY_OBJECT:
    {
      mandatorymask = xFieldTagMask & C_K_CMD_DELETEKEYOBJ_FIELDS_MANDATORY;
      optionalmask = xFieldTagMask & C_K_CMD_DELETEKEYOBJ_FIELDS_OPTIONAL;
      if ((mandatorymask == C_K_CMD_DELETEKEYOBJ_FIELDS_MANDATORY)
          || (optionalmask == C_K_CMD_DELETEKEYOBJ_FIELDS_OPTIONAL))
      {
        status = E_K_STATUS_OK;
      }
    }
    break;

    default:
    {
      /* Handle unsupported command tags */
    }
    break;
  }

  return status;
}

/**
 * @implements lKtaCmdValidateAndGetPayload
 *
 */
static TKStatus lKtaCmdValidateAndGetPayload
(
  TKIcppProtocolMessage* xpRecvMsg,
  TKcmdRespPayload*      xpCmdRespPayload,
  uint32_t               xCmdCount
)
{
  TKStatus  status              = E_K_STATUS_ERROR;
  uint32_t  commandsLoop        = 0;
  uint32_t  fieldsLoop          = 0;
  uint32_t  isErrorOccured      = 0;
  uint32_t  fieldTagMask        = 0;
  TKIcppFieldList* pFieldList   = NULL;

  for (;;)
  {
    if (0u == xpRecvMsg->commandsCount)
    {
      M_KTALOG__ERR("Command Count is 0");
      status = E_K_STATUS_PARAMETER;
      break;
    }

    commandsLoop = xCmdCount;

    for (; ((commandsLoop < xpRecvMsg->commandsCount) && (isErrorOccured == 0U)); commandsLoop++)
    {
      if ((E_K_ICPP_PARSER_COMMAND_TAG_GENERATE_KEY_PAIR !=
           xpRecvMsg->commands[commandsLoop].commandTag) &&
          (E_K_ICPP_PARSER_COMMAND_TAG_SET_OBJECT !=
           xpRecvMsg->commands[commandsLoop].commandTag)  &&
          (E_K_ICPP_PARSER_CMD_TAG_SET_OBJ_WITH_ASSOCIATION !=
           xpRecvMsg->commands[commandsLoop].commandTag)  &&
          (E_K_ICPP_PARSER_COMMAND_TAG_DELETE_OBJECT !=
           xpRecvMsg->commands[commandsLoop].commandTag)  &&
          (E_K_ICPP_PARSER_CMD_TAG_DELETE_KEY_OBJECT !=
           xpRecvMsg->commands[commandsLoop].commandTag))
      {
        M_KTALOG__ERR("Invalid command Tag %d", xpRecvMsg->commands[commandsLoop].commandTag);
        isErrorOccured = 1u;
        break;
      }

      pFieldList = &xpRecvMsg->commands[commandsLoop].data.fieldList;

      if (0u == pFieldList->fieldsCount)
      {
        M_KTALOG__ERR("Invalid fieldsCount : %d", pFieldList->fieldsCount);
        isErrorOccured = 1u;
        break;
      }

      for (; (fieldsLoop < pFieldList->fieldsCount) && (isErrorOccured == 0u); fieldsLoop++)
      {
        switch (pFieldList->fields[fieldsLoop].fieldTag)
        {
          // REQ RQ_M-KTA-OBJM-FN-0270_01(1) : Object Type
          // REQ RQ_M-KTA-OBJM-FN-0570_01(1) : Object Type
          // REQ RQ_M-KTA-OBJM-FN-0770_01(1) : Object Type
          case E_K_ICPP_PARSER_FIELD_TAG_CMD_OBJECT_TYPE:
          {
            if (C_K_KTA__CMD_FIELD_MAX_SIZE < pFieldList->fields[fieldsLoop].fieldLen)
            {
              M_KTALOG__ERR("Invalid obj length");
              isErrorOccured = 1u;
              break;
            }

            xpCmdRespPayload->objectType = pFieldList->fields[fieldsLoop].fieldValue[0];
            fieldTagMask |= E_K_CMD_FIELD_OBJECT_TYPE;
          }
          break;

          // REQ RQ_M-KTA-OBJM-FN-0070_01(1) : Key Id
          // REQ RQ_M-KTA-OBJM-FN-0270_02(1) : Object Id
          // REQ RQ_M-KTA-OBJM-FN-0570_02(1) : Object Id
          // REQ RQ_M-KTA-OBJM-FN-0770_02(1) : Object Id
          // REQ RQ_M-KTA-OBJM-FN-0970_01(2) : Key Id
          case E_K_ICPP_PARSER_FLD_TAG_CMD_OBJECT_ID:
          {
            if (C_K_KTA__CMD_FIELD_MAX_SIZE < pFieldList->fields[fieldsLoop].fieldLen)
            {
              M_KTALOG__ERR("Invalid Identifier length");
              isErrorOccured = 1u;
              break;
            }
            xpCmdRespPayload->objectId = ((uint32_t)pFieldList->fields[fieldsLoop].fieldValue[0] << 24) |
                                          ((uint32_t)pFieldList->fields[fieldsLoop].fieldValue[1] << 16) |
                                          ((uint32_t)pFieldList->fields[fieldsLoop].fieldValue[2] << 8) |
                                          (uint32_t)pFieldList->fields[fieldsLoop].fieldValue[3];
            fieldTagMask |= E_K_CMD_FIELD_OBJECT_ID;
          }
          break;

          // REQ RQ_M-KTA-OBJM-FN-0070_02(1) : Key Attributes
          // REQ RQ_M-KTA-OBJM-FN-0270_03(1) : Data Attributes
          // REQ RQ_M-KTA-OBJM-FN-0770_03(1) : Data Attributes
          case E_K_ICPP_PARSER_FIELD_TAG_CMD_DATA_ATTRIBUTES:
          {
            if (C_K_KTA__CMD_FIELD_MAX_SIZE < pFieldList->fields[fieldsLoop].fieldLen)
            {
              M_KTALOG__ERR("Invalid data attributes length");
              isErrorOccured = 1u;
              break;
            }

            xpCmdRespPayload->dataAttribtes.pValue = pFieldList->fields[fieldsLoop].fieldValue;
            xpCmdRespPayload->dataAttribtes.len = pFieldList->fields[fieldsLoop].fieldLen;
            fieldTagMask |= E_K_CMD_FIELD_DATA_ATTRIBUTES;
          }
          break;

          // REQ RQ_M-KTA-OBJM-FN-0270_04(1) : Data
          // REQ RQ_M-KTA-OBJM-FN-0770_04(1) : Data
          case E_K_ICPP_PARSER_FLD_TAG_CMD_DATA:
          {
            if (C_K_KTA__CMD_FIELD_MAX_SIZE < pFieldList->fields[fieldsLoop].fieldLen)
            {
              M_KTALOG__ERR("Invalid Data length");
              isErrorOccured = 1u;
              break;
            }

            xpCmdRespPayload->data.pValue = pFieldList->fields[fieldsLoop].fieldValue;
            xpCmdRespPayload->data.len = pFieldList->fields[fieldsLoop].fieldLen;
            fieldTagMask |= E_K_CMD_FIELD_DATA;
          }
          break;

          // REQ RQ_M-KTA-OBJM-FN-0770_05(1) : Association Info
          case E_K_ICPP_PARSER_FIELD_TAG_CMD_ASSOCIATION_INFO:
          {
            if (C_K_KTA__CMD_FIELD_MAX_SIZE < pFieldList->fields[fieldsLoop].fieldLen)
            {
              M_KTALOG__ERR("Invalid Data length");
              isErrorOccured = 1u;
              break;
            }

            if (NULL != pFieldList->fields[fieldsLoop].fieldValue)
            {
              xpCmdRespPayload->associationInfo.associatedKeyId =
                (uint32_t)((pFieldList->fields[fieldsLoop].fieldValue[0] << 24) |
                (pFieldList->fields[fieldsLoop].fieldValue[1] << 16) |
                (pFieldList->fields[fieldsLoop].fieldValue[2] << 8) |
                pFieldList->fields[fieldsLoop].fieldValue[3]);
              xpCmdRespPayload->associationInfo.associatedKeyIdDeprecated =
                (uint32_t)((pFieldList->fields[fieldsLoop].fieldValue[4] << 24) |
                (pFieldList->fields[fieldsLoop].fieldValue[5] << 16) |
                (pFieldList->fields[fieldsLoop].fieldValue[6] << 8) |
                pFieldList->fields[fieldsLoop].fieldValue[7]);
              xpCmdRespPayload->associationInfo.associatedObjectId =
                (uint32_t)((pFieldList->fields[fieldsLoop].fieldValue[8] << 24) |
                (pFieldList->fields[fieldsLoop].fieldValue[9] << 16) |
                (pFieldList->fields[fieldsLoop].fieldValue[10] << 8) |
                pFieldList->fields[fieldsLoop].fieldValue[11]);
              xpCmdRespPayload->associationInfo.associatedObjectIdDeprecated =
                (uint32_t)((pFieldList->fields[fieldsLoop].fieldValue[12] << 24) |
                (pFieldList->fields[fieldsLoop].fieldValue[13] << 16) |
                (pFieldList->fields[fieldsLoop].fieldValue[14] << 8) |
                pFieldList->fields[fieldsLoop].fieldValue[15]);
              xpCmdRespPayload->associationInfo.associatedObjectType =
                pFieldList->fields[fieldsLoop].fieldValue[16];
            }

            fieldTagMask |= E_K_CMD_FIELD_ASSOCIATION_INFO;
          }
          break;

          // REQ RQ_M-KTA-OBJM-FN-0070_03(1) : Object Owner
          // REQ RQ_M-KTA-OBJM-FN-0270_05(1) : Object Owner
          // REQ RQ_M-KTA-OBJM-FN-0570_03(1) : Object Owner
          case E_K_ICPP_PRSR_FLD_TAG_CMD_OBJECT_OWNER:
          {
            if (C_K_KTA__CMD_FIELD_MAX_SIZE < pFieldList->fields[fieldsLoop].fieldLen)
            {
              M_KTALOG__ERR("Invalid Object Owner length");
              isErrorOccured = 1u;
              break;
            }

            xpCmdRespPayload->objectOwner = (uint32_t)((pFieldList->fields[fieldsLoop].fieldValue[0] << 24) |
                                            (pFieldList->fields[fieldsLoop].fieldValue[1] << 16) |
                                            (pFieldList->fields[fieldsLoop].fieldValue[2] << 8) |
                                            pFieldList->fields[fieldsLoop].fieldValue[3]);
            fieldTagMask |= E_K_CMD_FIELD_OBJECT_OWNER;
          }
          break;

          default:
          {
            M_KTALOG__ERR("Unknown Field Tag %d", pFieldList->fields[fieldsLoop].fieldTag);
            isErrorOccured = 1u;
          }
          break;
        } /* switch. */
      }

      if (E_K_STATUS_OK != lKtaCmdCheckFieldTag(xpRecvMsg->commands[commandsLoop].commandTag,
                                                fieldTagMask))
      {
        M_KTALOG__ERR("Command does not have all mandatory fields");
        status = E_K_STATUS_ERROR;
        break;
      }

      if (isErrorOccured == 0u)
      {
        status = E_K_STATUS_OK;
        break;
      }

      M_KTALOG__ERR("Type %x Id %x", xpCmdRespPayload->objectType, xpCmdRespPayload->objectId);
    }

    break;
  }

  return status;
}
#endif /* OBJECT_MANAGEMENT_FEATURE */

#ifdef PLATFORM_PROCESS_FEATURE

/**
 * @implements lKtaSetThirdPartyData
 *
 */
static TKStatus lKtaSetThirdPartyData
(
  const uint8_t* xpData,
  const size_t   xDatadSize,
  uint8_t*       xpOutData,
  size_t*        xpOutDataSize
)
{
  TKStatus status = E_K_STATUS_ERROR;

  for (;;)
  {
    M_KTALOG__DEBUG("Processing 3rd party specific data...");
    status = salPlatformProcess(xpData, xDatadSize, xpOutData, xpOutDataSize);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("SAL API failed while processing 3rd party data, status = [%d]", status);
    }

    break;
  }

  return status;
}

#endif /* PLATFORM_PROCESS_FEATURE */

#ifdef OBJECT_MANAGEMENT_FEATURE

/**
 * @implements lKtaGenerateKeyPair
 *
 */
static TKStatus lKtaGenerateKeyPair
(
  TKIcppProtocolMessage* xpData,
  uint8_t*               xpOutData,
  size_t*                xpDataSize,
  uint8_t*               xpPlatformStatus,
  uint8_t                xCommandCount
)
{
  TKStatus status = E_K_STATUS_ERROR;
  TKcmdRespPayload resPayload = { 0 };

  M_KTALOG__DEBUG("Processing GenerateKeyPair specific data...");

  // REQ RQ_M-KTA-OBJM-FN-0070(1) : Check generate key pair data
  if (E_K_STATUS_OK != lKtaCmdValidateAndGetPayload(xpData, &resPayload, xCommandCount))
  {
    /* Original error status lost here on purpose of code size optimization. */
    M_KTALOG__ERR("Getting field identifier, data attributes and object owner failed");
    goto end;
  }

  M_KTALOG__DEBUG("resPayload.objectId %d", resPayload.objectId);
  M_KTALOG__DEBUG("resPayload.dataAttribtes.len %d", resPayload.dataAttribtes.len);

  status = salObjectKeyGen(resPayload.objectId,
                            resPayload.dataAttribtes.pValue,
                            resPayload.dataAttribtes.len,
                            xpOutData, xpDataSize,
                            (uint8_t*)xpPlatformStatus);

end:
  return status;
}

/**
 * @implements lKtaSetObject
 *
 */
static TKStatus lKtaSetObject
(
  TKIcppProtocolMessage* xpData,
  uint8_t*               xpPlatformStatus
)
{
  TKStatus status = E_K_STATUS_ERROR;
  TKcmdRespPayload   resPayload = { 0 };

  M_KTALOG__DEBUG("Processing SetObject specific data...");

  // REQ RQ_M-KTA-OBJM-FN-0270(1) : Check set object data
  if (E_K_STATUS_OK != lKtaCmdValidateAndGetPayload(xpData, &resPayload, 0))
  {
    /* Original error status lost here on purpose of code size optimization. */
    M_KTALOG__ERR("Getting field identifier, data attributes and object owner failed");
    goto end;
  }

  // REQ RQ_M-KTA-RENW-MCHP-FN-0020(1) : Update Device Certificate
  // REQ RQ_M-KTA-RENW-SLAB-FN-0020(1) : Update Device Certificate
  status = salObjectSet(resPayload.objectType,
                        resPayload.objectId,
                        resPayload.dataAttribtes.pValue,
                        resPayload.dataAttribtes.len,
                        resPayload.data.pValue,
                        resPayload.data.len,
                        xpPlatformStatus);

end:
  return status;
}

/**
 * @implements lKtaSetObjWithAssociation
 *
 */
static TKStatus lKtaSetObjWithAssociation
(
  TKIcppProtocolMessage* xpData,
  uint8_t*               xpPlatformStatus
)
{
  TKStatus status = E_K_STATUS_ERROR;
  TKcmdRespPayload  resPayload = { 0 };

  M_KTALOG__DEBUG("Processing SetObjectWithAssociation specific data...");

  // REQ RQ_M-KTA-OBJM-FN-0770(1) : Check set object with association data
  if (E_K_STATUS_OK != lKtaCmdValidateAndGetPayload(xpData, &resPayload, 0))
  {
    /* Original error status lost here on purpose of code size optimization. */
    M_KTALOG__ERR("Getting field identifier, association info and object owner failed");
    goto end;
  }

  status = salObjectSetWithAssociation(resPayload.objectType,
                                        resPayload.objectId,
                                        resPayload.dataAttribtes.pValue,
                                        resPayload.dataAttribtes.len,
                                        resPayload.data.pValue,
                                        resPayload.data.len,
                                        &(resPayload.associationInfo),
                                        xpPlatformStatus);

end:
  return status;
}

/**
 * @implements lKtaDeleteObject
 *
 */
static TKStatus lKtaDeleteObject
(
  TKIcppProtocolMessage* xpData,
  uint8_t*               xpPlatformStatus,
  uint32_t               xCmdCount
)
{
  TKStatus status = E_K_STATUS_ERROR;
  TKcmdRespPayload resPayload = { 0 };

  M_KTALOG__DEBUG("Processing DeleteObject specific data...");

  // REQ RQ_M-KTA-OBJM-FN-0570(1) : Check delete object data
  if (E_K_STATUS_OK != lKtaCmdValidateAndGetPayload(xpData, &resPayload, xCmdCount))
  {
    /* Original error status lost here on purpose of code size optimization. */
    M_KTALOG__ERR("Getting field identifier, data attributes and object owner failed");
    goto end;
  }

  // REQ RQ_M-KTA-RFSH-FN-0020(1) : Delete Keys/Certificates/Persistant Data
  // REQ RQ_M-KTA-STRT-FN-0410(1) : Delete Key/Certificate
  status = salObjectDelete(resPayload.objectType, resPayload.objectId, xpPlatformStatus);

end:
  return status;
}

/**
 * @implements lKtaDeleteKeyObject
 *
 */
static TKStatus lKtaDeleteKeyObject
(
  TKIcppProtocolMessage* xpData,
  uint8_t*               xpPlatformStatus,
  uint32_t               xCmdCount
)
{
  TKStatus status = E_K_STATUS_ERROR;
  TKcmdRespPayload resPayload = { 0 };

  M_KTALOG__DEBUG("Processing DeleteKeyObject specific data...");

  // REQ RQ_M-KTA-OBJM-FN-0970(2) : Check delete key object data
  if (E_K_STATUS_OK != lKtaCmdValidateAndGetPayload(xpData, &resPayload, xCmdCount))
  {
    /* Original error status lost here on purpose of code size optimization. */
    M_KTALOG__ERR("Getting the proper object ID got failed");
    goto end;
  }

  status = salObjectKeyDelete(resPayload.objectId,
                                xpPlatformStatus);

end:
  return status;
}

#endif

/**
 * @implements lProcessCmdPrepareResponse
 *
 */
static TKStatus lProcessCmdPrepareResponse
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  TKIcppProtocolMessage* xpSendProtoMessage,
  uint8_t*               xpCmdResponse,
  uint32_t               xCmdItemSize
#ifdef OBJECT_MANAGEMENT_FEATURE
  , uint8_t*              xpPlatformStatus
#endif
)
{
  TKStatus status     = E_K_STATUS_ERROR;
  size_t commandCount = 0;
  size_t dataSize     = 0;

  if ((NULL == xpRecvdProtoMessage) || (NULL == xpSendProtoMessage) ||
      (NULL == xpCmdResponse) || (0u == xCmdItemSize)
#ifdef OBJECT_MANAGEMENT_FEATURE
      || (NULL == xpPlatformStatus))
#else
      )
#endif
  {
    status = E_K_STATUS_PARAMETER;
  }
  else
  {
    xpCmdResponse[0] = 0;
    xpSendProtoMessage->commandsCount = xpRecvdProtoMessage->commandsCount;
    for (commandCount = 0; commandCount < xpRecvdProtoMessage->commandsCount; commandCount++)
    {
      switch (xpRecvdProtoMessage->commands[commandCount].commandTag)
      {
#ifdef PLATFORM_PROCESS_FEATURE

        // REQ RQ_M-KTA-TRDP-FN-0010(1) : Verify Third party Signature
        // REQ RQ_M-KTA-TRDP-FN-0110(1) : Third party response ICPP Message
        case E_K_ICPP_PARSER_COMMAND_TAG_THIRD_PARTY:
        {
          dataSize = xCmdItemSize;
          // REQ RQ_M-KTA-TRDP-FN-0070(1) : Check third party data
          status = lKtaSetThirdPartyData(
                     xpRecvdProtoMessage->commands[commandCount].data.cmdInfo.cmdValue,
                     xpRecvdProtoMessage->commands[commandCount].data.cmdInfo.cmdLen,
                     xpCmdResponse, &dataSize);

          if (E_K_STATUS_OK == status)
          {
            /**
             * Set the command tag with "E_K_ICCP_PARSER_COMMAND_TAG_THIRD_PARTY_FIELD"
             * to indicated the the command is for third party data.
             */
            // REQ RQ_M-KTA-TRDP-FN-0090(1) : Build Third party response
            // REQ RQ_M-KTA-TRDP-FN-0100(1) : Third party response data order.
            xpSendProtoMessage->commands[commandCount].commandTag =
              E_K_ICPP_PARSER_COMMAND_TAG_THIRD_PARTY;
            xpSendProtoMessage->commands[commandCount].data.cmdInfo.cmdValue = xpCmdResponse;
            xpSendProtoMessage->commands[commandCount].data.cmdInfo.cmdLen = dataSize;
            xpCmdResponse += dataSize;
          }
          else
          {
            M_KTALOG__ERR("Setting 3rd party data failed, status = [%d]", status);
            break;
          }
        }
        break;
#endif /* PLATFORM_PROCESS_FEATURE */

#ifdef OBJECT_MANAGEMENT_FEATURE

        // REQ RQ_M-KTA-OBJM-FN-0100(1) : Generate key pair ICPP Message
        case E_K_ICPP_PARSER_COMMAND_TAG_GENERATE_KEY_PAIR:
        {
          dataSize = xCmdItemSize;
          // REQ RQ_M-KTA-OBJM-FN-0010(1) : Verify Generate Key Pair Signature
          status = lKtaGenerateKeyPair(
                     xpRecvdProtoMessage, xpCmdResponse,
                     &dataSize, xpPlatformStatus, (uint8_t)commandCount);
          /**
           * Set the command tag with "E_K_ICPP_PARSER_COMMAND_TAG_GENERATE_KEY_PAIR"
           * to indicate that this command is to generate the key pair.
           */
          // REQ RQ_M-KTA-OBJM-FN-0090(1) : Build Generate key pair response
          xpSendProtoMessage->commands[commandCount].commandTag =
            E_K_ICPP_PARSER_COMMAND_TAG_GENERATE_KEY_PAIR;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fieldsCount = 2;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldTag =
            E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldLen =
            C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH;
          // REQ RQ_M-KTA-OBJM-FN-0090_01(1) : Command Processing Status
          // REQ RQ_M-KTA-OBJM-FN-0290_01(1) : Command Processing Status
          // REQ RQ_M-KTA-OBJM-FN-0590_01(1) : Command Processing Status
          xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldValue =
            (uint8_t*)xpPlatformStatus;
          // REQ RQ_M-KTA-OBJM-FN-0090_02(1) : Public Key
          // REQ RQ_M-KTA-OBJM-FN-0090_03(1) : Signed public key
          xpSendProtoMessage->commands[commandCount].data.fieldList.fields[1].fieldTag =
            (TKIcppFieldTag)C_K__ICPP_FIELD_TAG_PUB_KEY_VENDOR_SPECIFIC;

          if (E_K_STATUS_OK == status)
          {
            xpSendProtoMessage->commands[commandCount].data.fieldList.fields[1].fieldLen =
              dataSize;
            xpSendProtoMessage->commands[commandCount].data.fieldList.fields[1].fieldValue =
              xpCmdResponse;
          }
          else
          {
            M_KTALOG__ERR("Generate Key Pair command failed with status : [%d], setting field data to 0", status);
            xpSendProtoMessage->commands[commandCount].data.fieldList.fields[1].fieldLen = 0;
            xpSendProtoMessage->commands[commandCount].data.fieldList.fields[1].fieldValue = NULL;
          }
        }
        break;

        // REQ RQ_M-KTA-OBJM-FN-0300(1) : Set Object ICPP Message
        case E_K_ICPP_PARSER_COMMAND_TAG_SET_OBJECT:
        {
          // REQ RQ_M-KTA-OBJM-FN-0210(1) : Verify Set Object Signature
          status = lKtaSetObject(xpRecvdProtoMessage, xpPlatformStatus);

          /**
           * Set the command tag with "E_K_ICPP_PARSER_COMMAND_TAG_SET_OBJECT"
           * to indicate that this command is to set the data object.
           */
          // REQ RQ_M-KTA-OBJM-FN-0290(1) : Build Set Object response
          xpSendProtoMessage->commands[commandCount].commandTag =
            E_K_ICPP_PARSER_COMMAND_TAG_SET_OBJECT;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fieldsCount = 1;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldTag =
            E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldLen =
            C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH;
          if (E_K_STATUS_OK == status)
          {
            xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldValue =
            (uint8_t*)xpPlatformStatus;
          }
          else
          {
            M_KTALOG__ERR("Set object command failed with status = [%d]", status);
          }
        }
        break;

        // REQ RQ_M-KTA-OBJM-FN-0600(1) : Delete Object ICPP Message
        case E_K_ICPP_PARSER_COMMAND_TAG_DELETE_OBJECT:
        {
          // REQ RQ_M-KTA-OBJM-FN-0510(1) : Verify Delete Object Signature
          status = lKtaDeleteObject(xpRecvdProtoMessage, xpPlatformStatus, commandCount);

          /**
           * Set the command tag with "E_K_ICPP_PARSER_COMMAND_TAG_DELETE_OBJECT"
           * to indicate that this command is to delete the certificate object from device.
           */
          // REQ RQ_M-KTA-OBJM-FN-0590(1) : Build Delete Object response
          xpSendProtoMessage->commands[commandCount].commandTag =
            E_K_ICPP_PARSER_COMMAND_TAG_DELETE_OBJECT;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fieldsCount = 1;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldTag =
            E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldLen =
            C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH;
          // REQ RQ_M-KTA-OBJM-FN-0561_11(1) : Command Processing Status
          if (E_K_STATUS_OK == status)
          {
            xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldValue =
            (uint8_t*)xpPlatformStatus;
          }
          else
          {
            M_KTALOG__ERR("Delete object command failed with status = [%d]", status);
          }
        }
        break;

        // REQ RQ_M-KTA-OBJM-FN-1000(2) : Delete Key Object ICPP Message
        case E_K_ICPP_PARSER_CMD_TAG_DELETE_KEY_OBJECT:
        {
          // REQ RQ_M-KTA-OBJM-FN-0910(2) : Verify Delete Key Object Signature
          status = lKtaDeleteKeyObject(xpRecvdProtoMessage, xpPlatformStatus, commandCount);

          /**
           * Set the command tag with "E_K_ICPP_PARSER_CMD_TAG_DELETE_KEY_OBJECT"
           * to indicate that this command is to delete the key object from device.
           */
          // REQ RQ_M-KTA-OBJM-FN-0990(2) : Build Delete Key Object response
          xpSendProtoMessage->commands[commandCount].commandTag =
            E_K_ICPP_PARSER_CMD_TAG_DELETE_KEY_OBJECT;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fieldsCount = 1;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldTag =
            E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldLen =
            C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH;
          // REQ RQ_M-KTA-OBJM-FN-0990_01(2) : Command Processing Status
          if (E_K_STATUS_OK == status)
          {
            xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldValue =
            (uint8_t*)xpPlatformStatus;
          }
          else
          {
            M_KTALOG__ERR("Delete key object command failed with status = [%d]", status);
          }
        }
        break;

        // REQ RQ_M-KTA-OBJM-FN-0800(1) : Set Object With Association ICPP Message
        case E_K_ICPP_PARSER_CMD_TAG_SET_OBJ_WITH_ASSOCIATION:
        {
          // REQ RQ_M-KTA-OBJM-FN-0710(1) : Verify Set Object with Association Signature
          status = lKtaSetObjWithAssociation(xpRecvdProtoMessage, xpPlatformStatus);

          /**
           * Set the command tag with "E_K_ICPP_PARSER_CMD_TAG_SET_OBJ_WITH_ASSOCIATION"
           * to indicate that this command is for setting the association info data.
           */
          // REQ RQ_M-KTA-OBJM-FN-0790(1) : Build Set Object With Association response
          xpSendProtoMessage->commands[commandCount].commandTag =
            E_K_ICPP_PARSER_CMD_TAG_SET_OBJ_WITH_ASSOCIATION;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fieldsCount = 1;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldTag =
            E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS;
          xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldLen =
            C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH;
          // REQ RQ_M-KTA-OBJM-FN-0790_01(1) : Command Processing Status
          if (E_K_STATUS_OK == status)
          {
            xpSendProtoMessage->commands[commandCount].data.fieldList.fields[0].fieldValue =
            (uint8_t*)xpPlatformStatus;
          }
          else
          {
            M_KTALOG__ERR("Set object with association command failed with status = [%d]", status);
          }
        }
        break;
#endif /* OBJECT_MANAGEMENT_FEATURE */

        default:
          M_KTALOG__ERR("Received invalid command tag, cmdTag = [%x]",
                        xpRecvdProtoMessage->commands[commandCount].commandTag);
          break;
      }
    }
  }

  return status;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

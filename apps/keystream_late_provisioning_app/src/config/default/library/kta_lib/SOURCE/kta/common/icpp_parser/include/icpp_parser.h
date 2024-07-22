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
/** \brief keySTREAM Trusted Agent- ICPP parser to parse message received from
 *         keySTREAM.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file icpp_parser.h
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - ICPP parser to parse message received from
 *        keySTREAM.
 */

#ifndef ICPP_PARSER_H
#define ICPP_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include <stddef.h>
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */
/**
 * @brief ICPP total header size, in bytes.
 *
 * Header details(in bits)
 * +---------+---------+--------+---------+---------+-----+------------+-------+-------+-------+
 * | protocol| Reserved| crypto |Enc mode | Reserved| Msg | Transaction| Rot   | ROT   | Length|
 * | version |         | version|(partial/|         | type| id         | public| KeySet|       |
 * |         |         |       |full)    |          |     |            | UID   | Id    |       |
 * +---------+---------+--------+---------+---------+-----+------------+-------+-------+-------+
 * |   4     |    4    |    4   |      1  |     1   |   2 |   64       |  64   |  8    |  16   |
 * +---------+---------+--------+---------+---------+-----+------------+-------+-------+-------+
 */
#define C_K_ICPP_PARSER__HEADER_SIZE                             (21u)

/** @brief Maximum ICPP fields in the command. */
#define C_K_ICPP_PARSER__MAX_FIELDS_COUNT                        (8u)

/** @brief Maximum ICPP commands in the message. */
#define C_K_ICPP_PARSER__MAX_COMMANDS_COUNT                      (8u)

/** @brief ICPP RoT public UID size. */
#define C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES            (8u)

/** @brief ICPP RoT KeySet ID size. */
#define C_K_ICPP_PARSER__ROT_KEYSET_ID_SIZE_IN_BYTES             (1u)

/** @brief ICPP transaction ID size. */
#define C_K_ICPP_PARSER__TRANSACTION_ID_SIZE_IN_BYTES            (8u)

/** @brief ICPP command tag with length 1byte and field start. */
#define C_K_ICPP__1BYTE_START_WITH_FILED_RANGE_CMD_TAG    (0X40u)

/** @brief ICPP command tag with length 1byte and field end. */
#define C_K_ICPP__1BYTE_END_WITH_FILED_RANGE_CMD_TAG      (0X6Fu)

/** @brief ICPP command tag with length 1byte and without field start. */
#define C_K_ICPP__1BYTE_START_WITHOUT_FILED_RANGE_CMD_TAG (0X70u)

/** @brief ICPP command tag with length 1byte and without field end. */
#define C_K_ICPP__1BYTE_END_WITHOUT_FILED_RANGE_CMD_TAG   (0X7Fu)

/** @brief ICPP command tag with length 2byte and with field start. */
#define C_K_ICPP__2BYTE_START_WITH_FILED_RANGE_CMD_TAG    (0X80u)

/** @brief ICPP command tag with length 2byte and with field end. */
#define C_K_ICPP__2BYTE_END_WITH_FILED_RANGE_CMD_TAG      (0XAFu)

/** @brief ICPP command tag with length 2byte and with field start. */
#define C_K_ICPP__2BYTE_START_WITHOUT_FILED_RANGE_CMD_TAG (0XB0u)

/** @brief ICPP command tag with length 2byte and with field start. */
#define C_K_ICPP__2BYTE_END_WITHOUT_FILED_RANGE_CMD_TAG   (0XBFu)


/** @brief ICPP command tag has fields. */
#define M_K_ICPP_PARSER__COMMAND_TAG_HAS_FIELDS(x_cmdTag)   \
  ((((uint32_t)(x_cmdTag) >= C_K_ICPP__1BYTE_START_WITH_FILED_RANGE_CMD_TAG) && \
    ((uint32_t)(x_cmdTag) <= C_K_ICPP__1BYTE_END_WITH_FILED_RANGE_CMD_TAG)) || \
   (((uint32_t)(x_cmdTag) >= C_K_ICPP__2BYTE_START_WITH_FILED_RANGE_CMD_TAG) && \
    ((uint32_t)(x_cmdTag) <= C_K_ICPP__2BYTE_END_WITH_FILED_RANGE_CMD_TAG)))

/** @brief Supported status. */
typedef enum
{
  /**
   * Status OK, everything is fine.
   */
  E_K_ICPP_PARSER_STATUS_OK = 0x00u,
  /**
   * Bad parameter.
   */
  E_K_ICPP_PARSER_STATUS_PARAMETER,
  /**
   * Undefined error.
   */
  E_K_ICPP_PARSER_STATUS_ERROR,
  /**
   * No operation if header length 0.
   */
  E_K_ICPP_PARSER_STATUS_NO_OPERATION,
  /**
   * Command processing error from server.
   */
  E_K_ICPP_PARSER_STATUS_NOTIFICATION_CPERROR,
  /**
   * Number of status values.
   */
  E_K_ICPP_PARSER_NUM_STATUS
} TKParserStatus;

/** @brief Supported message types. */
typedef enum
{
  /**
   * Command message.
   */
  E_K_ICPP_PARSER_MESSAGE_TYPE_COMMAND = 0x00u,
  /**
   * Reponse message.
   */
  E_K_ICPP_PARSER_MESSAGE_TYPE_RESPONSE,
  /**
   * Notification message.
   */
  E_K_ICPP_PARSER_MESSAGE_TYPE_NOTIFICATION,
  /**
   * Message type reserved.
   */
  E_K_ICPP_PARSER_MSG_TYPE_RESERVED,
} TKIcppMessageType;


/** @brief Crypto key types used in operations. */
typedef enum
{
  /**
   * No encryption is done.
   */
  E_K_ICPP_PARSER_CRYPTO_TYPE_FULL_CLEAR = 0x00u,
  /**
   * L2 key based encryption is done.
   */
  E_K_ICPP_PARSER_CRYPTO_TYPE_L2_BASED   = 0x02u,
  /**
   * Dos key based encryption is done.
   */
  E_K_ICPP_PARSER_CRYPTO_TYPE_DOS_BASED  = 0x03u,
} TKIcppCryptoVersionType;

/** @brief Encryption modes. */
typedef enum
{
  /**
   * Full data is encrypted.
   */
  E_K_ICPP_PARSER_FULL_ENC_MODE = 0x00u,
  /**
   * Partial data is encrypted.
   */
  E_K_ICPP_PARSER_PARTIAL_ENC_MODE,
} TKIcppEncModeType;

/** @brief Supported command tags. */
typedef enum
{
  /**
   * Activation info tag.
   */
  E_K_ICPP_PARSER_COMMAND_TAG_ACTIVATION            = 0x83u,
  /**
   * Registeration info tag.
   */
  E_K_ICPP_PARSER_COMMAND_TAG_REGISTERATION_INFO    = 0x87u,
  /**
   * Third party tag - without fieds.
   */
  E_K_ICPP_PARSER_COMMAND_TAG_THIRD_PARTY           = 0xB0u,
  /**
   * Processing status tag.
   */
  E_K_ICPP_PARSER_COMMAND_TAG_PROCESSING_STATUS     = 0x70u,
  /**
   * Command processing error tag.
   */
  E_K_ICPP_PARSER_COMMAND_TAG_CMD_PROCESSING_ERROR  = 0x71u,
  /**
   * Generate key pair tag.
   */
  E_K_ICPP_PARSER_COMMAND_TAG_GENERATE_KEY_PAIR     = 0x90u,
  /**
   * Set object command tag.
   */
  E_K_ICPP_PARSER_COMMAND_TAG_SET_OBJECT            = 0x91u,
  /**
   * Set object with association command tag.
   */
  E_K_ICPP_PARSER_CMD_TAG_SET_OBJ_WITH_ASSOCIATION  = 0x92u,
  /**
   * Delete object command tag.
   */
  E_K_ICPP_PARSER_COMMAND_TAG_DELETE_OBJECT         = 0x50u,
  /**
   * Delete key object command tag.
   */
  E_K_ICPP_PARSER_CMD_TAG_DELETE_KEY_OBJECT         = 0x51u,
} TKIcppCommandTag;

/** @brief Supported Field Tags. */
typedef enum
{
  /**
   * Immutable device profile uid tag.
   */
  E_K_ICPP_PARSER_FIELD_TAG_DEVPROFUID            = 0x85u,
  /**
   * Mutable device profile uid tag.
   */
  E_K_ICPP_PARSER_FIELD_TAG_MUTABLE_DEVPROFUID    = 0x86u,
  /**
   * Rot Sol ID tag.
   */
  E_K_ICPP_PARSER_FIELD_TAG_ROT_SOL_ID            = 0x9Bu,
  /**
   * Chip uid tag.
   */
  E_K_ICPP_PARSER_FIELD_TAG_CHIP_UID              = 0xABu,
  /**
   * Rot Public uid tag.
   */
  E_K_ICPP_PARSER_FIELD_TAG_ROT_PUBLIC_UID        = 0xD1u,
  /**
   * Chip certificate tag.
   */
  E_K_ICPP_PARSER_FLD_TAG_CHIP_CERT               = 0xF3u,
  /**
   * Rot e_pk tag.
   */
  E_K_ICPP_PARSER_FIELD_TAG_ROT_E_PK              = 0xF4u,
  /**
   * Signed public key tag.
   */
  E_K_ICPP_PARSER_FIELD_TAG_SIGNED_PUB_KEY        = 0xF6u,
  /**
   * Chip attestation certificate tag.
   */
  E_K_ICPP_PARSER_FLD_TAG_CHIP_ATTEST_CERT        = 0xF9u,
  /**
   * Act seq_cnt tag.
   */
  E_K_ICPP_PARSER_FIELD_TAG_ACK_SEQ_CNT           = 0x92u,
  /**
   * keySTREAM Trusted Agent Capability tag.
   */
  E_K_ICPP_PARSER_FLD_TAG_KTA_CAPABILITY          = 0xACu,
  /**
   * keySTREAM Trusted Agent context profile uid tag.
   */
  E_K_ICPP_PARSER_FIELD_TAG_KTA_CTX_PRO_UID       = 0x8Du,
  /**
   * keySTREAM Trusted Agent context serial number tag.
   */
  E_K_ICPP_PARSER_FLD_TAG_KTA_CTX_SERIAL_NO       = 0x8Au,
  /**
   * keySTREAM Trusted Agent context version tag.
   */
  E_K_ICPP_PRSR_FLD_TAG_KTA_CTX_VER               = 0x8Cu,
  /**
   * keySTREAM Trusted Agent version tag.
   */
  E_K_ICPP_PARSER_FIELD_TAG_KTA_VER               = 0x89u,
  /**
   * Device serial number tag.
   */
  /* MISRA Rule 5.4 flags the following macro's name as ambiguous from the one */
  E_K_ICPP_PARSER_FIELD_TAG_DEV_SERIAL_NO         = 0x82u,
  /**
   * keySTREAM Ephemeral Public Key tag.
   */
  /* MISRA Rule 5.4 flags the following macro's name as ambiguous from the one */
  E_K_ICPP_PARSER_FIELD_TAG_KS_E_PK               = 0xF5u,
  /**
   * ObjectType field in command from keySTREAM.
   */
  E_K_ICPP_PARSER_FIELD_TAG_CMD_OBJECT_TYPE       = 0xB3u,
  /**
   * Identifier field in command from keySTREAM.
   */
  E_K_ICPP_PARSER_FLD_TAG_CMD_OBJECT_ID           = 0xB0u,
  /**
   * Data attributes field in command from keySTREAM.
   */
  E_K_ICPP_PARSER_FIELD_TAG_CMD_DATA_ATTRIBUTES   = 0xB1u,
  /**
   * Data field in command from keySTREAM.
   */
  E_K_ICPP_PARSER_FLD_TAG_CMD_DATA                = 0xF8u,
  /**
   * Association info field in command from keySTREAM.
   */
  E_K_ICPP_PARSER_FIELD_TAG_CMD_ASSOCIATION_INFO  = 0xB4u,
  /**
   * Object Owner optional field in command from keySTREAM.
   */
  /* MISRA Rule 5.4 flags the following macro's name as ambiguous from the one */
  E_K_ICPP_PRSR_FLD_TAG_CMD_OBJECT_OWNER          = 0xB2u,
  /**
   * Command processing status field tag in command from keySTREAM.
   */
  E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS = 0xBFu,
  /**
   * Public key field tag in command from keySTREAM.
   */
  E_K_ICPP_PARSER_FLD_TAG_CMD_PUBLIC_KEY          = 0xF7u,
} TKIcppFieldTag;

/** @brief ICPP field structure. */
typedef struct
{
  TKIcppFieldTag   fieldTag;
  /* ICPP field tag. */
  size_t           fieldLen;
  /* ICPP length of the field value. */
  uint8_t*         fieldValue;
  /* ICPP field value. */
} TKIcppField;

/** @brief ICPP field list structure. */
typedef struct
{
  size_t            fieldsCount;
  /* ICPP no of valid fields. */
  TKIcppField       fields[C_K_ICPP_PARSER__MAX_FIELDS_COUNT];
  /* List of ICPP fields. */
} TKIcppFieldList;

/** @brief ICPP command Info structure. */
typedef struct
{
  size_t           cmdLen;
  /* ICPP Length of the command value. */
  uint8_t*         cmdValue;
  /* ICPP command value. */
} TKIcppCommandInfo;

/** @brief ICPP command structure. */
typedef struct
{
  TKIcppCommandTag  commandTag;
  /* ICPP command tag. */
  union
  {
    TKIcppFieldList   fieldList;
    TKIcppCommandInfo cmdInfo;
  } data;
} TKIcppCommand;

/** @brief ICPP Message Structure. */
typedef struct
{
  uint8_t           cryptoVersion;
  /* Crypto version. */
  uint8_t           encMode;
  /* Encryption mode. */
  TKIcppMessageType msgType;
  /* ICPP Message type. */
  uint8_t           transactionId[C_K_ICPP_PARSER__TRANSACTION_ID_SIZE_IN_BYTES];
  /* Transaction ID to be used communicating with keySTREAM. */
  uint8_t           rotPublicUID[C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES];
  /* Device specific rot public UID. */
  uint8_t           rotKeySetId;
  /* Rot KeySet ID. */
  size_t            commandsCount;
  /* ICPP no of valid commands. */
  TKIcppCommand     commands[C_K_ICPP_PARSER__MAX_COMMANDS_COUNT];
  /* List of ICPP commands. */
} TKIcppProtocolMessage;

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Serialize the ICPP message.
 *
 * @param[in] xpIcppMessage
 *   Structure contains non serialized ICPP message.
 * @param[in,out] xpMessageToSend
 *   [in] Pointer to buffer to carry serialized message.
 *   [out] Actual serialized message.
 * @param[in,out] xpMessageToSendSize
 *   [in] Pointer to buffer to carry serialized message length.
 *   [out] Actual serialized message length.
 *
 * @return
 * - E_K_ICPP_PARSER_STATUS_OK in case of success.
 * - E_K_ICPP_PARSER_STATUS_PARAMETER for wrong input parameter.
 * - E_K_ICPP_PARSER_STATUS_ERROR for other errors.
 */
TKParserStatus ktaIcppParserSerializeMessage
(
  const TKIcppProtocolMessage* xpIcppMessage,
  uint8_t*                     xpMessageToSend,
  size_t*                      xpMessageToSendSize
);

/**
 * @brief
 *   Deserialize the ICPP message.
 *
 * @param[in] xpReceivedMessage
 *   Buffer contains ICPP message received from keySTREAM.
 * @param[in] xReceivedMessageSize
 *   Length of ICPP message received from keySTREAM.
 * @param[in,out] xpIcppMessage
 *   [in] Pointer to buffer to carry deserialized data.
 *   [out] Actual deserialized data.
 *
 * @return
 * - E_K_ICPP_PARSER_STATUS_OK in case of success.
 * - E_K_ICPP_PARSER_STATUS_PARAMETER for wrong input parameter.
 * - E_K_ICPP_PARSER_STATUS_ERROR for other errors.
 */
TKParserStatus ktaIcppParserDeserializeMessage
(
  const uint8_t*          xpReceivedMessage,
  const size_t            xReceivedMessageSize,
  TKIcppProtocolMessage*  xpIcppMessage
);

/**
 * @brief
 *   Deserialize ICPP header.
 *
 * @param[in] xpReceivedMessage
 *   Received ICPP message.
 * @param[in] xReceivedMessageSize
 *   Length of the received ICPP message.
 * @param[in,out] xpIcppMessage
 *   [in] Pointer to buffer to carry deserialized data header.
 *   [out] Actual deserialized data header.
 *
 * @return
 * - E_K_ICPP_PARSER_STATUS_OK in case of success.
 * - E_K_ICPP_PARSER_STATUS_PARAMETER for wrong input parameter.
 * - E_K_ICPP_PARSER_STATUS_ERROR for other errors.
 */
TKParserStatus ktaIcppParserDeserializeHeader
(
  const uint8_t*          xpReceivedMessage,
  const size_t            xReceivedMessageSize,
  TKIcppProtocolMessage*  xpIcppMessage
);

/**
 * @brief
 *   Update ICPP header length by substracting the existing length with the given value.
 *
 * @param[in] xpReceivedMessage
 *   ICPP Message.
 * @param[in] xLength
 *   Length to be substracted with the existing header length.
 *
 * @return
 * - E_K_ICPP_PARSER_STATUS_OK in case of success.
 * - E_K_ICPP_PARSER_STATUS_PARAMETER for wrong input parameter.
 * - E_K_ICPP_PARSER_STATUS_ERROR for other errors.
 */
TKParserStatus ktaIcppParserUpdateHeaderLength
(
  uint8_t* xpReceivedMessage,
  size_t   xLength
);

/**
 * @brief
 *   Update ICPP header length.
 *
 * @param[in] xpReceivedMessage
 *   ICPP Message.
 * @param[in] xLength
 *   Length to set in ICPP message header.
 *
 * @return
 * - E_K_ICPP_PARSER_STATUS_OK in case of success.
 * - E_K_ICPP_PARSER_STATUS_PARAMETER for wrong input parameter.
 * - E_K_ICPP_PARSER_STATUS_ERROR for other errors.
 */
TKParserStatus ktaIcppParserSetHeaderLength
(
  uint8_t* xpReceivedMessage,
  size_t   xLength
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // ICPP_PARSER_H
/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

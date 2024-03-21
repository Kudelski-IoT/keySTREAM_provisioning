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
/** \brief keySTREAM Trusted Agent - icpp parser.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file icpp_parser.c
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - ICPP parser to parse message received from
 *        keySTREAM.
 */

#include "icpp_parser.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "config.h"
#include "k_sal_storage.h"
#include "KTALog.h"

#include <string.h>
/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */
#if LOG_KTA_ENABLE
/* Module name used for logging */
static const char* gpModuleName = "KTAICPPPARSER";
#endif

/** @brief Max MAC size 16 bytes. */
#define C_K_ICPP_PARSER_HMACSHA256_SIZE             (16u)

/** @brief ICPP message protocol version. */
#define C_K_ICPP_PARSER_PROTOCOL_VERSION            (0x30u)

/** @brief ICPP message protocol version bit mask. */
#define C_K_ICPP_PARSER_PROTOCOL_VERSION_BIT_MASK   (0xF0u)

/** @brief ICPP message reserved  bits. */
#define C_K_ICPP_PARSER_RESERVED                    (0x00u)

/** @brief ICPP message crypto version bit mask. */
#define C_K_ICPP_PARSER_CRYPTO_VERSION_BIT_MASK     (0xF0u)

/** @brief ICPP message encryption mode bit mask. */
#define C_K_ICPP_PARSER_ENC_MODE_BIT_MASK           (0x08u)

/** @brief ICPP message type bit mask. */
#define C_K_ICPP_PARSER_MESSAGE_TYPE_BIT_MASK       (0x03u)

/** @brief ICPP size of length in header. */
#define C_K_ICPP_PARSER_LENGTH_SIZE_IN_HEADER       (2u)

/** @brief ICPP message protocol version | reserved index. */
#define C_K_ICPP_PARSER_VERSION_INDEX               (0u)

/** @brief ICPP message crypto version | encryption mode | reserved | message type index. */
#define C_K_ICPP_PARSER_MESSAGE_TYPE_INDEX          (1u)

/** @brief ICPP message transaction id index. */
#define C_K_ICPP_PARSER_TRANSACTION_ID_INDEX        (2u)

/** @brief ICPP message RoT public UID index. */
#define C_K_ICPP_PARSER_ROT_ID_INDEX                (10u)

/** @brief ICPP message ROT key set id index. */
#define C_K_ICPP_PARSER_ROT_KEYSET_ID_INDEX         (18u)

/** @brief ICPP message length index. */
#define C_K_ICPP_PARSER_LENGTH_INDEX                (19u)

/** @brief ICPP message header length for NoOp. */
#define C_K_ICPP_PARSER_HEADER_LENGTH_FOR_NOOP      (0u)

/** @brief ICPP No of bits in byte. */
#define C_K_ICPP_PARSER_NO_OF_BITS_IN_BYTE          (8u)

/** @brief ICPP Byte Mask. */
#define C_K_ICPP_PARSER_BYTE_MASK                   (0XFFu)

/** @brief ICPP Tag Size in byte. */
#define C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES           (1u)

/**
 * @brief Get the protocol version.
 *
 * @param[in] x_pBuffer Raw ICPP data.
 *
 */
#define M_ICPP_PARSER_GET_PROTOCOL_VERSION(x_pBuffer)  \
  (((x_pBuffer)[C_K_ICPP_PARSER_VERSION_INDEX]) & C_K_ICPP_PARSER_PROTOCOL_VERSION_BIT_MASK)

/**
 * @brief Get the crypto version.
 *
 * @param[in] x_pBuffer Raw ICPP data.
 *
 */
#define M_ICPP_PARSER_GET_CRYPTO_VERSION(x_pBuffer)    \
  ((((x_pBuffer)[C_K_ICPP_PARSER_MESSAGE_TYPE_INDEX]) \
    & C_K_ICPP_PARSER_CRYPTO_VERSION_BIT_MASK) >> 4)

/**
 * @brief Get the encryption mode.
 *
 * @param[in] x_pBuffer Raw ICPP data.
 *
 */
#define M_ICPP_PARSER_GET_ENC_MODE(x_pBuffer)    \
  ((((x_pBuffer)[C_K_ICPP_PARSER_MESSAGE_TYPE_INDEX]) & C_K_ICPP_PARSER_ENC_MODE_BIT_MASK) >> 3)

/**
 * @brief Get the mesage Type.
 *
 * @param[in] x_pBuffer Raw ICPP data.
 *
 */
#define M_ICPP_PARSER_GET_MESSAGE_TYPE(x_pBuffer)      \
  (((x_pBuffer)[C_K_ICPP_PARSER_MESSAGE_TYPE_INDEX]) & C_K_ICPP_PARSER_MESSAGE_TYPE_BIT_MASK)


/**
 * @brief Get the command length in bytes.
 *
 * @param[in] x_cmdTag Command tag.
 *
 */
#define M_K_ICPP_PARSER_GET_COMMAND_LENGTH(x_cmdTag)   \
  (((x_cmdTag) >= C_K_ICPP_PARSER__1BYTE_CMD_TAG_WITH_FILED_RANGE_START) && \
   ((x_cmdTag) <= C_K_ICPP_PARSER__1BYTE_CMD_TAG_WITHOUT_FILED_RANGE_END) ? 1u : 2u)

/**
 * @brief Get the field length for one 1 byte.
 *
 * @param[in] x_fieldTag Field tag.
 *
 */
#define M_K_ICPP_PARSER_GET_1_BYTE_FIELD_LENGTH(x_fieldTag)   (1u)

/**
 * @brief Get the field length for two bytes.
 *
 * @param[in] x_fieldTag Field tag.
 *
 */
#define M_K_ICPP_PARSER_GET_2_BYTE_FIELD_LENGTH(x_fieldTag)   (2u)

/** @brief ICPP Tag related enum. */
typedef enum
{
  E_ICPP_PARSER_TAG_TYPE_COMMAND = 0x00,
  /* Command Tag. */
  E_ICPP_PARSER_TAG_TYPE_FIELD,
  /* Field Tag. */
  E_ICPP_PARSER_NUM_TAG_TYPE
  /* Number of items in this enum. */
} TKIcppTagType;

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   Check whether the tag is valid or not.
 *
 * @param[in] xTagType
 *   Tag type.
 * @param[in] xTag
 *   Tag to check.
 * @param[in,out] xpTagLen
 *   [in] Pointer to buffer to carry tag length.
 *   [out] Actual tag length.
 *
 * @return
 * - E_K_ICPP_PARSER_STATUS_OK in case of success.
 * - E_K_ICPP_PARSER_STATUS_PARAMETER for wrong input parameter.
 * - E_K_ICPP_PARSER_STATUS_ERROR for other errors.
 */
static TKParserStatus lIcppParserIsValidTag
(
  const TKIcppTagType   xTagType,
  const uint32_t        xTag,
  uint32_t*             xpTagLen
);

/**
  * @brief Get command/field length from the buffer.
  *
  * @param[in]  xpSrcBuffer
  *   Buffer carrying tag.
  * @param[in]  xBufferLen
  *   Buffer length in bytes.
  * @param[in,out] xpBufferValue
  *   [in] Pointer to carry tag length value.
  *   [out] Actual tag length value.
  *
  */
static void lIcppParserGetTagLength
(
  const uint8_t*  xpSrcBuffer,
  uint32_t        xBufferLen,
  uint32_t*       xpBufferValue
);

/**
  * @brief Set command/field length in the buffer.
  *
  * @param[in,out] xpDstBuffer
  *   Destination buffer to set the the tag length.
  * @param[in] xBufferLen
  *   Length of value.
  * @param[in] xpBufferValue
  *   Value to set in the buffer.
  *
  */
static void lIcppParserSetTagLength
(
  uint8_t*  xpDstBuffer,
  uint32_t  xBufferLen,
  uint32_t  xBufferValue
);

/**
  * @brief Deserialize the fields.
  *
  * @param[in] xpMessage
  *   ICPP message raw buffer.
  * @param[in] xCommandLength
  *    Command data length in bytes.
  * @param[in,out] xpFieldList
  *   [in] Pointer to structure carrying field.
  *   [out] Parsed fields.
  *
  * @return
  * - E_K_ICPP_PARSER_STATUS_OK in case of success.
  * - E_K_ICPP_PARSER_STATUS_PARAMETER for wrong input parameter.
  * - E_K_ICPP_PARSER_STATUS_ERROR for other errors.
  */
static TKParserStatus lIcppParserDeserializeFields
(
  const uint8_t*          xpMessage,
  int32_t                 xCommandLength,
  TKIcppFieldList*        xpFieldList
);

/**
  * @brief Deserialize the commands.
  *
  * @param[in] xpReceivedMessage
  *   ICPP message raw data.
  * @param[in] xReceivedMessageSize
  *   Size of the xpReceivedMessage in bytes.
  * @param[in,out] xpIcppMessage
  *   [in] Pointer to structure to carry ICPP commands.
  *   [out] Actual structure containing ICPP commands.
  *
  * @return
  * - E_K_ICPP_PARSER_STATUS_OK in case of success.
  * - E_K_ICPP_PARSER_STATUS_PARAMETER for wrong input parameter.
  * - E_K_ICPP_PARSER_STATUS_ERROR for other errors.
  */
static TKParserStatus lIcppParserDeserializeCommands
(
  const uint8_t*          xpReceivedMessage,
  const size_t            xReceivedMessageSize,
  TKIcppProtocolMessage*  xpIcppMessage
);

/**
  * @brief Serialize the fields.
  *
  * @param[in] xpFieldList
  *   ICPP message structure carrying info of fields.
  * @param[in,out] xpMessageField
  *   [in] Pointer to buffer carrying serialized message.
  *   [out] serialized message.
  * @param[in,out] xpMessageFieldSize
  *   [in] Pointer to buffer carrying serialized message length.
  *   [out] serialized message length.
  * @return
  * - E_K_ICPP_PARSER_STATUS_OK in case of success.
  * - E_K_ICPP_PARSER_STATUS_PARAMETER for wrong input parameter.
  * - E_K_ICPP_PARSER_STATUS_ERROR for other errors.
  */
static TKParserStatus lIcppParserSerializeFields
(
  const TKIcppFieldList*  xpFieldList,
  uint8_t*                xpMessageField,
  uint32_t*               xpMessageFieldSize
);

/**
  * @brief Serialize the commands.
  *
  * @param[in] xpIcppMessage
  *   ICPP structure carrying command info.
  * @param[in,out] xpMessageCommand
  *   [in] Pointer to buffer carrying serialized data.
  *   [out] Actual serialzied data.
  * @param[in,out] xpMessageCommandSize
  *   [in] Pointer to buffer carrying serialized data length.
  *   [out] Actual serialzied data length.
  *
  * @return
  * - E_K_ICPP_PARSER_STATUS_OK in case of success.
  * - E_K_ICPP_PARSER_STATUS_PARAMETER for wrong input parameter.
  * - E_K_ICPP_PARSER_STATUS_ERROR for other errors.
  */
static TKParserStatus lIcppParserSerializeCommands
(
  const TKIcppProtocolMessage*  xpIcppMessage,
  uint8_t*                      xpMessageCommand,
  size_t*                      xpMessageCommandSize
);

/**
  * @brief Deserialize the header.
  *
  * @param[in] xpReceivedMessage
  *   ICPP message buffer.
  * @param[in] xReceivedMessageSize
  *   Length of ICPP message buffer.
  * @param[in,out] xpIcppMessage
  *   [in] Pointer to structure carrying header
  *   [out] Actual strcuture carrying deserialized header.
  *
  * @return
  * - E_K_ICPP_PARSER_STATUS_OK in case of success.
  * - E_K_ICPP_PARSER_STATUS_PARAMETER for wrong input parameter.
  * - E_K_ICPP_PARSER_STATUS_ERROR for other errors.
  */
static TKParserStatus lIcppParserDeserializeHeader
(
  const uint8_t*          xpReceivedMessage,
  const size_t            xReceivedMessageSize,
  TKIcppProtocolMessage*  xpIcppMessage
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */
/**
 * @brief implement ktaIcppParserSerializeMessage
 *
 */
TKParserStatus ktaIcppParserSerializeMessage
(
  const TKIcppProtocolMessage* xpIcppMessage,
  uint8_t*                     xpMessageToSend,
  size_t*                      xpMessageToSendSize
)
{
  TKParserStatus  status = E_K_ICPP_PARSER_STATUS_ERROR;
  size_t          bufferSize = 0;

  M_KTALOG__START("Start");

  // REQ RQ_M-KTA-ICPP-FN-0010(1): Input parameters check
  for (;;)
  {
    if (
      (NULL == xpIcppMessage) ||
      (NULL == xpMessageToSend) ||
      (NULL == xpMessageToSendSize) ||
      (0 == *xpMessageToSendSize) ||
      (E_K_ICPP_PARSER_MESSAGE_TYPE_RESERVED <= xpIcppMessage->msgType) ||
      (C_K_ICPP_PARSER__MAX_COMMANDS_COUNT < xpIcppMessage->commandsCount)
    )
    {
      M_KTALOG__ERR("Invalid parameters");
      status = E_K_ICPP_PARSER_STATUS_PARAMETER;
      break;
    }

    bufferSize = *xpMessageToSendSize;

    if (bufferSize < (int32_t)C_K_ICPP_PARSER__HEADER_SIZE)
    {
      M_KTALOG__ERR("Size is less than header size");
      status = E_K_ICPP_PARSER_STATUS_PARAMETER;
      break;
    }

    // REQ RQ_M-KTA-ICPP-FN-0070(1) : Serialize ICPP Header
    /* ICPP prtocol version | reserved data. */
    // REQ RQ_M-KTA-ICPP-FN-0120(1) : Check protocol version
    xpMessageToSend[C_K_ICPP_PARSER_VERSION_INDEX] =
      C_K_ICPP_PARSER_PROTOCOL_VERSION | C_K_ICPP_PARSER_RESERVED;

    /* ICPP crypto version | encryption mode | reserved | message type. */
    // REQ RQ_M-KTA-ICPP-FN-0130(1) : Check message type
    xpMessageToSend[C_K_ICPP_PARSER_MESSAGE_TYPE_INDEX] =
      ((xpIcppMessage->cryptoVersion & (uint8_t)0x0F) << (uint8_t)4) |
      ((xpIcppMessage->encMode & (uint8_t)0x01) << (uint8_t)3) |
      (xpIcppMessage->msgType & C_K_ICPP_PARSER_MESSAGE_TYPE_BIT_MASK);
    /* Transaction ID. */
    (void)memcpy(&xpMessageToSend[C_K_ICPP_PARSER_TRANSACTION_ID_INDEX],
                 xpIcppMessage->transactionId,
                 C_K_ICPP_PARSER__TRANSACTION_ID_SIZE_IN_BYTES);

    (void)memcpy(&xpMessageToSend[C_K_ICPP_PARSER_ROT_ID_INDEX],
                 xpIcppMessage->rotPublicUID,
                 C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES);

    xpMessageToSend[C_K_ICPP_PARSER_ROT_KEYSET_ID_INDEX] = xpIcppMessage->rotKeySetId;

    /* Reserved 2 bytes for length, length will be updated later. */
    bufferSize -= C_K_ICPP_PARSER__HEADER_SIZE;

    // REQ RQ_M-KTA-ICPP-FN-0040(1) : Serialize commands in message
    status = lIcppParserSerializeCommands(xpIcppMessage,
                                          &xpMessageToSend[C_K_ICPP_PARSER__HEADER_SIZE],
                                          &bufferSize);

    if (E_K_ICPP_PARSER_STATUS_OK != status)
    {
      M_KTALOG__ERR("Serialization of commands got Failed %d", status);
      break;
    }

    /* Update the total length of the data in header. */
    lIcppParserSetTagLength(&xpMessageToSend[C_K_ICPP_PARSER__HEADER_SIZE
                                             - C_K_ICPP_PARSER_LENGTH_SIZE_IN_HEADER],
                            C_K_ICPP_PARSER_LENGTH_SIZE_IN_HEADER,
                            bufferSize + C_K_ICPP_PARSER_HMACSHA256_SIZE);
    // REQ RQ_M-KTA-ICPP-FN-0210(1) : Set Header Length
    *xpMessageToSendSize = bufferSize + C_K_ICPP_PARSER__HEADER_SIZE;

    M_KTALOG__DEBUG("Buffer Size %d", bufferSize);
    break;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaIcppParserDeserializeMessage
 *
 */
TKParserStatus ktaIcppParserDeserializeMessage
(
  const uint8_t*          xpReceivedMessage,
  const size_t            xReceivedMessageSize,
  TKIcppProtocolMessage*  xpIcppMessage
)
{
  TKParserStatus  status = E_K_ICPP_PARSER_STATUS_ERROR;
  uint32_t        curPosition = C_K_ICPP_PARSER__HEADER_SIZE;

  M_KTALOG__START("Start");

  for (;;)
  {
    // REQ RQ_M-KTA-ICPP-FN-0110(1) : Deserialize Input Parameters Check
    if ((NULL == xpReceivedMessage) || (NULL == xpIcppMessage) || (0u == xReceivedMessageSize))
    {
      M_KTALOG__ERR("Invalid parameters");
      status = E_K_ICPP_PARSER_STATUS_PARAMETER;
      break;
    }

    // REQ RQ_M-KTA-NOOP-FN-0030(1) : Deserialize the decrypted No operation message data
    status = lIcppParserDeserializeHeader(xpReceivedMessage, xReceivedMessageSize, xpIcppMessage);

    if (E_K_ICPP_PARSER_STATUS_NO_OPERATION == status)
    {
      M_KTALOG__DEBUG("No Operation");
      break;
    }
    else if (E_K_ICPP_PARSER_STATUS_OK != status)
    {
      M_KTALOG__ERR("De-serialization of header got Failed");
      break;
    }
    else /* Added to resolve cppcheck errors. */
    {
      /* Not used. */
    }

    // REQ RQ_M-KTA-ICPP-FN-0150(1) : Deserialize Commands in message
    status = lIcppParserDeserializeCommands(&xpReceivedMessage[curPosition],
                                            xReceivedMessageSize - curPosition,
                                            xpIcppMessage);
    break;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaIcppParserDeserializeHeader
 *
 */
TKParserStatus ktaIcppParserDeserializeHeader
(
  const uint8_t*          xpReceivedMessage,
  const size_t            xReceivedMessageSize,
  TKIcppProtocolMessage*  xpIcppMessage
)
{
  TKParserStatus  status  = E_K_ICPP_PARSER_STATUS_ERROR;

  M_KTALOG__START("Start");

  for (;;)
  {
    if ((NULL == xpReceivedMessage) || (NULL == xpIcppMessage) || (0u == xReceivedMessageSize))
    {
      M_KTALOG__ERR("Invalid parameters");
      status = E_K_ICPP_PARSER_STATUS_PARAMETER;
      break;
    }

    M_KTALOG__DEBUG("[ktaIcppParserDeserializeHeader] Start");
    // REQ RQ_M-KTA-ICPP-FN-0170(1) : Deserialize ICPP Header
    status = lIcppParserDeserializeHeader(xpReceivedMessage, xReceivedMessageSize, xpIcppMessage);
    break;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaIcppParserUpdateHeaderLength
 *
 */
TKParserStatus ktaIcppParserUpdateHeaderLength
(
  uint8_t* xpReceivedMessage,
  size_t   xLength
)
{
  TKParserStatus  status = E_K_ICPP_PARSER_STATUS_ERROR;
  size_t tempLen = 0;

  M_KTALOG__START("Start");

  for (;;)
  {
    if ((NULL == xpReceivedMessage) || !(xLength < 0x10000))
    {
      status = E_K_ICPP_PARSER_STATUS_PARAMETER;
      M_KTALOG__ERR("Invalid parameter");
      break;
    }
    tempLen = ((xpReceivedMessage[C_K_ICPP_PARSER_LENGTH_INDEX] <<
               (C_K_ICPP_PARSER_NO_OF_BITS_IN_BYTE)) |
               xpReceivedMessage[(uint8_t)C_K_ICPP_PARSER_LENGTH_INDEX +
                C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES]);
    if (tempLen >= xLength)
    {
    tempLen -= xLength;
    }
    xpReceivedMessage[C_K_ICPP_PARSER_LENGTH_INDEX] = tempLen >>
     C_K_ICPP_PARSER_NO_OF_BITS_IN_BYTE;
    xpReceivedMessage[C_K_ICPP_PARSER_LENGTH_INDEX + C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES] =
      (tempLen & (size_t)C_K_ICPP_PARSER_BYTE_MASK);
    status = E_K_ICPP_PARSER_STATUS_OK;
    break;
  }

  M_KTALOG__END("End, status : %d", status);

  return status;
}

/**
 * @brief implement ktaIcppParserSetHeaderLength
 *
 */
TKParserStatus ktaIcppParserSetHeaderLength
(
  uint8_t* xpReceivedMessage,
  size_t   xLength
)
{
  TKParserStatus  status = E_K_ICPP_PARSER_STATUS_ERROR;

  M_KTALOG__START("Start");

  for (;;)
  {
    if ((NULL == xpReceivedMessage) || !(xLength < 0x10000))
    {
      status = E_K_ICPP_PARSER_STATUS_PARAMETER;
      M_KTALOG__ERR("Invalid parameter");
      break;
    }

    xpReceivedMessage[C_K_ICPP_PARSER_LENGTH_INDEX] = ((xLength >>
     C_K_ICPP_PARSER_NO_OF_BITS_IN_BYTE) & (size_t)C_K_ICPP_PARSER_BYTE_MASK);
    xpReceivedMessage[C_K_ICPP_PARSER_LENGTH_INDEX +
     C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES] = (xLength & (size_t)C_K_ICPP_PARSER_BYTE_MASK);
    status = E_K_ICPP_PARSER_STATUS_OK;
    break;
  }

  M_KTALOG__END("End, status : %d", status);

  return status;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */
/**
 * @implements lIcppParserIsValidTag
 *
 */
static TKParserStatus lIcppParserIsValidTag
(
  const TKIcppTagType   xTagType,
  const uint32_t        xTag,
  uint32_t*             xpTagLen
)
{
  TKParserStatus  status = E_K_ICPP_PARSER_STATUS_ERROR;

  for (;;)
  {

    if (E_ICPP_PARSER_TAG_TYPE_COMMAND == xTagType)
    {
      switch (xTag)
      {
        case E_K_ICPP_PARSER_COMMAND_TAG_ACTIVATION:
        case E_K_ICPP_PARSER_COMMAND_TAG_REGISTERATION_INFO:
        case E_K_ICPP_PARSER_COMMAND_TAG_THIRD_PARTY:
        case E_K_ICPP_PARSER_COMMAND_TAG_PROCESSING_STATUS:
        case E_K_ICPP_PARSER_COMMAND_TAG_CMD_PROCESSING_ERROR:
        case E_K_ICPP_PARSER_COMMAND_TAG_GENERATE_KEY_PAIR:
        case E_K_ICPP_PARSER_COMMAND_TAG_SET_OBJECT:
        case E_K_ICPP_PARSER_CMD_TAG_SET_OBJ_WITH_ASSOCIATION:
        case E_K_ICPP_PARSER_COMMAND_TAG_DELETE_OBJECT:
        case E_K_ICPP_PARSER_COMMAND_TAG_DELETE_KEY_OBJECT:
        {
          if (xpTagLen != NULL)
          {
            *xpTagLen = (uint32_t)(M_K_ICPP_PARSER_GET_COMMAND_LENGTH(xTag));
          }
        }
        status = E_K_ICPP_PARSER_STATUS_OK;
        break;

        default:
        {
          M_KTALOG__ERR("Unknown command xTag %u", xTag);
          status = E_K_ICPP_PARSER_STATUS_PARAMETER;
        }
        break;
      }

      break;
    }
    else if (E_ICPP_PARSER_TAG_TYPE_FIELD == xTagType)
    {
      switch (xTag)
      {
        case E_K_ICPP_PARSER_FIELD_TAG_DEVPROFUID:
        case E_K_ICPP_PARSER_FIELD_TAG_MUTABLE_DEVPROFUID:
        case E_K_ICPP_PARSER_FIELD_TAG_CHIP_UID:
        case E_K_ICPP_PARSER_FIELD_TAG_ROT_SOL_ID:
        case E_K_ICPP_PARSER_FIELD_TAG_ROT_PUBLIC_UID:
        case E_K_ICPP_PARSER_FIELD_TAG_ACK_SEQ_CNT:
        case E_K_ICPP_PARSER_FIELD_TAG_KTA_CAPABILITY:
        case E_K_ICPP_PARSER_FIELD_TAG_KTA_CTX_PRO_UID:
        case E_K_ICPP_PARSER_FIELD_TAG_KTA_CTX_SERIAL_NO:
        case E_K_ICPP_PARSER_FIELD_TAG_KTA_CTX_VER:
        case E_K_ICPP_PARSER_FIELD_TAG_KTA_VER:
        case E_K_ICPP_PARSER_FIELD_TAG_DEV_SERIAL_NO:
        case E_K_ICPP_PARSER_FIELD_TAG_CMD_OBJECT_ID:
        case E_K_ICPP_PARSER_FIELD_TAG_CMD_OBJECT_TYPE:
        case E_K_ICPP_PARSER_FIELD_TAG_CMD_DATA_ATTRIBUTES:
        case E_K_ICPP_PARSER_FIELD_TAG_CMD_ASSOCIATION_INFO:
        case E_K_ICPP_PARSER_FIELD_TAG_CMD_OBJECT_OWNER:
        case E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS:
        {
          if (xpTagLen != NULL)
          {
            *xpTagLen = M_K_ICPP_PARSER_GET_1_BYTE_FIELD_LENGTH(xTag);
          }
        }
        status = E_K_ICPP_PARSER_STATUS_OK;
        break;

        case E_K_ICPP_PARSER_FIELD_TAG_ROT_E_PK:
        case E_K_ICPP_PARSER_FIELD_TAG_CHIP_CERT:
        case E_K_ICPP_PARSER_FIELD_TAG_CHIP_ATTEST_CERT:
        case E_K_ICPP_PARSER_FIELD_TAG_SIGNED_PUB_KEY:
        case E_K_ICPP_PARSER_FIELD_TAG_KS_E_PK:
        case E_K_ICPP_PARSER_FIELD_TAG_CMD_PUBLIC_KEY:
        case E_K_ICPP_PARSER_FIELD_TAG_CMD_DATA:
        {
          if (xpTagLen != NULL)
          {
            *xpTagLen = M_K_ICPP_PARSER_GET_2_BYTE_FIELD_LENGTH(xTag);
          }
        }
        status = E_K_ICPP_PARSER_STATUS_OK;
        break;

        default:
        {
          M_KTALOG__ERR("Unknown field xTag %u", xTag);
          status = E_K_ICPP_PARSER_STATUS_PARAMETER;
        }
        break;
      }

      break;
    }
    else
    {
      M_KTALOG__ERR("Unknown  xTagType %u", xTagType);
      status = E_K_ICPP_PARSER_STATUS_PARAMETER;
    }

    break;
  }

  return status;
}

/**
 * @implements lIcppParserGetTagLength
 *
 */
static void lIcppParserGetTagLength
(
  const uint8_t*  xpSrcBuffer,
  uint32_t        xBufferLen,
  uint32_t*       xpBufferValue
)
{
  const uint8_t*  pSrcBuffer = xpSrcBuffer;
  uint32_t        bufferLen = xBufferLen;
  uint32_t*       pBufferValue = xpBufferValue;

  if ((NULL != pSrcBuffer) && (NULL != pBufferValue) && (0u < bufferLen))
  {
    *pBufferValue = *pSrcBuffer;
    bufferLen--;

    while (bufferLen > 0u)
    {
      pSrcBuffer++;
      *pBufferValue  = (*pSrcBuffer) | ((*pBufferValue) << C_K_ICPP_PARSER_NO_OF_BITS_IN_BYTE);
      bufferLen--;
    }
  }
}

/**
 * @implements lIcppParserSetTagLength
 *
 */
static void lIcppParserSetTagLength
(
  uint8_t*  xpDstBuffer,
  uint32_t  xBufferLen,
  uint32_t  xBufferValue
)
{
  uint32_t  bufferLen = xBufferLen;
  uint8_t*  pDstBuffer = xpDstBuffer;
  uint32_t  bufferValue = xBufferValue;

  if ((NULL != pDstBuffer) && (0u < bufferLen))
  {
    do
    {
      pDstBuffer[bufferLen - 1u] = (uint8_t)(bufferValue & (uint32_t)(C_K_ICPP_PARSER_BYTE_MASK));
      bufferValue >>= C_K_ICPP_PARSER_NO_OF_BITS_IN_BYTE;
      --bufferLen;
    } while (bufferLen > 0u);
  }
}

/**
 * @implements lIcppParserDeserializeFields
 *
 */
static TKParserStatus lIcppParserDeserializeFields
(
  const uint8_t*          xpMessage,
  int32_t                 xCommandLength,
  TKIcppFieldList*        xpFieldList
)
{
  /* status initialized to E_K_ICPP_PARSER_STATUS_OK for code size optimization */
  TKParserStatus  status = E_K_ICPP_PARSER_STATUS_OK;
  uint32_t        fieldsCount = 0;
  uint32_t        fieldLength = 0;
  uint32_t        tagLen = 0;
  uint32_t        curPosition = 0;
  uint32_t        cmdLength = 0;
  TKIcppField*     pField = NULL;

  for (;;)
  {
    cmdLength = xCommandLength;
    while (cmdLength > 0)
    {
      // REQ RQ_M-KTA-ICPP-CF-0050(1) : Max field Count
      if (fieldsCount > C_K_ICPP_PARSER__MAX_FIELDS_COUNT)
      {
        M_KTALOG__ERR("Exceeded the max no of fields[%u] in command", fieldsCount);
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }
      pField = &xpFieldList->fields[fieldsCount];
      pField->fieldTag = xpMessage[curPosition];

      tagLen = 0;

      if (E_K_ICPP_PARSER_STATUS_OK != lIcppParserIsValidTag(E_ICPP_PARSER_TAG_TYPE_FIELD,
                                                             pField->fieldTag,
                                                             &tagLen))
      {
        M_KTALOG__ERR("Invalid Field Tag %d", pField->fieldTag);
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }

      curPosition += C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES;
      if (curPosition+tagLen > xCommandLength)
      {
        M_KTALOG__ERR("Error while parsing taglen");
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }
      lIcppParserGetTagLength(&xpMessage[curPosition], tagLen, &fieldLength);
      M_KTALOG__DEBUG("Fields Count %u", fieldsCount);

      curPosition += tagLen;
      pField->fieldLen = fieldLength;
      if (curPosition+fieldLength > xCommandLength)
      {
        M_KTALOG__ERR("Error while parsing field length");
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }
      /* Storing the received data pointer to the field value. */
      pField->fieldValue = (uint8_t*)&xpMessage[curPosition];
      curPosition += fieldLength;
      cmdLength -= (int32_t)(fieldLength + C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES + tagLen);

      ++fieldsCount;
      M_KTALOG__DEBUG("Fields Count %u", fieldsCount);
    }

    xpFieldList->fieldsCount = fieldsCount;
    break;
  }

  return status;
}

/**
 * @implements lIcppParserDeserializeCommands
 *
 */
static TKParserStatus lIcppParserDeserializeCommands
(
  const uint8_t*          xpReceivedMessage,
  const size_t            xReceivedMessageSize,
  TKIcppProtocolMessage*  xpIcppMessage
)
{
  /* status initialized to E_K_ICPP_PARSER_STATUS_OK for code size optimization */
  TKParserStatus  status = E_K_ICPP_PARSER_STATUS_OK;
  TKIcppCommand*   pCommand = NULL;
  uint32_t        tagLen = 0;
  uint32_t        commandsCount = 0;
  uint32_t        commandLength = 0;
  uint32_t        curPosition = 0;

  for (;;)
  {
    while (curPosition < xReceivedMessageSize)
    {
      // REQ RQ_M-KTA-ICPP-CF-0020(1) : Max command Count
      if (commandsCount > C_K_ICPP_PARSER__MAX_COMMANDS_COUNT)
      {
        M_KTALOG__ERR("Exceeded the max no of commands %u", commandsCount);
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }
      /* Updating the command tag and length. */
      pCommand = &xpIcppMessage->commands[commandsCount];
      pCommand->commandTag = xpReceivedMessage[curPosition];

      if (E_K_ICPP_PARSER_STATUS_OK != lIcppParserIsValidTag(E_ICPP_PARSER_TAG_TYPE_COMMAND,
                                                             pCommand->commandTag,
                                                             &tagLen))
      {
        M_KTALOG__ERR("Invalid command tag %d", pCommand->commandTag);
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }

      curPosition += C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES;
      if ((xReceivedMessageSize - curPosition) < tagLen)
      {
        M_KTALOG__ERR("Error while parsing xReceivedMessageSize");
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }
      lIcppParserGetTagLength(&xpReceivedMessage[curPosition], tagLen, &commandLength);
      curPosition += tagLen;
      if ((xReceivedMessageSize - curPosition) < commandLength)
      {
        M_KTALOG__ERR("Error while parsing xReceivedMessageSize");
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }

      if (0U == commandLength)
      {
        M_KTALOG__ERR("Invalid command len %d", commandLength);
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }

      if (M_K_ICPP_PARSER__COMMAND_TAG_HAS_FIELDS(pCommand->commandTag) != 0)
      {
        M_KTALOG__DEBUG("Command Tag[%x] Len[%d]", pCommand->commandTag, commandLength);
        /* Deserialize Fields. */
        // REQ RQ_M-KTA-ICPP-FN-0160(1) : Deserialize Fileds in Commands
        status = lIcppParserDeserializeFields(&xpReceivedMessage[curPosition],
                                              commandLength,
                                              &pCommand->data.fieldList);

        if (status != E_K_ICPP_PARSER_STATUS_OK)
        {
          M_KTALOG__ERR("De-serialization of fields got Failed %d", status);
          break;
        }
      }
      else
      {
        /**
         * Some Command has the data directly instead of field tag and length
         * So that we can save 2 bytes.
         */
        pCommand->data.cmdInfo.cmdLen = commandLength;
        pCommand->data.cmdInfo.cmdValue = (uint8_t*)&xpReceivedMessage[curPosition];
      }

      // REQ RQ_M-KTA-ICPP-FN-0190(1) : Command processing error
      if (pCommand->commandTag == E_K_ICPP_PARSER_COMMAND_TAG_CMD_PROCESSING_ERROR)
      {
        M_KTALOG__WARN("Received E_K_ICPP_PARSER_COMMAND_TAG_CMD_PROCESSING_ERROR");
        status = E_K_ICPP_PARSER_STATUS_NOTIFICATION_CPERROR;
        break;
      }

      curPosition += commandLength;
      ++commandsCount;
      M_KTALOG__DEBUG("Commands Count %d", commandsCount);
    }

    xpIcppMessage->commandsCount = commandsCount;
    break;
  }

  return status;
}

/**
 * @implements lIcppParserSerializeFields
 *
 */
static TKParserStatus lIcppParserSerializeFields
(
  const TKIcppFieldList*  xpFieldList,
  uint8_t*                xpMessageField,
  uint32_t*               xpMessageFieldSize
)
{
  /* status initialized to E_K_ICPP_PARSER_STATUS_OK for code size optimization */
  TKParserStatus        status = E_K_ICPP_PARSER_STATUS_OK;
  const TKIcppField*     pField = NULL;
  uint32_t              fieldLoop = 0;
  uint32_t              tagLen = 0;
  uint32_t              curPosition = 0;
  size_t                bufferSize = 0;

  for (;;)
  {
    /* Check the max field limit. */
    if (
      (C_K_ICPP_PARSER__MAX_FIELDS_COUNT < xpFieldList->fieldsCount) ||
      (0 == xpFieldList->fieldsCount)
    )
    {
      M_KTALOG__ERR("Invalid Parameter");
      status = E_K_ICPP_PARSER_STATUS_PARAMETER;
      break;
    }

    bufferSize = *xpMessageFieldSize;

    /* Adding the fields data to the buffer. */
    for (fieldLoop = 0; fieldLoop < xpFieldList->fieldsCount; fieldLoop++)
    {
      pField = &xpFieldList->fields[fieldLoop];

      if (E_K_ICPP_PARSER_STATUS_OK != lIcppParserIsValidTag(E_ICPP_PARSER_TAG_TYPE_FIELD,
                                                             pField->fieldTag,
                                                             &tagLen))
      {
        M_KTALOG__ERR("Invalid Field Tag %d", pField->fieldTag);
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }

      if (0u == pField->fieldLen)
      {
        M_KTALOG__ERR("Invalid field length %ld", pField->fieldLen);
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }

      if ((uint32_t)bufferSize < (tagLen + C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES))
      {
        M_KTALOG__ERR("Size is less than required field %d", bufferSize);
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }

      bufferSize -= (int32_t)(tagLen + C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES);

      xpMessageField[curPosition] = pField->fieldTag;
      curPosition += C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES;
      /* Set the field length. */
      lIcppParserSetTagLength(&xpMessageField[curPosition], tagLen, pField->fieldLen);
      curPosition += tagLen;

      if (bufferSize < (int32_t)pField->fieldLen)
      {
        M_KTALOG__ERR("Size is less than required field len %d", bufferSize);
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }

      bufferSize -= pField->fieldLen;

      /* Copy the field data. */
      (void)memcpy(&xpMessageField[curPosition], pField->fieldValue, pField->fieldLen);
      curPosition += pField->fieldLen;
    }

    *xpMessageFieldSize = curPosition;
    break;
  }

  return status;
}

/**
 * @implements lIcppParserSerializeCommands
 *
 */
static TKParserStatus lIcppParserSerializeCommands
(
  const TKIcppProtocolMessage*  xpIcppMessage,
  uint8_t*                      xpMessageCommand,
  size_t*                      xpMessageCommandSize
)
{
  /* status initialized to E_K_ICPP_PARSER_STATUS_OK for code size optimization */
  TKParserStatus        status = E_K_ICPP_PARSER_STATUS_OK;
  size_t                bufferSize = 0;
  uint32_t              commandLoop = 0;
  uint32_t              tagLen = 0;
  uint32_t              curPosition = 0;
  uint8_t*               pCmdLengthOffset = NULL;
  uint32_t              commandsLength = 0;
  const TKIcppCommand*   pCommand = NULL;

  for (;;)
  {
    if (
      (C_K_ICPP_PARSER__MAX_COMMANDS_COUNT < xpIcppMessage->commandsCount)
    )
    {
      M_KTALOG__ERR("Invalid parameters");
      status = E_K_ICPP_PARSER_STATUS_PARAMETER;
      break;
    }

    bufferSize = *xpMessageCommandSize;

    /* Adding commands data to the buffer. */
    for (commandLoop = 0; commandLoop < xpIcppMessage->commandsCount; commandLoop++)
    {
      pCommand = &xpIcppMessage->commands[commandLoop];

      // REQ RQ_M-KTA-ICPP-FN-0030(1) : Check Commands in message
      if (E_K_ICPP_PARSER_STATUS_OK != lIcppParserIsValidTag(E_ICPP_PARSER_TAG_TYPE_COMMAND,
                                                             pCommand->commandTag,
                                                             &tagLen))
      {
        M_KTALOG__ERR("Invalid Command Tag %d", pCommand->commandTag);
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }

      if ((uint32_t)bufferSize < (tagLen + C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES))
      {
        M_KTALOG__ERR("Size is less than required command %d", bufferSize);
        status = E_K_ICPP_PARSER_STATUS_ERROR;
        break;
      }

      bufferSize -= (int32_t)(tagLen + C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES);
      xpMessageCommand[curPosition] = pCommand->commandTag;
      /**
       * Reserved for command length. Storing the command length address in pointer.
       * Will update the length later.
       */
      pCmdLengthOffset = &xpMessageCommand[curPosition + C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES];
      curPosition += tagLen + C_K_ICPP_PARSER_TAG_SIZE_IN_BYTES;
      if (M_K_ICPP_PARSER__COMMAND_TAG_HAS_FIELDS(pCommand->commandTag) != 0)
      {
        /* Check the max field limit. */
        if (
          (C_K_ICPP_PARSER__MAX_FIELDS_COUNT < pCommand->data.fieldList.fieldsCount) ||
          (0 == pCommand->data.fieldList.fieldsCount)
        )
        {
          M_KTALOG__ERR("Invalid field count %ld", pCommand->data.fieldList.fieldsCount);
          status = E_K_ICPP_PARSER_STATUS_ERROR;
          break;
        }

        commandsLength = bufferSize;
        /* Adding the fields data to the buffer. */
        // REQ RQ_M-KTA-ICPP-FN-0060(1) : Serialize Fileds in Commands
        status = lIcppParserSerializeFields(&pCommand->data.fieldList,
                                            &xpMessageCommand[curPosition],
                                            &commandsLength);

        if (E_K_ICPP_PARSER_STATUS_OK != status)
        {
          M_KTALOG__ERR("Serialization of fields got Failed %d", status);
          break;
        }

        bufferSize -= (int32_t)commandsLength;
        curPosition += commandsLength;
      }
      else
      {
        if (0u == pCommand->data.cmdInfo.cmdLen)
        {
          M_KTALOG__ERR("Invalid command length %ld", pCommand->data.cmdInfo.cmdLen);
          status = E_K_ICPP_PARSER_STATUS_ERROR;
          break;
        }

        /**
         * Some command has the data directly instead of field tag and length
         * So that we can save 2 bytes.
         */
        if (bufferSize < (int32_t)pCommand->data.cmdInfo.cmdLen)
        {
          M_KTALOG__ERR("Size is less than required command len %d", bufferSize);
          status = E_K_ICPP_PARSER_STATUS_ERROR;
          break;
        }

        bufferSize -= pCommand->data.cmdInfo.cmdLen;
        (void)memcpy(&xpMessageCommand[curPosition],
                     pCommand->data.cmdInfo.cmdValue,
                     pCommand->data.cmdInfo.cmdLen);
        curPosition += pCommand->data.cmdInfo.cmdLen;
        commandsLength = pCommand->data.cmdInfo.cmdLen;
        M_KTALOG__DEBUG("Command without fields %u #%u", pCommand->commandTag, commandLoop);
      }

      /* Update the command length. */
      lIcppParserSetTagLength(pCmdLengthOffset, tagLen, commandsLength);
    }

    *xpMessageCommandSize = curPosition;
    break;
  }

  return status;
}

/**
 * @implements lIcppParserDeserializeHeader
 *
 */
static TKParserStatus lIcppParserDeserializeHeader
(
  const uint8_t*          xpReceivedMessage,
  const size_t            xReceivedMessageSize,
  TKIcppProtocolMessage*  xpIcppMessage
)
{
  TKParserStatus status    = E_K_ICPP_PARSER_STATUS_ERROR;
  TKStatus eStatus         = E_K_STATUS_ERROR;
  uint32_t curPosition     = 0;
  uint32_t headerLength    = 0;
  uint8_t  protocolVersion = 0x00;

  for (;;)
  {
    if (
      (C_K_ICPP_PARSER__HEADER_SIZE > xReceivedMessageSize)
    )
    {
      M_KTALOG__ERR("Invalid parameters");
      status = E_K_ICPP_PARSER_STATUS_PARAMETER;
      break;
    }

    /* Validate the header. */
    protocolVersion = M_ICPP_PARSER_GET_PROTOCOL_VERSION(xpReceivedMessage);

    if (
      (C_K_ICPP_PARSER_PROTOCOL_VERSION != protocolVersion) ||
      (E_K_ICPP_PARSER_MESSAGE_TYPE_RESERVED <= M_ICPP_PARSER_GET_MESSAGE_TYPE(xpReceivedMessage))
    )
    {
      M_KTALOG__ERR("Invalid Header");
      break;
    }

    /* Setting the crypto version, encryption mode and message type. */
    // REQ RQ_M-KTA-NOOP-FN-0040(1) : Crypto version from keySTEREAM in NoOP message.
    xpIcppMessage->cryptoVersion = M_ICPP_PARSER_GET_CRYPTO_VERSION(xpReceivedMessage);
    xpIcppMessage->encMode = M_ICPP_PARSER_GET_ENC_MODE(xpReceivedMessage);
    xpIcppMessage->msgType = M_ICPP_PARSER_GET_MESSAGE_TYPE(xpReceivedMessage);

    if (
      (E_K_ICPP_PARSER_FULL_ENC_MODE != xpIcppMessage->encMode)
    )
    {
      M_KTALOG__ERR("Invalid encryption mode");
      break;
    }

    curPosition = C_K_ICPP_PARSER_TRANSACTION_ID_INDEX;

    /* Setting the Transaction ID. */
    // REQ RQ_M-KTA-ICPP-CF-0080(1) : Transaction Id Size
    (void)memcpy(xpIcppMessage->transactionId,
                 &xpReceivedMessage[curPosition],
                 C_K_ICPP_PARSER__TRANSACTION_ID_SIZE_IN_BYTES);
    curPosition += C_K_ICPP_PARSER__TRANSACTION_ID_SIZE_IN_BYTES;

    /* Setting the Rot public UID. */
    // REQ RQ_M-KTA-ICPP-CF-0090(1) : Rot Public Uid Size
    (void)memcpy(xpIcppMessage->rotPublicUID,
                 &xpReceivedMessage[curPosition],
                 C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES);
    curPosition += C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES;

    /* Store received rotpublicuid here. */
    // REQ RQ_M-KTA-ICPP-FN-0200(1) : Store the rot public uid
    eStatus = salStorageSetAndLockValue(C_K_KTA__ROT_PUBLIC_UID_STORAGE_ID,
                                        xpIcppMessage->rotPublicUID,
                                        C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES);

    if (eStatus != E_K_STATUS_OK)
    {
      M_KTALOG__ERR("salStorageSetAndLockValue failed[%d]", eStatus);
      break;
    }

    /* Setting the Rot KeySet id. */
    xpIcppMessage->rotKeySetId = xpReceivedMessage[curPosition];

    curPosition += C_K_ICPP_PARSER__ROT_KEYSET_ID_SIZE_IN_BYTES;

    headerLength = ((xpReceivedMessage[curPosition] <<
                     (uint8_t)C_K_ICPP_PARSER_NO_OF_BITS_IN_BYTE) |
                    xpReceivedMessage[curPosition + 1u]);

    curPosition += C_K_ICPP_PARSER_LENGTH_SIZE_IN_HEADER;

    /* Comparing the length with the length in the received data. */
    if ((xReceivedMessageSize - C_K_ICPP_PARSER__HEADER_SIZE) != headerLength)
    {
      M_KTALOG__ERR("Size and header length mismatching %lu %u",
                    xReceivedMessageSize - C_K_ICPP_PARSER__HEADER_SIZE,
                    headerLength);
      break;
    }

    /**
     * If length is C_K_ICPP_PARSER_HEADER_LENGTH_FOR_NOOP then return no operation.
     * ie command with no data.
     */

    // REQ RQ_M-KTA-ICPP-FN-0140(1) : Deserialize No Payload
    // REQ RQ_M-KTA-NOOP-FN-0010(1) : Verify NoOP Message Signature
    if (C_K_ICPP_PARSER_HEADER_LENGTH_FOR_NOOP == headerLength)
    {
      M_KTALOG__DEBUG("Command length is [%d]", C_K_ICPP_PARSER_HEADER_LENGTH_FOR_NOOP);
      status = E_K_ICPP_PARSER_STATUS_NO_OPERATION;
      break;
    }
    status = E_K_ICPP_PARSER_STATUS_OK;
    break;
  }

  return status;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

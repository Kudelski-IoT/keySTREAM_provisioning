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
/** \brief keySTREAM Trusted Agent manager.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/13
 *
 *  \file ktamgr.c
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent manager.
 */
#include "k_kta.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "acthandler.h"
#include "cmdhandler.h"
#include "reghandler.h"
#include "kta_version.h"
#include "config.h"
#include "k_crypto.h"
#include "general.h"
#include "icpp_parser.h"
#include "k_sal.h"
#include "k_sal_storage.h"
#include "k_sal_object.h"
#include "k_sal_crypto.h"
#include "cryptoConfig.h"
#include "KTALog.h"

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Maximum processing error code size. */
#define C_PROCESSING_ERROR_CODE_SIZE                (2u)

/** @brief Message authorization/Decryption Error. */
#define C_MSG_AUTH_DEC_ERROR                        (0X01u)

/** @brief Message format Error. */
#define C_MSG_FORMAT_ERROR                          (0X02u)

/** @brief keySTREAM Trusted Agent State. */
typedef enum
{
  /**
   * No call has been made yet.
   */
  E_KTA_STATE_INITIAL     = 0u,
  /**
   * The function ktaInitialize is called.
   */
  E_KTA_STATE_INITIALIZED = 1u,
  /**
   * The function ktaStartup is called.
   */
  E_KTA_STATE_STARTED     = 2u,
  /**
   * The function ktaSetDeviceInformation is called.
   */
  E_KTA_STATE_RUNNING     = 3u,
  /**
   * Invalid state.
   */
  E_KTA_STATE_INVALID     = 0xFFu
} TKtaState;

/** @brief keySTREAM Trusted Agent Life Cycle NVM Data. */
static const uint8_t gaKtaLifeCycleNVMVData[C_KTA_CONFIG__LIFE_CYCLE_MAX_STATE]
[C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE] =
{
  {0x00, 0x00, 0x00, 0x00},
  /* INIT */
  {0x53, 0x45, 0x41, 0x4C},
  /* SEAL */
  {0x41, 0x43, 0x54, 0x49},
  /* ACTI */
  {0x50, 0x52, 0x4F, 0x56},
  /* PROV */
  {0x43, 0x4F, 0x4E, 0x52},
  /* CONR */
  {0xFF, 0xFF, 0xFF, 0xFF}
  /* INVALID */
};

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/* Module name used for logging */
static const char* gpModuleName = "KTAMGR";

static TKtaState gKtaState                              = E_KTA_STATE_INITIAL;
static TKtaLifeCycleState gKtaLifeCycleState            = E_LIFE_CYCLE_STATE_INIT;
static uint8_t gaKtaVersion[C_K__VERSION_STORAGE_LENGTH] = {0};
static uint8_t  gKtaIsPreActivated                      = 0u;
static TKktaKeyStreamStatus gCommandStatus              = E_K_KTA_KS_STATUS_NONE;

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   Get lifecycle state from NVM.
 *
 * @param[in,out] xpLifeCycleStateLen
 *   [in] State len which need to to be passed to SAL api
 *   [out] Filled lifecycle length from sal api
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lgetNVMLifeCycleState
(
  size_t*  xpLifeCycleStateLen
);

/**
 * @brief
 *   Prepare processing status message.
 *
 * @param [in] xParserSatus
 *   Status to be handled
 * @param [in,out] xpErrorCode
 *   IN-OUT buffer passed in this function to handle the stack data preservation for this api.
 * @param [in] xErrorCodeSize
 *   Holds the size of each error code handling, it is 2 bytes.
 * @param[in,out] xpSendProtoMessage
 *   [in] Protocol message structure to contain notification message info
 *   [out] Filled protocol message structure with necessary info
 *   Output structure to send to server back.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lPrepareProcessingStatus
(
  TKIcppProtocolMessage*  xpSendProtoMessage,
  TKParserStatus          xParserSatus,
  uint8_t*                xpErrorCode,
  uint8_t                 xErrorCodeSize
);

/**
 * @brief
 *   Prepare processing status response message.
 *
 * @param [in] xpReceivedMsg
 *   Message received from the server.
 *   Should not be NULL.
 * @param [in] xReceivedMsgSize
 *   Received message size in bytes.
 * @param [in] xErrorCode
 *   Error code should be set in message.
 * @param[in,out] xpMessageToSend
 *   [in] Pointer to buffer to carry message to send to keySTREAM.
 *   [out] Actual message to provide to keySTREAM.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in,out] xpMessageToSendSize
 *   [in] Pointer to send size buffer.
 *   [out] Actual data length containing message to send to keySTREAM.
 *   xpMessageToSendSize will be filled with actual size received from the keySTREAM.
 *   Should be provided by the caller
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lBuildProcessingStatusRespMsg
(
  const uint8_t*  xpReceivedMsg,
  size_t          xReceivedMsgSize,
  uint32_t        xErrorCode,
  uint8_t*        xpMessageToSend,
  size_t*         xpMessageToSendSize
);

/**
 * @brief
 *   Prepare no operation notification message.
 *
 * @param[in,out] xpMessageToSend
 *   [in] Pointer to buffer to carry NoOp message to send to keySTREAM.
 *   [out] Actual message to provide to keySTREAM.
 *   Should not be NULL.
 *   Buffer must be provided by the caller.
 * @param[in,out] xpMessageToSendSize
 *   [in] Pointer to send size buffer.
 *   [out] Actual data length containing NoOp message to send to keySTREAM.
 *   xpMessageToSendSize will be filled with actual size received from the keySTREAM.
 *   Should be provided by the caller
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lPrepareNoOpNotificationRequest
(
  uint8_t*  xpMessageToSend,
  size_t*   xpMessageToSendSize

);

/**
 * @brief
 *   Does sanity check for message received from keySTREAM.
 *
 * @param[in] xpKs2ktaMsg
 *   Message received from the server.
 *   Should not be NULL
 * @param[in] xKs2ktaMsgLen
 *   Length of message received from the server.
 * @param[in,out] xpKta2ksMsg
 *   [in] Message buffer which need to be filled and send to server from
 *         keySTREAM Trusted Agent.
 *   [out] Filled buffer
 * @param[in,out] xpKta2ksMsgLen
 *   [in] Size of xpKta2ksMsg buffer.
 *   [out] Acutal filled message length.
 * @param[in,out] xpClearMsg
 *   [in] Buffer to store the decrypted message.
 *   [out] Filled buffer
 * @param[in] xClearMsgLen
 *   Size of xpClearMsg buffer.
 * @param[in,out] xpRecvdProtoMessage
 *   [in]  Structure which should be filled with deserilized data
 *   [out] Deserilized filled structure
 * @param[in,out] parserStatus
 *   [in]  parser status which should be filled in this method
 *   [out] Parser status
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameter.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lCheckKs2KtaMessage
(
  const uint8_t*          xpKs2ktaMsg,
  size_t                  xKs2ktaMsgLen,
  uint8_t*                xpKta2ksMsg,
  size_t*                 xpKta2ksMsgLen,
  uint8_t*                xpClearMsg,
  size_t                  xClearMsgLen,
  TKIcppProtocolMessage*  xpRecvdProtoMessage,
  TKParserStatus*         xpParserStatus
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */
/**
 * @brief implement ktaInitialize
 *
 */
TKStatus ktaInitialize
(
  void
)
{
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  if (E_KTA_STATE_INITIAL == gKtaState)
  {
    gKtaState = E_KTA_STATE_INITIALIZED;
    M_KTALOG__DEBUG("KTA initialization SUCCESS!!!");
    status = E_K_STATUS_OK;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaStartup
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaStartup
(
  const uint8_t*  xpL1SegSeed,
  const uint8_t*  xpKtaContextProfileUid,
  size_t          xKtaContextProfileUidLen,
  const uint8_t*  xpKtaContexSerialNumber,
  size_t          xKtaContexSerialNumberLen,
  const uint8_t*  xpKtaContextVersion,
  size_t          xKtaContextVersionLen
)
{
  TKStatus status = E_K_STATUS_ERROR;
  // REQ RQ_M-KTA-LCST-FN-0010(1) : Life Cycle State Size
  size_t  lifeCycleStateLen = C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE;
  size_t  ktaVersionLen = C_K__VERSION_STORAGE_LENGTH;

  M_KTALOG__START("Start");

  /* MISRA Rule 2.1 for loop increment is unreachable, loop will execute only once. Other codes follow the same. */
  // REQ RQ_M-KTA-STRT-FN-0008(1) : Input Parameters Check
  // REQ RQ_M-KTA-STRT-CF-0010(1) : Context Profile UID Size
  // REQ RQ_M-KTA-STRT-CF-0020(1) : Context Serial Number Size
  // REQ RQ_M-KTA-STRT-CF-0030(1) : Context Version Size
  if ((NULL == xpL1SegSeed) || (NULL == xpKtaContextProfileUid) ||
      (0U == xKtaContextProfileUidLen) ||
      (NULL == xpKtaContexSerialNumber) || (0u == xKtaContexSerialNumberLen) ||
      (NULL == xpKtaContextVersion) || (0u == xKtaContextVersionLen) ||
      (C_K__CONTEXT_PROFILE_UID_MAX_SIZE < xKtaContextProfileUidLen) ||
      (C_K__CONTEXT_SERIAL_NUMBER_MAX_SIZE < xKtaContexSerialNumberLen) ||
      (C_K__CONTEXT_VERSION_MAX_SIZE < xKtaContextVersionLen))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter passed");
  }
  else
  {
    // REQ RQ_M-KTA-STRT-FN-0002(1) : Check KTA State
    if (E_KTA_STATE_INITIALIZED == gKtaState)
    {
      M_KTALOG__DEBUG("Reading life cycle state from NVM...");
      // REQ RQ_M-KTA-LCST-FN-0020(1) : Power off in INIT|INITIALIZED state
      // REQ RQ_M-KTA-LCST-FN-0025(1) : Power off in INIT|STARTED state
      status = lgetNVMLifeCycleState(&lifeCycleStateLen);
      status = salStorageGetValue(C_K_KTA__VERSION_SLOT_ID, gaKtaVersion, &ktaVersionLen);
      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__DEBUG("Reading KTA Version failed with status:%d.\r\n", status);
        goto end;
      }
      M_KTALOG__DEBUG("KTA Version in device is: %s\r\n", ktaGetDecodedVersionStr(gaKtaVersion));

      if ((E_K_STATUS_OK != status) || (0U == lifeCycleStateLen))
      {
        M_KTALOG__ERR("Reading life cycle state from NVM failed, status = [%d]", status);
        goto end;
      }

      M_KTALOG__DEBUG("Setting kta context configuration data to the platform");
      // REQ RQ_M-KTA-STRT-FN-0050(1) : Set Context Information
      // REQ RQ_M-KTA-STRT-FN-0060(1) : Set L1 Segmentaion Seed
      status = ktaSetContextInfoConfig(xpL1SegSeed, xpKtaContextProfileUid,
                                      xKtaContextProfileUidLen, xpKtaContexSerialNumber,
                                      xKtaContexSerialNumberLen, xpKtaContextVersion,
                                      xKtaContextVersionLen, gKtaLifeCycleState);

      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("Setting kta context config data, status = [%d]", status);
        goto end;
      }

      // REQ RQ_M-KTA-LCST-FN-0050(1) : Power off in ACTIVATED|INITIALIZED state
      // REQ RQ_M-KTA-LCST-FN-0065(1) : Power off in PROVISIONED|INITIALIZED state
      if ((gKtaLifeCycleState == E_LIFE_CYCLE_STATE_ACTIVATED) ||
          (gKtaLifeCycleState == E_LIFE_CYCLE_STATE_PROVISIONED))
      {
        M_KTALOG__DEBUG("gKtaLifeCycleState = [%d], deriving L2Keys", gKtaLifeCycleState);
        // REQ RQ_M-KTA-STRT-FN-0070(1) : Derive L2 Keys
        status = ktaActDeriveL2Keys();

        if (status != E_K_STATUS_OK)
        {
          M_KTALOG__ERR("Deriving L2 key got failed, status = [%d]", status);
          goto end;
        }
      }

      // REQ RQ_M-KTA-STRT-FN-0003(1) : Set KTA State
      gKtaState = E_KTA_STATE_STARTED;
      M_KTALOG__DEBUG("KTA reached to STARTED state");
    }
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaSetDeviceInformation
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaSetDeviceInformation
(
  const uint8_t*  xpDeviceProfilePublicUid,
  size_t          xDeviceProfilePublicUidSize,
  const uint8_t*  xpDeviceSerialNum,
  size_t          xDeviceSerialNumSize,
  uint8_t*        xpConnectionRequest
)
{
  TKStatus status = E_K_STATUS_ERROR;
  uint8_t  aKtaVersion[C_K__VERSION_STORAGE_LENGTH] = C_K_KTA__ENCODED_VERSION;

  M_KTALOG__START("Start");

  // REQ RQ_M-KTA-STRT-FN-0090(1) : Input Parameters Check
  if ((NULL == xpDeviceProfilePublicUid) ||
      (0u == xDeviceProfilePublicUidSize) ||
      (C_K__DEVICE_PROFILE_PUBLIC_UID_MAX_SIZE < xDeviceProfilePublicUidSize) ||
      (NULL == xpDeviceSerialNum) ||
      (0u == xDeviceSerialNumSize) ||
      (C_K__DEVICE_SERIAL_NUM_MAX_SIZE < xDeviceSerialNumSize) ||
      (NULL == xpConnectionRequest))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter passed");
  }
  else
  {
    if (E_KTA_STATE_STARTED == gKtaState)
    {
      switch (gKtaLifeCycleState)
      {
        case E_LIFE_CYCLE_STATE_INIT:
        {
          M_KTALOG__DEBUG("Setting the Device info configuration data to the platform");
          // REQ RQ_M-KTA-STRT-FN-0130(1) : Update the device info
          status = ktaSetDeviceInfoConfig(xpDeviceProfilePublicUid,
                                          xDeviceProfilePublicUidSize,
                                          xpDeviceSerialNum,
                                          xDeviceSerialNumSize,
                                          gKtaLifeCycleState);

          if (E_K_STATUS_OK != status)
          {
            M_KTALOG__ERR("Setting Device info config data failed, status = [%d]", status);
            goto end;
          }

          M_KTALOG__DEBUG("Life cycle state reached to SEALED state, storing in persistent memory");
          // REQ RQ_M-KTA-LCST-FN-0040(1) : Power off in SEALED|STARTED state
          // REQ RQ_M-KTA-LCST-FN-0035(1) : Power off in SEALED|INITIALIZED state
          // REQ RQ_M-KTA-LCST-FN-0085(1) : Power off in CON_REQ|STARTED state
          // REQ RQ_M-KTA-LCST-FN-0080(1) : Power off in CON_REQ|INITIALIZED state
          status = salStorageSetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                                      gaKtaLifeCycleNVMVData[E_LIFE_CYCLE_STATE_SEALED],
                                      C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE);

          if (E_K_STATUS_OK != status)
          {
            M_KTALOG__ERR("SAL API failed while storing life cycle state, status = [%d]", status);
            goto end;
          }

          gKtaLifeCycleState = E_LIFE_CYCLE_STATE_SEALED;
          M_KTALOG__INFO("Setting life cycle state to SEALED state, gKtaLifeCycleState = [%d]",
                          gKtaLifeCycleState);
          *xpConnectionRequest = 1;
          M_KTALOG__DEBUG("Connection Request set to TRUE");
          status = E_K_STATUS_OK;
        }
        break;

        // REQ RQ_M-KTA-LCST-FN-0040(1) : Power off in SEALED|STARTED state
        // REQ RQ_M-KTA-LCST-FN-0055(1) : Power off in ACTIVATED|STARTED state
        // REQ RQ_M-KTA-LCST-FN-0085(1) : Power off in CON_REQ|STARTED state
        case E_LIFE_CYCLE_STATE_SEALED:
        {
          status = ktaSetDeviceInfoConfig(xpDeviceProfilePublicUid,
                                          xDeviceProfilePublicUidSize,
                                          xpDeviceSerialNum,
                                          xDeviceSerialNumSize,
                                          gKtaLifeCycleState);
        } /* No break, since need to execute below cases as well. */
        case E_LIFE_CYCLE_STATE_ACTIVATED:
        case E_LIFE_CYCLE_STATE_CON_REQ:
        {
          M_KTALOG__DEBUG("Connection Request1 set to TRUE");
          *xpConnectionRequest = 1;
          status = E_K_STATUS_OK;
        }
        break;

        // REQ RQ_M-KTA-LCST-FN-0070(1) : Power off in PROVISIONED|STARTED state
        case E_LIFE_CYCLE_STATE_PROVISIONED:
        {
          if (memcmp(gaKtaVersion, aKtaVersion, C_K__VERSION_STORAGE_LENGTH) < 0)
          {
              M_KTALOG__DEBUG("Connection Request set to TRUE");
              *xpConnectionRequest = 1;
              status = E_K_STATUS_OK;
              break;
          }
          M_KTALOG__DEBUG("Connection Request set to FALSE");
          *xpConnectionRequest = 0;
          status = E_K_STATUS_OK;
        }
        break;

        default:
        {
          M_KTALOG__ERR("Invalid state, gKtaLifeCycleState = [%d]",
                        gKtaLifeCycleState);
          *xpConnectionRequest = 0;
        }
        break;
      }

      // REQ RQ_M-KTA-STRT-FN-0080(1) : Set KTA State
      // REQ RQ_M-KTA-STRT-FN-0140(1) : Set KTA State
      if (status == E_K_STATUS_OK)
      {
        gKtaState = E_KTA_STATE_RUNNING;
        M_KTALOG__DEBUG("KTA reached to RUNNING state");
      }
    }
    // REQ RQ_M-KTA-STRT-FN-0040(1) : Invalid KTA State
    // REQ RQ_M-KTA-STRT-FN-0120(1) : Invalid KTA State
    else
    {
      M_KTALOG__ERR("Device in a bad state, gKtaState = [%d]", gKtaState);
    }
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaExchangeMessage
 *
 */
TKStatus ktaExchangeMessage
(
  const uint8_t*  xpKs2ktaMsg,
  size_t          xKs2ktaMsgLen,
  uint8_t*        xpKta2ksMsg,
  size_t*         xpKta2ksMsgLen
)
{

  TKIcppProtocolMessage recvdProtoMessage = {0};
  TKStatus              status = E_K_STATUS_ERROR;
  TKParserStatus        parserStatus = E_K_ICPP_PARSER_STATUS_ERROR;
  uint8_t               aClearMsg[C_K__ICPP_MSG_MAX_SIZE] = {0};

  M_KTALOG__START("Start");

  // REQ RQ_M-KTA-STRT-FN-0150(1) : Input Parameters Check
  // REQ RQ_M-KTA-STRT-CF-0160(1) : ICPP Message Max Size
  if (
      (NULL == xpKs2ktaMsg)     ||
      (NULL == xpKta2ksMsg)     ||
      (NULL == xpKta2ksMsgLen) ||
      (0u == *xpKta2ksMsgLen) ||
      (C_K__ICPP_MSG_MAX_SIZE < xKs2ktaMsgLen)

      )
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter passed");
  }
  else
  {
    // REQ RQ_M-KTA-STRT-FN-0170(1) : Invalid KTA State
    if (E_KTA_STATE_RUNNING != gKtaState)
    {
      M_KTALOG__ERR("Invalid KTA State");
    }
    else
    {
      switch (gKtaLifeCycleState)
      {
        // REQ RQ_M-KTA-LCST-FN-0030(1) : Power off in SEALED|RUNNING state
        case E_LIFE_CYCLE_STATE_SEALED:
          if (xKs2ktaMsgLen == 0u)
          {
            M_KTALOG__DEBUG("Preparing the activation request...");
            // REQ RQ_M-KTA-ACTV-FN-0005(1) :
            // Build Activation Request
            // REQ RQ_M-KTA-STRT-FN-0200(1) : Prepare the activation msg
            status = ktaActBuildActivationRequest(xpKta2ksMsg, xpKta2ksMsgLen);

            if (E_K_STATUS_OK != status)
            {
              *xpKta2ksMsgLen = 0;
              M_KTALOG__ERR("Preparing the activation request got failed, status = [%d]",
                            status);
              break;
            }
            M_KTALOG__INFO("Sending Activation Request");
          break;
          }

          M_KTALOG__DEBUG("Validating the msg received from the server...");
          status = lCheckKs2KtaMessage(xpKs2ktaMsg, xKs2ktaMsgLen,
                                        xpKta2ksMsg, xpKta2ksMsgLen,
                                        aClearMsg, sizeof(aClearMsg),
                                        &recvdProtoMessage, &parserStatus);

          if ((status != E_K_STATUS_OK) || (parserStatus != E_K_ICPP_PARSER_STATUS_OK))
          {
            M_KTALOG__ERR("Msg validation received from the server got failed,");
            M_KTALOG__ERR("status = [%d], parserStatus = [%d]", status, parserStatus);
            break;
          }

          if (gKtaIsPreActivated != (uint8_t)0)
          {
            uint8_t aL1KeyMaterial[C_KEY_CONFIG__MATERIAL_MAX_SIZE] = {0};

            M_KTALOG__DEBUG("KTA in PreActivated state, generating aL1KeyMaterial");
            status = ktaGetL1SegSeed(aL1KeyMaterial);

            if (E_K_STATUS_OK != status)
            {
              *xpKta2ksMsgLen = 0;
              M_KTALOG__ERR("Getting L1 SegSeed got failed, status = [%d]", status);
              break;
            }

            aL1KeyMaterial[C_K__L1_SEGMENTATION_SEED_SIZE] = recvdProtoMessage.rotKeySetId;
            status = ktaSetRotKeySetId(recvdProtoMessage.rotKeySetId);

            if (E_K_STATUS_OK != status)
            {
              *xpKta2ksMsgLen = 0;
              M_KTALOG__ERR("ktaSetRotKeySetId failed, status = [%d]", status);
              break;
            }

            M_KTALOG__DEBUG("Storing the received aL1KeyMaterial into persistent memory");
            status = salStorageSetValue(C_K_KTA__L1_KEY_MATERIAL_DATA_ID,
                                        aL1KeyMaterial, C_KEY_CONFIG__MATERIAL_MAX_SIZE);

            if (E_K_STATUS_OK != status)
            {
              *xpKta2ksMsgLen = 0;
              M_KTALOG__ERR("SAL API failed while storing the L1 key material, status = [%d]",
                            status);
              break;
            }

            M_KTALOG__DEBUG("Life cycle state reached to ACTIVATED state, "
                            "storing in persist memory");
            // REQ RQ_M-KTA-STRT-FN-0240(1) : Change LifeCycleState to Activated
            // after receiving the thirdParty Or Object Managment Commands

            // REQ RQ_M-KTA-LCST-FN-0050(1) : Power off in ACTIVATED|INITIALIZED state
            // REQ RQ_M-KTA-LCST-FN-0055(1) : Power off in ACTIVATED|STARTED state
            // REQ RQ_M-KTA-LCST-FN-0045(1) : Power off in ACTIVATED|RUNNING state
            status = salStorageSetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                                        gaKtaLifeCycleNVMVData[E_LIFE_CYCLE_STATE_ACTIVATED],
                                        C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE);

            if (E_K_STATUS_OK != status)
            {
              *xpKta2ksMsgLen = 0;
              M_KTALOG__ERR("SAL API failed while storing the life cycle state, status = [%d]",
                            status);
              break;
            }

            gKtaLifeCycleState = E_LIFE_CYCLE_STATE_ACTIVATED;
            M_KTALOG__INFO("Setting KTA Lifecycle state to Activated, state = [%d]", gKtaLifeCycleState);
            gKtaIsPreActivated = 0u;
            M_KTALOG__DEBUG("Processing 3rd party commands...");
            // REQ RQ_M-KTA-STRT-FN-0290(1) :
            /** Process the received commands.
             */
            // REQ RQ_M-KTA-OBJM-FN-0800(1) : Set Object With Association ICPP Message
            status = ktaCmdProcess(&recvdProtoMessage,
                                    xpKta2ksMsg,
                                    xpKta2ksMsgLen);

            if (E_K_STATUS_OK != status)
            {
              *xpKta2ksMsgLen = 0;
              M_KTALOG__ERR("Processing of 3rd party command got failed, status = [%d]",
                            status);
              break;
            }

            break;
          }
          else
          {
            M_KTALOG__DEBUG("Deriving L1 field key...");
            // REQ RQ_M-KTA-STRT-FN-0210(1) : Process the activation response msg
            status = ktaActResponseBuildL1Keys(&recvdProtoMessage);

            if (E_K_STATUS_OK != status)
            {
              *xpKta2ksMsgLen = 0;
              M_KTALOG__ERR("Deriving L1 field key failed, status = [%d]", status);
              break;
            }

            M_KTALOG__DEBUG("BuildL1Keys success, deriving L2 Keys...");
            status = ktaActDeriveL2Keys();

            if (E_K_STATUS_OK != status)
            {
              *xpKta2ksMsgLen = 0;
              M_KTALOG__ERR("Deriving L2 key got failed, status = [%d]", status);
              break;
            }

            M_KTALOG__DEBUG("Building registration request...");
            // REQ RQ_M-KTA-REGT-FN-0011(1) : Build Registeration Info Request
            // REQ RQ_M-KTA-STRT-FN-0220(1) :
            /* Prepare Reg Info msg after processing activation response msg. */
            status = ktaregBuildRegistrationRequest(&recvdProtoMessage,
                                                    xpKta2ksMsg,
                                                    xpKta2ksMsgLen);

            if (E_K_STATUS_OK != status)
            {
              *xpKta2ksMsgLen = 0;
              M_KTALOG__ERR("Building registration request got failed, status = [%d]", status);
              break;
            }
            M_KTALOG__INFO("Sending Registration Request");

            gKtaIsPreActivated = 1u;
          }

          break;

        case E_LIFE_CYCLE_STATE_ACTIVATED:
        case E_LIFE_CYCLE_STATE_PROVISIONED:
        case E_LIFE_CYCLE_STATE_CON_REQ:
          if (xKs2ktaMsgLen == 0U)
          {
            M_KTALOG__DEBUG("Preparing NoOp notification request...");
            // REQ RQ_M-KTA-NOOP-FN-0050(1) : Build NoOP message Message
            // REQ RQ_M-KTA-RENW-FN-0010(1) : NoOP Message
            // REQ RQ_M-KTA-RFSH-FN-0010(1) : NoOP Message from KTA
            // REQ RQ_M-KTA-STRT-FN-0280(1) : Prepare NoOP Message
            status = lPrepareNoOpNotificationRequest(xpKta2ksMsg, xpKta2ksMsgLen);

            if (E_K_STATUS_OK != status)
            {
              *xpKta2ksMsgLen = 0;
              M_KTALOG__ERR("Preparation of NoOp notification request got failed, status = [%d]",
                            status);
            }

            break;
          }

          if (E_LIFE_CYCLE_STATE_PROVISIONED == gKtaLifeCycleState)
          {
            /* Setting the state to connection request. */
            M_KTALOG__DEBUG("Life cycle state reached to CON_REQ state, "
                            "storing in persistent memory");
            status = salStorageSetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                                        gaKtaLifeCycleNVMVData[E_LIFE_CYCLE_STATE_CON_REQ],
                                        C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE);

            if (E_K_STATUS_OK != status)
            {
              M_KTALOG__ERR("SAL API failed while storing life cycle state, status = [%d]", status);
              *xpKta2ksMsgLen = 0;
              break;
            }

            gKtaLifeCycleState = E_LIFE_CYCLE_STATE_CON_REQ;
          }

          M_KTALOG__DEBUG("Validating the msg received from the server...");
          status = lCheckKs2KtaMessage(xpKs2ktaMsg, xKs2ktaMsgLen,
                                        xpKta2ksMsg, xpKta2ksMsgLen,
                                        aClearMsg, sizeof(aClearMsg),
                                        &recvdProtoMessage, &parserStatus);

          if ((status != E_K_STATUS_OK) || (parserStatus != E_K_ICPP_PARSER_STATUS_OK))
          {
            (*xpKta2ksMsgLen) = 0;

            if (parserStatus != E_K_ICPP_PARSER_STATUS_NO_OPERATION)
            {
              M_KTALOG__ERR("Validation of the msg received from the server failed,");
              M_KTALOG__ERR("status = [%d], parserStatus = [%d]", status, parserStatus);
            }

            break;
          }

          M_KTALOG__DEBUG("Reading rotKeySetId from NVM...");
          // REQ RQ_M-KTA-STRT-FN-0230(1) : Update rot key set Id
          // REQ RQ_M-KTA-STRT-FN-0250(1) : Update the rot key set id in the after
          // deserialize the received message.
          status = ktaGetRotKeySetId(&recvdProtoMessage.rotKeySetId);

          if (E_K_STATUS_OK != status)
          {
            *xpKta2ksMsgLen = 0;
            M_KTALOG__ERR("Reading of rotKeySetId from NVM got failed, status = [%d]",
                          status);
            break;
          }

          M_KTALOG__DEBUG("Processing 3rd party command...");
          // REQ RQ_M-KTA-STRT-FN-0260(1) : Process the ThirdParty/Object Commands.
          status = ktaCmdProcess(&recvdProtoMessage, xpKta2ksMsg, xpKta2ksMsgLen);

          if (E_K_STATUS_OK != status)
          {
            *xpKta2ksMsgLen = 0;
            M_KTALOG__ERR("Processing of 3rd party command failed, status = [%d]", status);
            break;
          }

          break;

        default:
          *xpKta2ksMsgLen = 0;
          M_KTALOG__ERR("Invalid life cycle state, [%d]", gKtaLifeCycleState);
          break;
      }
    }
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

#ifdef OBJECT_MANAGEMENT_FEATURE
/**
 * @brief implement ktaGetObjectWithAssociation
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaGetObjectWithAssociation
(
  uint32_t   xObjWithAssociationId,
  uint32_t*  xpAssociatedKeyId,
  uint32_t*  xpAssociatedObjId,
  uint8_t*   xpOutData,
  size_t*    xpOutDataLen
)
{
  uint8_t                 aPsaStatus[4] = {0};
  TKSalObjAssociationInfo associationInfoOut = {0};
  TKStatus                status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  // REQ RQ_M-KTA-STRT-FN-0400(1) : Input Parameters Check
  if ((0u == xObjWithAssociationId) ||
      (NULL == xpAssociatedKeyId)      ||
      (NULL == xpAssociatedObjId)      ||
      (NULL == xpOutData)              ||
      (NULL == xpOutDataLen)           ||
      (0u == *xpOutDataLen))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("Invalid parameter passed");
    goto end;
  }

  // REQ RQ_M-KTA-STRT-FN-0420(1) : Get Object with Association
  status = salObjectGetWithAssociation(xObjWithAssociationId,
                                        xpOutData,
                                        xpOutDataLen,
                                        &associationInfoOut,
                                        (uint8_t*)aPsaStatus);

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("salObjectGetWithAssociation failed[%d]", status);
    status = E_K_STATUS_ERROR;
    goto end;
  }

  /* Destroy the deprecated Key with specified ID from the storage.*/
  if (associationInfoOut.associatedKeyIdDeprecated != 0u)
  {
    // REQ RQ_M-KTA-STRT-FN-0410(1) : Delete Key/Certificate
    status = salObjectKeyDelete(associationInfoOut.associatedKeyIdDeprecated,
                                (uint8_t*)aPsaStatus);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("salObjectDelete failed[%d]", status);
      status = E_K_STATUS_ERROR;
      goto end;
    }

    associationInfoOut.associatedKeyIdDeprecated = 0;
    status = salObjectSetWithAssociation(C_SAL_OBJECT__TYPE_KEY,
                                          xObjWithAssociationId,
                                          aPsaStatus,
                                          0,
                                          xpOutData,
                                          *xpOutDataLen,
                                          &associationInfoOut,
                                          aPsaStatus);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("salObjectDelete failed[%d]", status);
      status = E_K_STATUS_ERROR;
      goto end;
    }
  }

  /* Destroy the deprecated object with specified ID from the storage.*/
  if (associationInfoOut.associatedObjectIdDeprecated != 0u)
  {
    // REQ RQ_M-KTA-STRT-FN-0410(1) : Delete Key/Certificate
    status = salObjectDelete(C_SAL_OBJECT__TYPE_CERTIFICATE,
                              associationInfoOut.associatedObjectIdDeprecated,
                              (uint8_t*)aPsaStatus);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR(" salObjectDelete failed[%d]", status);
      status = E_K_STATUS_ERROR;
      goto end;
    }
  }

  /* Provide the active Object and Key IDs back to the application */
  *xpAssociatedKeyId = associationInfoOut.associatedKeyId;
  *xpAssociatedObjId = associationInfoOut.associatedObjectId;
  status = E_K_STATUS_OK;

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaGetAssociatedObject
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaGetAssociatedObject
(
  uint32_t  xObjWithAssociationId,
  uint8_t*  xpOutData,
  size_t*   xpOutDataLen
)
{
  uint8_t         aPsaStatus[4] = {0};
  TKStatus        status = E_K_STATUS_ERROR;
  TKSalObjectType objType = C_SAL_OBJECT__TYPE_CERTIFICATE;

  M_KTALOG__START("Start");

  // REQ RQ_M-KTA-STRT-FN-0300(1) : Input Parameters Check
  if ((0UL == xObjWithAssociationId) ||
      (NULL == xpOutData)              ||
      (NULL == xpOutDataLen)           ||
      (0UL == *xpOutDataLen))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("[ktaGetAssociatedObject]Invalid parameter passed");
    goto end;
  }

  // REQ RQ_M-KTA-STRT-FN-0310(1) : Get Associated Object
  status = salObjectGet(objType,
                        xObjWithAssociationId,
                        xpOutData,
                        xpOutDataLen,
                        (uint8_t*)aPsaStatus);

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("[ktaGetAssociatedObject] failed[%d]", status);
    status = E_K_STATUS_ERROR;
    goto end;
  }

  status = E_K_STATUS_OK;

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktaSignHash
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaSignHash
(
  uint32_t  xKeyId,
  uint8_t*  xpHash,
  size_t    xHashLen,
  uint8_t*  xpSignedHashOutBuff,
  uint32_t  xSignedHashOutBuffLen,
  size_t*   xpActualSignedHashOutLen
)
{
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  // REQ RQ_M-KTA-STRT-FN-0600(1) : Input Parameters Check
  if (
    (NULL == xpHash) ||
    (NULL == xpSignedHashOutBuff) ||
    (NULL == xpActualSignedHashOutLen) ||
    (0u == xHashLen) ||
    (0u == xSignedHashOutBuffLen) ||
    (32u < xHashLen) ||
    (64u < xSignedHashOutBuffLen)
  )
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("[ktaSignHash]Invalid parameter passed");
    goto end;
  }

  // REQ RQ_M-KTA-STRT-FN-0610(1) : Sign hash data
  status = salSignHash(xKeyId, xpHash, xHashLen,
                        xpSignedHashOutBuff, xSignedHashOutBuffLen,
                        xpActualSignedHashOutLen);

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("ktaSignHash failed[%d]", status);
    goto end;
  }

  status = E_K_STATUS_OK;

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}
#endif

/**
 * @brief implement ktaKeyStreamStatus
 *
 */
TKStatus ktaKeyStreamStatus
(
  TKktaKeyStreamStatus*  xpKtaKSCmdStatus
)
{
  TKktaKeyStreamStatus  cmdStatus = E_K_KTA_KS_STATUS_NO_OPERATION;
  TKStatus              status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  //REQ RQ_M-KTA-STRT-FN-0500(1) : Input Parameters Check
  if (xpKtaKSCmdStatus == NULL)
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("[ktaKeyStreamStatus]Invalid parameter passed");
  }
  else
  {
    cmdStatus = gCommandStatus;
    // REQ RQ_M-KTA-STRT-FN-0510(1) : Get Key Stream Status
    *xpKtaKSCmdStatus = cmdStatus;

    if (gCommandStatus != E_K_KTA_KS_STATUS_REFURBISH)
    {
      gCommandStatus = E_K_KTA_KS_STATUS_NO_OPERATION;
    }

    status = E_K_STATUS_OK;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

#ifdef TEST_COVERAGE
void ktaReset
(
  void
)
{
  gKtaState = E_KTA_STATE_INITIAL;
  gKtaLifeCycleState = E_LIFE_CYCLE_STATE_INIT;
  gKtaIsPreActivated = 0u;
  ktaResetConfig();
}
#endif
/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @implements lgetNVMLifeCycleState
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
static TKStatus lgetNVMLifeCycleState
(
  size_t*  xpLifeCycleStateLen
)
{
  TKStatus  status = E_K_STATUS_ERROR;
  uint8_t   stateIndex = 0;
  uint8_t   aLifeCycleState[C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE] = {0x00, 0x00, 0x00, 0x00};

  status = salStorageGetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                              aLifeCycleState, xpLifeCycleStateLen);

  if ((E_K_STATUS_OK != status) || (0u == *xpLifeCycleStateLen))
  {
    M_KTALOG__WARN("Reading life cycle state failed!!, status = [%d]", status);
    status = salStorageSetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                                aLifeCycleState,
                                C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("Writing life cycle state failed, status = [%d]", status);
      goto end;
    }

    // REQ RQ_M-KTA-LCST-FN-0010(1) : Life Cycle State Size
    *xpLifeCycleStateLen = C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE;
  }

  for (; stateIndex < C_KTA_CONFIG__LIFE_CYCLE_MAX_STATE; stateIndex++)
  {
    if (0 == memcmp(gaKtaLifeCycleNVMVData[stateIndex],
                    aLifeCycleState, C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE))
    {
      gKtaLifeCycleState = (TKtaLifeCycleState)stateIndex;
      break;
    }
  }

  if (stateIndex >= (C_KTA_CONFIG__LIFE_CYCLE_MAX_STATE - 1U))
  {
    M_KTALOG__WARN("Wrong life cycle state found in storage!! Setting to Init state");
    (void)memset(aLifeCycleState, 0, C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE);
    status = salStorageSetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                                aLifeCycleState,
                                C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR(
        "SAL API failed while writing life cycle state in storage!!, status = [%d]",
        status);
      goto end;
    }

    gKtaLifeCycleState = E_LIFE_CYCLE_STATE_INIT;
  }
  else
  {
    gKtaLifeCycleState = (TKtaLifeCycleState)stateIndex;
  }

end:
  return status;
}

/**
 * @implements lPrepareProcessingStatus
 *
 */
static TKStatus lPrepareProcessingStatus
(
  TKIcppProtocolMessage*  xpSendProtoMessage,
  TKParserStatus          xParserSatus,
  uint8_t*                xpErrorCode,
  uint8_t                 xErrorCodeSize
)
{
  uint32_t currentPos = 0;
  TKStatus status     = E_K_STATUS_ERROR;

  /**
   * Only 1 command is available for this message type,
   * for handling single command to calculate commands count.
   */
  xpSendProtoMessage->commandsCount = xpSendProtoMessage->commandsCount + 1u;
  currentPos = xpSendProtoMessage->commandsCount - 1u;
  /**
   * Set the command tag with "E_K_ICCP_PARSER_COMMAND_TAG_PROCESSING_STATUS" to indicate
   * the command is for processing status.
   */
  xpSendProtoMessage->commands[currentPos].commandTag =
    E_K_ICPP_PARSER_COMMAND_TAG_PROCESSING_STATUS;

  switch (xParserSatus)
  {
    case E_K_ICPP_PARSER_STATUS_ERROR:
    {
      xpSendProtoMessage->commands[currentPos].data.cmdInfo.cmdLen = xErrorCodeSize;
      xpSendProtoMessage->commands[currentPos].data.cmdInfo.cmdValue = xpErrorCode;
    }
    break;

    default:
    M_KTALOG__ERR("Invalid xParserSatus, [%d]", xParserSatus);
    break;
  }

  status = E_K_STATUS_OK;

  return status;
}

/**
 * @implements lBuildProcessingStatusRespMsg
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
static TKStatus lBuildProcessingStatusRespMsg
(
  const uint8_t*  xpReceivedMsg,
  size_t          xReceivedMsgSize,
  uint32_t        xErrorCode,
  uint8_t*        xpMessageToSend,
  size_t*         xpMessageToSendSize
)
{
  TKIcppProtocolMessage sendProtoMessage = {0};
  TKStatus              status = E_K_STATUS_ERROR;
  uint8_t               aErrorCode[C_PROCESSING_ERROR_CODE_SIZE] = {0};

  aErrorCode[1] = (uint8_t)(xErrorCode & (uint32_t)0xFF);

  if (E_K_ICPP_PARSER_STATUS_OK != ktaIcppParserDeserializeHeader(xpReceivedMsg,
      xReceivedMsgSize,
      &sendProtoMessage))
  {
    M_KTALOG__ERR("ICCP parser de-serialization of header failed");
    goto end;
  }

  /**
   * Fill the messgae type with "E_K_ICCP_PARSER_MESSAGE_TYPE_RESPONSE" to indicate it is
   * registration notification message type (client -> server).
   */
  sendProtoMessage.msgType  = E_K_ICPP_PARSER_MESSAGE_TYPE_RESPONSE;

  /* In case of parser error prepare data for processing command. */
  // REQ RQ_M-KTA-OBJM-FN-0261(1) : Check Command Processing Error
  // REQ RQ_M-KTA-OBJM-FN-0561(1) : Check Command Processing Error
  // REQ RQ_M-KTA-OBJM-FN-0761(1) : Check Command Processing Error
  // REQ RQ_M-KTA-OBJM-FN-0961(2) : Check Command Processing Error
  status = lPrepareProcessingStatus(&sendProtoMessage,
                                    E_K_ICPP_PARSER_STATUS_ERROR,
                                    aErrorCode,
                                    (uint8_t)sizeof(aErrorCode));

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("Prepare processing status message failed, status = [%d]", status);
    goto end;
  }

  status = ktaGenerateResponse((C_GEN__SERIALIZE | C_GEN__PADDING |
                                C_GEN__ENCRYPT | C_GEN__SIGNING), &sendProtoMessage,
                                xpMessageToSend,
                                xpMessageToSendSize);

end:
  return status;
}

/**
 * @implements lPrepareNoOpNotificationRequest
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
static TKStatus lPrepareNoOpNotificationRequest
(
  uint8_t*  xpMessageToSend,
  size_t*   xpMessageToSendSize
)
{
  TKStatus              status = E_K_STATUS_ERROR;
  size_t                transactionIDLen = C_K_ICPP_PARSER__TRANSACTION_ID_SIZE_IN_BYTES;
  TKIcppProtocolMessage sendProtoMessage = {0};
  size_t                rotPublicUidLen = C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES;
  uint8_t               aSerializeBuffer[C_K__ICPP_MSG_MAX_SIZE] = {0};
  size_t                aSerializeBufferLen = C_K__ICPP_MSG_MAX_SIZE;
  TKParserStatus        parserStatus = E_K_ICPP_PARSER_STATUS_ERROR;
  uint8_t               aComputedMac[C_KTA_ACT__HMACSHA256_SIZE] = {0};

  /**
   * Fill the messgae type with "E_K_ICPP_PARSER_MESSAGE_TYPE_NOTIFICATION" to indicate it is
   * device start scenario  (client -> server).
   */
  // REQ RQ_M-KTA-NOOP-FN-0050_03(1) : message type
  sendProtoMessage.msgType  = E_K_ICPP_PARSER_MESSAGE_TYPE_NOTIFICATION;
  /* Retrive rot key set ID from NVM. */
  // REQ RQ_M-KTA-NOOP-FN-0050_05(1) : rot key set id
  status = ktaGetRotKeySetId(&(sendProtoMessage.rotKeySetId));

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("Retrieval of rot key set ID from NVM failed, status = [%d]", status);
    goto end;
  }

  /* Fill the crypto version to use (for activation request dos based encryption is used). */
  // REQ RQ_M-KTA-NOOP-FN-0050_01(1) : crypto version
  sendProtoMessage.cryptoVersion  = E_K_ICPP_PARSER_CRYPTO_TYPE_L2_BASED;
  /* Fill the mode of encryption (for no ops full encryption is used). */
  // REQ RQ_M-KTA-NOOP-FN-0050_02(1) : partial encryption mode
  sendProtoMessage.encMode  = E_K_ICPP_PARSER_FULL_ENC_MODE;
  /* Fill the random transaction ID. */
  // REQ RQ_M-KTA-NOOP-FN-0050_04(1) : transaction id
  status = salCryptoGetRandom(sendProtoMessage.transactionId,
                              &transactionIDLen);

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("SAL API while reading the transaction ID failed, status = [%d]", status);
    goto end;
  }

  // REQ RQ_M-KTA-NOOP-FN-0050_06(1) : rot public uid
  status = salStorageGetValue(C_K_KTA__ROT_PUBLIC_UID_STORAGE_ID,
                              sendProtoMessage.rotPublicUID,
                              &rotPublicUidLen);

  if ((E_K_STATUS_OK != status) && (0u == rotPublicUidLen))
  {
    M_KTALOG__ERR("rotPubUID reading failed, status = [%d], rotPubUidLen = [%d]",
                  status, rotPublicUidLen);
    (void)memset(sendProtoMessage.rotPublicUID,
                  0x00,
                  C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES);
  }

  sendProtoMessage.commandsCount = 0;

  M_KTALOG__DEBUG("ICCP parser serializing the message...");
  // REQ RQ_M-KTA-NOOP-FN-0060(1) : Serialize NoOP Message
  parserStatus = ktaIcppParserSerializeMessage(&sendProtoMessage,
                                                aSerializeBuffer,
                                                &aSerializeBufferLen);

  if (E_K_ICPP_PARSER_STATUS_OK != parserStatus)
  {
    status = E_K_STATUS_ERROR;
    M_KTALOG__ERR("Serialization of message failed, parserStatus = [%d]", parserStatus);
    goto end;
  }

  M_KTALOG__DEBUG("KTA cipher signing the message...");
  // REQ RQ_M-KTA-NOOP-FN-0070(1) : Sign the encrypted NoOP response message
  status = ktacipherSignMsg(aSerializeBuffer, aSerializeBufferLen, aComputedMac);

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("KTA cipher signing of message failed, status = [%d]", status);
    goto end;
  }

  (void)memcpy(xpMessageToSend, aSerializeBuffer, aSerializeBufferLen);
  (void)memcpy(&xpMessageToSend[aSerializeBufferLen], aComputedMac, C_KTA_ACT__HMACSHA256_SIZE);
  *xpMessageToSendSize =  aSerializeBufferLen + C_KTA_ACT__HMACSHA256_SIZE;

end:
  return status;
}

/**
 * @implements lCheckKs2KtaMessage
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
static TKStatus lCheckKs2KtaMessage
(
  const uint8_t*          xpKs2ktaMsg,
  size_t                  xKs2ktaMsgLen,
  uint8_t*                xpKta2ksMsg,
  size_t*                 xpKta2ksMsgLen,
  uint8_t*                xpClearMsg,
  size_t                  xClearMsgLen,
  TKIcppProtocolMessage*  xpRecvdProtoMessage,
  TKParserStatus*         xpParserStatus
)
{
  TKStatus  status = E_K_STATUS_ERROR;
  uint8_t   aKs2ktaMsgUpdatedHeaderLenBuffer[C_K_ICPP_PARSER__HEADER_SIZE] = {0};
  uint32_t  macOffset = 0;
  size_t    lenWithoutMACandPadding = 0;
  size_t    clearMsgLength = xClearMsgLen;
  uint8_t   aKtaVersion[C_K__VERSION_STORAGE_LENGTH] = C_K_KTA__ENCODED_VERSION;

  macOffset = xKs2ktaMsgLen - C_K_KTA__HMAC_MAX_SIZE;
  /* Verify the signature. */
  M_KTALOG__DEBUG("KTA cipher signature validation...");
  // REQ RQ_M-KTA-ACTV-FN-0055(1) : Verify Activation Response Signature
  status = ktacipherVerifySignedMsg(xpKs2ktaMsg,
                                    macOffset,
                                    &xpKs2ktaMsg[macOffset]);

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("KTA cipher signature validation failed, status = [%d]", status);
    goto end;
  }
  (void)memcpy(aKs2ktaMsgUpdatedHeaderLenBuffer, xpKs2ktaMsg, C_K_ICPP_PARSER__HEADER_SIZE);

  /* We have no operation command(command with payload as 0). */
  if (C_K_ICPP_PARSER__HEADER_SIZE < macOffset)
  {
    /* Decrypt the message. */
    // REQ RQ_M-KTA-ACTV-FN-0060(1) : Decrypt the Activation Response data.
    // REQ RQ_M-KTA-OBJM-FN-0020(1) : Decrypt the Generate Key Pair data
    // REQ RQ_M-KTA-OBJM-FN-0220(1) : Decrypt the Set Object data
    // REQ RQ_M-KTA-OBJM-FN-0520(1) : Decrypt the Delete Object data
    // REQ RQ_M-KTA-OBJM-FN-0720(1) : Decrypt the Set Object With Association data
    // REQ RQ_M-KTA-OBJM-FN-0920(2) : Decrypt the Delete Key Object data
    // REQ RQ_M-KTA-TRDP-FN-0020(1) : Decrypt the Third party data
    clearMsgLength = clearMsgLength - C_K_ICPP_PARSER__HEADER_SIZE;
    status = ktacipherDecrypt(&xpKs2ktaMsg[C_K_ICPP_PARSER__HEADER_SIZE],
                              macOffset - C_K_ICPP_PARSER__HEADER_SIZE,
                              &xpClearMsg[C_K_ICPP_PARSER__HEADER_SIZE],
                              &clearMsgLength);
    clearMsgLength = clearMsgLength + C_K_ICPP_PARSER__HEADER_SIZE;

    if (E_K_STATUS_OK != status)
    {
      // REQ RQ_M-KTA-NOOP-FN-0020(1) :
      /* Prepare error in case of decryption. */
      // REQ RQ_M-KTA-OBJM-FN-0040(1) :
      /* Prepare error in case of decryption or remove padding error. */
      // REQ RQ_M-KTA-OBJM-FN-0240(1) :
      /* Prepare error in case of decryption or remove padding error. */
      // REQ RQ_M-KTA-OBJM-FN-0540(1) :
      /* Prepare error in case of decryption or remove padding error. */
      // REQ RQ_M-KTA-OBJM-FN-0740(1) :
      /* Prepare error in case of decryption or remove padding error. */
      // REQ RQ_M-KTA-OBJM-FN-0940(2) :
      /* Prepare error in case of decryption or remove padding error. */
      // REQ RQ_M-KTA-TRDP-FN-0040(1) :
      /* Prepare error in case of decryption or remove padding error. */
      status = lBuildProcessingStatusRespMsg(xpKs2ktaMsg,
                                              xKs2ktaMsgLen,
                                              C_MSG_AUTH_DEC_ERROR,
                                              xpKta2ksMsg,
                                              xpKta2ksMsgLen);
      M_KTALOG__ERR("Processing status RespMsg failed, status = [%d]", status);
      goto end;
    }

    /* Remove the padding. */
    // REQ RQ_M-KTA-ACTV-FN-0061(1) : Remove data padding
    // REQ RQ_M-KTA-OBJM-FN-0030(1) : Remove data padding
    // REQ RQ_M-KTA-OBJM-FN-0230(1) : Remove data padding
    // REQ RQ_M-KTA-OBJM-FN-0530(1) : Remove data padding
    // REQ RQ_M-KTA-OBJM-FN-0730(1) : Remove data padding
    // REQ RQ_M-KTA-OBJM-FN-0930(2) : Remove data padding
    // REQ RQ_M-KTA-TRDP-FN-0030(1) : Remove data padding
    status = ktacipherRemovePadding(xpClearMsg, &clearMsgLength);

    if (E_K_STATUS_OK != status)
    {
      // REQ RQ_M-KTA-ACTV-FN-0065(1) : Prepare error in case of decryption or remove padding
      // error.
      // REQ RQ_M-KTA-OBJM-FN-0040(1) : Prepare error in case of decryption or remove padding
      // error.
      // REQ RQ_M-KTA-OBJM-FN-0240(1) : Prepare error in case of decryption or remove padding
      // error.
      // REQ RQ_M-KTA-OBJM-FN-0540(1) : Prepare error in case of decryption or remove padding
      // error.
      // REQ RQ_M-KTA-OBJM-FN-0740(1) : Prepare error in case of decryption or remove padding
      // error.
      // REQ RQ_M-KTA-TRDP-FN-0040(1) : Prepare error in case of decryption or remove padding
      // error.
      status = lBuildProcessingStatusRespMsg(xpKs2ktaMsg,
                                              xKs2ktaMsgLen,
                                              C_MSG_AUTH_DEC_ERROR,
                                              xpKta2ksMsg,
                                              xpKta2ksMsgLen);
      M_KTALOG__ERR("Processing status RespMsg failed, status = [%d]", status);
      goto end;
    }

    lenWithoutMACandPadding = xKs2ktaMsgLen - clearMsgLength;
    (void)ktaIcppParserUpdateHeaderLength(aKs2ktaMsgUpdatedHeaderLenBuffer, lenWithoutMACandPadding);
    (void)memcpy(xpClearMsg, aKs2ktaMsgUpdatedHeaderLenBuffer, C_K_ICPP_PARSER__HEADER_SIZE);
  }
  else
  {
    lenWithoutMACandPadding = C_K_KTA__HMAC_MAX_SIZE;
    (void)ktaIcppParserUpdateHeaderLength(aKs2ktaMsgUpdatedHeaderLenBuffer, lenWithoutMACandPadding);
    (void)memcpy(xpClearMsg, aKs2ktaMsgUpdatedHeaderLenBuffer, xKs2ktaMsgLen);
    clearMsgLength = macOffset;
  }

  /* Send the info to deserialize message. */
  // REQ RQ_M-KTA-ACTV-FN-0070(1) : Deserialize the decrypted activation response data.
  // REQ RQ_M-KTA-ICPP-FN-0180(1) : Deserialize the message
  // REQ RQ_M-KTA-OBJM-FN-0050(1) :
  /* Deserialize the decrypted generate key pair data. */
  // REQ RQ_M-KTA-OBJM-FN-0250(1) :
  /* Deserialize the decrypted set object data. */
  // REQ RQ_M-KTA-OBJM-FN-0550(1) :
  /* Deserialize the decrypted delete object data. */
  // REQ RQ_M-KTA-OBJM-FN-0750(1) :
  /* Deserialize the decrypted set object with association data. */
  // REQ RQ_M-KTA-OBJM-FN-0950(2) :
  /* Deserialize the decrypted key object data */
  // REQ RQ_M-KTA-TRDP-FN-0050(1) :
  /* Deserialize the decrypted Third party data. */
  *xpParserStatus = ktaIcppParserDeserializeMessage(xpClearMsg,
                                                    clearMsgLength,
                                                    xpRecvdProtoMessage);

  switch (*xpParserStatus)
  {
    // REQ RQ_M-KTA-STRT-FN-0270(1) : Process the NoOP Message received after
    // ThirdParty/Object Commands.
    case E_K_ICPP_PARSER_STATUS_NO_OPERATION:
    {
      // REQ RQ_M-KTA-LCST-FN-0010(1) : Life Cycle State Size
      size_t  lifeCycleStateLen = C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE;

      if (xpRecvdProtoMessage->cryptoVersion != (unsigned int)E_K_ICPP_PARSER_CRYPTO_TYPE_L2_BASED)
      {
        M_KTALOG__ERR("Invalid Cryto Version [%d]", xpRecvdProtoMessage->cryptoVersion);
        status = E_K_STATUS_ERROR;
        break;
      }

      M_KTALOG__DEBUG("Received NoOp, reading life cycle state from NVM...");
      status = lgetNVMLifeCycleState(&lifeCycleStateLen);

      if ((E_K_STATUS_OK != status) || (0U == lifeCycleStateLen))
      {
        M_KTALOG__ERR("Reading life cycle status from NVM failed, status = [%d]", status);
        break;
      }

      if (E_LIFE_CYCLE_STATE_INIT == gKtaLifeCycleState)
      {
        M_KTALOG__ERR("Device received refurbish command");
        gCommandStatus = E_K_KTA_KS_STATUS_REFURBISH;
        /* Reset the globals to inital value after refurbish. */
        gKtaState = E_KTA_STATE_INITIAL;
        gKtaIsPreActivated = 0;
        M_KTALOG__DEBUG("Life cycle state reached to SEALED state, "
                        "storing in persistent memory");
        status = salStorageSetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                                    gaKtaLifeCycleNVMVData[E_LIFE_CYCLE_STATE_SEALED],
                                    C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE);

        if (E_K_STATUS_OK != status)
        {
          M_KTALOG__ERR("Storing life cycle state failed, status = [%d]", status);
          *xpKta2ksMsgLen = 0;
          break;
        }

        gKtaLifeCycleState = E_LIFE_CYCLE_STATE_SEALED;
        M_KTALOG__INFO("Setting KTA Lifecycle state to SEALED, state = [%d]", gKtaLifeCycleState);
      }
      else if (E_LIFE_CYCLE_STATE_CON_REQ == gKtaLifeCycleState)
      {
        /* Setting the state to connection request. */
        M_KTALOG__DEBUG("Life cycle state reached to PROVISIONED state, "
                        "storing in persistent memory");
        // REQ RQ_M-KTA-LCST-FN-0035(1) : Power off in SEALED|INITIALIZED state
        // REQ RQ_M-KTA-LCST-FN-0040(1) : Power off in SEALED|STARTED state
        // REQ RQ_M-KTA-LCST-FN-0030(1) : Power off in SEALED|RUNNING state
        // REQ RQ_M-KTA-LCST-FN-0075(1) : Power off in CON_REQ|RUNNING state
        status = salStorageSetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                                    gaKtaLifeCycleNVMVData[E_LIFE_CYCLE_STATE_PROVISIONED],
                                    C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE);

        if (E_K_STATUS_OK != status)
        {
          M_KTALOG__ERR("Storing life cycle state failed, status = [%d]", status);
          *xpKta2ksMsgLen = 0;
          break;
        }

        gKtaLifeCycleState = E_LIFE_CYCLE_STATE_PROVISIONED;
        M_KTALOG__INFO("Setting KTA Lifecycle state to PROVISIONED, state = [%d]", gKtaLifeCycleState);
      }

      /* Break the chain no communication is needed. */
      else if (gKtaLifeCycleState == E_LIFE_CYCLE_STATE_ACTIVATED)
      {
        M_KTALOG__DEBUG("Life cycle state reached to PROVISIONED state, "
                        "storing in persist memory");
        // REQ RQ_M-KTA-LCST-FN-0065(1) : Power off in PROVISIONED|INITIALIZED state
        // REQ RQ_M-KTA-LCST-FN-0070(1) : Power off in PROVISIONED|STARTED state
        // REQ RQ_M-KTA-LCST-FN-0060(1) : Power off in PROVISIONED|RUNNING state
        status = salStorageSetValue(C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID,
                                    gaKtaLifeCycleNVMVData[E_LIFE_CYCLE_STATE_PROVISIONED],
                                    C_KTA_CONFIG__LIFE_CYCLE_EACH_STATE_SIZE);

        if (E_K_STATUS_OK != status)
        {
          M_KTALOG__ERR("Setting life cycle state failed, status = [%d]", status);
          break;
        }

          if (memcmp(gaKtaVersion, aKtaVersion, C_K__VERSION_STORAGE_LENGTH) < 0)
          {
              status = salStorageSetValue(C_K_KTA__VERSION_SLOT_ID,
                                          aKtaVersion,
                                          C_K__VERSION_STORAGE_LENGTH);
              if (E_K_STATUS_OK != status)
              {
                M_KTALOG__ERR("Storing KTA Version Failed, status = [%d]", status);
                break;
              }
              M_KTALOG__DEBUG("KTA Version [%s] Stored Successfully in device.\r\n",
                              ktaGetDecodedVersionStr(aKtaVersion));
          }

          gKtaLifeCycleState = E_LIFE_CYCLE_STATE_PROVISIONED;
          gCommandStatus = E_K_KTA_KS_STATUS_NO_OPERATION;
          M_KTALOG__DEBUG("Lifecycle provised and E_K_KTA_KS_STATUS_NO_OPERATION");
        }
        else
        {
          /* Do nothing. */
        }

      status = E_K_STATUS_OK;
    }
    break;

    case E_K_ICPP_PARSER_STATUS_ERROR:
    {
      M_KTALOG__ERR("ICPP Parse Error");
      // REQ RQ_M-KTA-ACTV-FN-0075(1) :
      /* Error in Deserialization. */
      // REQ RQ_M-KTA-OBJM-FN-0060(1) :
      /* Error in Deserialization. */
      // REQ RQ_M-KTA-OBJM-FN-0260(1) :
      /* Error in Deserialization. */
      // REQ RQ_M-KTA-OBJM-FN-0560(1) :
      /* Error in Deserialization. */
      // REQ RQ_M-KTA-OBJM-FN-0760(1) :
      /* Error in Deserialization. */
      // REQ RQ_M-KTA-OBJM-FN-0960(2) :
      /* Error in Deserialization */
      // REQ RQ_M-KTA-TRDP-FN-0060(1) :
      /* Error in Deserialization. */
      status = lBuildProcessingStatusRespMsg(xpKs2ktaMsg,
                                              xKs2ktaMsgLen,
                                              C_MSG_FORMAT_ERROR,
                                              xpKta2ksMsg,
                                              xpKta2ksMsgLen);
    }
    break;

    case E_K_ICPP_PARSER_STATUS_NOTIFICATION_CPERROR:
    {
      M_KTALOG__ERR("Received E_K_ICPP_PARSER_STATUS_NOTIFICATION_CPERROR from the parser");
      status = E_K_STATUS_ERROR;
    }
    break;

    case E_K_ICPP_PARSER_STATUS_OK:
    {
      M_KTALOG__DEBUG("Received E_K_ICPP_PARSER_STATUS_OK");

      if (E_LIFE_CYCLE_STATE_CON_REQ == gKtaLifeCycleState)
      {
        gCommandStatus = E_K_KTA_KS_STATUS_RENEW;
      }

      status = E_K_STATUS_OK;
    }
    break;

    default:
    {
      M_KTALOG__ERR("Received invalid status from the parser[%d]", *xpParserStatus);
    }
    break;
  }

end:
  return status;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

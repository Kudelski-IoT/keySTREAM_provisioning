/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2024 Nagravision SÃ rl

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
/** \brief  keySTREAM Trusted Agent - Hook for keySTREAM Trusted Agent
 *          initialization and field management.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file ktaFieldMgntHook.c
 ******************************************************************************/
/**
 * @brief   keySTREAM Trusted Agent - Hook for initialization and field management.
 */

#include "ktaFieldMgntHook.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_kta.h"
#include "ktaConfig.h"
#include "comm_if.h"
#include "cryptoConfig.h"

#include <stdbool.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */
/** @brief Device Serial Number */
#define C_KTA_APP_DEVICE_SERIAL_NUM    {0x22, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09}

/** @brief Context profile UID */
#define C_KTA_APP_CONTEXT_PROFILE_UID  {0x11, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a}

/** @brief Serial No */
#define C_KTA_APP_CONTEXT_SERIAL_NUM   {0x11, 0x22, 0x33, 0x04, 0x05, 0x06, 0x07, 0x08}

/** @brief Context Version */
#define C_KTA_APP_CONTEXT_VERSION      {0x22, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05}

/** @brief Boolean True */
#define L_POLL_TRUE 1

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/** @brief  Connection required by KTA status */
static uint8_t gConnectionReq = 0u;

/** @brief  KTA Initialized state */
static uint8_t gKtaInitialized = 0u;
uint8_t         gaSegSeed[C_K__L1_SEGMENTATION_SEED_SIZE] = C_KTA_APP__L1_SEG_SEED;
uint8_t*        gpDeviceProfPubUid                        = (uint8_t*)C_KTA_APP__DEVICE_PUBLIC_UID;
const uint8_t*  gpHost                                    = C_K_COMM__SERVER_HOST;

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   Sets keySTREAM Trusted Agent context configuration data to the platform.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input values.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lsetStartupInfo
(
  void
);

/**
 * @brief
 *   Set the device specific info.
 *
 * @param[in,out] xpConnectionReq
 *   [in] Connection required status pointer.
 *   [out] Updated connection status value.
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input values.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lsetDeviceInfo
(
  uint8_t*  xpConnectionReq
);

/**
 * @brief
 *   Initialize communication stack.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input values.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lcommInit
(
  void
);

/**
 * @brief
 *   Poll for keySTREAM updates.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input values.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lPollKeyStream
(
  void
);

/**
 * @brief
 *   Function to print buffer.
 *
 * @param[in]  xpBuff
 *   Buffer to print data.
 * @param[in]   xLen
 *   Length of the buffer to print data.
 */
static void lprintData
(
  uint8_t*  xpBuff,
  size_t    xLen
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */
/**
 * @brief  implement ktaKeyStreamInit
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaKeyStreamInit
(
  void
)
{
  TKStatus  retStatus = E_K_STATUS_ERROR;

  if (gKtaInitialized == 0u)
  {
    C_KTA_APP__LOG("[INFO] ktaInitialize\r\n");
    retStatus = ktaInitialize();

    if (E_K_STATUS_OK != retStatus)
    {
      C_KTA_APP__LOG("[ERROR] ktaInitialize Status[%d]\r\n", retStatus);
      goto end;
    }

    C_KTA_APP__LOG("[INFO] ktaInitialize Succeeded\r\n");

    retStatus = lsetStartupInfo();

    if (E_K_STATUS_OK != retStatus)
    {
      C_KTA_APP__LOG("[ERROR] ktaStartup Failed Status[%d]\r\n", retStatus);
      goto end;
    }

    C_KTA_APP__LOG("[INFO] ktaStartup Succeeded\r\n");

    retStatus = lsetDeviceInfo(&gConnectionReq);

    if (E_K_STATUS_OK != retStatus)
    {
      C_KTA_APP__LOG("ktaSetDeviceInformation Failed Status[%d]\r\n", retStatus);
      goto end;
    }

    C_KTA_APP__LOG("[INFO] ktaSetDeviceInformation Succeeded\r\n");
    gKtaInitialized = 1;
  }

  retStatus = E_K_STATUS_OK;
  goto end;

end:
  return retStatus;
}

/**
 * @brief  implement ktaKeyStreamFieldMgmt
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaKeyStreamFieldMgmt
(
  bool                   xIsFieldMgmtReq,
  TKktaKeyStreamStatus*  xpKtaKSCmdStatus
)
{
  TKStatus  retStatus = E_K_STATUS_ERROR;

  C_KTA_APP__LOG("[INFO] ktaKeyStreamFieldMgmt Start\r\n");

  if (xpKtaKSCmdStatus == NULL)
  {
    C_KTA_APP__LOG("[ERROR] Invalid parameter\r\n");
    retStatus = E_K_STATUS_PARAMETER;
    goto end;
  }

  if ((gConnectionReq == 1U) || (xIsFieldMgmtReq == true))
  {
    retStatus = lPollKeyStream();

    if (E_K_STATUS_OK != retStatus)
    {
      C_KTA_APP__LOG("[ERROR] lPollKeyStream failed %d\r\n", retStatus);
      goto end;
    }
  }
  else
  {
    C_KTA_APP__LOG("[INFO] KTA is in provisioned state, exit...\r\n");
  }

  retStatus = ktaKeyStreamStatus(xpKtaKSCmdStatus);

  if (E_K_STATUS_OK != retStatus)
  {
    C_KTA_APP__LOG("[ERROR] ktaKeyStreamStatus %d\r\n", retStatus);
    goto end;
  }

  if (E_K_KTA_KS_STATUS_REFURBISH == *xpKtaKSCmdStatus)
  {
    C_KTA_APP__LOG("[INFO] Device refurbished, reboot the device...\r\n");
    gKtaInitialized = 0;
  }

  goto end;

end:
  C_KTA_APP__LOG("[KTA] Done.\r\n");

  return retStatus;
}

/**
 * @brief  implement ktaKeyStreamUpdateConfig
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaKeyStreamUpdateConfig(uint8_t *xpSegSeed, uint8_t* xpKsUrl, uint8_t *xpDevProfileUid)
{
  TKStatus  retStatus = E_K_STATUS_ERROR;

  C_KTA_APP__LOG("[INFO] ktaKeyStreamUpdateConfig Start\r\n");

  if ((xpSegSeed == NULL) ||
      (xpKsUrl == NULL) ||
      (xpDevProfileUid == NULL))
  {
    C_KTA_APP__LOG("[ERROR] Invalid parameter\r\n");
    retStatus = E_K_STATUS_PARAMETER;
    goto end;
  }

  /* Update the global variables. */
  memcpy(gaSegSeed, xpSegSeed, C_K__L1_SEGMENTATION_SEED_SIZE);
  gpDeviceProfPubUid = xpDevProfileUid;
  gpHost = xpKsUrl;

  retStatus = E_K_STATUS_OK;
  goto end;
end:
  C_KTA_APP__LOG("[INFO] ktaKeyStreamUpdateConfig end\r\n");

  return retStatus;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */
/**
 * @implements lsetStartupInfo
 *
 */
static TKStatus lsetStartupInfo
(
  void
)
{
  uint8_t   aContextProfUid[] = C_KTA_APP_CONTEXT_PROFILE_UID;
  size_t    contextProfUidLen = sizeof(aContextProfUid);
  uint8_t   aContextSerNum[] = C_KTA_APP_CONTEXT_SERIAL_NUM;
  size_t    contextSerNumLen = sizeof(aContextSerNum);
  uint8_t   aContextVer[] = C_KTA_APP_CONTEXT_VERSION;
  size_t    contextVerLen = sizeof(aContextVer);

  /* Invoking ktaStartup by providing all valid input data */
  C_KTA_APP__LOG("[INFO] Calling ktaStartup\r\n");
  return ktaStartup(gaSegSeed,
                    aContextProfUid,
                    contextProfUidLen,
                    aContextSerNum,
                    contextSerNumLen,
                    aContextVer,
                    contextVerLen);
}

/**
 * @implements lsetDeviceInfo
 *
 */
static TKStatus lsetDeviceInfo
(
  uint8_t*  xpConnectionReq
)
{
  size_t    deviceProfPubUidLen = strlen((char*)gpDeviceProfPubUid);
  uint8_t   aSerialNum[] = C_KTA_APP_DEVICE_SERIAL_NUM;
  size_t    serialNumLen = sizeof(aSerialNum);
  TKStatus  retStatus = E_K_STATUS_ERROR;

  C_KTA_APP__LOG("[INFO] Calling ktaSetDeviceInformation\r\n");
  /* Set device information(Device public uid) */
  retStatus = ktaSetDeviceInformation(gpDeviceProfPubUid,
                                      deviceProfPubUidLen,
                                      aSerialNum,
                                      serialNumLen,
                                      xpConnectionReq);

  return retStatus;
}

/**
 * @implements lcommInit
 *
 */
static TKStatus lcommInit
(
  void
)
{
  uint16_t        port = C_K_COMM__SERVER_PORT;
  const uint8_t*  pUri = (const uint8_t*)C_K_COMM__SERVER_URI;
  TCommIfStatus   retStatus = E_COMM_IF_STATUS_ERROR;

  C_KTA_APP__LOG("[INFO] commInit aHost[%s] port[%d] uri[%s]\r\n", gpHost, port, pUri);
  retStatus = commInit(gpHost, port, pUri);

  return (TKStatus)retStatus;
}

/**
 * @implements lPollKeyStream
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
static TKStatus lPollKeyStream
(
  void
)
{
  static uint8_t  aRot2KsMsg[C_K__ICPP_MSG_MAX_SIZE] = { 0 };
  TKStatus        retStatus = E_K_STATUS_ERROR;
#ifdef NETWORK_STACK_AVAILABLE
  TCommIfStatus   commStatus = E_COMM_IF_STATUS_ERROR;
#endif
  uint8_t*        pKs2RotMsg = aRot2KsMsg;
  size_t          rot2ksMsgSize = 0;
  size_t          ks2rotMsgSize = 0;

  /* Initialize the communication stack to communicate with the keySTREAM. */
  retStatus = lcommInit();

  if (retStatus != E_K_STATUS_OK)
  {
    C_KTA_APP__LOG("ERROR: lcommInit failed, status:%d\r\n", retStatus);
    return retStatus;
  }

  while(L_POLL_TRUE)
  {
    C_KTA_APP__LOG("[INFO] Calling ktaExchangeMessage...\r\n");

    rot2ksMsgSize = C_K__ICPP_MSG_MAX_SIZE;
    retStatus = ktaExchangeMessage(pKs2RotMsg, ks2rotMsgSize, aRot2KsMsg, &rot2ksMsgSize);

    if (E_K_STATUS_OK != retStatus)
    {
      C_KTA_APP__LOG("[ERROR] ktaExchangeMessage Failed Status[%d]", retStatus);
      goto end;
    }

    if (0U == rot2ksMsgSize)
    {
      C_KTA_APP__LOG("[INFO] There is no message to send to keySTREAM.\r\n");
      goto end;
    }

    C_KTA_APP__LOG("[INFO] KTA msg generated successfully.\r\n");
    C_KTA_APP__LOG("[INFO] Sending [%d] bytes to KS\r\n", rot2ksMsgSize);
    lprintData(aRot2KsMsg, rot2ksMsgSize);

/**********************************************************************************/
/*                  Below code requires network stack integration                 */
/**********************************************************************************/
#ifdef NETWORK_STACK_AVAILABLE
    ks2rotMsgSize = C_K__ICPP_MSG_MAX_SIZE;
    commStatus = commMsgExchange(aRot2KsMsg, rot2ksMsgSize, pKs2RotMsg, &ks2rotMsgSize);

    if (commStatus == E_COMM_IF_STATUS_OK)
    {
      C_KTA_APP__LOG("[INFO] KS Resp MSG Length :[%zu]\r\n", ks2rotMsgSize);

      if (ks2rotMsgSize != 0U)
      {
        C_KTA_APP__LOG("[INFO] Received from KS :\r\n");
        lprintData(pKs2RotMsg, ks2rotMsgSize);
      }
      else
      {
        C_KTA_APP__LOG("[ERROR] KS_Resp msg length is = [%zu]\r\n", ks2rotMsgSize);
        retStatus = E_K_STATUS_ERROR;
        goto end;
      }
    }
    else
    {
      C_KTA_APP__LOG("[FAIL] commMsgExchange failed.\r\n");
      retStatus = E_K_STATUS_ERROR;
      goto end;
    }
#else
    break; // To be removed when network stack is integrated.
#endif
  }
end:
  if (E_COMM_IF_STATUS_OK != commTerm())
  {
    C_KTA_APP__LOG("[FAIL] Communication Stack Termination failed \r\n");
    retStatus = E_K_STATUS_ERROR;
  }
  return retStatus;
}

/**
 * @implements lprintData
 *
 */
static void lprintData
(
  uint8_t*  xpBuff,
  size_t    xLen
)
{
  size_t  i = 0;

  for (; i < xLen; i++)
  {
    C_KTA_APP__LOG("%02x", (int)xpBuff[i]);
  }

  C_KTA_APP__LOG("\r\n");
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

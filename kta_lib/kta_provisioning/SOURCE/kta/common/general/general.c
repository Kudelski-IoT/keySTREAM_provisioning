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
/** \brief keySTREAM Trusted Agent - General module to perform coding of data.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file general.c
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - General module to perform coding of data.
 */

#include "general.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_defs.h"
#include "icpp_parser.h"
#include "k_crypto.h"
#include "k_kta.h"
#include "k_sal.h"
#include "k_sal_crypto.h"
#include "cryptoConfig.h"
#include "KTALog.h"

#include <string.h>
/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
static const char* gpModuleName = "KTAGENERAL";


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
 * @brief implement ktaGenerateResponse
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
TKStatus ktaGenerateResponse
(
  uint8_t                      xTotalCodedSteps,
  const TKIcppProtocolMessage* xpSendProtoMessage,
  uint8_t*                     xpMessageToSend,
  size_t*                      xpMessageToSendSize
)
{
  TKStatus status = E_K_STATUS_ERROR;
  uint8_t  totSteps;
  uint8_t  aComputedMac[C_K_KTA__HMAC_MAX_SIZE] = {0};
  /* Not initialised these variable to 0 as bitmap step is coherent */
  size_t   serializedMsgLen = C_K__ICPP_MSG_MAX_SIZE;
  size_t   serializedPaddedMsgLen = C_K__ICPP_MSG_MAX_SIZE;
  size_t   encMsgLen = C_K__ICPP_MSG_MAX_SIZE;

  M_KTALOG__START("Start");
  if ((*xpMessageToSendSize < 0u) || (*xpMessageToSendSize > C_K__ICPP_MSG_MAX_SIZE))
  {
    M_KTALOG__ERR("Invalid xpMessageToSendSize : %d", *xpMessageToSendSize);
  }
  else
  {
    /* This check ensures that the Bitmap Step is Coherent */
    if (!((C_GEN__SERIALIZE == (xTotalCodedSteps & C_GEN__SERIALIZE)) &&
        (C_GEN__PADDING == (xTotalCodedSteps & C_GEN__PADDING)) &&
        (C_GEN__ENCRYPT == (xTotalCodedSteps & C_GEN__ENCRYPT)) &&
        (C_GEN__SIGNING == (xTotalCodedSteps & C_GEN__SIGNING))))
    {
      M_KTALOG__ERR("Invalid Bitmap");
    }
    else
    {
      totSteps = xTotalCodedSteps & C_GEN__SERIALIZE;

      if (C_GEN__SERIALIZE == totSteps)
      {
        /* Serialize the message. */
        // REQ RQ_M-KTA-REGT-FN-0030(1) : Serialize Registeration Info Message
        // REQ RQ_M-KTA-OBJM-FN-0110(1) : Serialize Generate key pair Message
        // REQ RQ_M-KTA-OBJM-FN-0310(1) : Serialize Set Object Message
        // REQ RQ_M-KTA-OBJM-FN-0610(1) : Serialize delete object Message
        // REQ RQ_M-KTA-OBJM-FN-0810(1) : Serialize Set Object With Association Message
        // REQ RQ_M-KTA-OBJM-FN-1010(2) : Serialize delete key object Message
        // REQ RQ_M-KTA-TRDP-FN-0120(1) : Serialize Third party response Message
        if (E_K_ICPP_PARSER_STATUS_OK != ktaIcppParserSerializeMessage(xpSendProtoMessage,
            xpMessageToSend,
            &serializedMsgLen))
        {
          status = E_K_STATUS_ERROR;
          M_KTALOG__ERR("ICCP parser serialization of message got failed");
          goto end;
        }
      }

      totSteps = xTotalCodedSteps & C_GEN__PADDING;

      if (C_GEN__PADDING == totSteps)
      {
        /* Add required padding bytes before encryption. */
        // REQ RQ_M-KTA-REGT-FN-0040(1) : Pad Registeration Info Serialized Message
        // REQ RQ_M-KTA-OBJM-FN-0120(1) : Pad Generate key pair Serialized Message
        // REQ RQ_M-KTA-OBJM-FN-0320(1) : Pad Set Object Serialized Message
        // REQ RQ_M-KTA-OBJM-FN-0620(1) : Pad delete object Serialized Message
        // REQ RQ_M-KTA-OBJM-FN-0820(1) : Pad Set Object With Association Serialized Message
        // REQ RQ_M-KTA-OBJM-FN-1020(2) : Pad delete key object Serialized Message
        // REQ RQ_M-KTA-TRDP-FN-0130(1) : Pad Third party response Serialized Message
        if (serializedMsgLen < C_K_ICPP_PARSER__HEADER_SIZE)
        {
          status = E_K_STATUS_ERROR;
          M_KTALOG__ERR("Invalid serializedMsgLen : %d", serializedMsgLen);
          goto end;
        }
        status = ktacipherAddPadding(&xpMessageToSend[C_K_ICPP_PARSER__HEADER_SIZE],
                                    serializedMsgLen - C_K_ICPP_PARSER__HEADER_SIZE,
                                    &xpMessageToSend[C_K_ICPP_PARSER__HEADER_SIZE],
                                    &serializedPaddedMsgLen);

        if (E_K_STATUS_OK != status)
        {
          M_KTALOG__ERR("Kta cipher padding got failed, status = [%d]", status);
          goto end;
        }
      }

      totSteps = xTotalCodedSteps & C_GEN__ENCRYPT;

      if (C_GEN__ENCRYPT == totSteps)
      {
        /* Encrypt data. */
        // REQ RQ_M-KTA-REGT-FN-0050(1) : Encrypt the padded registeration info message
        // REQ RQ_M-KTA-OBJM-FN-0130(1) : Encrypt the padded Generate key pair message
        // REQ RQ_M-KTA-OBJM-FN-0330(1) : Encrypt the padded set object message
        // REQ RQ_M-KTA-OBJM-FN-0630(1) : Encrypt the padded delete object message
        // REQ RQ_M-KTA-OBJM-FN-0830(1) : Encrypt the padded set object with association message
        // REQ RQ_M-KTA-OBJM-FN-1030(2) : Encrypt the padded delete key object message
        // REQ RQ_M-KTA-TRDP-FN-0140(1) : Encrypt the padded Third party response message
        status = ktacipherEncrypt(&xpMessageToSend[C_K_ICPP_PARSER__HEADER_SIZE],
                                  serializedPaddedMsgLen,
                                  &xpMessageToSend[C_K_ICPP_PARSER__HEADER_SIZE],
                                  &encMsgLen);

        if (E_K_STATUS_OK != status)
        {
          M_KTALOG__ERR("Kta cipher encryption of message failed, status = [%d]", status);
          goto end;
        }
        if (E_K_ICPP_PARSER_STATUS_OK !=
            ktaIcppParserSetHeaderLength(xpMessageToSend, encMsgLen + C_K_KTA__HMAC_MAX_SIZE))
        {
          M_KTALOG__ERR("Set header length failed");
          status = E_K_STATUS_ERROR;
          goto end;
        }
      }

      totSteps = xTotalCodedSteps & C_GEN__SIGNING;

      if (C_GEN__SIGNING == totSteps)
      {
        /* Calculate signature of data. */
        // REQ RQ_M-KTA-REGT-FN-0060(1) : Sign the encrypted registeration info message
        // REQ RQ_M-KTA-OBJM-FN-0140(1) : Sign the encrypted Generate key pair message
        // REQ RQ_M-KTA-OBJM-FN-0340(1) : Sign the encrypted set object message
        // REQ RQ_M-KTA-OBJM-FN-0640(1) : Sign the encrypted delete object message
        // REQ RQ_M-KTA-OBJM-FN-0840(1) : Sign the encrypted set object with association message
        // REQ RQ_M-KTA-OBJM-FN-1040(2) : Sign the encrypted delete key object message
        // REQ RQ_M-KTA-TRDP-FN-0150(1) : Sign the encrypted Third party response message
        status = ktacipherSignMsg(xpMessageToSend,
                                  encMsgLen + C_K_ICPP_PARSER__HEADER_SIZE,
                                  aComputedMac);

        if (E_K_STATUS_OK != status)
        {
          M_KTALOG__ERR("Kta cipher signing of message failed, status = [%d]", status);
          goto end;
        }

        (void)memcpy(&xpMessageToSend[encMsgLen + C_K_ICPP_PARSER__HEADER_SIZE],
                    aComputedMac,
                    C_K_KTA__HMAC_MAX_SIZE);
        *xpMessageToSendSize = encMsgLen + C_K_ICPP_PARSER__HEADER_SIZE + C_K_KTA__HMAC_MAX_SIZE;
      }

      status = E_K_STATUS_OK;
    }
  }

end:
  M_KTALOG__END("End, status : %d", status);
  return status;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

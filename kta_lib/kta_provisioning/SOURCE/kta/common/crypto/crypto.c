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
/** \brief keySTREAM Trusted Agent - Cipher module for cryptographic operations.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file crypto.c
 ******************************************************************************/

/**
 * @brief keySTREAM Trusted Agent - Cipher module for cryptographic operations.
 */

#include "k_crypto.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal.h"
#include "k_sal_crypto.h"
#include "KTALog.h"

#include <string.h>

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
#if LOG_KTA_ENABLE
static const char* gpModuleName = "KTACRYPTO";
#endif

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
 * @brief implement ktacipherEncrypt
 *
 */
TKStatus ktacipherEncrypt
(
  const uint8_t* xpClearMsg,
  size_t         xClearMsgLen,
  uint8_t*       xpEncryptedMsg,
  size_t*        xpEncryptedMsgLen
)
{
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  for (;;)
  {
    if ((NULL == xpClearMsg) || (0u == xClearMsgLen) || (NULL == xpEncryptedMsg) ||
        (NULL == xpEncryptedMsgLen) || (0u == *xpEncryptedMsgLen))
    {
      status = E_K_STATUS_PARAMETER;
      M_KTALOG__ERR("Invalid parameter passed");
      break;
    }

    status = salCryptoAesEnc(C_K_KTA__VOLATILE_3_ID,
                             xpClearMsg,
                             xClearMsgLen,
                             xpEncryptedMsg,
                             xpEncryptedMsgLen);
    break;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktacipherSignMsg
 *
 */
TKStatus ktacipherSignMsg
(
  const uint8_t* xpMsgToSign,
  size_t         xMsgToSignLen,
  uint8_t*       xpComputedMac
)
{
  TKStatus status  = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  for (;;)
  {
    if ((NULL == xpMsgToSign) || (0u == xMsgToSignLen) || (NULL == xpComputedMac))
    {
      status = E_K_STATUS_PARAMETER;
      M_KTALOG__ERR("Invalid parameter passed");
      break;
    }

    status  = salCryptoHmac(C_K_KTA__VOLATILE_2_ID, xpMsgToSign, xMsgToSignLen, xpComputedMac);
    break;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktacipherAddPadding
 *
 */
TKStatus ktacipherAddPadding
(
  const uint8_t* xpData,
  const size_t   xDataLength,
  uint8_t*       xpPaddedData,
  size_t*        xpPaddedDataLength
)
{
  size_t paddingBytesAmount = 0u;
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  for (;;)
  {
    if ((NULL == xpData) || (0u == xDataLength) || (NULL ==  xpPaddedData) ||
        (NULL ==  xpPaddedDataLength) || (0u == *xpPaddedDataLength))
    {
      status = E_K_STATUS_PARAMETER;
      M_KTALOG__ERR("Invalid parameter passed");
      break;
    }

    /* Number of bytes to be appended. */
    paddingBytesAmount = C_K_CRYPTO__AES_BLOCK_SIZE  - (xDataLength % C_K_CRYPTO__AES_BLOCK_SIZE);

    if ((xDataLength + paddingBytesAmount) > *xpPaddedDataLength)
    {
      M_KTALOG__ERR("(xDataLength + paddingBytesAmount) > *xpPaddedDataLength");
      break;
    }

    /* Copying the original data for further padding. */
    (void)memmove(xpPaddedData, xpData, xDataLength);
    /* Adding start of padding byte. */
    xpPaddedData[xDataLength] = C_K_CRYPTO__PAD_START_BYTE;
    for (size_t index = 1; index < paddingBytesAmount; index++)
    {
      xpPaddedData[xDataLength + index] = 0x00;
    }

    *xpPaddedDataLength = xDataLength + paddingBytesAmount;
    status = E_K_STATUS_OK;
    break;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktacipherDecrypt
 *
 */
TKStatus ktacipherDecrypt
(
  const uint8_t* xpEncryptedMsg,
  size_t         xEncryptedMsgLen,
  uint8_t*       xpClearMsg,
  size_t*        xpClearMsgLen
)
{
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  for (;;)
  {
    if ((NULL == xpEncryptedMsg) || (0u == xEncryptedMsgLen) || (NULL == xpClearMsg) ||
        (NULL == xpClearMsgLen) || (0u == *xpClearMsgLen))
    {
      status = E_K_STATUS_PARAMETER;
      M_KTALOG__ERR("Invalid parameter passed");
      break;
    }

    status = salCryptoAesDec(C_K_KTA__VOLATILE_3_ID,
                             xpEncryptedMsg,
                             xEncryptedMsgLen,
                             xpClearMsg,
                             xpClearMsgLen);
    break;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktacipherVerifySignedMsg
 *
 */
TKStatus ktacipherVerifySignedMsg
(
  const uint8_t* xpMsgToVerify,
  size_t         xMsgToVerifyLen,
  const uint8_t* xpMacToVerify
)
{
  TKStatus status  = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  for (;;)
  {
    if ((NULL == xpMsgToVerify) || (0u == xMsgToVerifyLen) || (NULL == xpMacToVerify))
    {
      status = E_K_STATUS_PARAMETER;
      M_KTALOG__ERR("Invalid parameter passed");
      break;
    }

    status  = salCryptoHmacVerify(C_K_KTA__VOLATILE_2_ID,
                                  xpMsgToVerify,
                                  xMsgToVerifyLen,
                                  xpMacToVerify);
    break;
  }

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/**
 * @brief implement ktacipherRemovePadding
 *
 */
TKStatus ktacipherRemovePadding
(
  const uint8_t* xpData,
  size_t*        xpDataLength
)
{
  size_t  loop = 0;
  size_t   len = 0;
  TKStatus  status = E_K_STATUS_ERROR;

  M_KTALOG__START("Start");

  for (;;)
  {
    if ((NULL == xpData) || (NULL ==  xpDataLength) ||
        (0 == *xpDataLength) || (C_K_CRYPTO__AES_BLOCK_SIZE > *xpDataLength))
    {
      status = E_K_STATUS_PARAMETER;
      M_KTALOG__ERR("Invalid parameter passed");
      break;
    }

    for (len = *xpDataLength - 1; loop < C_K_CRYPTO__AES_BLOCK_SIZE; loop++)
    {
      /* Skipping the bytes from end of the buffer until we hit the C_K_CRYPTO__PAD_START_BYTE. */
      if (C_K_CRYPTO__PAD_START_BYTE == xpData[len])
      {
        *xpDataLength = len;
        status = E_K_STATUS_OK;
        break;
      }
      else if (0u != xpData[len])
      {
        status = E_K_STATUS_ERROR;
        M_KTALOG__ERR("Invalid padding %u => %d", (unsigned int)len, (int8_t)xpData[len]);
        break;
      }
      else /* Added to resolve cppcheck errors. */
      {
        /* Not used. */
      }

      len--;
    }

    break;
  }
  /* Check for xpDataLength is a multiple of C_K_CRYPTO__AES_BLOCK_SIZE
   is already taken care during the Decryption process. */

  M_KTALOG__END("End, status : %d", status);
  return status;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

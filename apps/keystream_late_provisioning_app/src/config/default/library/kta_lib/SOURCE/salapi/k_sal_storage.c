/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2024 Nagravision SÃƒÂ rl

* Subject to your compliance with these terms, you may use the Nagravision SÃƒÂ rl
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
/** \brief  SAL storage for Microchip.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_storage.c
 ******************************************************************************/

/**
 * @brief SAL storage for Microchip.
 */

#include "k_sal_storage.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "atca_basic.h"
#include "slotConfig.h"
#include "config.h"
#include "KTALog.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief Device context profile UID. */
#define C_SAL_CONTEXT_PROFILE_UID                   \
  {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,\
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11}

/** @brief Device context profile UID length. */
#define C_SAL_CONTEXT_PROFILE_UID_LENGTH                (32u)

/** @brief Context serial no. */
#define C_SAL_CONTEXT_SERIAL_NO                     \
  {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22}

/** @brief Context serial no length. */
#define C_SAL_CONTEXT_SERIAL_NO_LENGTH                  (16u)

/** @brief Context version. */
#define C_SAL_CONTEXT_VERSION                        \
  {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33}

/** @brief Context version length. */
#define C_SAL_CONTEXT_VERSION_LENGTH                    (16u)

/** @brief keySTREAM Trusted Agent version. */
#define C_SAL_KTA_VERSION                            \
  {0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44}

/** @brief keySTREAM Trusted Agent version length. */
#define C_SAL_KTA_VERSION_LENGTH                        (16u)

/** @brief keySTREAM Trusted Agent version Storage length. */
#define C_SAL_KTA_VERSION_STORAGE_LENGTH            (2u)

/** @brief Device serial no. */
#define C_SAL_DEVICE_SERIAL_NO                      \
  {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55}

/** @brief Device seial no lenght. */
#define C_SAL_DEVICE_SERIAL_NO_LENGTH                   (16u)

/** @brief L1 seg seed. */
#define C_SAL_L1_SEG_SEED                          \
  {0x9c, 0x52, 0xb8, 0x13, 0x9f, 0xe5, 0xa3, 0xa9, 0xcc, 0xd9, 0x26, 0xe8, 0x41, 0x0b, 0x7a, 0x58}

/** @brief Max bytes can be written at a time to the storage. */
#define C_SAL_MCHP_MAX_DATA_SIZE                        (4u)

/** @brief L1 seg seed length. */
#define C_SAL_L1_SEG_SEED_LENGTH                        (16u)

/** @brief Sealed data id length. */
#define C_SAL_SEALED_DATA_STORAGE_ID_ACTUAL_LENGTH      (133u)

/** @brief Sealed data id length after truncating other data. */
#define C_SAL_SEALED_DATA_STORAGE_ID_SAL_LENGTH         (32u)


/** @brief L1 key material data id length. */
#define C_SAL_L1_KEY_MATERIAL_DATA_ID_ACTUAL_LENGTH     (17u)

/** @brief L1 key material data id length after truncating L1 seg seed. */
#define C_SAL_L1_KEY_MATERIAL_DATA_ID_SAL_LENGTH        (1u)

/** @brief L1 key material ID MCHP storage length. */
#define C_SAL_L1_KEY_MATERIAL_DATA_ID_MCHP_STORAGE_LENGTH \
  ((C_SAL_L1_KEY_MATERIAL_DATA_ID_SAL_LENGTH) + \
   ((C_SAL_MCHP_MAX_DATA_SIZE) - \
    ((C_SAL_L1_KEY_MATERIAL_DATA_ID_SAL_LENGTH) % (C_SAL_MCHP_MAX_DATA_SIZE))))

/** @brief L1 key material ID MCHP storage length. */
#define C_SAL_KTA_VERSION_STORAGE_LENGTH_MCHP_STORAGE_LENGTH \
  ((C_SAL_KTA_VERSION_STORAGE_LENGTH) + \
   ((C_SAL_MCHP_MAX_DATA_SIZE) - \
    ((C_SAL_KTA_VERSION_STORAGE_LENGTH) % (C_SAL_MCHP_MAX_DATA_SIZE))))


/** @brief Life cycle data id length. */
#define C_SAL_LIFE_CYCLE_STATE_STORAGE_ID_LENGTH        (4u)

/** @brief Rot public uid length. */
#define C_SAL_ROT_PUBLIC_UID_STORAGE_ID_LENGTH          (8u)


/** @brief Customer trust anchor metadata length */
#define C_SAL_CUSTOMER_TRUST_ANCHOR_METADATA_LENGTH     (32u)

/** @brief Customer sym key metadata length */
#define C_SAL__CUSTOMER_SYM_KEY_METADATA_LENGTH         (32u)

/** @brief Customer data metadata length */
#define C_SAL__CUSTOMER_DATA_METADATA_LENGTH            (32u)

/** @brief Customer trust anchor object uid length */
#define C_SAL__CUSTOMER_TRUST_ANCHOR_OBJECT_UID_LENGTH  (16u)

/** @brief customer sym key object uid length */
#define C_SAL_CUSTOMER_SYM_KEY_OBJECT_UID_LENGTH        (16u)

/** @brief Customer data length */
#define C_SAL_CUSTOMER_DATA_LENGTH                      (32u)

/** @brief Customer data length */
#define C_SAL_CUSTOMER_TRUST_ANCHOR_DATA_LENGTH         (64u)

/** @brief Customer data object uid length */
#define C_SAL_CUSTOMER_DATA_OBJECT_UID_LENGTH           (16u)

/** @brief L1 field key id length. */
#define C_SAL_L1_FIELD_KEY_ID_LENGTH                    (32u)

/** @brief Device certificate length. */
#define C_SAL_DEVICE_CERTIFICATE_ID_LENGTH              (72u)

/** @brief Signer certificate length. */
#define C_SAL_SIGNER_CERTIFICATE_ID_LENGTH              (72u)

/** @brief Max offset in a block. */
#define C_SAL_MCHP_MAX_OFFSET_IN_BLOCK                  (7u)

/** @brief Index for key ID in key material Data. */
#define C_SAL_KEY_SET_ID_OFFSET                         (16u)

/** @brief Index for Device profile UID in sealed info Data. */
#define C_SAL_DEVICE_PROFILE_UID_OFFSET                 (83u)

/** @brief Max buffer to store at a time. */
#define C_SAL_MAX_BUFFER_SIZE                           (72u)

/** @brief Signer Id data id length. */
#define C_SAL_SIGNER_ID_STORAGE_ID_LENGTH               (4u)

/** @brief Signer Id slot ID. */
#define C_SAL_SIGNER_ID_SLOT_ID                         (0x5000u)

/** @brief ENC KEY LEN */
#define C_SAL_CRYPTO_KEY_SIZE_32_BYTE                   (32u)

/** @brief Rot public uid length. */
#define C_SAL_CUSTOMER_TRUST_ANCHOR_METADATA_STORAGE_ID_LENGTH          (32u)

/** @brief Storage details. */
typedef struct
{
  uint32_t  storageID;
  uint8_t   slot;
  uint8_t   block;
  uint8_t   offset;
  size_t    length;
  /* If the length is not in multiple of 4 padd it. */
  size_t    storageLength;
} TKSalSrecord;

/** @brief Storage record. */
static TKSalSrecord gaSalStorageRecord[] =
{
  /* L1 Filed Key. */
  {
    C_K_KTA__L1_FIELD_KEY_ID, C_KTA__L1_FIELD_KEY_STORAGE_SLOT,
    C_KTA__L1_FIELD_KEY_STORAGE_BLOCK, C_KTA__L1_FIELD_KEY_STORAGE_OFFSET,
    C_SAL_L1_FIELD_KEY_ID_LENGTH, C_SAL_L1_FIELD_KEY_ID_LENGTH
  },
  /* Rot public key. */
  {
    C_K_KTA__ROT_PUBLIC_UID_STORAGE_ID, C_KTA__ROT_PUBLIC_UID_STORAGE_SLOT,
    C_KTA__ROT_PUBLIC_UID_STORAGE_BLOCK, C_KTA__ROT_PUBLIC_UID_STORAGE_OFFSET,
    C_SAL_ROT_PUBLIC_UID_STORAGE_ID_LENGTH, C_SAL_ROT_PUBLIC_UID_STORAGE_ID_LENGTH
  },
  /* LifeCycle State. */
  {
    C_K_KTA__LIFE_CYCLE_STATE_STORAGE_ID, C_KTA__STORAGE_SLOT_LIFE_CYCLE_STATE,
    C_KTA__STORAGE_BLOCK_LIFE_CYCLE_STATE, C_KTA__STORAGE_OFFSET_LIFE_CYCLE_STATE,
    C_SAL_LIFE_CYCLE_STATE_STORAGE_ID_LENGTH, C_SAL_LIFE_CYCLE_STATE_STORAGE_ID_LENGTH
  },
  /* L1 Key Material. */
  {
    C_K_KTA__L1_KEY_MATERIAL_DATA_ID, C_KTA__L1_KEY_DATA_STORAGE_SLOT,
    C_KTA__L1_KEY_DATA_STORAGE_BLOCK, C_KTA__L1_KEY_DATA_STORAGE_OFFSET,
    C_SAL_L1_KEY_MATERIAL_DATA_ID_ACTUAL_LENGTH, C_SAL_L1_KEY_MATERIAL_DATA_ID_MCHP_STORAGE_LENGTH
  },
  /* Device Ceritficate. */
  {
    C_K_KTA__DEVICE_CERTIFICATE_ID, C_KTA__DEVICE_CERT_STORAGE_SLOT,
    C_KTA__DEVICE_CERT_STORAGE_BLOCK, C_KTA__DEVICE_CERTIFICATE_STORAGE_OFFSET,
    C_SAL_DEVICE_CERTIFICATE_ID_LENGTH, C_SAL_DEVICE_CERTIFICATE_ID_LENGTH
  },
  /* Signer Ceritficate. */
  {
    C_K_KTA__SIGNER_CERTIFICATE_ID, C_KTA__SIGNER_CERT_STORAGE_SLOT,
    C_KTA__SIGNER_CERT_STORAGE_BLOCK, C_KTA__SIGNER_CERTIFICATE_STORAGE_OFFSET,
    C_SAL_SIGNER_CERTIFICATE_ID_LENGTH, C_SAL_SIGNER_CERTIFICATE_ID_LENGTH
  },
  /* Signer ID. */
  {
    C_SAL_SIGNER_ID_SLOT_ID, C_KTA__SIGNER_ID_STORAGE_SLOT,
    C_KTA__SIGNER_ID_STORAGE_BLOCK, C_KTA__STORAGE_OFFSET_LIFE_CYCLE_STATE,
    C_SAL_SIGNER_ID_STORAGE_ID_LENGTH, C_SAL_SIGNER_ID_STORAGE_ID_LENGTH
  },
  /* KTA Version. */
  {
    C_K_KTA__VERSION_SLOT_ID, C_KTA__VERSION_STORAGE_SLOT,
    C_KTA__VERSION_STORAGE_BLOCK, C_KTA__VERSION_STORAGE_OFFSET,
    C_SAL_KTA_VERSION_STORAGE_LENGTH, C_SAL_KTA_VERSION_STORAGE_LENGTH_MCHP_STORAGE_LENGTH
  },
  /* Singer Public Key. */
  {
    C_K_KTA__SIGNER_PUB_KEY_ID, C_KTA__SIGNER_PUB_KEY_STORAGE_SLOT,
    C_KTA__SIGNER_PUB_KEY_STORAGE_BLOCK, C_KTA__SIGNER_PUBLIC_KEY_STORAGE_OFFSET,
    C_KTA__SIGNER_PUBLIC_KEY_LENGTH, C_KTA__SIGNER_PUBLIC_KEY_LENGTH
  },
  /* Sealed Data. */
  {
    C_K_KTA__SEALED_DATA_STORAGE_ID, C_KTA__SEALED_DATA_STORAGE_SLOT,
    C_KTA__SEALED_DATA_STORAGE_BLOCK, C_KTA__SEALED_DATA_STORAGE_OFFSET,
    C_SAL_SEALED_DATA_STORAGE_ID_ACTUAL_LENGTH, C_SAL_SEALED_DATA_STORAGE_ID_SAL_LENGTH
  },
  /* Customer trust anchor metadata */
  {
    C_K_KTA__CUSTOMER_TRUST_ANCHOR_METADATA_ID, C_KTA__CUSTOMER_TRUST_ANCHOR_METADATA_STORAGE_SLOT,
    C_KTA__CUSTOMER_TRUST_ANCHOR_METADATA_STORAGE_BLOCK, C_KTA__CUSTOMER_TRUST_ANCHOR_METADATA_STORAGE_OFFSET,
    C_SAL_CUSTOMER_TRUST_ANCHOR_METADATA_STORAGE_ID_LENGTH, C_SAL_CUSTOMER_TRUST_ANCHOR_METADATA_STORAGE_ID_LENGTH
  },
  /* Customer sym key metadata */
  {
    C_K_KTA__CUSTOMER_SYM_KEY_METADATA_ID, C_KTA__CUSTOMER_SYM_KEY_METADATA_STORAGE_SLOT,
    C_KTA__CUSTOMER_SYM_KEY_METADATA_STORAGE_BLOCK, C_KTA__CUSTOMER_SYM_KEY_METADATA_STORAGE_OFFSET,
    C_SAL__CUSTOMER_SYM_KEY_METADATA_LENGTH, C_SAL__CUSTOMER_SYM_KEY_METADATA_LENGTH
  },
  /* Customer data metadata */
  {
    C_K_KTA__CUSTOMER_DATA_METADATA_ID, C_KTA__CUSTOMER_DATA_METADATA_STORAGE_SLOT,
    C_KTA__CUSTOMER_DATA_METADATA_STORAGE_BLOCK, C_KTA__CUSTOMER_DATA_METADATA_STORAGE_OFFSET,
    C_SAL__CUSTOMER_DATA_METADATA_LENGTH, C_SAL__CUSTOMER_DATA_METADATA_LENGTH
  },
  /* Customer trust anchor object uid */
  {
    C_K_KTA__CUSTOMER_TRUST_ANCHOR_OBJECT_UID_ID, C_KTA__CUSTOMER_TRUST_ANCHOR_OBJECT_UID_STORAGE_SLOT,
    C_KTA__CUSTOMER_TRUST_ANCHOR_OBJECT_UID_STORAGE_BLOCK, C_KTA__CUSTOMER_TRUST_ANCHOR_OBJECT_UID_STORAGE_OFFSET,
    C_SAL__CUSTOMER_TRUST_ANCHOR_OBJECT_UID_LENGTH, C_SAL__CUSTOMER_TRUST_ANCHOR_OBJECT_UID_LENGTH
  },
  /* customer sym key object uid */
  {
    C_K_KTA__CUSTOMER_SYM_KEY_OBJECT_UID_ID, C_KTA_CUSTOMER_SYM_KEY_OBJECT_UID_STORAGE_SLOT,
    C_KTA_CUSTOMER_SYM_KEY_OBJECT_UID_STORAGE_BLOCK, C_KTA_CUSTOMER_SYM_KEY_OBJECT_UID_STORAGE_OFFSET,
    C_SAL_CUSTOMER_SYM_KEY_OBJECT_UID_LENGTH, C_SAL_CUSTOMER_SYM_KEY_OBJECT_UID_LENGTH
  },
  /* customer data */
  {
    C_K_KTA__CUSTOMER_DATA_ID, C_KTA_CUSTOMER_DATA_STORAGE_SLOT,
    C_KTA_CUSTOMER_DATA_STORAGE_BLOCK, C_KTA_CUSTOMER_DATA_STORAGE_OFFSET,
    C_SAL_CUSTOMER_DATA_LENGTH, C_SAL_CUSTOMER_DATA_LENGTH
  },
  /* customer data object uid */
  {
    C_K_KTA__CUSTOMER_DATA_OBJECT_UID_ID, C_KTA_CUSTOMER_DATA_OBJECT_UID_STORAGE_SLOT,
    C_KTA_CUSTOMER_DATA_OBJECT_UID_STORAGE_BLOCK, C_KTA_CUSTOMER_DATA_OBJECT_UID_STORAGE_OFFSET,
    C_SAL_CUSTOMER_DATA_OBJECT_UID_LENGTH, C_SAL_CUSTOMER_DATA_OBJECT_UID_LENGTH
  },
  /* Customer Trust Anchor slot 14 data */
  {
    C_K_KTA__CUSTOMER_TRUST_ANCHOR_DATA_ID, C_KTA_CUSTOMER_TRUST_ANCHOR_DATA_STORAGE_SLOT,
    C_KTA_CUSTOMER_TRUST_ANCHOR_DATA_STORAGE_BLOCK, C_KTA_CUSTOMER_TRUST_ANCHOR_DATA_STORAGE_OFFSET,
    C_SAL_CUSTOMER_TRUST_ANCHOR_DATA_LENGTH, C_SAL_CUSTOMER_TRUST_ANCHOR_DATA_LENGTH
  },
  /* Customer Trust Anchor slot 14 data */
  {
    C_K_KTA__ENC_TEMPKEY_ID, C_KTA_ENC_TEMPKEY_STORAGE_SLOT,
    C_KTA_CUSTOMER_TRUST_ANCHOR_DATA_STORAGE_BLOCK, C_KTA_CUSTOMER_TRUST_ANCHOR_DATA_STORAGE_OFFSET,
    C_SAL_CRYPTO_KEY_SIZE_32_BYTE, C_SAL_CRYPTO_KEY_SIZE_32_BYTE
  }
};

/** @brief Operation to perform. */
typedef enum
{
  E_READ,
  E_WRITE
} TOperation;

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
static const char* gpModuleName = "SALSTORAGE";

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Function to operate on storage.
 * @param[in] xLoopindex
 *   Record index.
 * @param[in] xOpt
 *   Operation to perform.
 * @param[in] xpBuff
 *   Buffer to store.
 * @return
 * - E_K_STATUS_OK for success:
 * - E_K_STATUS_PARAMETER for wrong input values.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lstorageOperation
(
  uint32_t    xLoopIndex,
  TOperation  xOpt,
  uint8_t*    xpBuff
);

/**
 * @brief
 *   Search the index for given storage ID.
 * @param[in] xStorageDataId
 *   Key ID for storage.
 * @param[out] xpLoopcount
 *   Index for given storage ID.
 * @return
 * - E_K_STATUS_OK for success:
 * - E_K_STATUS_PARAMETER for wrong input values.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lSearchIndex
(
  uint32_t   xStorageDataId,
  uint32_t*  xpLoopcount
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief  implement salStorageSetAndLockValue
 *
 */
/**
 * SUPPRESS: misra_c2012_rule_15.4_violation
 * SUPPRESS: misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salStorageSetAndLockValue
(
  uint32_t        xStorageDataId,
  const uint8_t*  xpData,
  size_t          xDataLen
)
{
  TKStatus     status = E_K_STATUS_ERROR;
  uint32_t     loopIndex = 0;

  M_KTALOG__START("Start");

  if ((NULL == xpData) || (0U == xDataLen))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("bad parameter", status);
  }
  else
  {
    if (E_K_STATUS_OK != lSearchIndex(xStorageDataId, &loopIndex))
    {
      status = E_K_STATUS_PARAMETER;
      M_KTALOG__ERR("search index", status);
      goto end;
    }

    status = salStorageSetValue(xStorageDataId, xpData, xDataLen);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("set value ", status);
      status = E_K_STATUS_ERROR;
      goto end;
    }
  }

end:
  M_KTALOG__END("End status : %d", status);
  return status;
}

/**
 * @brief  implement salStorageSetValue
 *
 */
/**
 * SUPPRESS: misra_c2012_rule_15.4_violation
 * SUPPRESS: misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salStorageSetValue
(
  uint32_t        xStorageDataId,
  const uint8_t*  xpData,
  size_t          xDataLen
)
{
  TKStatus  status = E_K_STATUS_ERROR;
  uint32_t  loopIndex = 0;
  uint8_t   aTmpbuf[C_SAL_MAX_BUFFER_SIZE] = {0};

  M_KTALOG__START("Start");

  if ((NULL == xpData) ||
      (0U == xDataLen))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("bad parameter", status);
  }
  else
  {
    M_KTALOG__INFO("Set search index for", xStorageDataId);

    if (E_K_STATUS_OK != lSearchIndex(xStorageDataId, &loopIndex))
    {
      status = E_K_STATUS_PARAMETER;
      M_KTALOG__ERR("search Index ", status);
      goto end;
    }

    // Skip length check for specific storage IDs
    if ((xStorageDataId != C_K_KTA__CUSTOMER_TRUST_ANCHOR_METADATA_ID) &&
        (xStorageDataId != C_K_KTA__CUSTOMER_TRUST_ANCHOR_OBJECT_UID_ID) &&
        (xStorageDataId != C_K_KTA__CUSTOMER_SYM_KEY_METADATA_ID) &&
        (xStorageDataId != C_K_KTA__CUSTOMER_SYM_KEY_OBJECT_UID_ID))
    {
      if (gaSalStorageRecord[loopIndex].length != xDataLen)
      {
        status = E_K_STATUS_PARAMETER;
        M_KTALOG__ERR("unexpected length", status);
        goto end;
      }
    }

    if (C_K_KTA__L1_KEY_MATERIAL_DATA_ID == xStorageDataId)
    {
      status = lstorageOperation(loopIndex, E_READ, aTmpbuf);
      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("storage operation ", status);
        status = E_K_STATUS_ERROR;
        goto end;
      }
      (void)memcpy(&aTmpbuf[0],
                   &xpData[C_SAL_KEY_SET_ID_OFFSET],
                   C_SAL_L1_KEY_MATERIAL_DATA_ID_SAL_LENGTH);
    }
    else if (C_K_KTA__VERSION_SLOT_ID == xStorageDataId)
    {
      status = lstorageOperation(loopIndex, E_READ, aTmpbuf);
      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("storage operation ", status);
        status = E_K_STATUS_ERROR;
        goto end;
      }
      (void)memcpy(&aTmpbuf[1], xpData, xDataLen);
    }
    else if (C_K_KTA__SEALED_DATA_STORAGE_ID == xStorageDataId)
    {
      (void)memcpy(&aTmpbuf[0],
                   &xpData[C_SAL_DEVICE_PROFILE_UID_OFFSET],
                   C_SAL_SEALED_DATA_STORAGE_ID_SAL_LENGTH);
    }
    else if (C_K_KTA__CUSTOMER_TRUST_ANCHOR_METADATA_ID == xStorageDataId)
    {
      status = lstorageOperation(loopIndex, E_READ, aTmpbuf);
      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("storage operation ", status);
        status = E_K_STATUS_ERROR;
        goto end;
      }
      (void)memcpy(&aTmpbuf[0], xpData, xDataLen);
    }
    else if (C_K_KTA__CUSTOMER_TRUST_ANCHOR_OBJECT_UID_ID == xStorageDataId)
    {
      status = lstorageOperation(loopIndex, E_READ, aTmpbuf);
      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("storage operation ", status);
        status = E_K_STATUS_ERROR;
        goto end;
      }
      (void)memcpy(&aTmpbuf[0], xpData, xDataLen);
    }
    else if (C_K_KTA__CUSTOMER_SYM_KEY_METADATA_ID == xStorageDataId)
    {
      status = lstorageOperation(loopIndex, E_READ, aTmpbuf);
      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("storage operation ", status);
        status = E_K_STATUS_ERROR;
        goto end;
      }
      (void)memcpy(&aTmpbuf[0], xpData, xDataLen);
    }
    else if (C_K_KTA__CUSTOMER_SYM_KEY_OBJECT_UID_ID == xStorageDataId)
    {
      status = lstorageOperation(loopIndex, E_READ, aTmpbuf);
      if (E_K_STATUS_OK != status)
      {
        M_KTALOG__ERR("storage operation ", status);
        status = E_K_STATUS_ERROR;
        goto end;
      }
      (void)memcpy(&aTmpbuf[0], xpData, xDataLen);
    }
    else
    {
      (void)memcpy(aTmpbuf, xpData, xDataLen);
    }

    status = lstorageOperation(loopIndex, E_WRITE, aTmpbuf);

    if (E_K_STATUS_OK != status)
    {
      M_KTALOG__ERR("storage operation ", status);
      status = E_K_STATUS_ERROR;
      goto end;
    }
  }

end:
  M_KTALOG__END("End status : %d", status);
  return status;
}

/**
 * @brief  implement salStorageGetValue
 *
 */
/**
 * SUPPRESS: misra_c2012_rule_15.4_violation
 * SUPPRESS: misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
K_SAL_API TKStatus salStorageGetValue
(
  uint32_t  xStorageDataId,
  uint8_t*  xpData,
  size_t*   xpDataLen
)
{
  TKStatus  status = E_K_STATUS_ERROR;
  uint8_t   aL1SegSeed[] = C_SAL_L1_SEG_SEED;
  uint8_t   aContextProfileUID[] = C_SAL_CONTEXT_PROFILE_UID;
  uint8_t   aSerialNo[] = C_SAL_CONTEXT_SERIAL_NO;
  uint8_t   aContextVersion[] =  C_SAL_CONTEXT_VERSION;
  uint8_t   aKtaVersion[] = C_SAL_KTA_VERSION;
  uint8_t   aDeviceSerialNo[] = C_SAL_DEVICE_SERIAL_NO;
  uint32_t  loopIndex = 0;
  uint8_t   aTmpbuf[C_SAL_MAX_BUFFER_SIZE] = {0};
  uint8_t   len = 0;

  M_KTALOG__START("Start");

  if ((NULL == xpData) ||
      (NULL == xpDataLen) ||
      (0u == *xpDataLen))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("bad parameter ", status);
  }
  else
  {
    M_KTALOG__INFO("Get search index for", xStorageDataId);

    if (E_K_STATUS_OK != lSearchIndex(xStorageDataId, &loopIndex))
    {
      status = E_K_STATUS_PARAMETER;
      M_KTALOG__ERR("search index ", status);
      goto end;
    }

    // Skip length check for specific storage IDs
    if ((xStorageDataId != C_K_KTA__CUSTOMER_TRUST_ANCHOR_METADATA_ID) &&
        (xStorageDataId != C_K_KTA__CUSTOMER_TRUST_ANCHOR_OBJECT_UID_ID) &&
        (xStorageDataId != C_K_KTA__CUSTOMER_SYM_KEY_METADATA_ID) &&
        (xStorageDataId != C_K_KTA__CUSTOMER_SYM_KEY_OBJECT_UID_ID))
    {
      if (gaSalStorageRecord[loopIndex].length > *xpDataLen)
      {
        status = E_K_STATUS_PARAMETER;
        M_KTALOG__ERR("unexpected length", status);
        goto end;
      }
    }

    status = lstorageOperation(loopIndex, E_READ, aTmpbuf);

    if (E_K_STATUS_OK != status)
    {
      status = E_K_STATUS_PARAMETER;
      M_KTALOG__ERR("storage Operation ", status);
      goto end;
    }

    if (C_K_KTA__L1_KEY_MATERIAL_DATA_ID == xStorageDataId)
    {
      (void)memcpy(xpData, aL1SegSeed, C_SAL_L1_SEG_SEED_LENGTH);
      (void)memcpy(&xpData[C_SAL_L1_SEG_SEED_LENGTH],
                   aTmpbuf,
                   C_SAL_L1_KEY_MATERIAL_DATA_ID_SAL_LENGTH);
      *xpDataLen = ((size_t)C_SAL_L1_SEG_SEED_LENGTH +
                    (size_t)C_SAL_L1_KEY_MATERIAL_DATA_ID_SAL_LENGTH);
    }
    else if (C_K_KTA__VERSION_SLOT_ID == xStorageDataId)
    {
      (void)memcpy(xpData,
                   &aTmpbuf[C_SAL_KTA_VERSION_STORAGE_LENGTH - 1],
                   C_SAL_KTA_VERSION_STORAGE_LENGTH);
    }
    else if (C_K_KTA__SEALED_DATA_STORAGE_ID == xStorageDataId)
    {
      (void)memcpy(xpData, aContextProfileUID, C_SAL_CONTEXT_PROFILE_UID_LENGTH);
      len += C_SAL_CONTEXT_PROFILE_UID_LENGTH;
      xpData[len] = C_SAL_CONTEXT_PROFILE_UID_LENGTH;
      len += 1U;

      (void)memcpy(&xpData[len], aSerialNo, C_SAL_CONTEXT_SERIAL_NO_LENGTH);
      len += C_SAL_CONTEXT_SERIAL_NO_LENGTH;
      xpData[len] = C_SAL_CONTEXT_SERIAL_NO_LENGTH;
      len += 1U;

      (void)memcpy(&xpData[len], aContextVersion, C_SAL_CONTEXT_VERSION_LENGTH);
      len += C_SAL_CONTEXT_VERSION_LENGTH;
      xpData[len] = C_SAL_CONTEXT_VERSION_LENGTH;
      len += 1U;

      (void)memcpy(&xpData[len], aKtaVersion, C_SAL_KTA_VERSION_LENGTH);
      len += C_SAL_KTA_VERSION_LENGTH;

      (void)memcpy(&xpData[len], &aTmpbuf[0], C_SAL_SEALED_DATA_STORAGE_ID_SAL_LENGTH);
      len += C_SAL_SEALED_DATA_STORAGE_ID_SAL_LENGTH;
      xpData[len] = (uint8_t)strnlen((char *)aTmpbuf, C_SAL_SEALED_DATA_STORAGE_ID_SAL_LENGTH);
      len += 1U;

      (void)memcpy(&xpData[len], aDeviceSerialNo, C_SAL_DEVICE_SERIAL_NO_LENGTH);
      len += C_SAL_DEVICE_SERIAL_NO_LENGTH;
      xpData[len] = C_SAL_DEVICE_SERIAL_NO_LENGTH;
      len += 1U;
      *xpDataLen = len;
    }
    else
    {
      (void)memcpy(xpData, aTmpbuf, gaSalStorageRecord[loopIndex].length);
      *xpDataLen = gaSalStorageRecord[loopIndex].length;
    }
  }

end:
  M_KTALOG__END("End status : %d", status);
  return status;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @implements lSearchIndex
 *
 **/
static TKStatus lSearchIndex
(
  uint32_t   xStorageDataId,
  uint32_t*  xpLoopcount
)
{
  TKStatus  status = E_K_STATUS_ERROR;
  uint32_t  loopCount = 0;
  uint32_t  noOfItems = 0;

  M_KTALOG__START("Start");

  if (NULL == xpLoopcount)
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("bad parameter", status);
  }
  else
  {
    noOfItems = sizeof(gaSalStorageRecord) / sizeof(gaSalStorageRecord[0]);

    for (; loopCount < noOfItems; loopCount++)
    {
      if (gaSalStorageRecord[loopCount].storageID == xStorageDataId)
      {
        *xpLoopcount = loopCount;
        status = E_K_STATUS_OK;
        break;
      }
    }
  }

  M_KTALOG__END("End");
  return status;
}

/**
 * @implements lstorageOperation
 *
 **/
static TKStatus lstorageOperation
(
  uint32_t    xLoopIndex,
  TOperation  xOpt,
  uint8_t*    xpBuff
)
{
  TKStatus     status = E_K_STATUS_ERROR;
  ATCA_STATUS  storageStatus = ATCA_STATUS_UNKNOWN;
  ATCADevice   device = NULL;
  uint32_t     iteration = 0;
  uint8_t      block = 0;
  uint8_t      offset = 0;
  uint32_t     count = 0;

  M_KTALOG__START("Start");

  if ((xLoopIndex >= sizeof(gaSalStorageRecord) / sizeof(gaSalStorageRecord[0])) ||
      ((xOpt != E_READ) && (xOpt != E_WRITE)) ||
      (NULL == xpBuff))
  {
    status = E_K_STATUS_PARAMETER;
    M_KTALOG__ERR("bad parameter", status);
  }
  else
  {
    M_KTALOG__HEX("", xpBuff, gaSalStorageRecord[xLoopIndex].storageLength);
    iteration = gaSalStorageRecord[xLoopIndex].storageLength / C_SAL_MCHP_MAX_DATA_SIZE;
    block = gaSalStorageRecord[xLoopIndex].block;
    offset = gaSalStorageRecord[xLoopIndex].offset;
    device = atcab_get_device();

    if (NULL != device)
    {
      for (uint32_t i = 0 ; i < iteration; i++)
      {
        if (E_READ == xOpt)
        {
          storageStatus = calib_read_zone(device,
                                          ATCA_ZONE_DATA,
                                          gaSalStorageRecord[xLoopIndex].slot,
                                          block,
                                          offset,
                                          &xpBuff[count],
                                          C_SAL_MCHP_MAX_DATA_SIZE);
        }
        else
        {
          storageStatus = calib_write_zone(device,
                                          ATCA_ZONE_DATA,
                                          gaSalStorageRecord[xLoopIndex].slot,
                                          block,
                                          offset,
                                          &xpBuff[count],
                                          C_SAL_MCHP_MAX_DATA_SIZE);
        }

        if (ATCA_SUCCESS != storageStatus)
        {
          M_KTALOG__ERR("Storage operation %d", storageStatus);
          break;
        }

        if (offset >= C_SAL_MCHP_MAX_OFFSET_IN_BLOCK)
        {
          block++;
          offset = 0;
        }
        else
        {
          offset++;
        }

        count += C_SAL_MCHP_MAX_DATA_SIZE;
      }
    }

    if (ATCA_SUCCESS == storageStatus)
    {
      status = E_K_STATUS_OK;
    }
  }

  M_KTALOG__END("End");
  return status;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

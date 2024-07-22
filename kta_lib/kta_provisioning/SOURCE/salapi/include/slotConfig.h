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
/** \brief  Microchip slot configuration.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file slotConfig.h
 ******************************************************************************/

/**
 * @brief Microchip slot configuration.
 */

#ifndef SLOT_CONFIG_H
#define SLOT_CONFIG_H


/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief 32 byte block index within the zone.
 */
#define C_KTA__SLOT_BLOCK_0            (0x00u)
/**
 * @brief 32 byte block index within the zone.
 */
#define C_KTA__SLOT_BLOCK_1            (0x01u)
#define C_KTA__SLOT_BLOCK_4            (0x04u)
#define C_KTA__SLOT_BLOCK_5            (0x05u)

/** @brief 4 byte work index within the block. Ignored for 32 byte reads.*/
#define C_KTA__BLOCK_OFFSET_0          (0x00u)
/** @brief 4 byte work index within the block. Ignored for 32 byte reads.*/
#define C_KTA__BLOCK_OFFSET_1          (0x01u)
/** @brief 4 byte work index within the block. Ignored for 32 byte reads.*/
#define C_KTA__BLOCK_OFFSET_2          (0x02u)
/** @brief 4 byte work index within the block. Ignored for 32 byte reads.*/
#define C_KTA__BLOCK_OFFSET_3          (0x03u)
/** @brief 4 byte work index within the block. Ignored for 32 byte reads.*/
#define C_KTA__BLOCK_OFFSET_4          (0x04u)
/** @brief 4 byte work index within the block. Ignored for 32 byte reads.*/
#define C_KTA__BLOCK_OFFSET_5          (0x05u)
/** @brief 4 byte work index within the block. Ignored for 32 byte reads.*/
#define C_KTA__BLOCK_OFFSET_6          (0x06u)
/** @brief 4 byte work index within the block. Ignored for 32 byte reads.*/
#define C_KTA__BLOCK_OFFSET_7          (0x07u)

/**
 * @brief
 * Generate key pair during the activation request
 * Will be used to store the device private key as well.
 */
#define C_KTA__GEN_KEY_PAIR_SLOT                   (0X00u)
/**
 * @brief
 * Device private key storage slot.
 */
#define C_KTA__DEVICE_PRIVATE_STORAGE_SLOT         (C_KTA__GEN_KEY_PAIR_SLOT)

/** @brief Default slot used for key attestation. */
#define C_SAL__CRYPTO_SLOT_FOR_ATTESTAION          (0x0001u)

/** @brief Microchip default ecc private key slot. */
#define C_KTA__CHIP_SK_STORAGE_SLOT                (0x03u)

/** @brief L1 Field key storage slot. */
#define C_KTA__L1_FIELD_KEY_STORAGE_SLOT           (0x04u)
/** @brief L1 Field key storage block. */
#define C_KTA__L1_FIELD_KEY_STORAGE_BLOCK          (C_KTA__SLOT_BLOCK_0)
/** @brief L1 Field key storage offset. */
#define C_KTA__L1_FIELD_KEY_STORAGE_OFFSET         (C_KTA__BLOCK_OFFSET_0)

/** Slot 8 Structure
 * +---------------------+----------+
 * | Data                |   Size   |
 * |                     |  (Bytes) |
 * +---------------------+----------+
 * | life_cycle_state    |  4       |
 * +---------------------+----------+
 * | rot_public_uid      |  8       |
 * +---------------------+----------+
 * | key_set_id||padding | 1 || 3   |
 * | (write granularity  |          |
 * |  is 4-Bytes)  1||3  |          |
 * +---------------------+----------+
 * |  RFU 16             |          |
 * +---------------------+----------+
 * | Signer id ||padding | 4 || 28   |
 * | (write granularity  |          |
 * |  is 32-Bytes)  4||28  |          |
 * +---------------------+----------+
 * |  RFU 352             |          |
 * +---------------------+----------+
 */
/* MISRA Rule 5.4 first 31 characters of macro should not be same.
 * Other files follow the same.
*/
/** @brief Life Cycle State - 4 bytes. */
/** @brief Life Cycle State - storage slot. */
#define C_KTA__STORAGE_SLOT_LIFE_CYCLE_STATE       (0x08u)
/** @brief Life Cycle State - storage block. */
#define C_KTA__STORAGE_BLOCK_LIFE_CYCLE_STATE      (C_KTA__SLOT_BLOCK_0)
/** @brief Life Cycle State - storage offset. */
#define C_KTA__STORAGE_OFFSET_LIFE_CYCLE_STATE     (C_KTA__BLOCK_OFFSET_0)

/** @brief Rot Public UID storage slot - 8 bytes. */
#define C_KTA__ROT_PUBLIC_UID_STORAGE_SLOT         (0x08u)
/** @brief Rot Public UID storage block. */
#define C_KTA__ROT_PUBLIC_UID_STORAGE_BLOCK        (C_KTA__SLOT_BLOCK_0)
/** @brief Rot Public UID storage offset. */
#define C_KTA__ROT_PUBLIC_UID_STORAGE_OFFSET       (C_KTA__BLOCK_OFFSET_1)

/**
 * @brief
 * L1 Key Material - Segmentation seed is hardcoded in SAL.
 * So it has only KeySetId|Padding(1|3) byte.
 */
/** @brief L1 Key Material data storage slot. */
#define C_KTA__L1_KEY_DATA_STORAGE_SLOT            (0x08u)
/** @brief L1 Key Material data storage block. */
#define C_KTA__L1_KEY_DATA_STORAGE_BLOCK           (C_KTA__SLOT_BLOCK_0)
/** @brief L1 Key Material data storage offset. */
#define C_KTA__L1_KEY_DATA_STORAGE_OFFSET          (C_KTA__BLOCK_OFFSET_3)

/** @brief KTA Version storage slot. */
#define C_KTA__VERSION_STORAGE_SLOT                (0x08u)
/** @brief KTA Version storage block. */
#define C_KTA__VERSION_STORAGE_BLOCK               (C_KTA__SLOT_BLOCK_0)
/** @brief KTA Version storage offset. */
#define C_KTA__VERSION_STORAGE_OFFSET              (C_KTA__BLOCK_OFFSET_3)

/** @brief Signer Id - 4 bytes. */
/** @brief Signer Id - storage slot. */
#define C_KTA__SIGNER_ID_STORAGE_SLOT              (0x08u)
/** @brief Signer Id - storage block. */
#define C_KTA__SIGNER_ID_STORAGE_BLOCK             (C_KTA__SLOT_BLOCK_1)
/** @brief Signer Id - storage offset. */
#define C_KTA__SIGNER_ID_STORAGE_OFFSET            (C_KTA__BLOCK_OFFSET_0)

/** @brief Sealed Storage Data. */
/** @brief Sealed Data Storage Slot */
#define C_KTA__SEALED_DATA_STORAGE_SLOT            (0x02u)
/** @brief Sealed Data Storage Block . */
#define C_KTA__SEALED_DATA_STORAGE_BLOCK           (C_KTA__SLOT_BLOCK_0)
/** @brief Sealed Data Storage Offset. */
#define C_KTA__SEALED_DATA_STORAGE_OFFSET          (C_KTA__BLOCK_OFFSET_0)

/** @brief Device Certificate. */
/** @brief Device Certificate storage slot. */
#define C_KTA__DEVICE_CERT_STORAGE_SLOT            (0x0Au)
/** @brief Device Certificate storage block */
#define C_KTA__DEVICE_CERT_STORAGE_BLOCK           (C_KTA__SLOT_BLOCK_0)
/** @brief Device Certificate storage offset */
#define C_KTA__DEVICE_CERTIFICATE_STORAGE_OFFSET   (C_KTA__BLOCK_OFFSET_0)

/** @brief Singer public key. */
/** @brief Singer public key storage slot. */
#define C_KTA__SIGNER_PUB_KEY_STORAGE_SLOT         (0x0Bu)
/** @brief Singer public key storage block. */
#define C_KTA__SIGNER_PUB_KEY_STORAGE_BLOCK        (C_KTA__SLOT_BLOCK_0)
/** @brief Singer public key storage offset. */
#define C_KTA__SIGNER_PUBLIC_KEY_STORAGE_OFFSET    (C_KTA__BLOCK_OFFSET_0)
/** @brief Singer public key length. */
#define C_KTA__SIGNER_PUBLIC_KEY_LENGTH            (64u)

/** @brief Singer Certificate. */
/** @brief Singer Certificate storage slot. */
#define C_KTA__SIGNER_CERT_STORAGE_SLOT            (0x0Cu)
/** @brief Singer Certificate storage block. */
#define C_KTA__SIGNER_CERT_STORAGE_BLOCK           (C_KTA__SLOT_BLOCK_0)
/** @brief Singer Certificate storage offset. */
#define C_KTA__SIGNER_CERTIFICATE_STORAGE_OFFSET   (C_KTA__BLOCK_OFFSET_0)

/** @brief KDF slots. */
/** @brief KDF target slot. */
#define C_KTA__KDF_TARGET_SLOT                     (C_KTA__L1_FIELD_KEY_STORAGE_SLOT)
/** @brief KDF source slot. */
#define C_KTA__KDF_SOURCE_SLOT                     (0x00u)
/** @brief KDF key ID. */
#define C_KTA__KDF_KEY_ID                          ((C_KTA__KDF_TARGET_SLOT << 8) | \
                                                    C_KTA__KDF_SOURCE_SLOT)

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

#endif // SLOT_CONFIG_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

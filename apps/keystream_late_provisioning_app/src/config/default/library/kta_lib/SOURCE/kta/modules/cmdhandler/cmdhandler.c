/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2026 Nagravision SÃ rl

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
/** \brief keySTREAM Trusted Agent - ICPP command handler.
 *
 * \author Kudelski Labs
 *
 * \date 2025/06/12
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
#include "kta_version.h"


#include <string.h>
#include <stdbool.h>

#ifdef FOTA_ENABLE
#include "k_sal_fota.h"
#endif

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

/** @brief Challenge size in bytes */
#define C_K_ICPP_PARSER_KTA_CHALLENGE_SIZE              (32u)

/** @brief Utility macro to validate field length */
#define CHECK_FIELD_LEN(field, max) \
  do { \
    if ((field)->fieldLen > (max)) { \
      M_KTALOG__ERR("Invalid field length for tag %d", (field)->fieldTag); \
      return 1u; \
    } \
  } while (0)

/** @brief Utility macro to suppress unused parameter warnings for FOTA indices */
#ifdef FOTA_ENABLE
#define UNUSED_FOTA_PARAMS() \
  do { \
    (void)xpTargetNameIndex; \
    (void)xpTargetVersionIndex; \
    (void)xpTargetUrlIndex; \
  } while (0)
#else
#define UNUSED_FOTA_PARAMS() do {} while (0)
#endif

#if defined(OBJECT_MANAGEMENT_FEATURE) || defined(PLATFORM_PROCESS_FEATURE)
/** @brief Mandatory generate key pair fields. */
#define C_K_CMD_GENKEYPAIR_FIELDS_MANDATORY \
  (E_K_CMD_FIELD_IDENTIFIER)

/** @brief Optional generate key pair fields. */
#define C_K_CMD_GENKEYPAIR_FIELDS_OPTIONAL \
  ((E_K_CMD_FIELD_IDENTIFIER | E_K_CMD_FIELD_ATTRIBUTES \
   | E_K_CMD_FIELD_OBJECT_OWNER | E_K_CMD_FIELD_NONCE))

/** @brief Mandatory set object fields. */
#define C_K_CMD_SETOBJ_FIELDS_MANDATORY \
  ((E_K_CMD_FIELD_OBJECT_TYPE | E_K_CMD_FIELD_IDENTIFIER \
   | E_K_CMD_FIELD_DATA))

/** @brief Optional set object fields. */
#define C_K_CMD_SETOBJ_FIELDS_OPTIONAL \
  ((E_K_CMD_FIELD_OBJECT_TYPE | E_K_CMD_FIELD_IDENTIFIER \
   | E_K_CMD_FIELD_DATA | E_K_CMD_FIELD_ATTRIBUTES \
   | E_K_CMD_FIELD_OBJECT_OWNER))

/** @brief Mandatory set object association fields. */
#define C_K_CMD_SETOBJASSOCIATION_FIELDS_MANDATORY \
  ((E_K_CMD_FIELD_OBJECT_TYPE | E_K_CMD_FIELD_IDENTIFIER \
   | E_K_CMD_FIELD_DATA | E_K_CMD_FIELD_ASSOCIATION_INFO))

/** @brief Mandatory delete object fields. */
#define C_K_CMD_DELETEOBJ_FIELDS_MANDATORY \
  ((E_K_CMD_FIELD_IDENTIFIER | E_K_CMD_FIELD_OBJECT_TYPE))

/** @brief Optional delete object fields. */
#define C_K_CMD_DELETEOBJ_FIELDS_OPTIONAL \
  ((E_K_CMD_FIELD_IDENTIFIER | E_K_CMD_FIELD_OBJECT_TYPE \
   | E_K_CMD_FIELD_OBJECT_OWNER))

/** @brief Mandatory delete key object fields. */
#define C_K_CMD_DELETEKEYOBJ_FIELDS_MANDATORY \
  (E_K_CMD_FIELD_IDENTIFIER)

/** @brief Optional delete key object fields. */
#define C_K_CMD_DELETEKEYOBJ_FIELDS_OPTIONAL \
  ((E_K_CMD_FIELD_IDENTIFIER | E_K_CMD_FIELD_OBJECT_TYPE))

/** @brief Mandatory get challenge fields. */
#define C_K_CMD_GETCHALLENGE_FIELDS_MANDATORY \
  (E_K_CMD_FIELD_OBJECT_ID)

/** @brief  Command response fields structure. */
typedef struct
{
  uint8_t *pValue;
  /* Response field value. */
  size_t    len;
  /* Response field length. */
} TKcmdRespFields;

/** @brief  Command response payload structure. */
typedef struct
{
  uint32_t objectType;
  /* Type of the object. */
  uint32_t identifier;
  /* Persistent object identifier. */
  uint32_t objectOwner;
  /* Object owner. */
  uint8_t *fotaName;
  /* Fota Name. */
  uint8_t fotaNameLen;
  /* Fota Name Length. */
  uint8_t *fotaMetadata;
  /* Fota Metadata */
  uint8_t fotaMetadataLen;
  /* Fota Metadata Length. */
  TKcmdRespFields attributes;
  /* Response data attributes. */
  TKSalObjAssociationInfo associationInfo;
  /* Object association info. */
  object_t object;
  /* Object Data. */
#ifdef FOTA_ENABLE
  TTargetComponent xTargetComponents[COMPONENTS_MAX];
  /* Components. */
#endif
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
  E_K_CMD_FIELD_IDENTIFIER = 0x02u,
  /**
   * Data Attribute field mask
   */
  E_K_CMD_FIELD_ATTRIBUTES = 0x04u,
  /**
   * Data field mask
   */
  E_K_CMD_FIELD_DATA = 0x08u,
  /**
   * Association info field mask
   */
  E_K_CMD_FIELD_OBJECT_UID = 0x10u,
  /**
   * Object uid field mask
   */
  E_K_CMD_FIELD_CUSTOMER_METADATA = 0x20u,
  /**
   * Customer Metadata field mask
   */
  E_K_CMD_FIELD_ASSOCIATION_INFO = 0x40u,
  /**
   * Object owner field mask
   */
  E_K_CMD_FIELD_OBJECT_OWNER = 0x80u,
  /**
   * Object owner field mask
   */
  E_K_CMD_FIELD_NONCE = 0x0100u

} TKcmdFieldTag;

/**
 * @brief Handler function signature for field processing
 */
typedef uint32_t (*FieldHandler)
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  ,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
);

/**
 * @brief Field tag lookup table entry
 */
typedef struct
{
  uint32_t tag;
  FieldHandler handler;
  uint32_t maskBit;
} FieldTagEntry;

#endif

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */
/**
 * SUPPRESS: MISRA_DEV_KTA_009 : misra_c2012_rule_5.9_violation
 * The identifier gpModuleName is intentionally defined as a common global for logging purposes
 */
#if LOG_KTA_ENABLE != C_KTA_LOG_LEVEL_NONE
static const char *gpModuleName = "KTACMDHANDLER";
#endif

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Handler for Object Type field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index (FOTA only).
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index (FOTA only).
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index (FOTA only).
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleObjectType
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  , uint32_t* xpTargetNameIndex,
   uint32_t* xpTargetVersionIndex,
   uint32_t* xpTargetUrlIndex
#endif
);

/**
 * @brief
 *   Handler for Identifier field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index (FOTA only).
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index (FOTA only).
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index (FOTA only).
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleIdentifier
(
  TKIcppField *xpField,
  TKcmdRespPayload *xpCmdRespPayload
#ifdef FOTA_ENABLE
  , uint32_t *xpTargetNameIndex,
  uint32_t *xpTargetVersionIndex,
  uint32_t *xpTargetUrlIndex
#endif
);

/**
 * @brief
 *   Handler for Attributes field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index (FOTA only).
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index (FOTA only).
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index (FOTA only).
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleAttributes
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  , uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
);

/**
 * @brief
 *   Handler for Nonce field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index (FOTA only).
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index (FOTA only).
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index (FOTA only).
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleNonce
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  , uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
);

/**
 * @brief
 *   Handler for Data field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index (FOTA only).
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index (FOTA only).
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index (FOTA only).
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleData
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  , uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
);

/**
 * @brief
 *   Handler for Object UID field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index (FOTA only).
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index (FOTA only).
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index (FOTA only).
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleObjectUid
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  , uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
);

/**
 * @brief
 *   Handler for Customer Metadata field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index (FOTA only).
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index (FOTA only).
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index (FOTA only).
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleCustomerMetadata
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  , uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
);

/**
 * @brief
 *   Handler for Association Info field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index (FOTA only).
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index (FOTA only).
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index (FOTA only).
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleAssociationInfo
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  , uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
);

/**
 * @brief
 *   Handler for Object Owner field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index (FOTA only).
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index (FOTA only).
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index (FOTA only).
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleObjectOwner
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  , uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
);

#ifdef FOTA_ENABLE
/**
 * @brief
 *   Handler for FOTA Name field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index.
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index.
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index.
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleFota
(
  TKIcppField *xpField,
  TKcmdRespPayload *xpCmdRespPayload,
  uint32_t *xpTargetNameIndex,
  uint32_t *xpTargetVersionIndex,
  uint32_t *xpTargetUrlIndex
);

/**
 * @brief
 *   Handler for FOTA Metadata field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index.
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index.
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index.
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleFotaMetadata
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
);

/**
 * @brief
 *   Handler for FOTA Component Target field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index.
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index.
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index.
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleFotaComponentTarget
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
);

/**
 * @brief
 *   Handler for FOTA Component Version field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index.
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index.
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index.
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleFotaComponentVersion
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
);

/**
 * @brief
 *   Handler for FOTA Component URL field processing.
 *
 * @param[in] xpField
 *   Pointer to the ICPP field.
 * @param[in,out] xpCmdRespPayload
 *   Pointer to command response payload.
 * @param[in] xpTargetNameIndex
 *   Pointer to target name index.
 * @param[in] xpTargetVersionIndex
 *   Pointer to target version index.
 * @param[in] xpTargetUrlIndex
 *   Pointer to target URL index.
 *
 * @return
 * - 0 on success.
 * - Non-zero on error.
 */
static uint32_t lHandleFotaComponentUrl
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
);
#endif

/**
 * @brief Field tag lookup table
 */
static const FieldTagEntry kFieldTagTable[] =
{
#ifdef OBJECT_MANAGEMENT_FEATURE
  { E_K_ICPP_PARSER_FIELD_TAG_CMD_OBJECT_TYPE,                 lHandleObjectType,            E_K_CMD_FIELD_OBJECT_TYPE },
  { E_K_ICPP_PARSER_FLD_TAG_CMD_IDENTIFIER,                    lHandleIdentifier,            E_K_CMD_FIELD_IDENTIFIER },
  { E_K_ICPP_PARSER_FIELD_TAG_CMD_ATTRIBUTES,                  lHandleAttributes,            E_K_CMD_FIELD_ATTRIBUTES },
  { E_K_ICPP_PARSER_FLD_TAG_KTA_NONCE,                         lHandleNonce,                 E_K_CMD_FIELD_NONCE },
  { E_K_ICPP_PARSER_FLD_TAG_CMD_DATA,                          lHandleData,                  E_K_CMD_FIELD_DATA },
  { E_K_ICPP_PARSER_FIELD_TAG_CMD_OBJECT_UID,                  lHandleObjectUid,             E_K_CMD_FIELD_OBJECT_UID },
  { E_K_ICPP_PARSER_FIELD_TAG_CMD_CUSTOMER_METADATA,           lHandleCustomerMetadata,      E_K_CMD_FIELD_CUSTOMER_METADATA },
  { E_K_ICPP_PARSER_FIELD_TAG_CMD_ASSOCIATION_INFO,            lHandleAssociationInfo,       E_K_CMD_FIELD_ASSOCIATION_INFO },
  { E_K_ICPP_PRSR_FLD_TAG_CMD_OBJECT_OWNER,                    lHandleObjectOwner,           E_K_CMD_FIELD_OBJECT_OWNER },
#endif
#ifdef FOTA_ENABLE
  { E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA,                         lHandleFota,                 E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA },
  { E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_METADATA,                lHandleFotaMetadata,         E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_METADATA },
  { E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_COMPONENT_TARGET,        lHandleFotaComponentTarget,  E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_COMPONENT_TARGET },
  { E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_COMPONENT_VERSION,       lHandleFotaComponentVersion, E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_COMPONENT_VERSION },
  { E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_COMPONENT_URL,           lHandleFotaComponentUrl,     E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_COMPONENT_URL },
#endif
};

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
 *   Helper function to process field tags in command validation.
 *
 * @param[in] xpField
 *   Field to process.
 * @param[out] xpCmdRespPayload
 *   Response payload to fill.
 * @param[in,out] xpFieldTagMask
 *   Field tag mask to update.
 * @param[in,out] xpTargetNameIndex
 *   Target name index (FOTA only).
 * @param[in,out] xpTargetVersionIndex
 *   Target version index (FOTA only).
 * @param[in,out] xpTargetUrlIndex
 *   Target URL index (FOTA only).
 *
 * @return
 * - 0 if successful.
 * - 1 if error occurred.
 */
static uint32_t lProcessFieldTag
(
  TKIcppField*       xpField,
  TKcmdRespPayload*  xpCmdRespPayload,
  uint32_t*          xpFieldTagMask
#ifdef FOTA_ENABLE
  ,
  uint32_t*          xpTargetNameIndex,
  uint32_t*          xpTargetVersionIndex,
  uint32_t*          xpTargetUrlIndex
#endif
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

/**
 * @brief
 *   After receiving Get Challenge command. Call the SAL API to generate Challenge (temp_key),
 *   by using the random number generated by device.
 *
 * @param[out] xpChallenge
 *   32 byte Challenge (temp_key) to be sent to keySTREAM in response.
 * @param[out] xpPlatformStatus
 *   Platform status from SAL API.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
static TKStatus lKtaGetChallenge
(
  uint8_t* xpChallenge,
  uint8_t* xpPlatformStatus
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

/**
 * @brief
 *   Initialize send protocol message from received message.
 */
static inline void lInitSendProtoMessage
(
  const TKIcppProtocolMessage* xpRecvd,
  TKIcppProtocolMessage*       xpSend
)
{
  xpSend->msgType = E_K_ICPP_PARSER_MESSAGE_TYPE_RESPONSE;
  xpSend->cryptoVersion = xpRecvd->cryptoVersion;
  xpSend->encMode = xpRecvd->encMode;
  (void)memcpy(xpSend->rotPublicUID, xpRecvd->rotPublicUID,
               C_K_ICPP_PARSER__ROT_PUBLIC_UID_SIZE_IN_BYTES);
  (void)memcpy(xpSend->transactionId, xpRecvd->transactionId,
               C_K_ICPP_PARSER__TRANSACTION_ID_SIZE_IN_BYTES);
  xpSend->rotKeySetId = xpRecvd->rotKeySetId;
}

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
  TKStatus status = E_K_STATUS_ERROR;
  TKStatus cmdProcessStatus;
  TKStatus genRespStatus;
#ifdef OBJECT_MANAGEMENT_FEATURE
  uint8_t aPlatformStatus[4] = {0};
  uint8_t aCmdResponse[C_K__ICPP_CMD_RESPONSE_SIZE_VENDOR_SPECIFIC] = {0};
  size_t cmdResponseSize = sizeof(aCmdResponse);
#else
  uint8_t aCmdResponse[C_K_ICPP_PARSER_MAX_COUNT_THIRDPARTY_ERROR_SIZE] = {0};
  size_t cmdResponseSize = C_K_ICPP_PARSER_MAX_COUNT_THIRDPARTY_ERROR_SIZE;
#endif

  M_KTALOG__START("Start");

  // REQ RQ_M-KTA-STRT-FN-0300(1) : Input Parameters Check.
  if ((NULL == xpRecvdProtoMessage) || (NULL == xpMessageToSend) || (NULL == xpMessageToSendSize) || (0u == *xpMessageToSendSize))
  {
    M_KTALOG__ERR("Invalid parameter passed");
    status = E_K_STATUS_PARAMETER;
    goto end;
  }

  if ((uint8_t)E_K_ICPP_PARSER_CRYPTO_TYPE_L2_BASED != xpRecvdProtoMessage->cryptoVersion)
  {
    M_KTALOG__ERR("Invalid crypto version received from the server, cryptoVersion = [%d]", xpRecvdProtoMessage->cryptoVersion);
    goto end;
  }

  // REQ RQ_M-KTA-OBJM-FN-0100_03(1) : message type
  // REQ RQ_M-KTA-TRDP-FN-0110_03(1) : Third Party Response message type
  // REQ RQ_M-KTA-OBJM-FN-0100_01(1) : crypto version
  // REQ RQ_M-KTA-OBJM-FN-0080(1) : Crypto version from keySTEREAM in Generate key pair.
  // REQ RQ_M-KTA-OBJM-FN-0280(1) : Crypto version from keySTEREAM in Set Object.
  // REQ RQ_M-KTA-OBJM-FN-0580(1) : Crypto version from keySTEREAM in Delete Object.
  // REQ RQ_M-KTA-OBJM-FN-0780(1) : Crypto version from keySTEREAM in Set Object With Association.
  // REQ RQ_M-KTA-OBJM-FN-0980(2) : Crypto version from keySTEREAM in Delete Key Object
  // REQ RQ_M-KTA-TRDP-FN-0080(1) : Crypto version from keySTEREAM in Third Party.
  // REQ RQ_M-KTA-TRDP-FN-0110_01(1) : Third Party Response crypto version.
  // REQ RQ_M-KTA-OBJM-FN-0100_02(1) : partial encryption mode
  // REQ RQ_M-KTA-TRDP-FN-0110_02(1) : Third Party Response partial encryption mode
  // REQ RQ_M-KTA-OBJM-FN-0100_06(1) : rot public uid
  // REQ RQ_M-KTA-TRDP-FN-0110_06(1) : Third Party Response rot public uid
  // REQ RQ_M-KTA-OBJM-FN-0100_04(1) : transaction id
  // REQ RQ_M-KTA-TRDP-FN-0110_04(1) : Third Party Response transaction id
  // REQ RQ_M-KTA-OBJM-FN-0100_05(1) : rot key set id
  // REQ RQ_M-KTA-TRDP-FN-0110_05(1) : Third Party Response rot key set id
  lInitSendProtoMessage(xpRecvdProtoMessage, &sendProtoMessage);

  status = lProcessCmdPrepareResponse(xpRecvdProtoMessage, &sendProtoMessage, aCmdResponse, cmdResponseSize
#ifdef OBJECT_MANAGEMENT_FEATURE
                                      , (uint8_t*)&aPlatformStatus
#endif
                                     );

  if (E_K_STATUS_OK != status)
  {
      M_KTALOG__ERR("Processing command or preparing response failed, status = [%d]", status);
      M_KTALOG__INFO("Command processing failed, but will still generate error response");
      // Don't goto end - still need to generate error response
  }

  M_KTALOG__INFO("Calling ktaGenerateResponse with %zu commands", sendProtoMessage.commandsCount);
  cmdProcessStatus = status;  // Preserve the command processing status
  genRespStatus = ktaGenerateResponse((C_GEN__SERIALIZE | C_GEN__PADDING | C_GEN__ENCRYPT | C_GEN__SIGNING),
                                      &sendProtoMessage, xpMessageToSend, xpMessageToSendSize);
  if (E_K_STATUS_OK != genRespStatus)
  {
    M_KTALOG__ERR("ktaGenerateResponse failed, status = [%d]", genRespStatus);
    status = genRespStatus;  // Only update status if response generation failed
  }
  else
  {
    M_KTALOG__INFO("ktaGenerateResponse succeeded, response size = %zu", *xpMessageToSendSize);
    // If command processing had an error, preserve it; otherwise keep OK status
    if (E_K_STATUS_OK != cmdProcessStatus)
    {
      status = cmdProcessStatus;
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
 * @brief Utility macro to read 32-bit big-endian value
 */
#define READ_U32_BE(p)  (((uint32_t)(p)[0] << 24) | \
                         ((uint32_t)(p)[1] << 16) | \
                         ((uint32_t)(p)[2] << 8) | \
                         (uint32_t)(p)[3])

/**
 * @brief Handler for Object Type field
 */
static uint32_t lHandleObjectType
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  ,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
)
{
  UNUSED_FOTA_PARAMS();
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  // REQ RQ_M-KTA-OBJM-FN-0270_01(1) : Object Type
  // REQ RQ_M-KTA-OBJM-FN-0570_01(1) : Object Type
  // REQ RQ_M-KTA-OBJM-FN-0770_01(1) : Object Type
  xpCmdRespPayload->objectType = xpField->fieldValue[0];
  return 0u;
}

/**
 * @brief Handler for Identifier field
 */
static uint32_t lHandleIdentifier
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  ,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
)
{
  UNUSED_FOTA_PARAMS();
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  // REQ RQ_M-KTA-OBJM-FN-0270_02(2) : Identifier
  xpCmdRespPayload->identifier = READ_U32_BE(xpField->fieldValue);
  return 0u;
}

/**
 * @brief Handler for Attributes field
 */
static uint32_t lHandleAttributes
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  ,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
)
{
  UNUSED_FOTA_PARAMS();
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  // REQ RQ_M-KTA-OBJM-FN-0270_03(2) : Attributes
  xpCmdRespPayload->attributes.pValue = xpField->fieldValue;
  xpCmdRespPayload->attributes.len = xpField->fieldLen;
  return 0u;
}

/**
 * @brief Handler for Nonce field
 */
static uint32_t lHandleNonce
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  ,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
)
{
  UNUSED_FOTA_PARAMS();
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  memcpy(xpCmdRespPayload->attributes.pValue + 8, xpField->fieldValue, xpField->fieldLen);
  xpCmdRespPayload->attributes.len += xpField->fieldLen;
  M_KTALOG__HEX("NONCE-ATTRIBUTES", xpCmdRespPayload->attributes.pValue + 8, xpCmdRespPayload->attributes.len);
  return 0u;
}

/**
 * @brief Handler for Data field
 */
static uint32_t lHandleData
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  ,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
)
{
  UNUSED_FOTA_PARAMS();
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  // REQ RQ_M-KTA-OBJM-FN-0270_04(1) : Data
  xpCmdRespPayload->object.data = xpField->fieldValue;
  xpCmdRespPayload->object.dataLen = xpField->fieldLen;
  return 0u;
}

/**
 * @brief Handler for Object UID field
 */
static uint32_t lHandleObjectUid
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  ,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
)
{
  UNUSED_FOTA_PARAMS();
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  // REQ RQ_M-KTA-OBJM-FN-0270_05(2) : Object Uid
  xpCmdRespPayload->object.objectUid = xpField->fieldValue;
  xpCmdRespPayload->object.objectUidLen = xpField->fieldLen;
  return 0u;
}

/**
 * @brief Handler for Customer Metadata field
 */
static uint32_t lHandleCustomerMetadata
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  ,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
)
{
  UNUSED_FOTA_PARAMS();
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  // REQ RQ_M-KTA-OBJM-FN-0270_06(1) : Customer Metadata
  xpCmdRespPayload->object.customerMetadata = xpField->fieldValue;
  xpCmdRespPayload->object.customerMetadataLen = xpField->fieldLen;
  return 0u;
}

/**
 * @brief Handler for Association Info field
 */
static uint32_t lHandleAssociationInfo
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  ,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
)
{
  UNUSED_FOTA_PARAMS();
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  if (NULL == xpField->fieldValue)
  {
    return 1u;
  }

  uint8_t* v = xpField->fieldValue;
  xpCmdRespPayload->associationInfo.associatedKeyId = READ_U32_BE(v);
  xpCmdRespPayload->associationInfo.associatedKeyIdDeprecated = READ_U32_BE(v + 4);
  xpCmdRespPayload->associationInfo.associatedObjectId = READ_U32_BE(v + 8);
  xpCmdRespPayload->associationInfo.associatedObjectIdDeprecated = READ_U32_BE(v + 12);
  xpCmdRespPayload->associationInfo.associatedObjectType = v[16];
  return 0u;
}

/**
 * @brief Handler for Object Owner field
 */
static uint32_t lHandleObjectOwner
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload
#ifdef FOTA_ENABLE
  ,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
#endif
)
{
  UNUSED_FOTA_PARAMS();
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  xpCmdRespPayload->objectOwner = READ_U32_BE(xpField->fieldValue);
  return 0u;
}

#ifdef FOTA_ENABLE
/**
 * @brief Handler for FOTA Name field
 */
static uint32_t lHandleFota
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
)
{
  (void)xpTargetNameIndex;
  (void)xpTargetVersionIndex;
  (void)xpTargetUrlIndex;
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  xpCmdRespPayload->fotaName = xpField->fieldValue;
  xpCmdRespPayload->fotaNameLen = xpField->fieldLen;
  return 0u;
}

/**
 * @brief Handler for FOTA Metadata field
 */
static uint32_t lHandleFotaMetadata
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
)
{
  (void)xpTargetNameIndex;
  (void)xpTargetVersionIndex;
  (void)xpTargetUrlIndex;
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  xpCmdRespPayload->fotaMetadata = xpField->fieldValue;
  xpCmdRespPayload->fotaMetadataLen = xpField->fieldLen;
  return 0u;
}

/**
 * @brief Handler for FOTA Component Target field
 */
static uint32_t lHandleFotaComponentTarget(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
)
{
  (void)xpTargetVersionIndex;
  (void)xpTargetUrlIndex;
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  if (*xpTargetNameIndex < COMPONENTS_MAX)
  {
    xpCmdRespPayload->xTargetComponents[*xpTargetNameIndex].componentTargetName = xpField->fieldValue;
    xpCmdRespPayload->xTargetComponents[*xpTargetNameIndex].componentTargetNameLen = xpField->fieldLen;
  }
  (*xpTargetNameIndex)++;
  return 0u;
}

/**
 * @brief Handler for FOTA Component Version field
 */
static uint32_t lHandleFotaComponentVersion(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
)
{
  (void)xpTargetNameIndex;
  (void)xpTargetUrlIndex;
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  if (*xpTargetVersionIndex < COMPONENTS_MAX)
  {
    xpCmdRespPayload->xTargetComponents[*xpTargetVersionIndex].componentTargetVersion = xpField->fieldValue;
    xpCmdRespPayload->xTargetComponents[*xpTargetVersionIndex].componentTargetVersionLen = xpField->fieldLen;
  }
  (*xpTargetVersionIndex)++;
  return 0u;
}

/**
 * @brief Handler for FOTA Component URL field
 */
static uint32_t lHandleFotaComponentUrl
(
  TKIcppField* xpField,
  TKcmdRespPayload* xpCmdRespPayload,
  uint32_t* xpTargetNameIndex,
  uint32_t* xpTargetVersionIndex,
  uint32_t* xpTargetUrlIndex
)
{
  (void)xpTargetNameIndex;
  (void)xpTargetVersionIndex;
  CHECK_FIELD_LEN(xpField, C_K_KTA__CMD_FIELD_MAX_SIZE);
  if (*xpTargetUrlIndex < COMPONENTS_MAX)
  {
    xpCmdRespPayload->xTargetComponents[*xpTargetUrlIndex].componentUrl = xpField->fieldValue;
    xpCmdRespPayload->xTargetComponents[*xpTargetUrlIndex].componentUrlLen = xpField->fieldLen;
  }
  (*xpTargetUrlIndex)++;
  return 0u;
}
#endif

/**
 * @implements lProcessFieldTag
 *
 */
static uint32_t lProcessFieldTag
(
  TKIcppField*       xpField,
  TKcmdRespPayload*  xpCmdRespPayload,
  uint32_t*          xpFieldTagMask
#ifdef FOTA_ENABLE
  ,
  uint32_t*          xpTargetNameIndex,
  uint32_t*          xpTargetVersionIndex,
  uint32_t*          xpTargetUrlIndex
#endif
)
{
  size_t i;
  uint32_t tableSize = sizeof(kFieldTagTable) / sizeof(kFieldTagTable[0]);

  for (i = 0; i < tableSize; ++i)
  {
    if (kFieldTagTable[i].tag == xpField->fieldTag)
    {
      uint32_t err = kFieldTagTable[i].handler(xpField, xpCmdRespPayload
#ifdef FOTA_ENABLE
                                                , xpTargetNameIndex,
                                                xpTargetVersionIndex,
                                                xpTargetUrlIndex
#endif
                                                );
      if (0u == err)
      {
        *xpFieldTagMask |= kFieldTagTable[i].maskBit;
      }
      return err;
    }
  }

  M_KTALOG__ERR("Unknown Field Tag %d", xpField->fieldTag);
  return 1u;
}

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

    case E_K_ICPP_PARSER_CMD_TAG_GET_CHALLENGE:
#ifdef FOTA_ENABLE
    case E_K_ICPP_PARSER_CMD_TAG_INSTALL_FOTA:
    case E_K_ICPP_PARSER_CMD_TAG_GET_FOTA_STATUS:
    case E_K_ICPP_PARSER_COMMAND_TAG_DEVICE_INFO:
#endif
    {
      /* These commands have no fields. Returning OK */
      status = E_K_STATUS_OK;
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
static TKStatus   lKtaCmdValidateAndGetPayload
(
  TKIcppProtocolMessage* xpRecvMsg,
  TKcmdRespPayload*      xpCmdRespPayload,
  uint32_t               xCmdCount
)
{
  uint32_t  commandsLoop        = 0;
  uint32_t  fieldsLoop          = 0;
  uint32_t  isErrorOccured      = 0;
  uint32_t  fieldTagMask        = 0;
  TKIcppFieldList* pFieldList   = NULL;
#ifdef FOTA_ENABLE
  uint32_t  targetNameIndex     = 0;
  uint32_t  targetVersionIndex  = 0;
  uint32_t  targetUrlIndex      = 0;
#endif

  if (0u == xpRecvMsg->commandsCount)
  {
    M_KTALOG__ERR("Command Count is 0");
    return E_K_STATUS_PARAMETER;
  }

  commandsLoop = xCmdCount;

  for (; ((commandsLoop < xpRecvMsg->commandsCount) && (isErrorOccured == 0U)); commandsLoop++)
  {
    TKIcppCommandTag tag = xpRecvMsg->commands[commandsLoop].commandTag;
    
    if ((tag != E_K_ICPP_PARSER_COMMAND_TAG_GENERATE_KEY_PAIR) &&
        (tag != E_K_ICPP_PARSER_COMMAND_TAG_SET_OBJECT) &&
        (tag != E_K_ICPP_PARSER_CMD_TAG_SET_OBJ_WITH_ASSOCIATION) &&
        (tag != E_K_ICPP_PARSER_COMMAND_TAG_DELETE_OBJECT) &&
        (tag != E_K_ICPP_PARSER_CMD_TAG_DELETE_KEY_OBJECT) &&
        (tag != E_K_ICPP_PARSER_CMD_TAG_INSTALL_FOTA) &&
        (tag != E_K_ICPP_PARSER_CMD_TAG_GET_FOTA_STATUS))
    {
      M_KTALOG__ERR("Invalid command Tag %d", tag);
      return E_K_STATUS_ERROR;
    }

    pFieldList = &xpRecvMsg->commands[commandsLoop].data.fieldList;

    if (0u == pFieldList->fieldsCount)
    {
      M_KTALOG__ERR("Invalid fieldsCount : %d", pFieldList->fieldsCount);
      return E_K_STATUS_ERROR;
    }

    for (; (fieldsLoop < pFieldList->fieldsCount) && (isErrorOccured == 0u); fieldsLoop++)
    {
      isErrorOccured = lProcessFieldTag(&pFieldList->fields[fieldsLoop],
                                        xpCmdRespPayload,
                                        &fieldTagMask
#ifdef FOTA_ENABLE
                                        , &targetNameIndex,
                                        &targetVersionIndex,
                                        &targetUrlIndex
#endif
                                        );
    }

    if (E_K_STATUS_OK != lKtaCmdCheckFieldTag(tag, fieldTagMask))
    {
      M_KTALOG__ERR("Command does not have all mandatory fields");
      return E_K_STATUS_ERROR;
    }

    if (isErrorOccured != 0u)
    {
      M_KTALOG__ERR("Type %x Id %x", xpCmdRespPayload->objectType, xpCmdRespPayload->identifier);
      return E_K_STATUS_ERROR;
    }

    return E_K_STATUS_OK;
  }

  return E_K_STATUS_ERROR;
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
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
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

  M_KTALOG__DEBUG("resPayload.identifier %d", resPayload.identifier);
  M_KTALOG__DEBUG("resPayload.attributes.len %d", resPayload.attributes.len);

  status = salObjectKeyGen(resPayload.identifier,
                            resPayload.attributes.pValue,
                            resPayload.attributes.len,
                            xpOutData, xpDataSize,
                            (uint8_t*)xpPlatformStatus);

end:
  return status;
}

/**
 * @implements lKtaSetObject
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
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
                        resPayload.identifier,
                        resPayload.attributes.pValue,
                        resPayload.attributes.len,
                        &resPayload.object,
                        xpPlatformStatus);

end:
  return status;
}

#ifdef FOTA_ENABLE
/**
 * @implements lktaInstallFota
 *
 */
static TKStatus lktaInstallFota
(
  TKIcppProtocolMessage* xpData,
  uint8_t*               fotaName,
  uint8_t*               fotaNameLen,
  TComponent             xComponents[COMPONENTS_MAX],
  TFotaError*            xpFotaError,
  uint8_t*               xpPlatformStatus
)
{
  TKFotaStatus status = E_K_FOTA_ERROR;
  TKcmdRespPayload   resPayload = { 0 };

  M_KTALOG__DEBUG("Processing Install Fota specific data...");

  if (E_K_STATUS_OK != lKtaCmdValidateAndGetPayload(xpData, &resPayload, 0))
  {
    /* Original error status lost here on purpose of code size optimization. */
    M_KTALOG__ERR("Getting field identifier, data attributes and object owner failed");
    goto end;
  }

  status = salFotaInstall(resPayload.fotaName,
                          resPayload.fotaNameLen,
                          resPayload.fotaMetadata,
                          resPayload.fotaMetadataLen,
                          resPayload.xTargetComponents,
                          xpFotaError,
                          xComponents);

  memcpy(fotaName, resPayload.fotaName, resPayload.fotaNameLen);
  *fotaNameLen = resPayload.fotaNameLen;

end:
  (void)memcpy((void *)xpPlatformStatus, (const void *)&status, sizeof(status));
  return status;
}

/**
 * @implements lktasalfotagetstatus
 *
 */
static TKStatus lktasalfotagetstatus
(
  TKIcppProtocolMessage* xpData,
  uint8_t*               fotaName,
  uint8_t*               fotaNameLen,
  TFotaError*            xpFotaError,
  uint8_t*               xpPlatformStatus,
  TComponent             xComponents[COMPONENTS_MAX]
)
{
  TKFotaStatus status = E_K_FOTA_ERROR;
  TKcmdRespPayload   resPayload = { 0 };

  M_KTALOG__DEBUG("Processing Install Fota specific data...");

  if (E_K_STATUS_OK != lKtaCmdValidateAndGetPayload(xpData, &resPayload, 0))
  {
    /* Original error status lost here on purpose of code size optimization. */
    M_KTALOG__ERR("Getting field identifier, data attributes and object owner failed");
    goto end;
  }

  status = salFotaGetStatus(resPayload.fotaName,
                            resPayload.fotaNameLen,
                            xpFotaError,
                            xComponents);


  memcpy(fotaName, resPayload.fotaName, resPayload.fotaNameLen);
  *fotaNameLen = resPayload.fotaNameLen;

end:
  (void)memcpy((void *)xpPlatformStatus, (const void *)&status, sizeof(status));
  return status;
}
#endif /* FOTA_ENABLE */

/**
 * @implements lKtaSetObjWithAssociation
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
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
                                        resPayload.identifier,
                                        resPayload.attributes.pValue,
                                        resPayload.attributes.len,
                                        resPayload.object.data,
                                        resPayload.object.dataLen,
                                        &(resPayload.associationInfo),
                                        xpPlatformStatus);

end:
  return status;
}

/**
 * @implements lKtaDeleteObject
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
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
  status = salObjectDelete(resPayload.objectType, resPayload.identifier, xpPlatformStatus);

end:
  return status;
}

/**
 * @implements lKtaDeleteKeyObject
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
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

  status = salObjectKeyDelete(resPayload.identifier,
                                xpPlatformStatus);

end:
  return status;
}

/**
 * @implements lKtaGetChallenge
 *
 */
static TKStatus lKtaGetChallenge
(
  uint8_t* xpChallenge,
  uint8_t* xpPlatformStatus
)
{
  TKStatus status = E_K_STATUS_ERROR;

  M_KTALOG__DEBUG("Processing GetChallenge specific data...");

  /* Calculate random and temp_key. Return temp_key */
  status = salGetChallenge(xpChallenge, xpPlatformStatus);

  return status;
}

#endif

#ifdef FOTA_ENABLE
/**
 * @brief Helper function to convert LSB to MSB byte order for 32-bit value
 */
static uint32_t lSwapByteOrder(uint32_t xValue)
{
  return ((xValue & 0x000000FFu) << 24) |
         ((xValue & 0x0000FF00u) << 8)  |
         ((xValue & 0x00FF0000u) >> 8)  |
         ((xValue & 0xFF000000u) >> 24);
}

/**
 * @brief Helper function to add KTA version and components to field list
 */
static void lAddVersionAndComponents
(
  TKIcppFieldList* xpFieldList,
  uint8_t* xpFieldIndex,
  TComponent* xpComponents
)
{
  const char* ktaVersion = (const char*)ktaGetVersion();

  xpFieldList->fields[*xpFieldIndex].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_KTA_VER;
  xpFieldList->fields[*xpFieldIndex].fieldValue = (uint8_t*)ktaVersion;
  xpFieldList->fields[*xpFieldIndex].fieldLen = strlen(ktaVersion);
  (*xpFieldIndex)++;

  for (size_t i = 0; i < COMPONENTS_MAX; i++)
  {
    if ((xpComponents[i].componentNameLen > 0) && (xpComponents[i].componentVersionLen > 0))
    {
      xpFieldList->fields[*xpFieldIndex].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_COMPONENT_TARGET;
      xpFieldList->fields[*xpFieldIndex].fieldValue = (uint8_t*)xpComponents[i].componentName;
      xpFieldList->fields[*xpFieldIndex].fieldLen = xpComponents[i].componentNameLen;
      (*xpFieldIndex)++;

      xpFieldList->fields[*xpFieldIndex].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_COMPONENT_VERSION;
      xpFieldList->fields[*xpFieldIndex].fieldValue = (uint8_t*)xpComponents[i].componentVersion;
      xpFieldList->fields[*xpFieldIndex].fieldLen = xpComponents[i].componentVersionLen;
      (*xpFieldIndex)++;
    }
  }
}

/**
 * @brief Helper function to populate common FOTA response fields
 */
static void lPopulateFotaCommonFields
(
  TKIcppCommand* xpCommand,
  uint8_t* xpFieldIndex,
  uint32_t* xpPlatformStatusMSB,
  uint8_t* xpFotaName,
  uint8_t xFotaNameLen
)
{
  xpCommand->data.fieldList.fields[*xpFieldIndex].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS;
  xpCommand->data.fieldList.fields[*xpFieldIndex].fieldLen = C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH;
  xpCommand->data.fieldList.fields[*xpFieldIndex].fieldValue = (uint8_t*)xpPlatformStatusMSB;
  (*xpFieldIndex)++;

  xpCommand->data.fieldList.fields[*xpFieldIndex].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA;
  xpCommand->data.fieldList.fields[*xpFieldIndex].fieldValue = xpFotaName;
  xpCommand->data.fieldList.fields[*xpFieldIndex].fieldLen = xFotaNameLen;
  (*xpFieldIndex)++;
}

/**
 * @brief Helper function to handle FOTA error response
 */
static void lHandleFotaErrorResponse
(
  TKIcppCommand *xpCommand,
  uint8_t *xpFieldIndex,
  TFotaError *xpFotaError,
  TComponent *xpComponents,
  uint32_t *xpErrorCodeMSB
)
{
  *xpErrorCodeMSB = ((xpFotaError->fotaErrorCode[3] & 0x000000FFu) << 24) |
                    ((xpFotaError->fotaErrorCode[2] & 0x000000FFu) << 16) |
                    ((xpFotaError->fotaErrorCode[1] & 0x000000FFu) << 8)  |
                    ((xpFotaError->fotaErrorCode[0] & 0x000000FFu));

  xpCommand->data.fieldList.fields[*xpFieldIndex].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_ERROR_CODE;
  xpCommand->data.fieldList.fields[*xpFieldIndex].fieldValue = (uint8_t*)xpErrorCodeMSB;
  xpCommand->data.fieldList.fields[*xpFieldIndex].fieldLen = xpFotaError->fotaErrorCodeLen;
  (*xpFieldIndex)++;

  xpCommand->data.fieldList.fields[*xpFieldIndex].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CMD_FOTA_ERROR_CAUSE;
  xpCommand->data.fieldList.fields[*xpFieldIndex].fieldValue = (uint8_t*)xpFotaError->fotaErrorCause;
  xpCommand->data.fieldList.fields[*xpFieldIndex].fieldLen = xpFotaError->fotaErrorCauseLen;
  (*xpFieldIndex)++;

  lAddVersionAndComponents(&xpCommand->data.fieldList, xpFieldIndex, xpComponents);
}

/**
 * @brief Process FOTA install command
 */
static TKStatus lProcessInstallFotaCmd
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  TKIcppCommand* xpSendCommand,
  uint8_t* xpFotaName,
  uint8_t* xpFotaNameLen,
  TComponent* xpComponents,
  TFotaError* xpFotaError,
  uint8_t* xpPlatformStatus
)
{
  TKFotaStatus fotaStatus;
  uint8_t fieldIndex = 0;
  uint32_t platformStatusMSB;
  uint32_t errorCodeMSB = 0;

  fotaStatus = lktaInstallFota(xpRecvdProtoMessage, xpFotaName, xpFotaNameLen, xpComponents, xpFotaError, xpPlatformStatus);
  platformStatusMSB = lSwapByteOrder(*((uint32_t*)xpPlatformStatus));

  xpSendCommand->commandTag = E_K_ICPP_PARSER_CMD_TAG_INSTALL_FOTA;
  xpSendCommand->data.fieldList.fieldsCount = 2;

  lPopulateFotaCommonFields(xpSendCommand, &fieldIndex, &platformStatusMSB, xpFotaName, *xpFotaNameLen);

  if (E_K_FOTA_ERROR == fotaStatus)
  {
    lHandleFotaErrorResponse(xpSendCommand, &fieldIndex, xpFotaError, xpComponents, &errorCodeMSB);
  }
  else if (E_K_FOTA_SUCCESS == fotaStatus)
  {
    lAddVersionAndComponents(&xpSendCommand->data.fieldList, &fieldIndex, xpComponents);
  }
  else if (E_K_FOTA_IN_PROGRESS == fotaStatus)
  {
    M_KTALOG__INFO("FOTA IN PROGRESS\r\n");
  }
  else
  {
    M_KTALOG__ERR("Install FOTA command failed with status = [%d]\r\n", fotaStatus);
  }

  xpSendCommand->data.fieldList.fieldsCount = fieldIndex;
  return E_K_STATUS_OK;
}

/**
 * @brief Process FOTA get status command
 */
static TKStatus lProcessGetFotaStatusCmd
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  TKIcppCommand* xpSendCommand,
  uint8_t* xpFotaName,
  uint8_t* xpFotaNameLen,
  TComponent* xpComponents,
  TFotaError* xpFotaError,
  uint8_t* xpPlatformStatus
)
{
  TKFotaStatus fotaStatus;
  uint8_t fieldIndex = 0;
  uint32_t platformStatusMSB;
  uint32_t errorCodeMSB = 0;

  fotaStatus = lktasalfotagetstatus(xpRecvdProtoMessage, xpFotaName, xpFotaNameLen, xpFotaError, xpPlatformStatus, xpComponents);
  platformStatusMSB = lSwapByteOrder(*((uint32_t*)xpPlatformStatus));

  xpSendCommand->commandTag = E_K_ICPP_PARSER_CMD_TAG_GET_FOTA_STATUS;
  xpSendCommand->data.fieldList.fieldsCount = 2;

  lPopulateFotaCommonFields(xpSendCommand, &fieldIndex, &platformStatusMSB, xpFotaName, *xpFotaNameLen);

  if (E_K_FOTA_ERROR == fotaStatus)
  {
    lHandleFotaErrorResponse(xpSendCommand, &fieldIndex, xpFotaError, xpComponents, &errorCodeMSB);
  }
  else if (E_K_FOTA_SUCCESS == fotaStatus)
  {
    lAddVersionAndComponents(&xpSendCommand->data.fieldList, &fieldIndex, xpComponents);
  }
  else if (E_K_FOTA_IN_PROGRESS == fotaStatus)
  {
    M_KTALOG__INFO("FOTA IN PROGRESS\r\n");
  }
  else
  {
    M_KTALOG__ERR("Install FOTA command failed with status = [%d]\r\n", fotaStatus);
  }

  xpSendCommand->data.fieldList.fieldsCount = fieldIndex;
  return E_K_STATUS_OK;
}
#endif /* FOTA_ENABLE */

#ifdef PLATFORM_PROCESS_FEATURE
/**
 * @brief Process third party command
 */
static TKStatus lProcessThirdPartyCmd
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  TKIcppCommand* xpSendCommand,
  size_t xCommandIndex,
  uint8_t* xpCmdResponse,
  uint32_t xCmdItemSize,
  uint8_t** xppCmdResponse
)
{
  TKStatus status;
  size_t dataSize = xCmdItemSize;

  status = lKtaSetThirdPartyData(
             xpRecvdProtoMessage->commands[xCommandIndex].data.cmdInfo.cmdValue,
             xpRecvdProtoMessage->commands[xCommandIndex].data.cmdInfo.cmdLen,
             xpCmdResponse, &dataSize);

  if (E_K_STATUS_OK != status)
  {
    M_KTALOG__ERR("Setting 3rd party data failed, status = [%d]", status);
    return status;
  }

  xpSendCommand->commandTag = E_K_ICPP_PARSER_COMMAND_TAG_THIRD_PARTY;
  xpSendCommand->data.cmdInfo.cmdValue = xpCmdResponse;
  xpSendCommand->data.cmdInfo.cmdLen = dataSize;
  *xppCmdResponse += dataSize;

  return E_K_STATUS_OK;
}
#endif /* PLATFORM_PROCESS_FEATURE */

#ifdef OBJECT_MANAGEMENT_FEATURE
/**
 * @brief Process generate key pair command
 */
static TKStatus lProcessGenerateKeyPairCmd
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  TKIcppCommand* xpSendCommand,
  size_t xCommandIndex,
  uint8_t* xpCmdResponse,
  uint32_t xCmdItemSize,
  uint8_t* xpPlatformStatus
)
{
  TKStatus status;
  size_t dataSize = xCmdItemSize;

  status = lKtaGenerateKeyPair(xpRecvdProtoMessage, xpCmdResponse, &dataSize, xpPlatformStatus, (uint8_t)xCommandIndex);

  xpSendCommand->commandTag = E_K_ICPP_PARSER_COMMAND_TAG_GENERATE_KEY_PAIR;
  xpSendCommand->data.fieldList.fields[0].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS;
  xpSendCommand->data.fieldList.fields[0].fieldLen = C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH;
  xpSendCommand->data.fieldList.fields[0].fieldValue = (uint8_t*)xpPlatformStatus;

  if (E_K_STATUS_OK == status)
  {
    xpSendCommand->data.fieldList.fieldsCount = 2;
    xpSendCommand->data.fieldList.fields[1].fieldTag = (TKIcppFieldTag)C_K__ICPP_FIELD_TAG_PUB_KEY_VENDOR_SPECIFIC;
    xpSendCommand->data.fieldList.fields[1].fieldLen = dataSize;
    xpSendCommand->data.fieldList.fields[1].fieldValue = xpCmdResponse;
  }
  else
  {
    M_KTALOG__ERR("Generate Key Pair command failed with status : [%d], returning only platform status", status);
    xpSendCommand->data.fieldList.fieldsCount = 1;
  }

  return status;
}

/**
 * @brief Process set object command
 */
static TKStatus lProcessSetObjectCmd
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  TKIcppCommand* xpSendCommand,
  uint8_t* xpPlatformStatus
)
{
  TKStatus status;

  M_KTALOG__INFO("lProcessSetObjectCmd: Processing SET_OBJECT command");
  status = lKtaSetObject(xpRecvdProtoMessage, xpPlatformStatus);
  M_KTALOG__INFO("lProcessSetObjectCmd: lKtaSetObject returned status=%d", status);

  xpSendCommand->commandTag = E_K_ICPP_PARSER_COMMAND_TAG_SET_OBJECT;
  xpSendCommand->data.fieldList.fieldsCount = 1;
  xpSendCommand->data.fieldList.fields[0].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS;
  xpSendCommand->data.fieldList.fields[0].fieldLen = C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH;

  if (E_K_STATUS_OK == status)
  {
    xpSendCommand->data.fieldList.fields[0].fieldValue = (uint8_t*)xpPlatformStatus;
  }
  else
  {
    M_KTALOG__ERR("Set object command failed with status = [%d]", status);
    xpSendCommand->data.fieldList.fields[0].fieldValue = (uint8_t*)xpPlatformStatus;
  }

  return status;
}

/**
 * @brief Process delete object command
 */
static TKStatus lProcessDeleteObjectCmd
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  TKIcppCommand* xpSendCommand,
  size_t xCommandIndex,
  uint8_t* xpPlatformStatus
)
{
  TKStatus status;

  status = lKtaDeleteObject(xpRecvdProtoMessage, xpPlatformStatus, xCommandIndex);

  xpSendCommand->commandTag = E_K_ICPP_PARSER_COMMAND_TAG_DELETE_OBJECT;
  xpSendCommand->data.fieldList.fieldsCount = 1;
  xpSendCommand->data.fieldList.fields[0].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS;
  xpSendCommand->data.fieldList.fields[0].fieldLen = C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH;

  if (E_K_STATUS_OK == status)
  {
    xpSendCommand->data.fieldList.fields[0].fieldValue = (uint8_t*)xpPlatformStatus;
  }
  else
  {
    M_KTALOG__ERR("Delete object command failed with status = [%d]", status);
    xpSendCommand->data.fieldList.fields[0].fieldValue = (uint8_t*)xpPlatformStatus;
  }

  return status;
}

/**
 * @brief Process delete key object command
 */
static TKStatus lProcessDeleteKeyObjectCmd
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  TKIcppCommand* xpSendCommand,
  size_t xCommandIndex,
  uint8_t* xpPlatformStatus
)
{
  TKStatus status;

  status = lKtaDeleteKeyObject(xpRecvdProtoMessage, xpPlatformStatus, xCommandIndex);

  xpSendCommand->commandTag = E_K_ICPP_PARSER_CMD_TAG_DELETE_KEY_OBJECT;
  xpSendCommand->data.fieldList.fieldsCount = 1;
  xpSendCommand->data.fieldList.fields[0].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS;
  xpSendCommand->data.fieldList.fields[0].fieldLen = C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH;

  if (E_K_STATUS_OK == status)
  {
    xpSendCommand->data.fieldList.fields[0].fieldValue = (uint8_t*)xpPlatformStatus;
  }
  else
  {
    M_KTALOG__ERR("Delete key object command failed with status = [%d]", status);
    xpSendCommand->data.fieldList.fields[0].fieldValue = (uint8_t*)xpPlatformStatus;
  }

  return status;
}

/**
 * @brief Process set object with association command
 */
static TKStatus lProcessSetObjWithAssociationCmd
(
  TKIcppProtocolMessage* xpRecvdProtoMessage,
  TKIcppCommand* xpSendCommand,
  uint8_t* xpPlatformStatus
)
{
  TKStatus status;

  status = lKtaSetObjWithAssociation(xpRecvdProtoMessage, xpPlatformStatus);

  xpSendCommand->commandTag = E_K_ICPP_PARSER_CMD_TAG_SET_OBJ_WITH_ASSOCIATION;
  xpSendCommand->data.fieldList.fieldsCount = 1;
  xpSendCommand->data.fieldList.fields[0].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS;
  xpSendCommand->data.fieldList.fields[0].fieldLen = C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH;

  if (E_K_STATUS_OK == status)
  {
    xpSendCommand->data.fieldList.fields[0].fieldValue = (uint8_t*)xpPlatformStatus;
  }
  else
  {
    M_KTALOG__ERR("Set object with association command failed with status = [%d]", status);
    xpSendCommand->data.fieldList.fields[0].fieldValue = (uint8_t*)xpPlatformStatus;
  }

  return status;
}

/**
 * @brief Process get challenge command
 */
static TKStatus lProcessGetChallengeCmd
(
  TKIcppCommand* xpSendCommand,
  uint8_t* xpChallenge,
  uint8_t* xpPlatformStatus
)
{
  TKStatus status;

  status = lKtaGetChallenge(xpChallenge, xpPlatformStatus);

  xpSendCommand->commandTag = E_K_ICPP_PARSER_CMD_TAG_GET_CHALLENGE;
  xpSendCommand->data.fieldList.fieldsCount = 2;
  xpSendCommand->data.fieldList.fields[0].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CMD_PROCESSING_STATUS;
  xpSendCommand->data.fieldList.fields[0].fieldLen = C_K_ICPP_PARSER_PROCESSING_STATUS_FIELD_LENGTH;


  if (E_K_STATUS_OK == status)
  {
    xpSendCommand->data.fieldList.fields[1].fieldTag = E_K_ICPP_PARSER_FIELD_TAG_CHALLENGE;
    xpSendCommand->data.fieldList.fields[1].fieldLen = C_K_ICPP_PARSER_KTA_CHALLENGE_SIZE;
    xpSendCommand->data.fieldList.fields[0].fieldValue = (uint8_t*)xpPlatformStatus;
    xpSendCommand->data.fieldList.fields[1].fieldValue = (uint8_t*)xpChallenge;
  }
  else
  {
    M_KTALOG__ERR("Get Challenge command failed with status = [%d]", status);
    xpSendCommand->data.fieldList.fields[0].fieldValue = (uint8_t*)xpPlatformStatus;
  }

  return status;
}
#endif /* OBJECT_MANAGEMENT_FEATURE */

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
  TKStatus status = E_K_STATUS_ERROR;
  size_t commandCount = 0;
  uint8_t challenge[C_K_ICPP_PARSER_KTA_CHALLENGE_SIZE] = {0};
#ifdef FOTA_ENABLE
  uint8_t errorCode = 0;
  uint8_t fotaErrorCause[CURRENT_MAX_LENGTH] = {0};
  TFotaError fotaError = {0};
  TComponent components[COMPONENTS_MAX] = {0};
  uint8_t fotaName[CURRENT_MAX_LENGTH] = {0};
  uint8_t fotaNameLen = 0;

  fotaError.fotaErrorCause = (uint8_t*)&fotaErrorCause;
  fotaError.fotaErrorCauseLen = sizeof(fotaErrorCause);
  fotaError.fotaErrorCode = (uint8_t*)&errorCode;
  fotaError.fotaErrorCodeLen = ERROR_CODE_LEN;
#endif

  /* Validate input parameters */
  if ((NULL == xpRecvdProtoMessage) || (NULL == xpSendProtoMessage) ||
      (NULL == xpCmdResponse) || (0u == xCmdItemSize)
#ifdef OBJECT_MANAGEMENT_FEATURE
      || (NULL == xpPlatformStatus)
#endif
  )
  {
    return E_K_STATUS_PARAMETER;
  }

  /* Initialize response buffer */
  xpCmdResponse[0] = 0;
  xpSendProtoMessage->commandsCount = xpRecvdProtoMessage->commandsCount;

  /* Process each command */
  for (commandCount = 0; commandCount < xpRecvdProtoMessage->commandsCount; commandCount++)
  {
    status = E_K_STATUS_OK;

    M_KTALOG__INFO("Processing command %zu: tag=0x%x (decimal %d)",
                   commandCount,
                   xpRecvdProtoMessage->commands[commandCount].commandTag,
                   xpRecvdProtoMessage->commands[commandCount].commandTag);

    switch (xpRecvdProtoMessage->commands[commandCount].commandTag)
    {
#ifdef PLATFORM_PROCESS_FEATURE
      /* REQ RQ_M-KTA-TRDP-FN-0010(1) : Verify Third party Signature */
      /* REQ RQ_M-KTA-TRDP-FN-0110(1) : Third party response ICPP Message */
      case E_K_ICPP_PARSER_COMMAND_TAG_THIRD_PARTY:
        /* REQ RQ_M-KTA-TRDP-FN-0070(1) : Check third party data */
        /* REQ RQ_M-KTA-TRDP-FN-0090(1) : Build Third party response */
        /* REQ RQ_M-KTA-TRDP-FN-0100(1) : Third party response data order */
        status = lProcessThirdPartyCmd(xpRecvdProtoMessage,
                                       &xpSendProtoMessage->commands[commandCount],
                                       commandCount, xpCmdResponse, xCmdItemSize, &xpCmdResponse);
        break;
#endif /* PLATFORM_PROCESS_FEATURE */

#ifdef OBJECT_MANAGEMENT_FEATURE
      /* REQ RQ_M-KTA-OBJM-FN-0100(1) : Generate key pair ICPP Message */
      case E_K_ICPP_PARSER_COMMAND_TAG_GENERATE_KEY_PAIR:
        /* REQ RQ_M-KTA-OBJM-FN-0010(1) : Verify Generate Key Pair Signature */
        /* REQ RQ_M-KTA-OBJM-FN-0090(1) : Build Generate key pair response */
        /* REQ RQ_M-KTA-OBJM-FN-0090_01(1) : Command Processing Status */
        /* REQ RQ_M-KTA-OBJM-FN-0290_01(1) : Command Processing Status */
        /* REQ RQ_M-KTA-OBJM-FN-0590_01(1) : Command Processing Status */
        /* REQ RQ_M-KTA-OBJM-FN-0090_02(1) : Public Key */
        /* REQ RQ_M-KTA-OBJM-FN-0090_03(1) : Signed public key */
        status = lProcessGenerateKeyPairCmd(xpRecvdProtoMessage,
                                            &xpSendProtoMessage->commands[commandCount],
                                            commandCount, xpCmdResponse, xCmdItemSize, xpPlatformStatus);
        break;

      /* REQ RQ_M-KTA-OBJM-FN-0300(1) : Set Object ICPP Message */
      case E_K_ICPP_PARSER_COMMAND_TAG_SET_OBJECT:
        M_KTALOG__INFO("Processing SET_OBJECT command (0x%x)", E_K_ICPP_PARSER_COMMAND_TAG_SET_OBJECT);
        /* REQ RQ_M-KTA-OBJM-FN-0210(1) : Verify Set Object Signature */
        /* REQ RQ_M-KTA-OBJM-FN-0290(1) : Build Set Object response */
        status = lProcessSetObjectCmd(xpRecvdProtoMessage,
                                      &xpSendProtoMessage->commands[commandCount],
                                      xpPlatformStatus);
        M_KTALOG__INFO("SET_OBJECT command completed with status=%d", status);
        break;

      /* REQ RQ_M-KTA-OBJM-FN-0600(1) : Delete Object ICPP Message */
      case E_K_ICPP_PARSER_COMMAND_TAG_DELETE_OBJECT:
        /* REQ RQ_M-KTA-OBJM-FN-0510(1) : Verify Delete Object Signature */
        /* REQ RQ_M-KTA-OBJM-FN-0590(1) : Build Delete Object response */
        /* REQ RQ_M-KTA-OBJM-FN-0561_11(1) : Command Processing Status */
        status = lProcessDeleteObjectCmd(xpRecvdProtoMessage,
                                         &xpSendProtoMessage->commands[commandCount],
                                         commandCount, xpPlatformStatus);
        break;

      /* REQ RQ_M-KTA-OBJM-FN-1000(2) : Delete Key Object ICPP Message */
      case E_K_ICPP_PARSER_CMD_TAG_DELETE_KEY_OBJECT:
        /* REQ RQ_M-KTA-OBJM-FN-0910(2) : Verify Delete Key Object Signature */
        /* REQ RQ_M-KTA-OBJM-FN-0990(2) : Build Delete Key Object response */
        /* REQ RQ_M-KTA-OBJM-FN-0990_01(2) : Command Processing Status */
        status = lProcessDeleteKeyObjectCmd(xpRecvdProtoMessage,
                                            &xpSendProtoMessage->commands[commandCount],
                                            commandCount, xpPlatformStatus);
        break;

      /* REQ RQ_M-KTA-OBJM-FN-0800(1) : Set Object With Association ICPP Message */
      case E_K_ICPP_PARSER_CMD_TAG_SET_OBJ_WITH_ASSOCIATION:
        /* REQ RQ_M-KTA-OBJM-FN-0710(1) : Verify Set Object with Association Signature */
        /* REQ RQ_M-KTA-OBJM-FN-0790(1) : Build Set Object With Association response */
        /* REQ RQ_M-KTA-OBJM-FN-0790_01(1) : Command Processing Status */
        status = lProcessSetObjWithAssociationCmd(xpRecvdProtoMessage,
                                                  &xpSendProtoMessage->commands[commandCount],
                                                  xpPlatformStatus);
        break;

      /* REQ RQ_M-KTA-OBJM-FN-0880(1) : Get challenge With ICPP Message */
      case E_K_ICPP_PARSER_CMD_TAG_GET_CHALLENGE:
        /* REQ RQ_M-KTA-OBJM-FN-0850(1) : Verify Get challenge */
        /* REQ RQ_M-KTA-OBJM-FN-0890(1) : Build Get Challenege Response */
        /* REQ RQ_M-KTA-OBJM-FN-0910_01(1) : Command Processing Status */
        status = lProcessGetChallengeCmd(&xpSendProtoMessage->commands[commandCount],
                                         challenge, xpPlatformStatus);
        break;

#ifdef FOTA_ENABLE
      case E_K_ICPP_PARSER_CMD_TAG_INSTALL_FOTA:
        status = lProcessInstallFotaCmd(xpRecvdProtoMessage,
                                        &xpSendProtoMessage->commands[commandCount],
                                        fotaName, &fotaNameLen, components, &fotaError, xpPlatformStatus);
        break;

      case E_K_ICPP_PARSER_CMD_TAG_GET_FOTA_STATUS:
        status = lProcessGetFotaStatusCmd(xpRecvdProtoMessage,
                                          &xpSendProtoMessage->commands[commandCount],
                                          fotaName, &fotaNameLen, components, &fotaError, xpPlatformStatus);
        break;
#endif /* FOTA_ENABLE */
#endif /* OBJECT_MANAGEMENT_FEATURE */

      default:
        M_KTALOG__ERR("Received invalid command tag, cmdTag = 0x%x (decimal %d)",
                      xpRecvdProtoMessage->commands[commandCount].commandTag,
                      xpRecvdProtoMessage->commands[commandCount].commandTag);
        M_KTALOG__ERR("Expected SET_OBJECT = 0x%x but feature may not be enabled", E_K_ICPP_PARSER_COMMAND_TAG_SET_OBJECT);
        status = E_K_STATUS_ERROR;
        break;
    }
  }

  return status;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

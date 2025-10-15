#ifndef K_SAL_FOTA_H
#define K_SAL_FOTA_H

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup g_sal_api SAL Interface */

/** @addtogroup g_sal_api
 * @{
*/

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include <stddef.h>
#include <stdint.h>
#include "k_kta.h"

#ifndef K_SAL_API
/** @brief K Sal Api. */
#define K_SAL_API
#endif /* K_SAL_API */
/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

#define CURRENT_MAX_LENGTH                                              (16u)
#define COMPONENTS_MAX                                                  (8u)
#define ERROR_CODE_LEN                                                  (1u)
#define FOTA_STATE_LEN                                                  (1u)

/** @brief Structure holds the FOTA error information. */
typedef struct SFotaError {
  uint8_t *fotaErrorCode;
  /* Pointer to the FOTA error code. */
  size_t fotaErrorCodeLen;
  /* Length of the FOTA error code. */
  uint8_t *fotaErrorCause;
  /* Pointer to the cause of the FOTA error (read-only). */
  size_t fotaErrorCauseLen;
  /* Length of the FOTA error cause. */
} TFotaError;

/** @brief Structure holds the component information. */
typedef struct SComponent {
  uint8_t componentName[CURRENT_MAX_LENGTH];
  /* Pointer to the component name. */
  size_t  componentNameLen;
  /* Pointer to the length of the component name. */
  uint8_t componentVersion[CURRENT_MAX_LENGTH];
  /* Pointer to the component version. */
  size_t componentVersionLen;
  /* Pointer to the length of the component version. */
} TComponent;

/** @brief Structure holds the target component information for FOTA. */
typedef struct STargetComponent {
  uint8_t *componentTargetName;
  /* Pointer to the target component name. */
  size_t componentTargetNameLen;
  /* Length of the target component name. */
  uint8_t *componentTargetVersion;
  /* Pointer to the target component version. */
  size_t componentTargetVersionLen;
  /* Length of the target component version. */
  uint8_t *componentUrl;
  /* Pointer to the URL of the target component. */
  size_t componentUrlLen;
  /* Length of the URL of the target component. */
} TTargetComponent;

/** @brief Enumeration for the status of operations. */
typedef enum {
  E_K_FOTA_SUCCESS = 0u,
  /* Operation was successful. */
  E_K_FOTA_IN_PROGRESS = 1u,
  /* Operation is in progress. */
  E_K_FOTA_ERROR = 2u
  /* Operation encountered an error. */
} TKFotaStatus;


/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initiate FOTA installation.
 *
 * @param[in] xpFotaName
 *   Buffer containing the name of the FOTA campaign.
 * @param[in] xFotaNameLen
 *   Length of the 'xpFotaName' buffer (in Bytes).
 * @param[in] xpFotaMetadata
 *   Buffer containing customer-specific metadata.
 * @param[in] xFotaMetadataLen
 *   Length of the 'xpFotaMetadata' buffer (in Bytes).
 * @param[in] xTargetComponents
 *   Array of target components to be installed.
 * @param[out] xpFotaError
 *   Pointer to store errors related to the FOTA campaign.
 * @param[out] xComponents
 *   Array of currently installed components.
 *
 * @return TKFotaStatus
 *   SUCCESS, IN_PROGRESS, or ERROR based on the operation.
 */
K_SAL_API TKFotaStatus salFotaInstall
(
  const uint8_t          *xpFotaName,
  const size_t           xFotaNameLen,
  const uint8_t          *xpFotaMetadata,
  const size_t           xFotaMetadataLen,
  const TTargetComponent xTargetComponents[COMPONENTS_MAX],
  TFotaError           * xpFotaError,
  TComponent             xComponents[COMPONENTS_MAX]
);

/**
 * @brief Get the status of the FOTA campaign.
 *
 * @param[in] xpFotaName
 *   Name of the FOTA campaign.
 * @param[in] xFotaNameLen
 *   Length of the 'xpFotaName' buffer (in Bytes).
 * @param[out] xpFotaError
 *   Pointer to store errors related to the FOTA campaign.
 * @param[out] xComponents
 *   Array of currently installed components.
 *
 * @return TKFotaStatus
 *   Status of the operation.
 */
K_SAL_API TKFotaStatus salFotaGetStatus
(
  const uint8_t        *xpFotaName,
  size_t               xFotaNameLen,
  TFotaError         * xpFotaError,
  TComponent           xComponents[COMPONENTS_MAX]
);

/**
 * @brief Get device information.
 *
 * @param[out] xComponents
 *   Array of components.
 *
 * @return TKFotaStatus
 *   Status of the operation.
 */
K_SAL_API TKFotaStatus salDeviceGetInfo
(
  TComponent xComponents[COMPONENTS_MAX]
);

/** @} g_sal_api */

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_FOTA_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

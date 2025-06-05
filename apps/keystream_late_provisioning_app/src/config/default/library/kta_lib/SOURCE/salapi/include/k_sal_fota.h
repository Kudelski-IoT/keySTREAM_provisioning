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

#define CURRENT_MAX_LENGTH                                              16
#define COMPONENTS_MAX                                                  8
#define ERROR_CODE_LEN                                                  1

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
 * @param[in] fota_name
 *   Buffer containing the name of the FOTA campaign.
 * @param[in] fota_name_len
 *   Length of the 'fota_name' buffer (in Bytes).
 * @param[in] fota_metadata
 *   Buffer containing customer-specific metadata.
 * @param[in] fota_metadata_len
 *   Length of the 'fota_metadata' buffer (in Bytes).
 * @param[in] target_components
 *   Array of target components to be installed.
 * @param[out] fota_error
 *   Pointer to store errors related to the FOTA campaign.
 * @param[out] components
 *   Array of currently installed components.
 *
 * @return TKFotaStatus
 *   SUCCESS, IN_PROGRESS, or ERROR based on the operation.
 */
K_SAL_API TKFotaStatus salFotaInstall
(
  const uint8_t          *xpfota_name,
  const size_t           fota_name_len,
  const uint8_t          *xpfota_metadata,
  const size_t           fota_metadata_len,
  const TTargetComponent target_components[COMPONENTS_MAX],
  TFotaError           * xpfota_error,
  TComponent             components[COMPONENTS_MAX]
);

/**
 * @brief Get the status of the FOTA campaign.
 *
 * @param[in] fota_name
 *   Name of the FOTA campaign.
 * @param[in] fota_name_len
 *   Length of the 'fota_name' buffer (in Bytes).
 * @param[out] fota_error
 *   Pointer to store errors related to the FOTA campaign.
 * @param[out] components
 *   Array of currently installed components.
 *
 * @return TKFotaStatus
 *   Status of the operation.
 */
K_SAL_API TKFotaStatus salFotaGetStatus
(
  const uint8_t      *xpfota_name,
  size_t             fota_name_len,
  TFotaError       * xpfota_error,
  TComponent         components[COMPONENTS_MAX]
);

/**
 * @brief Get device information.
 *
 * @param[out] components
 *   Array of components.
 *
 * @return TKFotaStatus
 *   Status of the operation.
 */
K_SAL_API TKFotaStatus salDeviceGetInfo
(
  TComponent components[COMPONENTS_MAX]
);

/** @} g_sal_api */

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_FOTA_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

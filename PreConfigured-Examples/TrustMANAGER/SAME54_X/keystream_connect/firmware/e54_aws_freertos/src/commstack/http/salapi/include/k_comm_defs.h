/******************************************************************************
 * Copyright (c) 2023-2023 Nagravision Sàrl. All rights reserved.
 ******************************************************************************/
/** \brief  Communication interface - Type definitions.
 *
 *  \author Griffin_Team
 *
 *  \date 2023/06/12
 *
 *  \file k_comm_defs.h
 ******************************************************************************/
/**
 * @brief IoT keySTREAM Trust Agent public interface - Type definitions.
 */

#ifndef K_COMM_DEFS_H
#define K_COMM_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */

#ifdef INCLUDE_OBFUSCATE_SYMBOLS
#include "symbols.h"
#endif /* INCLUDE_OBFUSCATE_SYMBOLS. */

#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */
#ifndef K_KTA_API
#define K_KTA_API
#endif /* K_KTA_API. */

#ifndef K_SAL_API
#define K_SAL_API
#endif /* K_SAL_API. */

/**
 * @ingroup   g_k_types
 * @brief     Length of an address in IP V6 protocol, in bytes.
 */
#define C_K__IP_V6_ADDRESS_NUM_BYTES    (16u)

/**
 * @ingroup   g_k_types.
 * @brief     keySTREAM Commstack SAL return status codes.
 */
 typedef enum
 {
    E_K_COMM_STATUS_OK,
    /* Status OK, everything is fine. */
    E_K_COMM_STATUS_PARAMETER,
    /* Bad parameter. */
    E_K_COMM_STATUS_DATA,
    /* Inconsistent or unsupported data. */
    E_K_COMM_STATUS_ERROR,
    /* Undefined error. */
    E_K_COMM_STATUS_NOT_SUPPORTED,
    /* The requested action is unsupported. */
    E_K_COMM_STATUS_STATE,
    /* Call cannot be done at this time (due to call sequence or life cycle). */
    E_K_COMM_STATUS_MEMORY,
    /* Failure on memory allocation. */
    E_K_COMM_STATUS_RESOURCE,
    /* Resource missing, not available or busy. */
    E_K_COMM_STATUS_TIMEOUT,
    /* A timeout occurred. */
    E_K_COMM_STATUS_MISSING,
    /* Missing, empty or non-available data. */
    E_K_COMM_STATUS_REMOTE_STATE,
    /* Remote object is not in the expected state. */
    E_K_COMM_STATUS_NOT_PERSONALIZED,
    /* RoT must be personalized before start up. */
    E_K_COMM_STATUS_NOT_AUTHORIZED,
    /* Service is not authorized due to some policy. */
    E_K_COMM_STATUS_NETWORK,
    /* The network connectivity is not available. */
    E_K_COMM_NUM_STATUS
    /* Number of status values. */
} TKCommStatus;

/* Time, expressed in milliseconds */
typedef uint32_t TKSalMsTime;

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_COMM_DEFS_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

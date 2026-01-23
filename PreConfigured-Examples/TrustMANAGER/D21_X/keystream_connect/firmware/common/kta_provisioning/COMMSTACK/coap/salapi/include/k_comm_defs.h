/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2025 Nagravision Sàrl

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
/** \brief keySTREAM Trusted Agent public interface - type definitions for communication.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_comm_defs.h
 ******************************************************************************/
/**
 * @brief keySTREAM Trusted Agent public interface type definition for communication.
 */

#ifndef K_COMM_DEFS_H
#define K_COMM_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */

#include <stdint.h>

#ifdef INCLUDE_OBFUSCATE_SYMBOLS
#include "symbols.h"
#endif /* INCLUDE_OBFUSCATE_SYMBOLS */

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */
#ifndef K_KTA_API
#define K_KTA_API
#endif /* K_KTA_API */

#ifndef K_SAL_API
#define K_SAL_API
#endif /* K_SAL_API */

/* @defgroup g_k_types keySTREAM Trusted Agent common types. */

/** @brief Length of an address in IP V6 protocol, in bytes. */
#define C_K__IP_V6_ADDRESS_NUM_BYTES    (16u)

/** @brief keySTREAM commstack SAL return status codes. */
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

/** @brief Supported IP protocols. */
typedef enum
{
  E_K_IP_PROTOCOL_V4,
  /* IP V4. */
  E_K_IP_PROTOCOL_V6,
  /* IP V6. */
  E_K_IP_NUM_PROTOCOLS
  /* Number of supported IP protocols. */
} TKIpProtocol;

/** @brief Socket IP address, expressed for protocol IP V4. */
typedef struct
{
  uint32_t       address;
  /* IP V4 address, MSBF; e.g. 192.168.15.4 = 0xC0A80F04 */
  uint16_t       port;
  /* IP port. */
} TKSocketAddressV4;

/** @brief Socket IP address, expressed for protocol IP V6. */
typedef struct
{
  unsigned char  address[C_K__IP_V6_ADDRESS_NUM_BYTES];
  /* IP V6 address. */
  uint16_t       port;
  /* IP port. */
} TKSocketAddressV6;

/** @brief Socket IP protocol info. */
typedef struct
{
  TKIpProtocol         protocol;
  /* Protocol used (IP V4 or IP V6). */
  union
  {
    TKSocketAddressV4  v4;
    /**
     *  Address expressed for protocol IP V4;
     *  Used only if protocol = E_K_IP_PROTOCOL_V4.
     */
    TKSocketAddressV6  v6;
    /**
     *  Address expressed for protocol IP V6;
     *  Used only if protocol = E_K_IP_PROTOCOL_V6.
     */
  } address;
  /* IP address, The used field depends on protocol value. */
} TKSocketIp;

/** @brief Time, expressed in milliseconds. */
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

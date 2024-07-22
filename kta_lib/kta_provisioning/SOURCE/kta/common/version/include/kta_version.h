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
/** \brief keySTREAM Trusted Agent - Version module
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file kta_version.h
 ******************************************************************************/

#ifndef VERSION_H
#define VERSION_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* --------------------------------------------------------------------------------------------- */
/* IMPORTS                                                                                       */
/* --------------------------------------------------------------------------------------------- */
#include "k_version.h"

#include <stdint.h>
#include <stddef.h>

/* --------------------------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                                        */
/* --------------------------------------------------------------------------------------------- */
/** @brief Convert int to string. */
#define M_K_VERSION_INT_TO_STR(x_val) #x_val

/** @brief Convert version to string. */
#define M_K_VERSION_STR(x_major, x_minor, x_patch) \
  M_K_VERSION_INT_TO_STR(x_major)"."M_K_VERSION_INT_TO_STR(x_minor)"." \
  M_K_VERSION_INT_TO_STR(x_patch) \
  C_K_VERSION__HOT_FIX

/** @brief Get software version as string. */
#define M_K_VERSION_GET_STR() \
  M_K_VERSION_STR(C_K_KTA__VERSION_MAJOR, C_K_KTA__VERSION_MINOR, C_K_KTA__VERSION_PATCH)

#define C_K_KTA__ENCODED_VERSION {((C_K_KTA__VERSION_MAJOR << 4) | C_K_KTA__VERSION_MINOR), C_K_KTA__VERSION_PATCH}

#define C_K_KTA__VERSION_STRING_BUFFER_SIZE (16u)
/* --------------------------------------------------------------------------------------------- */
/* VARIABLES                                                                                     */
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/* FUNCTIONS                                                                                     */
/* --------------------------------------------------------------------------------------------- */
/**
 * @brief
 *   Get the keySTREAM Trusted Agent software version as string.
 */
uint8_t* ktaGetVersion
(
  void
);

/**
 * @brief
 *   Get the keySTREAM Trusted Agent software decoded version as string.
 */
const char* ktaGetDecodedVersionStr
(
  const uint8_t version[]
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // VERSION_H
/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

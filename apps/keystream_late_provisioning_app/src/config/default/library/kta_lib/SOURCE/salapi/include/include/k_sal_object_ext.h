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
/** \brief  SAL - Interface for object operation.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_object_ext.h
 ******************************************************************************/

/**
 * @brief SAL interface for object operation. These APIs are only for internal testing and
 *        should not be exposed.
 */

#ifndef K_SAL_OBJECT_EXT_H
#define K_SAL_OBJECT_EXT_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_defs.h"
#include "k_sal.h"

#include <stdint.h>
#include <stddef.h>
/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Gets provisioned certificates from persistent storage.
 *
 * @return
 * - E_K_STATUS_OK in case of success.
 * - E_K_STATUS_PARAMETER for wrong input parameters.
 * - E_K_STATUS_ERROR for other errors.
 */
TKStatus salObjectGetProvCertificates
(
  uint8_t*  xpDeviceCert,
  size_t*   xpDeviceCertSize,
  uint8_t*  xpSigningCert,
  size_t*   xpSigningCertSize,
  uint8_t*  xpDevicePrivKey,
  size_t*   xpDevicePrivKeySize
);
#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_OBJECT_EXT_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */


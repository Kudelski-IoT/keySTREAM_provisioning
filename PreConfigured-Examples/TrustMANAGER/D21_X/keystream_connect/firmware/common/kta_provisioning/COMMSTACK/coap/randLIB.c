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
/** \brief Random utility for mbed coap.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file randLIB.c
 ******************************************************************************/
/**
 * @brief Random utility for mbed coap.
 */

#include "randLIB.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "comm_interface_util.h"

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */

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
 * @brief  implement randLIB_get_random_in_range
 *
 */
uint16_t randLIB_get_random_in_range
(
  uint16_t  xXRange,
  uint16_t  xYRange
)
{
  TKCommStatus  status = E_K_COMM_STATUS_ERROR;
  uint16_t      randomRangeValue = 0;
  uint16_t      randomNo = 0;

  M_COMM__API_START();

  status = salRandomGet((unsigned char*)&randomNo, sizeof(randomNo));

  if (E_K_COMM_STATUS_OK != status)
  {
    M_COMM__ERROR(("salRandomGet failed"));
  }
  randomRangeValue = (randomNo % (xYRange - xXRange + 1u)) + xXRange;
  M_COMM__INFO(("Random No %d", randomRangeValue));

  M_COMM__API_END();

  return randomRangeValue;
}

/**
 * @brief  implement randLIB_get_16bit
 *
 */
uint16_t randLIB_get_16bit
(
  void
)
{
  TKCommStatus  status = E_K_COMM_STATUS_ERROR;
  uint16_t      randomNo = 0;

  M_COMM__API_START();

  status = salRandomGet((unsigned char*)&randomNo, sizeof(randomNo));

  if (E_K_COMM_STATUS_OK != status)
  {
    M_COMM__ERROR(("salRandomGet failed"));
  }

  M_COMM__INFO(("Random No %d", randomNo));

  M_COMM__API_END();

  return randomNo;
}

/**
 * @brief  implement randLIB_seed_random
 *
 */
void randLIB_seed_random
(
  void
)
{
  /*
   * nothing is peformed here, assuming keySTREAM Trusted Agent applicaation
   * is already initialized, the internal random generated is thus propery seeded.
   */
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

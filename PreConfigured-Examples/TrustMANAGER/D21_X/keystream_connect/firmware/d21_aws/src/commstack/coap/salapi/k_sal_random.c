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
/** \brief SAL for random number generation.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_random.c
 ******************************************************************************/
/**
 * @brief SAL for random number generation.
 */

#include  "k_sal_log_extended.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_random.h"
#include "k_sal_log.h"
#include "k_sal_os.h"
#include "atca_status.h"
#include "atca_basic.h"

#include <stdlib.h>

/** @brief SAL random log enable flag. */
#define C_SAL_RANDOM_ENABLE_LOG 0

/** @brief Module name to display. */
#define C_SAL_RANDOM_MODULE_NAME "RANDOM"

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

#if (C_SAL_RANDOM_ENABLE_LOG == 1)
/**
  * @brief
  *   Display a text if log is enabled.
  *
  * @param[in] x_pcText
  *   The text to display.
  */
#define M_SAL_RANDOM_LOG(x_pcText)\
  (M_SAL_LOG(C_SAL_RANDOM_MODULE_NAME, x_pcText))
#else
#define M_SAL_RANDOM_LOG(x_pcText) {}
#endif

#if (C_SAL_RANDOM_ENABLE_LOG == 1)
/**
 * @brief
 *   Display a variable list of arguments if log is enabled.
 *
 * @param[in] x_pcFormat
 *   Formatting string (like printf).
 * @param[in] x_varArgs
 *   A variable list of parameters to display.
 */
#define M_SAL_RANDOM_LOG_VAR(x_pcFormat, x_varArgs)\
  (M_SAL_LOG_VAR(C_SAL_RANDOM_MODULE_NAME, x_pcFormat, x_varArgs))
#else
#define M_SAL_RANDOM_LOG_VAR(x_pcFormat, x_varArgs) {}
#endif

#if (C_SAL_RANDOM_ENABLE_LOG == 1)
/**
 * @brief
 *   Display the buffer name, content and size if log is enabled.
 *
 * @param[in] x_pcBuffName
 *   Buffer name.
 * @param[in] x_pucBuff
 *   Pointer on buffer.
 * @param[in] x_u16BuffSize
 *   Buffer size.
 */
#define M_SAL_RANDOM_LOG_BUFF(x_pcBuffName, x_pucBuff, x_u16BuffSize)\
  (M_SAL_LOG_BUFF(C_SAL_RANDOM_MODULE_NAME, x_pcBuffName, x_pucBuff, x_u16BuffSize))
#else
#define M_SAL_RANDOM_LOG_BUFF(x_pcBuffName, x_pucBuff, x_u16BuffSize) {}
#endif

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
 * @brief  implement salRandomGet
 *
 */
TKCommStatus salRandomGet
(
  unsigned char*  xpRandomBuffer,
  const size_t    xSize
)
{
  TKCommStatus status =  E_K_COMM_STATUS_ERROR;
  size_t i;
  uint8_t buf[32] = {0};

  M_SAL_RANDOM_LOG_VAR("Start of %s", __func__);
  for (;;)
  {
    if ((NULL == xpRandomBuffer) || (0U == xSize))
    {
      M_SAL_RANDOM_LOG("ERROR : Random buffer or size are invalid");
      status = E_K_COMM_STATUS_PARAMETER;
      break;
    }

    if (ATCA_SUCCESS != atcab_random((uint8_t *)buf))
    {
      M_SAL_RANDOM_LOG("ERROR : Random Number Generation failed");
      status = E_K_COMM_STATUS_ERROR;
      break;
    }
    for (i = 0; i < xSize; i++)
    {
      xpRandomBuffer[i] = buf[i];
    }

    status = E_K_COMM_STATUS_OK;
    break;
  }

  M_SAL_RANDOM_LOG_VAR("End of %s", __func__);

  return status;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

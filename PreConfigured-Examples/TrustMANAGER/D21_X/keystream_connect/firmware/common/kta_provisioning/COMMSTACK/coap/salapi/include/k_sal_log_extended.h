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
/** \brief  Interface to extended log functions.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_log_extended.h
 ******************************************************************************/
/**
 * @brief Interface to extended log functions.
 */

#ifndef K_SAL_LOG_EXTENDED_H
#define K_SAL_LOG_EXTENDED_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_comm_defs.h"

#include  <stdio.h>

/* -------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                     */
/* -------------------------------------------------------------------------- */

#if (C_ENABLE_SAL_LOG == 1)
/**
 * @brief
 *   Display a text if log is enabled.
 *
 * @param[in] x_pcModuleName
 *   Module name. e.g. "I2C".
 * @param[in] x_pcText
 *   Text to display.
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_003 : misra_c2012_rule_21.6_violation
 * SUPPRESS: MISRA_DEV_KTA_001 : misra_c2012_rule_17.1_violation
 * Using printf for logging.
 * Not checking the return status of printf, since not required.
 **/
  #define M_SAL__LOG(x_pcModuleName, x_pcText)\
  {\
    printf("SAL %s> %s\n", x_pcModuleName, x_pcText);\
  }
#else
  #define M_SAL__LOG(x_pcModuleName, x_pctext) {}
#endif /* C_ENABLE_SAL_LOG */

#if (C_ENABLE_SAL_LOG == 1)
/**
 * @brief
 *   Display the log with arguments.
 *
 * @param[in] x_pcModuleName
 *   Module name. e.g. "I2C".
 * @param[in] x_pcFormat
 *   Formatting string (like printf).
 * @param[in] x_varArgs      A variable list of parameters to display.
 *
 */
  #define M_SAL__LOG_VAR(x_pcModuleName, x_pcFormat, x_varArgs)\
  {\
    salLogModPrint(x_pcModuleName, x_pcFormat, x_varArgs);\
  }
#else
  #define M_SAL__LOG_VAR(x_pcModuleName, x_pcFormat, x_varArgs) {}
#endif

#if (C_ENABLE_SAL_LOG == 1)
/**
 * @brief
 *   Display the buffer name, content and size if log is enabled.
 *
 * @param[in] x_pcModuleName
 *   Module name. e.g. "I2C".
 * @param[in] x_pcBuffName
 *   Buffer name to display.
 * @param[in] x_pucBuff
 *   Pointer on buffer.
 * @param[in] x_u16BuffSize
 *   Buffer size.
 */
  #define M_SAL__LOG_BUFF(x_pcModuleName, x_pcBuffName, x_pucBuff, x_u16BuffSize)\
  {\
    salLogModDisplayBuffer(x_pcModuleName, x_pcBuffName, x_pucBuff, x_u16BuffSize);\
  }
#else
  #define M_SAL__LOG_BUFF(x_pcModuleName, x_pcBuffName, x_pucBuff, x_u16BuffSize) {}
#endif
/* -------------------------------------------------------------------------- */
/* VARIABLES                                                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Print the module name and then a message formatted like for printf.
 *
 * @param[in] x_pcModuleName
 *   Module name. e.g. "I2C".
 * @param[in] x_pcFormat
 *   Formatting string (like printf).
 * @param[in] ...
 *   A variable list of parameters to display.
 */
void salLogModPrint(
  const char*  x_pcModuleName,
  const char*  x_pcFormat,
  ...
);

/**
 * @brief
 *   Display the buffer name, content and size for a given module.
 *
 * @param[in] x_pcModuleName
 *   Module name to display. e.g. "I2C".
 * @param[in] x_pcBuffName
 *   Buffer name to display.
 * @param[in] x_pucBuff
 *   Pointer on buffer.
 * @param[in] x_u16BuffSize
 *   Buffer size.
 */
void salLogModDisplayBuffer(
  char*                 x_pcModuleName,
  char*                 x_pcBuffName,
  const unsigned char*  x_pucBuff,
  uint16_t              x_u16BuffSize
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_LOG_EXTENDED_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

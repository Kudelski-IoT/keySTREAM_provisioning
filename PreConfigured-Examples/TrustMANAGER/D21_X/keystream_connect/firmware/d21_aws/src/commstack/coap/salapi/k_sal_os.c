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
/** \brief  SAL for operating system.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_os.c
 ******************************************************************************/
/**
 * @brief SAL for operating system.
 */

#include  "k_sal_os.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include  "k_sal_log_extended.h"
#include  "k_sal_log.h"

#include  <stdlib.h>
#include  <stdio.h>
#include  <time.h>
#include  <unistd.h>
#include  <nm_common.h>

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/** @brief SAL OS Log enable flag. */
#define C_SAL_OS_ENABLE_LOG 0

/** @brief Module name to display. */
#define C_SAL_OS_MODULE_NAME "OS"

#if (C_SAL_OS_ENABLE_LOG == 1)
/**
  * @brief
  *   Display a text if log is enabled.
  *
  * @param[in] x_pcText
  *   The text to display.
  */
#define M_SAL_OS_LOG(x_pcText)\
  (M_SAL_LOG(C_SAL_OS_MODULE_NAME, x_pcText))
#else
#define M_SAL_OS_LOG(x_pcText) {}
#endif

#if (C_SAL_OS_ENABLE_LOG == 1)
/**
 * @brief
 *   Display a variable list of arguments if log is enabled.
 *
 * @param[in] x_pcFormat
 *   Formatting string (like printf).
 * @param[in] x_varArgs
 *   A variable list of parameters to display.
 */
#define M_SAL_OS_LOG_VAR(x_pcFormat, x_varArgs)\
  (M_SAL_LOG_VAR(C_SAL_OS_MODULE_NAME, x_pcFormat, x_varArgs))
#else
#define M_SAL_OS_LOG_VAR(x_pcFormat, x_varArgs) {}
#endif

#if (C_SAL_OS_ENABLE_LOG == 1)
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
#define M_SAL_OS_LOG_BUFF(x_pcBuffName, x_pucBuff, x_u16BuffSize)\
  (M_SAL_LOG_BUFF(C_SAL_OS_MODULE_NAME, x_pcBuffName, x_pucBuff, x_u16BuffSize))
#else
#define M_SAL_OS_LOG_BUFF(x_pcBuffName, x_pucBuff, x_u16BuffSize) {}
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
 * @brief  implement salTimeGetRelative
 *
 */
K_SAL_API TKSalMsTime salTimeGetRelative
(
  void
)
{
  TKSalMsTime     time = 0;
  struct timespec ts;

  M_SAL_OS_LOG_VAR("Start of %s", __func__);

  for (;;)
  {
    if (0 != clock_gettime(CLOCK_MONOTONIC, &ts))
    {
      M_SAL_OS_LOG("ERROR: clock_gettime has failed");
      break;
    }
    time  = (TKSalMsTime)(ts.tv_sec * 1000);
    time += (TKSalMsTime)(ts.tv_nsec / 1000000);
    break;
  }

  M_SAL_OS_LOG_VAR("End of %s", __func__);

  return time;
}

/**
 * @brief  implement salTimeMilliSleep
 *
 */
void salTimeMilliSleep
(
  const TKSalMsTime xWaitTime
)
{
  nm_sleep(xWaitTime);
}

/**
 * @brief  implement kta_pSalMemoryAllocate
 *
 */
void* kta_pSalMemoryAllocate
(
  const size_t   xSize
)
{
  void* pBlock = NULL;

  M_SAL_OS_LOG_VAR("Start of %s", __func__);

  for (;;)
  {
    if (0U == xSize)
    {
      M_SAL_OS_LOG("ERROR: Memory size is equal to 0");
      break;
    }

    pBlock = malloc(xSize);
    break;
  }

  M_SAL_OS_LOG_VAR("End of %s", __func__);

  return pBlock;
}

/**
 * @brief  implement kta_pSalMemoryReallocate
 *
 */
void* kta_pSalMemoryReallocate
(
  void*         xpBlock,
  const size_t  xNewSize
)
{
  void* pBlock = xpBlock;

  M_SAL_OS_LOG_VAR("Start of %s", __func__);

  for (;;)
  {
    /* If xpBlock is NULL and xNewSize is not NULL, we consider that
     * a new block must be allocatted. Use case of test
     * salOsTestAllocationReallocNullStd.
     */
    if ((NULL == xpBlock) && (0U != xNewSize))
    {
      pBlock = malloc(xNewSize);
      break;
    }

    if (NULL == xpBlock)
    {
      M_SAL_OS_LOG("ERROR: Invalid memory block");
      break;
    }

    if (0U == xNewSize)
    {
      M_SAL_OS_LOG("ERROR: Invalid size of memory block, equal to 0");
      salMemoryFree(xpBlock);
      pBlock = NULL;
      break;
    }

    pBlock = realloc(xpBlock, xNewSize);
    break;
  }

  M_SAL_OS_LOG_VAR("End of %s", __func__);

  return pBlock;
}

/**
 * @brief  implement salMemoryFree
 *
 */
void salMemoryFree
(
  void*   xpBlock
)
{
  M_SAL_OS_LOG_VAR("Start of %s", __func__);

  for (;;)
  {
    if (NULL == xpBlock)
    {
      M_SAL_OS_LOG("ERROR: Invalid memory block");
      break;
    }

    free(xpBlock);
    break;
  }

  M_SAL_OS_LOG_VAR("End of %s", __func__);
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

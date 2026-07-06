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
/** \brief  SAL for operating system - Baremetal implementation.
 *
 *  \author Kudelski IoT
 *
 *  \date 2025/01/08
 *
 *  \file k_sal_os.c
 *
 *  This is a template for baremetal implementation. Users must provide
 *  their own hardware timer functions.
 ******************************************************************************/
/**
 * @brief SAL for operating system - Baremetal implementation.
 */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_os.h"

/* -------------------------------------------------------------------------- */
/* PLATFORM-SPECIFIC IMPORTS                                                  */
/* -------------------------------------------------------------------------- */
/* TODO: Include your platform-specific timer/delay headers here
 * Examples:
 * - #include "hal_timer.h"
 * - #include "system_timer.h"
 * - #include "hardware_delay.h"
 */

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS                                                            */
/* -------------------------------------------------------------------------- */

/* TODO: Define your hardware timer constants
 * Examples:
 * - #define C_SAL_OS_TIMER_FREQ_HZ    (1000000U)  // 1MHz timer
 * - #define C_SAL_OS_CYCLES_PER_MS    (F_CPU / 1000U)
 */

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Get current tick count in milliseconds.
 *
 * @return Current tick count.
 *
 * TODO: Implement this function using your hardware timer
 */
static uint32_t lGetTickMs(void);

/**
 * @brief Busy-wait delay implementation.
 *
 * @param[in] xDelayMs Delay in milliseconds.
 *
 * TODO: Implement this function using cycle counting or hardware timer
 */
static void lBusyWaitMs
(
  uint32_t xDelayMs
);

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Sleep for specified number of milliseconds using baremetal delay.
 *
 * @param[in] xWaitTime
 *   Time to sleep in milliseconds.
 *
 * NOTE: This is a template implementation. Users must customize based on
 *       their hardware platform's timer/delay capabilities.
 */
void salTimeMilliSleep
(
  const TKSalMsTime xWaitTime
)
{
  if (0U != xWaitTime)
  {
#if defined(USE_HARDWARE_TIMER)
    /* Option 1: Use hardware timer for accurate delays */
    uint32_t xStartTick = lGetTickMs();
    uint32_t xCurrentTick = 0U;
    
    while (((xCurrentTick = lGetTickMs()) - xStartTick) < xWaitTime)
    {
      /* Busy wait or enter low-power mode */
      /* TODO: Add platform-specific low-power mode entry if available */
    }
#else
    /* Option 2: Use busy-wait implementation */
    lBusyWaitMs(xWaitTime);
#endif
  }
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Get current tick count in milliseconds.
 *
 * @return Current tick count.
 */
static uint32_t lGetTickMs(void)
{
  uint32_t xTickMs = 0U;
  
  /* TODO: Implement using your hardware timer
   * Examples:
   * - xTickMs = HAL_GetTick();
   * - xTickMs = SystemTimer_GetMs();
   * - xTickMs = (TIM2->CNT / (F_CPU / 1000U));
   */
  
  return xTickMs;
}

/**
 * @brief Busy-wait delay implementation.
 *
 * @param[in] xDelayMs Delay in milliseconds.
 */
static void lBusyWaitMs
(
  uint32_t xDelayMs
)
{
  /* TODO: Implement cycle-accurate delay
   * Example for ARM Cortex-M with known CPU frequency:
   *
   * volatile uint32_t xCycles = (F_CPU / 1000U) * xDelayMs / 3U;
   * while (xCycles > 0U)
   * {
   *   xCycles--;
   * }
   */
  
  /* Simple fallback (VERY INACCURATE - for reference only) */
  volatile uint32_t i = 0U;
  volatile uint32_t j = 0U;
  
  for (i = 0U; i < xDelayMs; i++)
  {
    for (j = 0U; j < 1000U; j++)
    {
      /* Busy wait */
    }
  }
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

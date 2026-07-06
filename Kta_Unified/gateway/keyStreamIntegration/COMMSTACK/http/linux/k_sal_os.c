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
/** \brief  SAL for operating system - Linux implementation.
 *
 *  \author Kudelski IoT
 *
 *  \date 2025/01/08
 *
 *  \file k_sal_os.c
 ******************************************************************************/
/**
 * @brief SAL for operating system - Linux implementation.
 */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_os.h"
#include <unistd.h>
#include <time.h>

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS                                                            */
/* -------------------------------------------------------------------------- */

/** @brief Nanoseconds per millisecond */
#define C_SAL_OS_NSEC_PER_MSEC    (1000000U)

/** @brief Microseconds per millisecond */
#define C_SAL_OS_USEC_PER_MSEC    (1000U)

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Sleep for specified number of milliseconds using POSIX nanosleep.
 *
 * @param[in] xWaitTime
 *   Time to sleep in milliseconds.
 */
void salTimeMilliSleep
(
  const TKSalMsTime xWaitTime
)
{
  if (0U != xWaitTime)
  {
    struct timespec xReq;
    struct timespec xRem;
    
    xReq.tv_sec = (time_t)(xWaitTime / 1000U);
    xReq.tv_nsec = (long)((xWaitTime % 1000U) * C_SAL_OS_NSEC_PER_MSEC);
    
    /* Use nanosleep which can be interrupted by signals */
    while (0 != nanosleep(&xReq, &xRem))
    {
      /* If interrupted, continue sleeping for remaining time */
      xReq = xRem;
    }
  }
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

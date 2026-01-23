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
/** \brief  SAL for operating system.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file k_sal_os.h
 ******************************************************************************/
/**
 * @brief SAL for operating system.
 */

#ifndef K_SAL_OS_H
#define K_SAL_OS_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_comm_defs.h"
#ifdef SAL_OS_WRAP
#include "sal_os_wrap.h"
#endif /* SAL_OS_WRAP */

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
 *   Retrieve a relative time in milliseconds.
 *   Can be based on an internal scheduler tick.
 *   Although the returned value is expressed in milliseconds, a granularity of
 *   10 ms is enough.
 *
 * @return
 *   Relative time expressed in milliseconds.
 */
K_SAL_API TKSalMsTime salTimeGetRelative
(
  void
);


/**
 * @brief
 *   Suspend the processing for a minimum amount of time.
 *   This is a blocking function.
 *   The wait time is expressed in milliseconds.
 *   The actual wait time shall be at least the given time, but it can be
 *   longer, depending on the underlying primitives.
 *
 * @param[in]  xWaitTime
 *   Wait time in milliseconds.
 */
K_SAL_API void salTimeMilliSleep
(
  const TKSalMsTime  xWaitTime
);

/******************************************************************************/
/*                             heap memory                                    */
/******************************************************************************/

/**
 * @brief
 *   Allocate a block of memory. The allocated memory can be taken either
 *   from the main memory heap or from a dedicated memory pool.
 *
 * @param[in] xSize
 *   Size in bytes to allocate;
 *   Should not be 0.
 *
 * @return
 *   Pointer to the allocated memory block if successful, NULL otherwise.
 */
K_SAL_API void* kta_pSalMemoryAllocate
(
  const size_t  xSize
);

/**
 * @brief
 *   Resize a memory block, potentially moving it.
 *
 * @param[in] xpBlock
 *   Original memory block to resize.
 * @param[in] xNewSize
 *   New size of the memory block; may be 0 to free the block.
 *
 * @return
 *   Pointer to the reallocated memory block if successful, NULL otherwise
 *   (in that case, the original memory block should be preserved). May be the
 *   same pointer as xpBlock.
 */
K_SAL_API void* pSalMemoryReallocate
(
  void*         xpBlock,
  const size_t  xNewSize
);

/**
 * @brief
 *   Free a memory block previously allocated.
 *
 * @param[in]  xpBlock
 *   Memory block to free;
 *   Should not be NULL.
 */
K_SAL_API void salMemoryFree
(
  void*  xpBlock
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // K_SAL_OS_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************
* (c) 2023-2025 Nagravision Sarl
********************************************************************************/
/** \brief Windows SAL log backend for the KTA gateway.
 *
 *  The KTA log module (KTALog.c) routes every formatted log line through
 *  salPrint(). On the MCU this writes to SYS_CONSOLE/UART; on the Windows
 *  gateway we simply forward to stdout so KTA logs appear alongside the
 *  gateway's data dumps when LOG_KTA_ENABLE is set in ktaConfig.h.
 *
 *  \file k_sal_log_win.c
 ******************************************************************************/

#include "k_sal_log.h"
#include <stdio.h>

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

void salPrint
(
  const char* xpBuffer
)
{
  if (NULL != xpBuffer)
  {
    (void)fputs(xpBuffer, stdout);
    (void)fflush(stdout);
  }
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

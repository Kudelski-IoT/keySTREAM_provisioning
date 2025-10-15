/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2025 Nagravision SÃ rl

* Subject to your compliance with these terms, you may use the Nagravision SÃ rl
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
/** \brief keySTREAM Trusted Agent - Log module.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file KTALog.c
 ******************************************************************************/
/**
 * @brief keySTREAM Trusted Agent Log module.
 */

#include "KTALog.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "k_sal_log.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Maximum buffer size.
 * NOTE : Don't reduce this value below 100.
 */
#define C_MAX_BUFFER_SIZE (250u)

/** @brief Maximum bytes to display per line. */
#define C_LOG_COL_SIZE (32u)

/** @brief Maximum module name size. */
#define C_MAX_MODULE_NAME_SIZE (15u)

/** @brief Maximum value size. */
#define C_MAX_VALUE_SIZE (4u)

/** @brief Maximum Modules for log */
#define C_MAX_MODULES (17u)

/** @brief MAximum Log Levels */
#define C_MAX_LOG_LEVELS (6u)

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */

/** @brief Module level config structure. */
typedef struct {
  char aModuleName[C_MAX_MODULE_NAME_SIZE];
  int level;
} moduleLevelConfig;

/* Module level configs for each module. */
static moduleLevelConfig gaLogConfigs[C_MAX_MODULES] = {
  {"KTAMGR", LOG_KTA_ENABLE},
  {"KTACONFIG", LOG_KTA_ENABLE},
  {"KTAGENERAL", LOG_KTA_ENABLE},
  {"KTACMDHANDLER", LOG_KTA_ENABLE},
  {"KTAREGHANDLER", LOG_KTA_ENABLE},
  {"KTAACTHANDLER", LOG_KTA_ENABLE},
  {"KTACRYPTO", LOG_KTA_ENABLE},
  {"KTAICPPPARSER", LOG_KTA_ENABLE},
  {"SALSTORAGE", LOG_KTA_ENABLE},
  {"SALCRYPTO", LOG_KTA_ENABLE},
  {"SALOBJECT", LOG_KTA_ENABLE},
  {"SALTHIRDPARTY", LOG_KTA_ENABLE},
  {"SALFOTA", LOG_KTA_ENABLE},
  {"FOTAAGENT", LOG_KTA_ENABLE},
  {"FOTAPROCESS", LOG_KTA_ENABLE},
  {"FOTAPLATFORM", LOG_KTA_ENABLE},
  {"KSALFOTASTORAGE", LOG_KTA_ENABLE}
};

/** @brief Log event info structure. */
typedef struct {
  va_list ap;
  const char *pFmt;
  const char *pFile;
  const char *pFunc;
  struct tm *pTime;
  int line;
  int level;
} logEvent;

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */
/**
 * @brief
 *   Prepares the buffer with log information and sends it to salPrint
 *   for console output.
 *   This function is responsible for preparing the log buffer
 *   based on the provided logEvent structure.
 *
 * @param[in] ev
 *   Pointer to the logEvent structure that holds the basic information to log.
 */
static void logPrepare
(
  logEvent *xpEv
);

/**
 * @brief
 *   Initializes the log event by capturing the start time.
 *   This function is intended for internal use by the log API.
 *   It captures the start time of the log event.
 *
 * @param[in] ev
 *   Pointer to the logEvent structure representing the log event.
 */
static void initEvent
(
  logEvent *xpEvent
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief implement ktaLog_Fct
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_010 : misra_c2012_rule_8.7_violation
 * Function ktaLog_Fct is defined with external linkage to provide a public API for other modules.
 * It is referenced in the header file for external use, even if not called within this source file.
 */
void ktaLog_Fct
(
  int xLevel,
  const char* xpModuleName,
  const char* xpFile,
  const char* xpFunc,
  int         xLine,
  const char* xpFmt,
  ...
)
{
  /* To fix the misra-c2012-15.5-A function should have a single point of exit at the end*/
  bool bretvalue = true;

  if ((xLevel < 0) || (xLevel > 5)) 
  {
    bretvalue = false;
  }

  if ( bretvalue )
  {
      logEvent ev = {
      .pFmt   = xpFmt,
      .pFile  = xpFile, /* Name of the file from which log is triggered. */
      .pFunc = xpFunc,  /* Name of the function. */
      .line  = xLine,   /* At what line number in the file error occured. */
      .level = xLevel,  /**
                        * Decides whether it is debug, or trace or other levels,
                        * keep 0 for debug and so on
                        */
    };
    int moduleLogLevel = E_KTALOG_LEVEL_DEBUG;

    for (int i = 0; i < (sizeof(gaLogConfigs) / sizeof(gaLogConfigs[0])); i++)
    {
      if (strncmp(gaLogConfigs[i].aModuleName, xpModuleName,
          strlen(gaLogConfigs[i].aModuleName)) == 0)
      {
        moduleLogLevel = gaLogConfigs[i].level;
        break;
      }
    }

    if (xLevel >= moduleLogLevel)
    {
      initEvent(&ev);
      va_start(ev.ap, xpFmt);
      logPrepare(&ev);
      va_end(ev.ap);
    }
 }

  return;
}

/**
 * @brief implement ktaLog_PrintBuffer
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_002 : misra_c2012_rule_17.7_violation
 * Not using the return value of snprintf
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_010 : misra_c2012_rule_8.7_violation
 * Function ktaLog_PrintBuffer is defined with external linkage to provide a public API for other modules.
 * It is referenced in the header file for external use, even if not called within this source file.
 */
void ktaLog_PrintBuffer
(
  int             xLevel,
  const char      xaModuleName[],
  const char*     xpFile,
  const char*     xpFunc,
  int             xLine,
  const char*     xpFmt,
  const uint8_t*  xpBuffer,
  int             xSize
)
{
  M_UNUSED(xpFile);
  M_UNUSED(xpFunc);
  M_UNUSED(xLine);

  int Index = 0;
  char aValue[C_MAX_VALUE_SIZE] = {0};
  char aBuffer[C_MAX_BUFFER_SIZE] = {0};
  int moduleLogLevel = E_KTALOG_LEVEL_DEBUG;

  for (int i = 0; i < (sizeof(gaLogConfigs) / sizeof(gaLogConfigs[0])); i++)
  {
    if (strncmp(gaLogConfigs[i].aModuleName, xaModuleName,
        strlen(gaLogConfigs[i].aModuleName)) == 0)
    {
      moduleLogLevel = gaLogConfigs[i].level;
      break;
    }
  }

  if (xLevel >= moduleLogLevel)
  {
    snprintf(aBuffer, C_MAX_BUFFER_SIZE, "%s : [%d]\r\n", xpFmt, xSize);
    salPrint(aBuffer);

    for (Index = 0; Index < xSize; Index++)
    {
      snprintf(aValue, C_MAX_VALUE_SIZE, "%02X ", xpBuffer[Index]);
      salPrint(aValue);
      if ((Index % (int32_t)C_LOG_COL_SIZE) == (C_LOG_COL_SIZE - 1u))
      {
        /* Line full. */
        salPrint("\r\n");
      }
    }
    if ((xSize % (int32_t)C_LOG_COL_SIZE) != 0)
    {
      /* Last line not full. */
      salPrint("\r\n");
    }
    salPrint("\r\n");
  }
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */
/**
 * @implements logPrepare
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_002 : misra_c2012_rule_17.7_violation
 * Not using the return value of snprintf
 */
static void logPrepare(logEvent* xpEv)
{
  const char * const gapLevelStrings[C_MAX_LOG_LEVELS] = {
      "NONE",
      "DEBUG",
      "INFO",
      "WARNING",
      "ENTRY_EXIT",
      "ERROR"
  };

  char aBuffer[C_MAX_BUFFER_SIZE + 1] = {0};
  size_t buf_len;

  /* Write prefix into the buffer */
  (void)snprintf(aBuffer,
                 C_MAX_BUFFER_SIZE,
                 "[%s] [%s:%d] ",
                 gapLevelStrings[xpEv->level],
                 xpEv->pFunc,
                 xpEv->line);

  /* Now measure the current length */
  buf_len = strlen(aBuffer);

  if (buf_len < C_MAX_BUFFER_SIZE)
  {
    va_list args;
    va_copy(args, xpEv->ap);

    (void)vsnprintf(&aBuffer[buf_len],
                    C_MAX_BUFFER_SIZE - buf_len,
                    xpEv->pFmt,
                    args);

    va_end(args);
  }

  salPrint(aBuffer);
  salPrint("\r\n");
}

/**
 * @implements initEvent
 *
 */
/**
 * SUPPRESS: misra_c2012_rule_21.10_violation
 * Use of time library for logging purpose.
 */
static void initEvent
(
  logEvent *xpEvent
)
{
  if (xpEvent->pTime == NULL) {
    time_t startTime = time(NULL);

    xpEvent->pTime = localtime(&startTime);
  }
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

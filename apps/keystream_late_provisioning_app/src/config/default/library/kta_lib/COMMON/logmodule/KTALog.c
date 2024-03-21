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

#if LOG_KTA_ENABLE
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

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */

/** @brief Module level config structure. */
typedef struct {
  char aModuleName[C_MAX_MODULE_NAME_SIZE];
  int level;
} moduleLevelConfig;

/* Module level configs for each module. */
static moduleLevelConfig gaLogConfigs[] = {
  {"KTAMGR", E_KTALOG_LEVEL_ERROR},
  {"KTACONFIG", E_KTALOG_LEVEL_ERROR},
  {"KTAGENERAL", E_KTALOG_LEVEL_ERROR},
  {"KTACMDHANDLER", E_KTALOG_LEVEL_ERROR},
  {"KTAREGHANDLER", E_KTALOG_LEVEL_ERROR},
  {"KTAACTHANDLER", E_KTALOG_LEVEL_ERROR},
  {"KTACRYPTO", E_KTALOG_LEVEL_ERROR},
  {"KTAICPPPARSER", E_KTALOG_LEVEL_ERROR},
  {"SALSTORAGE", E_KTALOG_LEVEL_ERROR},
  {"SALCRYPTO", E_KTALOG_LEVEL_ERROR},
  {"SALOBJECT", E_KTALOG_LEVEL_ERROR},
  {"SALTHIRDPARTY", E_KTALOG_LEVEL_ERROR}
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

typedef void (*log_LogFn)(logEvent *ev);

/* Log levels */
static const char *gapLevelStrings[] = {
                                      "DEBUG",
                                      "INFO",
                                      "WARNING",
                                      "ENTRY_EXIT",
                                      "ERROR"
};

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
  logEvent *ev
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
  logEvent *ev
);

/**
 * @brief
 *   Enables locking on the log API to prevent data corruption from parallel calls.
 *
 *   This function is responsible for enabling a lock mechanism on the log API to ensure that
 *   data corruption does not occur when multiple calls are made in parallel.
 */
static void lock
(
  void
);

/**
 * @brief
 *   Makes the log API available for use.
 *
 *   This function is responsible for enabling the log API after it has been locked or disabled.
 *   Once called, the log API becomes accessible for use.
 */
static void unlock
(
  void
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
  if (xLevel < 0u || xLevel > 4u) {
    return;
  }
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

  for (int i = 0; i < sizeof(gaLogConfigs) / sizeof(gaLogConfigs[0]); i++)
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
    lock();
    initEvent(&ev);
    va_start(ev.ap, xpFmt);
    logPrepare(&ev);
    va_end(ev.ap);
    unlock();
  }
}

/**
 * @brief implement ktaLog_PrintBuffer
 *
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
  int  Index = 0;
  char aValue[C_MAX_VALUE_SIZE] = {0};
  char aBuffer[C_MAX_BUFFER_SIZE] = {0};
  int moduleLogLevel = E_KTALOG_LEVEL_ERROR;

  for (int i = 0; i < sizeof(gaLogConfigs) / sizeof(gaLogConfigs[0]); i++)
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
      if ((Index % C_LOG_COL_SIZE) == (C_LOG_COL_SIZE - 1))
      {
        /* Line full. */
        salPrint("\r\n");
      }
    }
    if ((xSize % C_LOG_COL_SIZE) != 0)
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
static void logPrepare
(
  logEvent* xpEv
)
{
  char aBuffer[C_MAX_BUFFER_SIZE + 1] = {0};

  snprintf(aBuffer,
           C_MAX_BUFFER_SIZE,
           "[%s] [%s:%d] ",
           gapLevelStrings[xpEv->level],
           xpEv->pFunc,
           xpEv->line);
  if (strlen(aBuffer) < C_MAX_BUFFER_SIZE)
  {
    vsnprintf(aBuffer + strlen(aBuffer),
               C_MAX_BUFFER_SIZE - strlen(aBuffer),
               xpEv->pFmt,
               xpEv->ap);
  }
  salPrint(aBuffer);
  salPrint("\r\n");
}

/**
 * @implements initEvent
 *
 */
static void initEvent
(
  logEvent *xpEvent
)
{
  if (!xpEvent->pTime) {
    time_t startTime = time(NULL);

    xpEvent->pTime = localtime(&startTime);
  }
}

/**
 * @implements lock
 *
 */
static void lock(void)
{
}

/**
 * @implements unlock
 *
 */
static void unlock(void)
{
}

#endif /* LOG_KTA_ENABLE */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

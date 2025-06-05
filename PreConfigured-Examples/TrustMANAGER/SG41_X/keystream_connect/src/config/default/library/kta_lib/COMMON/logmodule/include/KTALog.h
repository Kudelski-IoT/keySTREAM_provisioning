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
 *  \file KTALog.h
 ******************************************************************************/
/**
 * @brief keySTREAM Trusted Agent Log module.
 */

#ifndef KTALOG_H
#define KTALOG_H

#ifdef __cplusplus
extern "C" {
#endif /* C++ */
/* --------------------------------------------------------------------------------------------- */
/* IMPORTS                                                                                       */
/* --------------------------------------------------------------------------------------------- */
#include <stdint.h>

/* --------------------------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                                        */
/* --------------------------------------------------------------------------------------------- */

/** @brief Set an argument/return value as unused */
#define M_UNUSED(xArg)            (void)(xArg)

#if !defined(LOG_KTA_ENABLE)
/** @brief keySTREAM Trusted Agent log level. */
#define LOG_KTA_ENABLE E_KTALOG_LEVEL_NONE
#endif

/** @brief keySTREAM Trusted Agent log level start. */
#define M_KTALOG__START(...)          \
    ktaLog_Fct(E_KTALOG_LEVEL_ENTRY_EXIT, gpModuleName, __FILE__, __func__, __LINE__, __VA_ARGS__)
/** @brief keySTREAM Trusted Agent log level end. */
#define M_KTALOG__END(...)            \
    ktaLog_Fct(E_KTALOG_LEVEL_ENTRY_EXIT, gpModuleName, __FILE__, __func__, __LINE__, __VA_ARGS__)
/** @brief keySTREAM Trusted Agent log level error. */
#define M_KTALOG__ERR(...)             \
    ktaLog_Fct(E_KTALOG_LEVEL_ERROR, gpModuleName, __FILE__, __func__, __LINE__, __VA_ARGS__)
/** @brief keySTREAM Trusted Agent log level warning. */
#define M_KTALOG__WARN(...)              \
    ktaLog_Fct(E_KTALOG_LEVEL_WARN, gpModuleName, __FILE__, __func__, __LINE__, __VA_ARGS__)
/** @brief keySTREAM Trusted Agent log level info. */
#define M_KTALOG__INFO(...)              \
    ktaLog_Fct(E_KTALOG_LEVEL_INFO, gpModuleName, __FILE__, __func__, __LINE__, __VA_ARGS__)
/** @brief keySTREAM Trusted Agent log level debug. */
#define M_KTALOG__DEBUG(...)             \
    ktaLog_Fct(E_KTALOG_LEVEL_DEBUG, gpModuleName, __FILE__, __func__, __LINE__, __VA_ARGS__)
/** @brief keySTREAM Trusted Agent log in hex format. */
#define M_KTALOG__HEX(format, x_buffer, size)  \
    ktaLog_PrintBuffer(E_KTALOG_LEVEL_DEBUG, gpModuleName, __FILE__, \
                   __func__, __LINE__, format, x_buffer, size)

/* --------------------------------------------------------------------------------------------- */
/* VARIABLES                                                                                     */
/* --------------------------------------------------------------------------------------------- */
/** @brief Supported log levels enum. */
enum {
  E_KTALOG_LEVEL_DEBUG = 1,
  /* Debug level prints */
  E_KTALOG_LEVEL_INFO,
  /* Info level prints */
  E_KTALOG_LEVEL_WARN,
  /* Warning level prints */
  E_KTALOG_LEVEL_ENTRY_EXIT,
  /* Entry/Exit level prints */
  E_KTALOG_LEVEL_ERROR,
  /* Error level prints */
  E_KTALOG_LEVEL_NONE
  /* No log prints */
};

/* --------------------------------------------------------------------------------------------- */
/* FUNCTIONS                                                                                     */
/* --------------------------------------------------------------------------------------------- */
/**
 * @brief
 *   Log API called every time a log event is generated.
 *
 *   This API is responsible for handling log events and printing log
 *   information based on the provided parameters.
 *
 * @param[in] xLevel
 *   The log level of the event.
 * @param[in] xpModuleName
 *   The name of the module generating the log event.
 * @param[in] xpFile
 *   The name of the source file where the log event occurred.
 * @param[in] xpFunc
 *   The name of the function where the log event occurred.
 * @param[in] xLine
 *   The line number in the source file where the log event occurred.
 * @param[in] xpFmt
 *   The format string for the log message.
 * @param[in] ...
 *   Additional arguments for the format string.
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
);

/**
 * @brief
 *   Log API called every time a log event is generated.
 *
 *   This function is responsible for handling log events and printing
 *   log data in hex value based on the provided parameters.
 *
 * @param[in] xLevel
 *   The log level of the event.
 * @param[in] xaModuleName
 *   The name of the module generating the log event.
 * @param[in] xpFile
 *   The name of the source file where the log event occurred.
 * @param[in] xpFunc
 *   The name of the function where the log event occurred.
 * @param[in] xLine
 *   The line number in the source file where the log event occurred.
 * @param[in] xpFmt
 *   The format string for the log message.
 * @param[in] xpBuffer
 *   Pointer to the buffer containing log data.
 * @param[in] xSize
 *   The size of the buffer.
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
);

#ifdef __cplusplus
}
#endif /* C++ */

#endif // KTALOG_H

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

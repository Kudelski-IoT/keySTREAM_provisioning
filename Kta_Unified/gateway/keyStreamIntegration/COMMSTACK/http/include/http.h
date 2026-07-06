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
/** \brief  HTTP - Type definations.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file http.h
 ******************************************************************************/
/**
 * @brief HTTP Type definations.
 */

#ifndef HTTP_H
#define HTTP_H

/* --------------------------------------------------------------------------------------------- */
/* IMPORTS                                                                                       */
/* --------------------------------------------------------------------------------------------- */
#include "k_sal_com.h"

/* --------------------------------------------------------------------------------------------- */
/* CONSTANTS, TYPES, ENUM                                                                        */
/* --------------------------------------------------------------------------------------------- */

/** @brief HTTP boolean types. */
#define C_HTTP__TRUE                  (1u)
#define C_HTTP__FALSE                 (0u)

/** @brief HTTP header field size. */
#define C_HTTP__HEADER_FIELD_SIZE     (64u)

typedef uint8_t BOOL;

/** @brief Structure to store HTTP header data. */
typedef struct
{
  char  method[8];
  int   status;
  char  contentType[C_HTTP__HEADER_FIELD_SIZE];
  long  contentLength;
  BOOL  chunked;
  BOOL  close;
  char  location[C_HTTP__HEADER_FIELD_SIZE];
  char  cookie[C_HTTP__HEADER_FIELD_SIZE];
} TKHttpHeader;

/** @brief Structure to store HTTP url. */
typedef struct
{
  uint8_t  host[256];
  uint8_t  port[8];
  uint8_t  path[C_HTTP__HEADER_FIELD_SIZE];
} TKHttpUrl;

/** @brief Structure to store HTTP information. */
typedef struct
{
  TKHttpUrl     url;
  TKHttpHeader  request;
  TKHttpHeader  response;
  void*         pTls;
  long          length;
  char*         pRecvBuf;
  size_t        recvLen;
  BOOL          headerEnd;
  uint8_t*      body;
  long          bodySize;
  long          bodyLen;
} TKHttpInfo;

/* --------------------------------------------------------------------------------------------- */
/* VARIABLES                                                                                     */
/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/* FUNCTIONS                                                                                     */
/* --------------------------------------------------------------------------------------------- */

#endif // HTTP_H

/* --------------------------------------------------------------------------------------------- */
/* END OF FILE                                                                                   */
/* --------------------------------------------------------------------------------------------- */


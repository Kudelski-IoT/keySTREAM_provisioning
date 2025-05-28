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
/** \brief HTTP Client code.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/02
 *
 *  \file http.c
 *
 ******************************************************************************/
/**
 * @brief HTTP Client code.
 */

#include "http.h"
/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */
#include "http_if.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */
/** @brief Maximal HTTP data length. */
#define C_HTTP_MAX_DATA_LEN          (2560u)
/** @brief HTTP read timeout in miliseconds. */
#define C_HTTP_READ_TIMEOUT_IN_MS    (10000u)
/** @brief HTTP connection timeout in miliseconds. */
#define C_HTTP_CONNECT_TIMEOUT_IN_MS (5000u)
/** @brief HTTP success code. */
#define C_HTTP_SUCCESS_STATUS_CODE   (200u)
/** @brief HTTP token max len. */
#define C_HTTP_TOKEN_MAX_LEN   (256u)

/******************************************************************************/
/* LOCAL MACROS                                                               */
/******************************************************************************/

#ifdef DEBUG
/** @brief Enable HTTP logs. */
#define M_INTL_HTTP_DEBUG(__PRINT__)   do { \
                                          printf("HTTP %d>", __LINE__); \
                                          printf __PRINT__; \
                                          printf("\r\n"); \
                                        } while (0)
#define M_INTL_HTTP_ERROR(__PRINT__)   do { \
                                          printf("HTTP %d> ERROR ", __LINE__); \
                                          printf __PRINT__; \
                                          printf("\r\n"); \
                                        } while (0)
#else
#define M_INTL_HTTP_DEBUG(__PRINT__)
#define M_INTL_HTTP_ERROR(__PRINT__)
#endif /* DEBUG. */

/**
 * @brief Set an argument/return value as unused.
 *
 * @param[in] xArg
 *
 */
#define M_UNUSED(xArg)            (void)(xArg)

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */

static TKHttpInfo gHttpInfo = {0};

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief
 *   Extract the token one by one.
 *
 * @param[in] xpSrc
 *   HTTP response; should not be NULL. It must have '\0' character at the end.
 * @param[out] xpDst
 *   Extracted token.
 * @param[in] xSize
 *   HTTP response size.
 *
 * @return
 * -
 */
static char* pStrToken
(
  char*  xpSrc,
  char*  xpDst,
  int    xSize
);

/**
 * @brief
 *   Extract the HTTP header.
 *
 * @param[in] xpHttpInfo
 *   Structure to store HTTP information.
 * @param[in] xpParam
 *   HTTP response. Should not be NULL.
 *
 * @return
 * - 0, in case of success.
 * - -1, in case of error.
 */
static int httpHeader
(
  TKHttpInfo*  xpHttpInfo,
  char*        xpParam
);

/**
 * @brief
 *   This API fetches the HTTP body and status code.
 *
 * @param[in] xpHttpInfo
 *   Structure with HTTP information.
 *
 * @return
 * - 0, in case of success.
 * - -1, in case of error.
 */
static int httpParse
(
  TKHttpInfo*  xpHttpInfo
);

/**
 * @brief
 *   To send the HTTP post request.
 *
 * @param[in] xpHttpInfo
 *   Structure with HTTP information.
 * @param[in] xpDir
 * @param[in] xpData
 * @param[in] xDataLen
 * @param[in] xpResponse
 *   HTTP response body.
 * @param[in] xSize
 *   HTTP reponse body size.
 *
 * @return
 * - 0, in case of success.
 * - -1, in case of error.
 */
static int httpPost
(
  TKHttpInfo*     xpHttpInfo,
  const uint8_t*  xpDir,
  const uint8_t*  xpData,
  size_t          xDataLen,
  uint8_t*        xpResponse,
  size_t          xSize
);

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief  implement httpInit
 *
 */
/**
 * SUPPRESS: MISRA_DEV_KTA_002 : misra_c2012_rule_17.7_violation
 * Not using the return value of non-void functions
 */
TCommIfStatus httpInit
(
  const TCommIfIpProtocol  xIpProtocol,
  const uint8_t*           xpUri,
  const uint8_t*           xpHost,
  const uint16_t           xPort
)
{
  TKCommStatus status = E_COMM_IF_STATUS_ERROR;
  const uint8_t* pHost = NULL;
  int retVal;

  M_UNUSED(xIpProtocol);
  M_INTL_HTTP_DEBUG(("Start of %s", __func__));

  if ((NULL == xpUri) || (NULL == xpHost) || (0U == xPort))
  {
    M_INTL_HTTP_ERROR(("Invalid Parameter"));
    status = E_COMM_IF_STATUS_PARAMETER;
  }
  else
  {
    retVal = strncmp((const char*)xpHost, "http://", 7);
    if (retVal == 0)
    {
      pHost = &xpHost[7];
    }
    else
    {
      pHost = &xpHost[0];
    }
    (void)memset(&gHttpInfo, 0, sizeof(gHttpInfo));
    (void)strncpy((char*)gHttpInfo.url.host, (const char*)pHost, sizeof(gHttpInfo.url.host)-1UL);
    (void)snprintf((char*)gHttpInfo.url.port, sizeof(gHttpInfo.url.port), "%d", xPort);
    (void)strncpy((char*)gHttpInfo.url.path, (const char*)xpUri, sizeof(gHttpInfo.url.path)-1UL);

    status = salComInit(C_HTTP_CONNECT_TIMEOUT_IN_MS,
                        C_HTTP_READ_TIMEOUT_IN_MS,
                        &gHttpInfo.pTls);
    if (status == (TKCommStatus)E_COMM_IF_STATUS_OK)
    {
      status = salComConnect(gHttpInfo.pTls, gHttpInfo.url.host, gHttpInfo.url.port);
      if ((TKCommStatus)E_COMM_IF_STATUS_OK != status)
      {
        M_INTL_HTTP_ERROR(("salComConnect Failed"));
      }
    }
  }

  M_INTL_HTTP_DEBUG(("End of %s", __func__));
  return (TCommIfStatus)status;
}

/**
 * @brief  implement httpMsgExchange
 *
 */
TCommIfStatus httpMsgExchange
(
  const uint8_t*  xpMsgToSend,
  const size_t    xSendSize,
  uint8_t*        xpRecvMsgBuffer,
  size_t*         xpRecvMsgBufferSize
)
{
  TCommIfStatus status = E_COMM_IF_STATUS_ERROR;
  int ret = 0;

  M_INTL_HTTP_DEBUG(("Start of %s", __func__));

  if ((NULL == xpMsgToSend) ||
      (0UL == xSendSize) ||
      (NULL == xpRecvMsgBuffer) ||
      (NULL == xpRecvMsgBufferSize) ||
      (0u == *xpRecvMsgBufferSize))
  {
    M_INTL_HTTP_ERROR(("Invalid Parameter"));
    status = E_COMM_IF_STATUS_PARAMETER;
  }
  else
  {
    ret = httpPost(&gHttpInfo,
                    gHttpInfo.url.path,
                    xpMsgToSend,
                    xSendSize,
                    xpRecvMsgBuffer,
                    *xpRecvMsgBufferSize);
    M_INTL_HTTP_DEBUG(("httpPost %d", ret));
    if (ret == C_HTTP_SUCCESS_STATUS_CODE)
    {
      status = E_COMM_IF_STATUS_OK;
      M_INTL_HTTP_DEBUG(("Received Data %ld", gHttpInfo.bodyLen));
      *xpRecvMsgBufferSize = (size_t)gHttpInfo.bodyLen;
    }
  }

  M_INTL_HTTP_DEBUG(("End of %s", __func__));
  return status;
}

/**
 * @brief  implement httpTerm
 *
 */
TCommIfStatus httpTerm(void)
{
  TKCommStatus status = E_COMM_IF_STATUS_ERROR;

  M_INTL_HTTP_DEBUG(("Start of %s", __func__));
  status = salComTerm(gHttpInfo.pTls);
  gHttpInfo.pTls = NULL;
  M_INTL_HTTP_DEBUG(("End of %s", __func__));
  return (TCommIfStatus)status;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/**
 * @implements pStrToken
 *
 **/
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
static char* pStrToken
(
  char*  xpSrc,
  char*  xpDst,
  int    xSize
)
{
  char *pPtr;
  char *pSt;
  char *pEd;
  int len = 0;
  ptrdiff_t diff;
  /* L-trim. */
  pPtr = xpSrc;

  /* Using the variable while(C_HTTP_TRUE) instead of while(1), to resolve the misra warning
   * Warning: 1 is not a boolean type
   */
  while (C_HTTP__TRUE)
  {
    if ((*pPtr == '\n') || (*pPtr == (char)0))
    {
      pPtr = NULL; /* Value does not exists. */
      goto end;
    }
    if ((*pPtr != ' ') && (*pPtr != '\t'))
    {
      break;
    }
    pPtr++;
  }

  pSt = pPtr;

  /* Using the variable while(C_HTTP_TRUE) instead of while(1), to resolve the misra warning
   * Warning: 1 is not a boolean type
   */
  while (C_HTTP__TRUE)
  {
    pEd = pPtr;
    if ((*pPtr == ' ') || (*pPtr == '\t'))
    {
      pEd--;
      pPtr++;
      break;
    }
    if ((*pPtr == '\n') || (*pPtr == (char)0))
    {
      break;
    }
    pPtr++;
  }

  diff = pEd - pSt;
  len = (int)(diff + 1);
  if ((xSize > 0) && (len >= xSize))
  {
    len = xSize - 1;
  }

  (void)strncpy(xpDst, pSt, (unsigned int)len);
  xpDst[len] = '\0';

end:
  return pPtr;
}

/**
 * @implements lstrncasecmp
 *
 **/
static int lstrncasecmp
(
  const char*  xpS1,
  const char*  xpS2,
  size_t       n
)
{
  int matched = 1;
  size_t i = 0;

  for (; i < n; i++)
  {
    if (tolower((unsigned char)xpS1[i]) != tolower((unsigned char)xpS2[i]))
    {
      matched = 0;
      break;
    }
  }

  return (matched == 0);
}

/**
 * @implements httpHeader
 *
 **/
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
static int httpHeader
(
  TKHttpInfo*  xpHttpInfo,
  char*        xpParam
)
{
  char* pToken;
  char  aT1[C_HTTP_TOKEN_MAX_LEN] = {0};
  char  aT2[C_HTTP_TOKEN_MAX_LEN] = {0};
  int   len;
  int   retVal = 0;

  pToken = xpParam;

  pToken = pStrToken(pToken, aT1, (int)C_HTTP_TOKEN_MAX_LEN);
  if (pToken == NULL)
  {
    retVal = -1;
    goto end;
  }
  pToken = pStrToken(pToken, aT2, (int)C_HTTP_TOKEN_MAX_LEN);
  if (pToken == NULL)
  {
    retVal = -1;
    goto end;
  }

  M_INTL_HTTP_DEBUG(("%s %s", aT1, aT2));
  if (lstrncasecmp(aT1, "HTTP", 4) == 0)
  {
    errno = 0;
    xpHttpInfo->response.status = strtol(aT2, NULL, 10);
    if (0 != errno)
    {
      M_INTL_HTTP_ERROR(("Error Occured %d", errno));
      retVal = -1;
      goto end;
    }
  }
  else if (lstrncasecmp(aT1, "set-cookie:", 11) == 0)
  {
    /**
     * SUPPRESS: MISRA_DEV_KTA_002 : misra_c2012_rule_17.7_violation
     * Not using the return value of snprintf
     */
    (void)snprintf(xpHttpInfo->response.cookie, C_HTTP__HEADER_FIELD_SIZE, "%s", aT2);
  }
  else if (lstrncasecmp(aT1, "location:", 9) == 0)
  {
    len = (int)strlen(aT2);
    (void)strncpy(xpHttpInfo->response.location, aT2, (unsigned int)len);
    xpHttpInfo->response.location[len] = '\0';
  }
  else if (lstrncasecmp(aT1, "content-length:", 15) == 0)
  {
    errno = 0;
    xpHttpInfo->response.contentLength = strtol(aT2, NULL, 10);
    if (0 != errno)
    {
      M_INTL_HTTP_ERROR(("Error Occured %d", errno));
      retVal = -1;
      goto end;
    }
  }
  else if (lstrncasecmp(aT1, "transfer-encoding:", 18) == 0)
  {
    if (lstrncasecmp(aT2, "chunked", 7) == 0)
    {
      xpHttpInfo->response.chunked = C_HTTP__TRUE;
      M_INTL_HTTP_DEBUG(("chunked set"));
    }
  }
  else if (lstrncasecmp(aT1, "connection:", 11) == 0)
  {
    if (lstrncasecmp(aT2, "close", 5) == 0)
    {
      xpHttpInfo->response.close = C_HTTP__TRUE;
    }
  }
  else
  {
    goto end;
  }

end:
  return retVal;
}

/**
 * @implements httpParse
 *
 **/
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
static int httpParse
(
  TKHttpInfo*  xpHttpInfo
)
{
  char *pPtr1;
  char *pPtr2;
  long len;
  int  retVal = -1;
  ptrdiff_t offset = 0;
  size_t ptrOffset;

  if (xpHttpInfo->recvLen == 0U)
  {
    retVal = -1;
  }
  else
  {
    pPtr1 = (char *)xpHttpInfo->pRecvBuf;

    /* Using the variable while(C_HTTP_TRUE) instead of while(1), to resolve the misra warning
    * Warning: 1 is not a boolean type
    */
    while (C_HTTP__TRUE)
    {
      if (xpHttpInfo->headerEnd == C_HTTP__FALSE)     /* Header parser. */
      {
        pPtr2 = strstr(pPtr1, "\r\n");
        if (pPtr2 != NULL)
        {
          len = (long)(pPtr2 - pPtr1);
          *pPtr2 = '\0';

          if (len > 0)
          {
            M_INTL_HTTP_DEBUG(("header: %s(%ld)..", pPtr1, len));
            if (httpHeader(xpHttpInfo, pPtr1) != 0)
            {
              M_INTL_HTTP_ERROR(("httpHeader returned Error"));
              retVal = -1;
              goto end;
            }
            pPtr1 = &pPtr2[2];    /* Skip CR+LF. */
          }
          else
          {

            xpHttpInfo->headerEnd = C_HTTP__TRUE; /* Reach the header-end. */

            M_INTL_HTTP_DEBUG(("header_end ...."));

            pPtr1 = &pPtr2[2];    /* Skip CR+LF. */

            if (xpHttpInfo->response.chunked == C_HTTP__TRUE)
            {
              offset = pPtr1 - (char *)xpHttpInfo->pRecvBuf;
              len = (unsigned long)((size_t)xpHttpInfo->recvLen - (size_t)offset);
              if (len > 0)
              {
                pPtr2 = strstr(pPtr1, "\r\n");
                if (pPtr2 != NULL)
                {
                  *pPtr2 = '\0';
                  errno = 0;
                  xpHttpInfo->length = strtol(pPtr1, NULL, 16);
                  if (0 != errno)
                  {
                    M_INTL_HTTP_ERROR(("Error Occured %d", errno));
                  }
                  if (xpHttpInfo->length == 0)
                  {
                      xpHttpInfo->response.chunked = C_HTTP__FALSE;
                  }
                  else
                  {
                      xpHttpInfo->response.contentLength += xpHttpInfo->length;
                  }
                  pPtr1 = &pPtr2[2];    /* Skip CR+LF. */
                }
                else
                {
                  /* Copy the data as chunked size. */
                  (void)strncpy((char *)xpHttpInfo->pRecvBuf, pPtr1, (unsigned int)len);
                  xpHttpInfo->pRecvBuf[len] = '\0';
                  xpHttpInfo->recvLen = (size_t)len;
                  xpHttpInfo->length = -1;
                  retVal = 0;
                  goto end;
                }
              }
              else
              {
                xpHttpInfo->recvLen = 0;
                xpHttpInfo->length = -1;
                retVal = 0;
                goto end;
              }
            }
            else
            {
              xpHttpInfo->length = xpHttpInfo->response.contentLength;
            }
          }
        }
        else
        {
          offset = pPtr1 - (char *)xpHttpInfo->pRecvBuf;
          len = (unsigned long)((size_t)xpHttpInfo->recvLen - (size_t)offset);
          if (len  > 0)
          {
            /* Keep the partial header data. */
            (void)strncpy((char*)xpHttpInfo->pRecvBuf, pPtr1, (unsigned int)len);
            xpHttpInfo->pRecvBuf[len] = '\0';
            xpHttpInfo->recvLen = (size_t)len;
          }
          else
          {
            xpHttpInfo->recvLen = 0;
          }
          retVal = 0;
          goto end;
        }
      }
      else    /* Body parser. */
      {

        if ((xpHttpInfo->response.chunked == C_HTTP__TRUE) && (xpHttpInfo->length == -1))
        {
          offset = pPtr1 - (char *)xpHttpInfo->pRecvBuf;
          len = (unsigned long)((size_t)xpHttpInfo->recvLen - (size_t)offset);
          if (len > 0)
          {
            pPtr2 = strstr(pPtr1, "\r\n");
            if (pPtr2 != NULL)
            {
              *pPtr2 = '\0';
              errno = 0;
              xpHttpInfo->length = strtol(pPtr1, NULL, 16);
              if (0 != errno)
              {
                M_INTL_HTTP_ERROR(("Error Occured %d", errno));
              }
              if (xpHttpInfo->length == 0)
              {
                  xpHttpInfo->response.chunked = C_HTTP__FALSE;
              }
              else
              {
                  xpHttpInfo->response.contentLength += xpHttpInfo->length;
              }

              pPtr1 = &pPtr2[2];    /* Skip CR+LF */
            }
            else
            {
              /* Copy the remain data as chunked size. */
              (void)strncpy((char*)xpHttpInfo->pRecvBuf, pPtr1, (unsigned int)len);
              xpHttpInfo->pRecvBuf[len] = '\0';
              xpHttpInfo->recvLen = (size_t)len;
              xpHttpInfo->length = -1;
              retVal = 0;
              goto end;
            }
          }
          else
          {
            xpHttpInfo->recvLen = 0;
            retVal = 0;
            goto end;
          }
        }
        else
        {
            if (xpHttpInfo->length > 0)
            {
              offset = pPtr1 - (char *)xpHttpInfo->pRecvBuf;
              len = (unsigned long)((size_t)xpHttpInfo->recvLen - (size_t)offset);

              if (len > xpHttpInfo->length)
              {
                /* Copy the data for response. */
                if (xpHttpInfo->bodyLen < xpHttpInfo->bodySize)
                {
                  if (xpHttpInfo->bodySize > (xpHttpInfo->bodyLen + xpHttpInfo->length))
                  {
                    (void)memcpy((void *)&(xpHttpInfo->body[xpHttpInfo->bodyLen]),
                            (const void *)pPtr1,
                            (unsigned int)xpHttpInfo->length);
                    xpHttpInfo->bodyLen += xpHttpInfo->length;
                  }
                  else
                  {
                    (void)memcpy((void *)&(xpHttpInfo->body[xpHttpInfo->bodyLen]),
                            (const void *)pPtr1,
                            (unsigned int)(xpHttpInfo->bodySize - xpHttpInfo->bodyLen));
                    xpHttpInfo->bodyLen = xpHttpInfo->bodySize;
                  }
                }

                ptrOffset = xpHttpInfo->length;
                pPtr1 = &pPtr1[ptrOffset];
                len -= xpHttpInfo->length;

                if ((xpHttpInfo->response.chunked == C_HTTP__TRUE) && (len >= 2))
                {
                  pPtr1 = &pPtr1[2];    /* Skip CR+LF. */
                  xpHttpInfo->length = -1;
                }
                else
                {
                  retVal = -1;
                  goto end;
                }
              }
              else
              {
                /* Copy the data for response. */
                if (xpHttpInfo->bodyLen < xpHttpInfo->bodySize)
                {
                  if (xpHttpInfo->bodySize > (xpHttpInfo->bodyLen + len))
                  {
                    (void)memcpy((void *)&(xpHttpInfo->body[xpHttpInfo->bodyLen]), (const void *)pPtr1, (unsigned int)len);
                    xpHttpInfo->bodyLen += len;
                  }
                  else
                  {
                    (void)memcpy((void *)&(xpHttpInfo->body[xpHttpInfo->bodyLen]),
                              (const void *)pPtr1,
                              (unsigned int)(xpHttpInfo->bodySize - xpHttpInfo->bodyLen));
                    xpHttpInfo->bodyLen = xpHttpInfo->bodySize;
                  }
                }

                xpHttpInfo->length -= len;
                xpHttpInfo->recvLen = 0;

                if ((xpHttpInfo->response.chunked == C_HTTP__FALSE) && (xpHttpInfo->length <= 0))
                {
                  retVal = 1;
                  goto end;
                }
                retVal = 0;
                goto end;
              }
            }
            else
            {
              if (xpHttpInfo->response.chunked == C_HTTP__FALSE)
              {
                retVal = 1;
                goto end;
              }

              /* Chunked size check. */
              if ((xpHttpInfo->recvLen > 2u) && (strncmp(pPtr1, "\r\n", 2) == 0))
              {
                pPtr1 = &pPtr1[2];
                xpHttpInfo->length = -1;
              }
              else
              {
                xpHttpInfo->length = -1;
                xpHttpInfo->recvLen = 0;
              }
            }
        }
      }
    }
end:
    retVal = 0;
  }
  return retVal;
}

/**
 * @implements httpPost
 *
 **/
/**
 * SUPPRESS: MISRA_DEV_KTA_005 : misra_c2012_rule_15.4_violation
 * SUPPRESS: MISRA_DEV_KTA_004 : misra_c2012_rule_15.1_violation
 * Using goto for breaking during the error and return cases.
 **/
static int httpPost
(
  TKHttpInfo*     xpHttpInfo,
  const uint8_t*  xpDir,
  const uint8_t*  xpData,
  size_t          xDataLen,
  uint8_t*        xpResponse,
  size_t          xSize
)
{
  TKCommStatus status = E_K_COMM_STATUS_ERROR;
  uint8_t aBuffer[C_HTTP_MAX_DATA_LEN] = {0};
  unsigned int len;
  int retVal = -1;

  M_INTL_HTTP_DEBUG(("Start of %s", __func__));

  if (NULL == xpHttpInfo)
  {
    retVal = -1;
  }
  else
  {
    /* Send HTTP buffer. */
    len = (unsigned int)snprintf((char *)aBuffer, C_HTTP_MAX_DATA_LEN,
                    "POST %s HTTP/1.1\r\n"
                    "Host: %s:%s\r\n"
                    "Connection: Keep-Alive\r\n"
                    "Content-Type: application/octet-stream\r\n"
                    "Content-Length: %d\r\n"
                    "Cookie: %s\r\n"
                    "\r\n",
                    (const char *)xpDir,
                    xpHttpInfo->url.host,
                    xpHttpInfo->url.port,
                    (int)xDataLen,
                    xpHttpInfo->request.cookie);

    M_INTL_HTTP_DEBUG(("Post Header len %d", len));
    (void)memcpy(&aBuffer[len], xpData, xDataLen);
    len = len + xDataLen;
    if (len > C_HTTP_MAX_DATA_LEN)
    {
      M_INTL_HTTP_ERROR(("Buffer Overflow"));
      (void)salComTerm(xpHttpInfo->pTls);
      retVal = -1;
      goto end;
    }
    status = salComWrite(xpHttpInfo->pTls, aBuffer, len);
    if (E_K_COMM_STATUS_OK != status)
    {
      M_INTL_HTTP_ERROR(("salComWrite Failed"));
      (void)salComTerm(xpHttpInfo->pTls);
      retVal = -1;
      goto end;
    }

    xpHttpInfo->response.status = 0;
    xpHttpInfo->response.contentLength = 0;
    xpHttpInfo->response.close = C_HTTP__FALSE;

    xpHttpInfo->recvLen = 0;
    xpHttpInfo->headerEnd = C_HTTP__FALSE;

    xpHttpInfo->body = xpResponse;
    xpHttpInfo->bodySize = (long)xSize;
    xpHttpInfo->bodyLen = 0;
    xpHttpInfo->body[0] = 0;
    xpHttpInfo->recvLen = sizeof(aBuffer);
    xpHttpInfo->pRecvBuf = (char *)aBuffer;

    status = salComRead(xpHttpInfo->pTls, aBuffer, &xpHttpInfo->recvLen);
    if (E_K_COMM_STATUS_OK != status)
    {
      M_INTL_HTTP_ERROR(("salComRead Failed"));
      (void)salComTerm(xpHttpInfo->pTls);
      retVal = -1;
      goto end;
    }
    if (httpParse(xpHttpInfo) != 0)
    {
      M_INTL_HTTP_ERROR(("httpHeader returned Error"));
      retVal = -1;
      goto end;
    }

    if ((int)xpHttpInfo->response.close == 1)
    {
      (void)salComTerm(xpHttpInfo->pTls);
    }

    M_INTL_HTTP_DEBUG(("status  : %d", xpHttpInfo->response.status));
    M_INTL_HTTP_DEBUG(("cookie  : %s", xpHttpInfo->response.cookie));
    M_INTL_HTTP_DEBUG(("location: %s", xpHttpInfo->response.location));
    M_INTL_HTTP_DEBUG(("length  : %ld", xpHttpInfo->response.contentLength));
    M_INTL_HTTP_DEBUG(("body    : %ld", xpHttpInfo->bodyLen));
    retVal = xpHttpInfo->response.status;
    goto end;
  }

end:
  return retVal;
}

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

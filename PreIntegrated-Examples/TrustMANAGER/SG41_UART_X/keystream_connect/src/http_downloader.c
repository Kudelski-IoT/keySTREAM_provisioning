#include "http_downloader.h"

#include <string.h>

THttpStatus httpParseUrl(const char* url, THttpUrl* parsedUrl)
{
    if ((url == NULL) || (parsedUrl == NULL))
    {
        return HTTP_STATUS_INVALID_URL;
    }

    (void)memset(parsedUrl, 0, sizeof(*parsedUrl));
    return HTTP_STATUS_ERROR;
}

THttpStatus httpResolveHostname(const char* hostname, char* ipAddress)
{
    (void)hostname;
    (void)ipAddress;
    return HTTP_STATUS_DNS_FAILED;
}

THttpStatus httpDownloadFile(const THttpUrl* parsedUrl, const char* ipAddress)
{
    (void)parsedUrl;
    (void)ipAddress;
    return HTTP_STATUS_ERROR;
}

THttpStatus httpSendGetRequest(uint32_t socketId, const char* hostname, const char* path)
{
    (void)socketId;
    (void)hostname;
    (void)path;
    return HTTP_STATUS_REQUEST_FAILED;
}

THttpStatus httpReadResponse(uint32_t socketId)
{
    (void)socketId;
    return HTTP_STATUS_RESPONSE_ERROR;
}

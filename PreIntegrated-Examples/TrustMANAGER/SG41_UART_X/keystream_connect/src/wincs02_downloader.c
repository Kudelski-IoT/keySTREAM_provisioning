#include <stddef.h>
#include <stdint.h>

typedef enum {
    DOWNLOAD_STATUS_OK = 0,
    DOWNLOAD_STATUS_ERROR,
    DOWNLOAD_STATUS_PARAMETER_ERROR,
    DOWNLOAD_STATUS_NETWORK_ERROR,
} TDownloadStatus;

TDownloadStatus downloadFile(const char* url, uint8_t* outputBuffer, size_t outputBufferSize, size_t* downloadedSize)
{
    (void)url;
    (void)outputBuffer;
    (void)outputBufferSize;
    if (downloadedSize != NULL)
    {
        *downloadedSize = 0;
    }
    return DOWNLOAD_STATUS_NETWORK_ERROR;
}

void downloadFileExample(void)
{
}

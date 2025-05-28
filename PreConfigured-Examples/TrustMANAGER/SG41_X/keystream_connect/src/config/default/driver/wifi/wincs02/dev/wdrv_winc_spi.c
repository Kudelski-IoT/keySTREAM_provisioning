/*******************************************************************************
  WINC Wireless Driver SPI Communication Support

  File Name:
    wdrv_winc_spi.c

  Summary:
    WINC Wireless Driver SPI Communications Support

  Description:
    Supports SPI communications to the WINC module.
 *******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2024 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

#include "configuration.h"
#include "definitions.h"
#include "osal/osal.h"
#include "wdrv_winc_common.h"
#include "wdrv_winc_spi.h"

// *****************************************************************************
// *****************************************************************************
// Section: Data Type Definitions
// *****************************************************************************
// *****************************************************************************

typedef struct
{
    bool                                isInit;
    bool                                isOpen;
    WDRV_WINC_SPI_CFG                   cfg;
    OSAL_SEM_HANDLE_TYPE                syncSem;
} WDRV_WINC_SPIDCPT;

// *****************************************************************************
// *****************************************************************************
// Section: Global Data
// *****************************************************************************
// *****************************************************************************

static WDRV_WINC_SPIDCPT spiDcpt = {.isInit = false};
static CACHE_ALIGN uint8_t dummyDataTx[CACHE_ALIGNED_SIZE_GET(4)];

// *****************************************************************************
// *****************************************************************************
// Section: File scope functions
// *****************************************************************************
// *****************************************************************************

static void lDRV_SPI_PlibCallbackHandler(uintptr_t contextHandle)
{
    OSAL_SEM_PostISR((OSAL_SEM_HANDLE_TYPE*)contextHandle);
}

//*******************************************************************************
/*
  Function:
    bool WDRV_WINC_SPISendReceive(void* pTransmitData, void* pReceiveData, size_t size)

  Summary:
    Sends and receives data from the module through the SPI bus.

  Description:
    This function sends and receives data from the module through the SPI bus.

  Remarks:
    See wdrv_winc_spi.h for usage information.
 */

bool WDRV_WINC_SPISendReceive(void* pTransmitData, void* pReceiveData, size_t size)
{
    if ((false == spiDcpt.isInit) || (false == spiDcpt.isOpen))
    {
        return false;
    }

#ifdef WDRV_WINC_SSN_Clear
    WDRV_WINC_SSN_Clear();
#endif

    size_t txSize, rxSize;

    if ((NULL == spiDcpt.cfg.callbackRegister) || (NULL == spiDcpt.cfg.writeRead))
    {
        return false;
    }

    if (NULL == pTransmitData)
    {
        pTransmitData = &dummyDataTx;
        txSize = 1;
    }
    else
    {
        txSize = size;
    }

    if (NULL == pReceiveData)
    {
        rxSize = 0;
    }
    else
    {
        rxSize = size;
    }

    spiDcpt.cfg.callbackRegister(lDRV_SPI_PlibCallbackHandler, (uintptr_t)&spiDcpt.syncSem);
    spiDcpt.cfg.writeRead(pTransmitData, txSize, pReceiveData, rxSize);

    while (OSAL_RESULT_FALSE == OSAL_SEM_Pend(&spiDcpt.syncSem, OSAL_WAIT_FOREVER))
    {
    }

#ifdef WDRV_WINC_SSN_Set
    WDRV_WINC_SSN_Set();
#endif

    return true;
}

//*******************************************************************************
/*
  Function:
    bool WDRV_WINC_SPIOpen(void)

  Summary:
    Opens the SPI object for the WiFi driver.

  Description:
    This function opens the SPI object for the WiFi driver.

  Remarks:
    See wdrv_winc_spi.h for usage information.
 */

bool WDRV_WINC_SPIOpen(void)
{
    if (false == spiDcpt.isInit)
    {
        return false;
    }

    if (true == spiDcpt.isOpen)
    {
        return true;
    }

    if (OSAL_RESULT_TRUE != OSAL_SEM_Create(&spiDcpt.syncSem, OSAL_SEM_TYPE_COUNTING, 10, 0))
    {
        return false;
    }


    spiDcpt.isOpen = true;

    return true;
}

//*******************************************************************************
/*
  Function:
    void WDRV_WINC_SPIInitialize(const WDRV_WINC_SPI_CFG *const pInitData)

  Summary:
    Initializes the SPI object for the WiFi driver.

  Description:
    This function initializes the SPI object for the WiFi driver.

  Remarks:
    See wdrv_winc_spi.h for usage information.
 */

void WDRV_WINC_SPIInitialize(const WDRV_WINC_SPI_CFG *const pInitData)
{
    if ((NULL == pInitData) || (true == spiDcpt.isInit))
    {
        return;
    }

    (void)memcpy(&spiDcpt.cfg, pInitData, sizeof(WDRV_WINC_SPI_CFG));
    (void)memset(dummyDataTx, 0xff, sizeof(dummyDataTx));

    spiDcpt.isOpen = false;
    spiDcpt.isInit = true;
}

//*******************************************************************************
/*
  Function:
    void WDRV_WINC_SPIDeinitialize(void)

  Summary:
    Deinitializes the SPI object for the WiFi driver.

  Description:
    This function deinitializes the SPI object for the WiFi driver.

  Remarks:
    See wdrv_winc_spi.h for usage information.
 */

void WDRV_WINC_SPIDeinitialize(void)
{
    if (false == spiDcpt.isInit)
    {
        return;
    }

    (void)OSAL_SEM_Post(&spiDcpt.syncSem);
    (void)OSAL_SEM_Delete(&spiDcpt.syncSem);

    spiDcpt.isOpen = false;
    spiDcpt.isInit = false;
}

//DOM-IGNORE-END

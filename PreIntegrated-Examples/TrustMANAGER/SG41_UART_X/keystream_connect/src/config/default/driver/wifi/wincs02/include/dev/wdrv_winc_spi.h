/*******************************************************************************
  Company:
    Microchip Technology Inc.

  File Name:
    wdrv_winc_spi.h

  Summary:
    WINC wireless driver SPI APIs.

  Description:
    Provides interface for using the SPI bus.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2023 Microchip Technology Inc. and its subsidiaries.
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
// DOM-IGNORE-END

#ifndef WDRV_WINC_SPI_H
#define WDRV_WINC_SPI_H

#include "peripheral/sercom/spi_master/plib_sercom_spi_master_common.h"

typedef void (*WDRV_WINC_SPI_PLIB_CALLBACK_REGISTER)(SERCOM_SPI_CALLBACK, uintptr_t);
typedef bool (*WDRV_WINC_SPI_PLIB_WRITE_READ)(void*, size_t, void *, size_t);

// *****************************************************************************
/*  SPI Configuration Structure

  Summary:
    Structure containing SPI configuration parameters.

  Description:
    Structure containing SPI configuration parameters.

  Remarks:
    None.

*/

typedef struct
{
    /* SPI PLIB writeRead API */
    WDRV_WINC_SPI_PLIB_WRITE_READ writeRead;

    /* SPI PLIB callback register API */
    WDRV_WINC_SPI_PLIB_CALLBACK_REGISTER callbackRegister;
} WDRV_WINC_SPI_CFG;

//*******************************************************************************
/*
  Function:
    bool WDRV_WINC_SPISendReceive(void* pTransmitData, void* pReceiveData, size_t size)

  Summary:
    Sends and receives data from the module through the SPI bus.

  Description:
    This function sends and receives data from the module through the SPI bus.

  Precondition:
    WDRV_WINC_SPIInitialize must have been called.

  Parameters:
    pTransmitData - Pointer to buffer containing data to send, or NULL.
    pReceiveData  - Pointer to buffer to receive data into, of NULL.
    size          - The size of the data transfer.

  Returns:
    true  - Indicates success.
    false - Indicates failure.

  Remarks:
    Only half-duplex operation is required, so either pTransmitData or pReceiveData
    will be valid pointers while the other is set to NULL.
 */

bool WDRV_WINC_SPISendReceive(void* pTransmitData, void* pReceiveData, size_t size);

//*******************************************************************************
/*
  Function:
    bool WDRV_WINC_SPIOpen(void)

  Summary:
    Opens the SPI object for the WiFi driver.

  Description:
    This function opens the SPI object for the WiFi driver.

  Precondition:
    WDRV_WINC_SPIInitialize must have been called.

  Parameters:
    None.

  Returns:
    true of false indicating success of operation.

  Remarks:
    None.
 */

bool WDRV_WINC_SPIOpen(void);

//*******************************************************************************
/*
  Function:
    void WDRV_WINC_SPIInitialize(const WDRV_WINC_SPI_CFG *const pInitData)

  Summary:
    Initializes the SPI object for the WiFi driver.

  Description:
    This function initializes the SPI object for the WiFi driver.

  Precondition:
    None.

  Parameters:
    pInitData - Pointer to initialization data.

  Returns:
    None.

  Remarks:
    None.
 */

void WDRV_WINC_SPIInitialize(const WDRV_WINC_SPI_CFG *const pInitData);

//*******************************************************************************
/*
  Function:
    void WDRV_WINC_SPIDeinitialize(void)

  Summary:
    Deinitializes the SPI object for the WiFi driver.

  Description:
    This function deinitializes the SPI object for the WiFi driver.

  Precondition:
    WDRV_WINC_SPIInitialize must have been called.

  Parameters:
    None.

  Returns:
    None.

  Remarks:
    None.
 */

void WDRV_WINC_SPIDeinitialize(void);

#endif /* WDRV_WINC_SPI_H */

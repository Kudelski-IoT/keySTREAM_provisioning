/*
  * Copyright 2017 Microchip Technology Incorporated and its subsidiaries.
  * 
  * Amazon FreeRTOS PKCS#11 for Curiosity PIC32MZEF V1.0.4
  * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy of 
  * this software and associated documentation files (the "Software"), to deal in 
  * the Software without restriction, including without limitation the rights to 
  * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
  * of the Software, and to permit persons to whom the Software is furnished to do 
  * so, subject to the following conditions:
  * The above copyright notice and this permission notice shall be included in all 
  * copies or substantial portions of the Software.

  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  * SOFTWARE
  ******************************************************************************
 */

/* PKCS11 storage implementation for Microchip PIC32MZ platform. */


#ifndef _PKCS11_NVM_H_
#define _PKCS11_NVM_H_

#include <stdint.h>
#include <stdbool.h>
#include "device.h"
#include "peripheral/nvmctrl/plib_nvmctrl.h"

#define AWS_NVM_ROW_SIZE            (2 * 1024)
#define AWS_NVM_PAGE_SIZE           (512)
#define AWS_NVM_BLOCK_SIZE           (8192UL)
#define AWS_NVM_FLASH_SIZE          (NVMCTRL_FLASH_SIZE/2) // Size of each bank
#define AWS_NVM_FLASH_START_ADDRESS (NVMCTRL_FLASH_START_ADDRESS)
#define AWS_NVM_ERASE_PAGES_MAX     (32)    // max number of pages that could be erased once
#define AWS_NVM_ERASE_PAGES_MIN     (16)    // min number of pages that could be erased once
#define AWS_NVM_LOCK_REGION_PAGES   (32)    // number of pages in a lock region
#define AWS_QUAD_SIZE               (16)
#define AWS_NVM_BOOTLOADER_SIZE     (0x10000)

bool AWS_NVM_WriteDescriptor(uint8_t * ptrFlash, uint8_t *pData, int iSize);

// erases the boot upper page 4
bool AWS_FlashRegionProtect(int regionNo, uint32_t regionSize, const void* flashAddress, bool enable);

bool AWS_FlashEraseUpdateBank();
// writes a row in the upper boot flash
// the row is indicated by the ptrFlash address
// the data is pointed at by rowData 
bool AWS_FlashRegionUnlock(const uint32_t* ptrFlash);

bool AWS_NVM_QuadWordWrite(uint8_t * ptrFlash , uint8_t * pData,int nBlocks);
// disables the write protection to the upper boot page 4
bool AWS_FlashPagesErase(const uint32_t* ptrFlash, int nPages);

// enables the write protection to the upper boot page 4
bool AWS_FlashPagesWrite(const uint32_t* ptrFlash, const uint32_t* pageData, int nPages);

bool AWS_FlashProgramBlock(const uint8_t* ptrFlash, const uint8_t* pData, uint32_t size);


// returns true if the upper boot page 4 is write protected
bool AWS_FlashRegionLock(const uint32_t* ptrFlash);
bool AWS_FlashRegionUnlock(const uint32_t* ptrFlash);


#endif // _PKCS11_NVM_H_


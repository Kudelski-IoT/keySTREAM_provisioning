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

#include "FreeRTOS.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pkcs11_nvm.h"


#include "device.h"
#include "system/int/sys_int.h"
#include "peripheral/nvmctrl/plib_nvmctrl.h"

#define AWS_RETRY_COUNT 0

// mask corresponding to AWS_NVM_PAGE_SIZE == 512 = 2^9
#define AWS_NVM_PAGE_MASK   (~((1 << 9) - 1))


// page command args for erasing min/max pages
#define AWS_NVM_ERASE_COMMAND_ARG_MIN   2
#define AWS_NVM_ERASE_COMMAND_ARG_MAX   3

// force erase in MIN pages, if needed for some reason
#define AWS_NVM_ERASE_FORCE_MIN         0

typedef enum estNVMErr
{
    eNoError=0,
    eRetry=1,
    eNvmErr=2
} enumNvmErr;

void delay_50()
{
    for(int i=0;i<100;i++)
        __NOP();
}

static enumNvmErr AWS_NVM_Error()
{
    uint16_t temp = NVMCTRL_ErrorGet();
    uint32_t temp_new=0;
   temp_new = NVMCTRL_REGS->NVMCTRL_ECCERR; // read error.
    if( !((temp == 0x01 || temp == 0x0)))
        configPRINTF(("NVM Error FATAL !!! error = 0x%x\r\n",temp));


    if(temp & (0x1 << 6))
        return eNvmErr;
    else
    {
        if( (temp == 0x01) || (temp == 0x0)) return eNoError;
      

    }
   configPRINTF(("NVM Error ECC !!! error = 0x%x\r\n",temp_new));
   return eRetry;
 
}
bool AWS_FlashRegionLock(const uint32_t* ptrFlash)
{
    NVMCTRL_RegionLock(ptrFlash);
    return (NVMCTRL_RegionLockStatusGet() == 0)?true:false;
}

bool AWS_FlashRegionUnlock(const uint32_t* ptrFlash)
{
    while(NVMCTRL_IsBusy());
    NVMCTRL_RegionUnlock(ptrFlash);
    while(NVMCTRL_IsBusy());
    return (NVMCTRL_RegionLockStatusGet() == 0)?true:false;
}

// nPages needs to be multiple of AWS_NVM_ERASE_PAGES_MIN/AWS_NVM_ERASE_PAGES_MAX
bool AWS_FlashBlockErase(const uint32_t* ptrFlash, int nBlocks)
{
    bool bResult=true;
    uint32_t addr = ptrFlash;
    int iRetry=AWS_RETRY_COUNT;
    int i=0;
    enumNvmErr eState;
    do
    {
       //while(NVMCTRL_IsBusy());
       //AWS_FlashRegionUnlock(addr + (AWS_NVM_BLOCK_SIZE*i));
       _Bool int_status = SYS_INT_Disable();
       while(NVMCTRL_IsBusy());
       eState = AWS_NVM_Error(); // Dummy Read!!!
       while(!(NVMCTRL_StatusGet() & 0x01));
       bResult = NVMCTRL_BlockErase( (addr + (AWS_NVM_BLOCK_SIZE*i)));
       delay_50();
       while(NVMCTRL_IsBusy());
        eState = AWS_NVM_Error();
        SYS_INT_Restore(int_status);
        if( (eState == eRetry) && iRetry)
         {
           configPRINTF(("Erase NVM Error ocurrred when i = %d \r\n",i));
           iRetry--;continue;
         }

        i++;
        bResult = !eState;
        iRetry=AWS_RETRY_COUNT;
    }while( (i<nBlocks) && bResult);
    
    return bResult;
}


bool AWS_FlashPagesWrite(const uint32_t* ptrFlash, const uint32_t* pageData, int nPages)
{
    bool bResult=true;
    uint32_t addr = ptrFlash;
    int i=0;
    int iRetry=AWS_RETRY_COUNT;
    enumNvmErr eState;
    do 
    {

        addr = (uint32_t)(ptrFlash + (NVMCTRL_FLASH_PAGESIZE*i)/sizeof(uint32_t));
        _Bool int_status = SYS_INT_Disable();
        while(NVMCTRL_IsBusy());
        eState = AWS_NVM_Error(); // dummy read
        while(!(NVMCTRL_StatusGet() & 0x01));
        bResult = NVMCTRL_PageWrite(pageData + (NVMCTRL_FLASH_PAGESIZE*i)/sizeof(uint32_t), addr );
        while(NVMCTRL_IsBusy());
        eState = AWS_NVM_Error();
        SYS_INT_Restore(int_status);
        if( (eState == eRetry) && iRetry)
        {
            configPRINTF(("NVM Error ocurrred when i = %d \r\n",i));
            iRetry--;continue;
        }
        bResult = !eState;
        iRetry=AWS_RETRY_COUNT;
        i++;

    }while( (i<nPages) && bResult);
    
    return bResult;
}


bool AWS_FlashEraseUpdateBank()
{
    int nBlocks = (AWS_NVM_FLASH_SIZE-AWS_NVM_BOOTLOADER_SIZE)/AWS_NVM_BLOCK_SIZE;
    const uint32_t * ptrFlash = (const uint32_t *)(AWS_NVM_FLASH_START_ADDRESS + AWS_NVM_FLASH_SIZE + AWS_NVM_BOOTLOADER_SIZE);
    // Erases upper Bank.
    return AWS_FlashBlockErase(ptrFlash, nBlocks);
    
}

bool AWS_FlashProgramBlock(const uint8_t* ptrFlash, const uint8_t* pData, uint32_t size)
{
    uint8_t bMem[AWS_NVM_PAGE_SIZE];
    uint32_t size2 = size;
    int nBlocks = (size2 - (size2 % AWS_NVM_PAGE_SIZE))/AWS_NVM_PAGE_SIZE;
    const uint8_t *pAddr =(const uint8_t *)ptrFlash;
    bool bResult = true;
    if(size >= AWS_NVM_PAGE_SIZE)
    {
        bResult = AWS_FlashPagesWrite(ptrFlash,pData,nBlocks);
      
        if(memcmp(ptrFlash, pData, nBlocks * AWS_NVM_PAGE_SIZE))
        {
            configPRINTF(("Page write Error = 0x%x, Rem Size = %d\r\n",bResult,size2));
            return false;
        }
         size2 = size2 % AWS_NVM_PAGE_SIZE;
    }
    
    if(size2 > 0 && bResult)
    {
        memcpy(bMem, (const uint8_t *) (pData + (nBlocks * AWS_NVM_PAGE_SIZE)), size2);
        bResult = AWS_FlashPagesWrite(pAddr + (nBlocks * AWS_NVM_PAGE_SIZE),bMem,1);
        if(memcmp((pAddr + (nBlocks * AWS_NVM_PAGE_SIZE)), bMem, size2))
        {
            configPRINTF(("Last page write Result #3= %d, Rem Size = %d\r\n",bResult,size2));
            //return false;
        }
    }
  
   if(memcmp(ptrFlash, pData, size ))
       return false;

    return bResult;
}

bool AWS_NVM_WriteDescriptor(uint8_t * ptrFlash, uint8_t *pData, int iSize)
{
    //Erase the descriptor first!!!.
    uint32_t addr = ptrFlash;
    addr = addr - (addr % AWS_NVM_BLOCK_SIZE);
    AWS_FlashBlockErase( addr, 1);
    AWS_NVM_QuadWordWrite(ptrFlash, pData, iSize/AWS_QUAD_SIZE);
    return !memcmp(ptrFlash, pData, iSize);
}

bool AWS_NVM_QuadWordWrite(uint8_t * ptrFlash , uint8_t * pData,int nBlocks) 
{
    uint32_t addr = ptrFlash;
    uint32_t data_addr = pData;
    bool bResult=true;
    while(NVMCTRL_IsBusy());
    for(int i=0;i<nBlocks && bResult; i++)
    {
        bResult = !AWS_NVM_Error();
        while(NVMCTRL_IsBusy());
        bResult = NVMCTRL_QuadWordWrite(data_addr + i*AWS_QUAD_SIZE,addr + i*AWS_QUAD_SIZE);
        while(NVMCTRL_IsBusy());
        bResult = !AWS_NVM_Error();
        
    }
    while(NVMCTRL_IsBusy());
    
    if(memcmp(ptrFlash, pData, nBlocks * AWS_QUAD_SIZE ))
        return false;
    return bResult;
}

int mbedtls_hardware_poll( void * data,
                           unsigned char * output,
                           size_t len,
                           size_t * olen )
{

 

    ((void) data);

 

    union
    {
        uint64_t    v64;
        uint8_t     v8[8];
    }suint_64;

 

    int n8Chunks = len / 8;
    int nLeft = len % 8;

 

    while(n8Chunks--)
    {
        suint_64.v64 = TRNG_ReadData();
        suint_64.v64 = suint_64.v64 << 32;
        suint_64.v64 |= TRNG_ReadData();
        memcpy(output, suint_64.v8, sizeof(suint_64.v8));
        output += sizeof(suint_64.v8);
    }

 

    if(nLeft)
    {
        suint_64.v64 = TRNG_ReadData();
        suint_64.v64 = suint_64.v64 << 32;
        suint_64.v64 |= TRNG_ReadData();
        memcpy(output, suint_64.v8, nLeft);
    }

 


  *olen = len;
  
  return 0;
}
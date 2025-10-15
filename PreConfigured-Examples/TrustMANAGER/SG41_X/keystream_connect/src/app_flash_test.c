#include "app_flash_test.h"
#include "system/console/sys_console.h"
#include <string.h> 

#define TEST_FLASH_ADDRESS 0x000000
#define TEST_FLASH_PAGE_SIZE 256

static SYS_FLASH_STATES flashState = SYS_FLASH_STATE_INIT;
static DRV_HANDLE sst26Handle = DRV_HANDLE_INVALID;

static uint8_t writeBuffer[TEST_FLASH_PAGE_SIZE] = "Flash test string MASTERS 25";
static uint8_t readBuffer[TEST_FLASH_PAGE_SIZE];

static bool flashDone = false;
static bool flashError = false;



void SYS_Flash_Initialize(void)
{
    SYS_CONSOLE_PRINT("\r\n");
    SYS_CONSOLE_PRINT("########################################\r\n");
    SYS_CONSOLE_PRINT("Initializing SST26 Flash Test Sequence...\r\n");
    SYS_CONSOLE_PRINT("########################################\r\n");

    flashState = SYS_FLASH_STATE_INIT;
    flashDone = false;
    flashError = false;
}

void SYS_Flash_Tasks(void)
{
    switch(flashState)
    {
        case SYS_FLASH_STATE_INIT:
            SYS_CONSOLE_PRINT("[INIT] Opening SST26 driver...\r\n");
            sst26Handle = DRV_SST26_Open(DRV_SST26_INDEX, DRV_IO_INTENT_READWRITE);
            if (sst26Handle != DRV_HANDLE_INVALID)
            {
                SYS_CONSOLE_PRINT("[INIT] SST26 driver opened successfully.\r\n");
                //DRV_SST26_EventHandlerSet(sst26Handle, SYS_FlashEventHandler, 0);
                flashState = SYS_FLASH_STATE_ERASE;
            }
            else
            {
                SYS_CONSOLE_PRINT("[INIT] Failed to open SST26 driver!\r\n");
                flashState = SYS_FLASH_STATE_ERROR;
            }
            break;

        case SYS_FLASH_STATE_ERASE:
            SYS_CONSOLE_PRINT("[ERASE] Issuing Sector Erase at address 0x%06X...\r\n", TEST_FLASH_ADDRESS);
            if (DRV_SST26_SectorErase(sst26Handle, TEST_FLASH_ADDRESS))
            {
                flashState = SYS_FLASH_STATE_WAIT_ERASE;
            }
            else
            {
                SYS_CONSOLE_PRINT("[ERASE] Sector Erase command failed!\r\n");
                flashState = SYS_FLASH_STATE_ERROR;
            }
            break;

        case SYS_FLASH_STATE_WAIT_ERASE:
            if (DRV_SST26_TransferStatusGet(sst26Handle) == DRV_SST26_TRANSFER_COMPLETED)
            {
                SYS_CONSOLE_PRINT("[ERASE] Sector Erase completed.\r\n");
                flashState = SYS_FLASH_STATE_WRITE;
            }
            break;

        case SYS_FLASH_STATE_WRITE:
            SYS_CONSOLE_PRINT("[WRITE] Writing test data to address 0x%06X...\r\n", TEST_FLASH_ADDRESS);
            if (DRV_SST26_PageWrite(sst26Handle, writeBuffer, TEST_FLASH_ADDRESS))
            {
                flashState = SYS_FLASH_STATE_WAIT_WRITE;
            }
            else
            {
                SYS_CONSOLE_PRINT("[WRITE] Page Write command failed!\r\n");
                flashState = SYS_FLASH_STATE_ERROR;
            }
            break;

        case SYS_FLASH_STATE_WAIT_WRITE:
            if (DRV_SST26_TransferStatusGet(sst26Handle) == DRV_SST26_TRANSFER_COMPLETED)
            {
                SYS_CONSOLE_PRINT("[WRITE] Page Write completed.\r\n");
                flashState = SYS_FLASH_STATE_READ;
            }
            break;

        case SYS_FLASH_STATE_READ:
            SYS_CONSOLE_PRINT("[READ] Reading back data from address 0x%06X...\r\n", TEST_FLASH_ADDRESS);
            if (DRV_SST26_Read(sst26Handle, readBuffer, sizeof(readBuffer), TEST_FLASH_ADDRESS))
            {
                flashState = SYS_FLASH_STATE_WAIT_READ;
            }
            else
            {
                SYS_CONSOLE_PRINT("[READ] Read command failed!\r\n");
                flashState = SYS_FLASH_STATE_ERROR;
            }
            break;

        case SYS_FLASH_STATE_WAIT_READ:
            if (DRV_SST26_TransferStatusGet(sst26Handle) == DRV_SST26_TRANSFER_COMPLETED)
            {
                SYS_CONSOLE_PRINT("[READ] Read operation completed.\r\n");
                SYS_CONSOLE_PRINT(TERM_GREEN"[READ] Data :\r\n  %.32s\r\n"TERM_RESET, readBuffer);
                flashState = SYS_FLASH_STATE_VERIFY;
            }
            break;

        case SYS_FLASH_STATE_VERIFY:
            SYS_CONSOLE_PRINT("[VERIFY] Comparing written and read data...\r\n");
            if (memcmp(writeBuffer, readBuffer, sizeof(writeBuffer)) == 0)
            {
                SYS_CONSOLE_PRINT("SST26 Flash verify SUCCESS! Data matches.\r\n");
                flashDone = true;
                flashState = SYS_FLASH_STATE_DONE;
            }
            else
            {
                SYS_CONSOLE_PRINT("SST26 Flash verify FAILED! Data mismatch.\r\n");
                flashError = true;
                flashState = SYS_FLASH_STATE_ERROR;
            }
            break;

        case SYS_FLASH_STATE_DONE:
            SYS_CONSOLE_PRINT("Flash operation completed successfully!\r\n");
            SYS_CONSOLE_PRINT("########################################\r\n");
            break;

        case SYS_FLASH_STATE_ERROR:
            SYS_CONSOLE_PRINT("Flash operation encountered an ERROR!\r\n");
            SYS_CONSOLE_PRINT("########################################\r\n");
            flashError = true;
            break;

        default:
            break;
    }
}

bool SYS_Flash_IsDone(void)
{
    return flashDone;
}

bool SYS_Flash_HasError(void)
{
    return flashError;
}

/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include "cryptoauthlib.h"
#include "metadata.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
#define PASSIVE_BANK_BASE_ADDRESS   (uint32_t)0x00080000

/* Mirrors gaKtaLifeCycleNVMVData[] in ktamgr.c */
static const uint8_t bl_lifecycle_nvm_data[5][LIFECYCLE_STATE_SIZE] =
{
    {0x00, 0x00, 0x00, 0x00}, /* INIT       */
    {0x53, 0x45, 0x41, 0x4C}, /* "SEAL"     */
    {0x41, 0x43, 0x54, 0x49}, /* "ACTI"     */
    {0x50, 0x52, 0x4F, 0x56}, /* "PROV"     */
    {0x43, 0x4F, 0x4E, 0x52}  /* "CONR"     */
};

static const char* const bl_lifecycle_state_name[] =
{
    "INIT", "SEALED", "ACTIVATED", "PROVISIONED", "CON_REQ"
};

#define BOOT_DEBUG_ENABLED
#ifdef BOOT_DEBUG_ENABLED
/** @brief Enable debug logs. */
#define BOOT_DEBUG(fmt, ...)          printf("[BOOT]: " fmt "\r\n", ##__VA_ARGS__)
#define BOOT_DEBUG_GREEN(fmt, ...)    printf("\x1B[32m[BOOT]: " fmt "\r\n\x1B[0m", ##__VA_ARGS__)
#define BOOT_DEBUG_ERROR(fmt, ...)    printf("\x1B[31m[BOOT <%d>]: " fmt "\r\n\x1B[0m", __LINE__, ##__VA_ARGS__)
#else
#define BOOT_DEBUG(fmt, ...)
#define BOOT_DEBUG_GREEN(fmt, ...)
#define BOOT_DEBUG_ERROR(fmt, ...)
#endif /* DEBUG. */

#define BLOCK_OFFSET_SLOT14 2 
// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/
extern ATCAIfaceCfg atecc608_0_init_data;
uint8_t __attribute__((persistent, address(0x20000000))) mau8JumpSignature[32];

uint32_t image_address;
uint8_t digest[ATCA_SHA2_256_DIGEST_SIZE];
char displaystr[200];
size_t displaylen;
ATCA_STATUS status;

APP_DATA appData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;



    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            printf(TERM_YELLOW"\r\n########################################\r\n");
            printf(TERM_CYAN"keySTREAM FoTA Usecase - Bootloader\r\n");
            printf("   Built: %s %s\r\n",  __DATE__, __TIME__);
            printf("   Bootloader Version: %s\r\n", BOOT_COMPONENT_VERSION);
            printf(TERM_YELLOW"########################################\r\n"TERM_RESET);

            if (ATCA_SUCCESS != (status = atcab_init(&atecc608_0_init_data)))
            {
                BOOT_DEBUG_ERROR("ECC608-TMNGTLS Initialization failed with status {%02X}.", status);
                appData.state = APP_STATE_FINISH;
                break;
            }
            BOOT_DEBUG_GREEN("ECC608-TMNGTLS Initialization is successful");
            appData.state = APP_STATE_READ_SER_NUM;
            break;
        }

        case APP_STATE_READ_SER_NUM:
        {
            uint8_t sernum[ATCA_SERIAL_NUM_SIZE];
            if (ATCA_SUCCESS != (status = atcab_read_serial_number(sernum)))
            {
                BOOT_DEBUG_ERROR("Read Serial Number failed with status {%02X}.", status);
                appData.state = APP_STATE_FINISH;
                break;
            }
            displaylen = sizeof(displaystr);
            atcab_bin2hex(sernum, ATCA_SERIAL_NUM_SIZE, displaystr, &displaylen);
            BOOT_DEBUG("Serial Number: %s", displaystr);
            appData.state = APP_STATE_CHECK_LIFECYCLE;
            break;
        }

        case APP_STATE_CHECK_LIFECYCLE:
        {
            uint8_t lifecycle_raw[LIFECYCLE_STATE_SIZE] = {0};
            BL_LifeCycleState lifecycle = BL_LIFE_CYCLE_STATE_INVALID;

            /* --- Step 1: Read life cycle state from ATECC608 slot 8 --- */
            status = atcab_read_zone(ATCA_ZONE_DATA,
                                     LIFECYCLE_STATE_SLOT,
                                     LIFECYCLE_STATE_BLOCK,
                                     LIFECYCLE_STATE_OFFSET,
                                     lifecycle_raw,
                                     LIFECYCLE_STATE_SIZE);
            if (ATCA_SUCCESS != status)
            {
                BOOT_DEBUG_ERROR("Reading life cycle state failed with status {%02X}", status);
                appData.state = APP_STATE_BANK_SWAP_ON_FAIL;
                break;
            }

            displaylen = sizeof(displaystr);
            atcab_bin2hex(lifecycle_raw, LIFECYCLE_STATE_SIZE, displaystr, &displaylen);
            BOOT_DEBUG("Life cycle state raw: %s", displaystr);

            for (uint8_t idx = 0;
                 idx < (uint8_t)(sizeof(bl_lifecycle_nvm_data) / sizeof(bl_lifecycle_nvm_data[0]));
                 idx++)
            {
                if (0 == memcmp(bl_lifecycle_nvm_data[idx], lifecycle_raw, LIFECYCLE_STATE_SIZE))
                {
                    lifecycle = (BL_LifeCycleState)idx;
                    break;
                }
            }

            if (BL_LIFE_CYCLE_STATE_INVALID == lifecycle)
            {
                BOOT_DEBUG_ERROR("Unknown life cycle state, aborting");
                appData.state = APP_STATE_BANK_SWAP_ON_FAIL;
                break;
            }

            BOOT_DEBUG("Life cycle state: %s", bl_lifecycle_state_name[lifecycle]);

            /* --- Step 2: Check whether slot 14 holds a provisioned public key.
             * Mirrors lstorageOperation(): uses calib_read_zone() directly with
             * the ATCADevice pointer from atcab_get_device().
             *
             * Slot 14 ECC-public-key on-chip layout (one word = 4 bytes):
             *   Block 0, offset 0 : 4-byte pad  (always 0x00 even when provisioned)
             *   Block 0, offset 1 : PubKey X[0..3]  <-- first real key bytes
             * Reading offset 1 is sufficient: all-zero or all-0xFF means blank. */
            bool slot14_key_valid = false;
            ATCADevice device = atcab_get_device();
            if (NULL != device)
            {
                uint8_t slot14_word[4u] = {0};
                ATCA_STATUS rd_status = calib_read_zone(device,
                                                        ATCA_ZONE_DATA,
                                                        14u,   /* slot  */
                                                        0u,    /* block  */
                                                        1u,    /* word offset (past 4-byte pad) */
                                                        slot14_word,
                                                        4u);
                if (ATCA_SUCCESS == rd_status)
                {
                    displaylen = sizeof(displaystr);
                    atcab_bin2hex(slot14_word, 4u, displaystr, &displaylen);
                    BOOT_DEBUG("Slot14 key word (blk0/off1): %s", displaystr);

                    bool is_zero = (slot14_word[0] == 0x00u && slot14_word[1] == 0x00u &&
                                    slot14_word[2] == 0x00u && slot14_word[3] == 0x00u);

                    bool is_ff   = (slot14_word[0] == 0xFFu && slot14_word[1] == 0xFFu &&
                                    slot14_word[2] == 0xFFu && slot14_word[3] == 0xFFu);

                    slot14_key_valid = (!is_zero && !is_ff);
                }
                else
                {
                    BOOT_DEBUG_ERROR("calib_read_zone(slot14) failed with status {%02X}", rd_status);
                }
            }
            else
            {
                BOOT_DEBUG_ERROR("atcab_get_device() returned NULL");
            }

            /* --- Decision ---
             * ( lifecycle == PROVISIONED ) && ( slot14 key exists )
             *   -> proceed with SHA256 + signature verification
             * else
             *   -> skip signature verification and boot the application directly */
            if (((lifecycle == BL_LIFE_CYCLE_STATE_PROVISIONED) || (lifecycle == BL_LIFE_CYCLE_STATE_CON_REQ)) && slot14_key_valid)
            {
                BOOT_DEBUG_GREEN("Slot14 key valid: proceeding with signature verification");
                appData.state =  APP_STATE_CALCULATE_APP_DIGEST;
            }
            else
            {
                if (lifecycle != BL_LIFE_CYCLE_STATE_PROVISIONED)
                {
                    BOOT_DEBUG("Lifecycle state '%s': skipping signature verification, booting directly",
                               bl_lifecycle_state_name[lifecycle]);
                }
                else
                {
                    BOOT_DEBUG("Slot14 key not provisioned: skipping signature verification, booting directly");
                }

                /* Resolve image_address from app metadata before jumping */
                firmware_metadata_t* app_meta_skip =
                    (firmware_metadata_t*)bootloader_metadata.application_metadata_start;

                image_address = app_meta_skip->image_address;
                if (image_address == 0xFFFFFFFF)
                {
                    BOOT_DEBUG_ERROR("App Image address is invalid ie 0xFFFFFFFF, cannot boot");
                    appData.state = APP_STATE_BANK_SWAP_ON_FAIL;
                }
                else
                {
                    appData.state = APP_STATE_JUMP_TO_APPLICATION;
                }
            }
            break;
        }

        case APP_STATE_CALCULATE_APP_DIGEST:
        {
            firmware_metadata_t* app_meta = (firmware_metadata_t*)bootloader_metadata.application_metadata_start;
            image_address = app_meta->image_address;
            uint32_t image_size = app_meta->image_size;

            if (image_address == 0xFFFFFFFF || image_size == 0xFFFFFFFF)
            {
                BOOT_DEBUG_ERROR("App Image address or size is invalid ie 0xFFFFFFFF");
                appData.state = APP_STATE_BANK_SWAP_ON_FAIL;
                break;
            }

            BOOT_DEBUG("SHA256 calculation is started for 0x%lX:0x%lX", image_address, image_size);
            if (ATCA_SUCCESS != (status = atcac_sw_sha2_256((uint8_t *)image_address, (size_t)image_size, digest))) {
                BOOT_DEBUG_ERROR("atcac_sw_sha2_256 failed with status {%02X}", status);
                appData.state = APP_STATE_BANK_SWAP_ON_FAIL;
                break;
            }
            displaylen = sizeof(displaystr);
            atcab_bin2hex(digest, sizeof(digest), displaystr, &displaylen);
            BOOT_DEBUG("Digest: \r\n%s", displaystr);
            appData.state = APP_STATE_VERIFY_APP_SIGNATURE;
            break;
        }

        case APP_STATE_VERIFY_APP_SIGNATURE:
        {
            bool is_verified = false;
            firmware_metadata_t* app_meta = (firmware_metadata_t*)bootloader_metadata.application_metadata_start;
            uint32_t signature_address = app_meta->signature_address;

            if (signature_address == 0xFFFFFFFF)
            {
                BOOT_DEBUG_ERROR("App Signature address is invalid ie 0xFFFFFFFF");
                appData.state = APP_STATE_BANK_SWAP_ON_FAIL;
                break;
            }
           is_verified =  VerifySignature( signature_address);

            if (!is_verified)
            {
                BOOT_DEBUG_ERROR("Application Signature Verification: Failed");
                appData.state = APP_STATE_BANK_SWAP_ON_FAIL;
            }
            else
           {
                BOOT_DEBUG_GREEN("Application Signature Verification: Success");
                appData.state = APP_STATE_JUMP_TO_APPLICATION;
            }

            break;
        }

        case APP_STATE_JUMP_TO_APPLICATION:
        {
            uint32_t msp = *(uint32_t *)(image_address);
            BOOT_DEBUG("Jumping to the application at location 0x%lX", image_address);
            if (msp == 0xffffffffU)
            {
                BOOT_DEBUG_ERROR("MSP pointer is invalid ie 0xFFFFFFFF");
                appData.state = APP_STATE_BANK_SWAP_ON_FAIL;
                break;
            }
            strcpy((char*)mau8JumpSignature, "ExecuteAppImage");
            NVIC_SystemReset();
        }

        case APP_STATE_BANK_SWAP_ON_FAIL:
        {
            /* Assuming that the other bank has valid image and allowing it to execute */
            uint32_t *app_vector = (uint32_t *)PASSIVE_BANK_BASE_ADDRESS;
            uint32_t initial_sp = app_vector[0];
            uint32_t reset_handler = app_vector[1];
            if ((initial_sp != 0xFFFFFFFF) && (reset_handler != 0xFFFFFFFF)) {
                BOOT_DEBUG("Switching Memory Bank");
                NVMCTRL_BankSwap();
            }
            else {
                BOOT_DEBUG_ERROR("No Initial SP and Reset handler at other bank(0x%lX)", PASSIVE_BANK_BASE_ADDRESS);
                appData.state = APP_STATE_FINISH;
            }
            break;
        }

        case APP_STATE_FINISH:
            break;

        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

void jumpToApplicationImage(void)
{
    firmware_metadata_t* app_meta = (firmware_metadata_t*)bootloader_metadata.application_metadata_start;
    uint32_t app_addr = app_meta->image_address;

    if(0 == strcmp((const char*)mau8JumpSignature, "ExecuteAppImage"))
    {
        strcpy((char*)mau8JumpSignature, "InvalidateThisData");
        __disable_irq();
        uint32_t msp            = *(uint32_t *)(app_addr);
        uint32_t reset_vector   = *(uint32_t *)(app_addr + 4U);
        SCB->VTOR = app_addr & SCB_VTOR_TBLOFF_Msk;
        __enable_irq();
        __set_MSP(msp);
        asm("bx %0"::"r" (reset_vector));
    }
}

bool VerifySignature ( uint32_t signature_address )
{
    // Signature verification using Slot14 only.
    bool is_verified = false;
    const uint8_t sboot_pubkey_slot = 14;
    uint8_t slot14_full_pubkey[64] = {0};
    ATCA_STATUS status;

    uint16_t block_offset = 0;

    BOOT_DEBUG("Signature location: 0x%lX", signature_address);
    displaylen = sizeof(displaystr);

    atcab_bin2hex((uint8_t*)signature_address, 64, displaystr, &displaylen);
    BOOT_DEBUG("Signature: \r\n%s", displaystr);

    /* Read Block 0 (first 32 bytes of public key) */
    /* Read Block 1 (second 32 bytes of public key) */
    for ( uint16_t ui16loopCnt = 0; ui16loopCnt < BLOCK_OFFSET_SLOT14; ++ui16loopCnt)
    {
        status = atcab_read_zone(ATCA_ZONE_DATA,
                            sboot_pubkey_slot,
                            ui16loopCnt,  /* block 0 */
                            0,  /* offset */
                            &slot14_full_pubkey[block_offset],
                            32);

        if (ATCA_SUCCESS != status)
        {
            BOOT_DEBUG_ERROR("atcab_read_zone(slot%d, block0) failed {%02X}: key absent, booting directly",
                                sboot_pubkey_slot, status);
            appData.state = APP_STATE_JUMP_TO_APPLICATION;
            break;
        }

        block_offset += 32;
    }

    displaylen = sizeof(displaystr);
    atcab_bin2hex(slot14_full_pubkey, 64, displaystr, &displaylen);
    BOOT_DEBUG("Slot%d Public Key (X||Y, 64B):\r\n%s", sboot_pubkey_slot, displaystr);

    /* Debug: Print digest being verified */
    displaylen = sizeof(displaystr);
    atcab_bin2hex(digest, 32, displaystr, &displaylen);
    BOOT_DEBUG("Digest to verify (32B):\r\n%s", displaystr);

    /* Debug: Print signature being verified */
    displaylen = sizeof(displaystr);
    atcab_bin2hex((uint8_t*)signature_address, 64, displaystr, &displaylen);
    BOOT_DEBUG("Signature to verify (64B):\r\n%s", displaystr);

    BOOT_DEBUG("Signature verification with Slot%d (using verify_extern)", sboot_pubkey_slot);

    /* Use atcab_verify_extern instead of atcab_verify_stored because slot 14
        * was written via encrypted-write with raw 64-byte (X||Y) format,
        * NOT the ECC public key slot format that requires 4-byte padding
        * before X and before Y. atcab_verify_stored would fail because the
        * chip parses slot bytes as: [4B pad][32B X][4B pad][32B Y].
        * atcab_verify_extern accepts the raw 64-byte public key directly.
        * 
    */
    status = atcab_verify_extern(digest, (uint8_t*)signature_address, slot14_full_pubkey, &is_verified);

    if (!is_verified)
    {
        BOOT_DEBUG_ERROR("Signature Verification is:Failed-0x%lX~Status:0x%02X", signature_address , status);
        appData.state = APP_STATE_BANK_SWAP_ON_FAIL;
    }
    else 
    {
        BOOT_DEBUG_GREEN("Signature Verification is:Success-0x%lX", signature_address);
        appData.state = APP_STATE_JUMP_TO_APPLICATION;
    }

    return is_verified;
}


/*******************************************************************************
 End of File
 */

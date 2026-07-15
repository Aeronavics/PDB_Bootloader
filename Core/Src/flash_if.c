/*
 * flash_if.c
 *
 *  Created on: Nov 14, 2024
 *      Author: jmorritt
 */

/* Includes ------------------------------------------------------------------*/
#include "flash_if.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Unlocks Flash for write access
 * @param  None
 * @retval None
 */
void FLASH_If_Init(void) {
    /* Unlock the Program memory */
    HAL_FLASH_Unlock();

    /* Clear all FLASH flags */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
    /* Unlock the Program memory */
    HAL_FLASH_Lock();
}
/**
 * @brief  This function does an erase of a single user flash page
 * @param  start: start of page to erase
 * @retval FLASHIF_OK : user flash area successfully erased
 *         FLASHIF_ERASEKO : error occurred
 */
uint32_t FLASH_If_Erase_Page(uint32_t start) {
    uint32_t NbrOfPages = 0;
    uint32_t PageError = 0;
    FLASH_EraseInitTypeDef pEraseInit;
    HAL_StatusTypeDef status = HAL_OK;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    /* Get the sector where start the user flash area */
    if (start < USER_FLASH_BANK1_END_ADDRESS) {
        NbrOfPages = 1;//((USER_FLASH_BANK1_END_ADDRESS + 1) - start) / FLASH_PAGE_SIZE;
        pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
        pEraseInit.PageAddress = start;
        pEraseInit.Banks = FLASH_BANK_1;
        pEraseInit.NbPages = NbrOfPages;
        status = HAL_FLASHEx_Erase(&pEraseInit, &PageError);
    }
    /* Lock the Flash to disable the flash control register access (recommended
       to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock();

    if (status != HAL_OK) {
        /* Error occurred while page erase */
        return FLASHIF_ERASEKO;
    }

    return FLASHIF_OK;
}

/**
 * @brief  This function does an erase of all user flash area
 * @param  start: start of user flash area
 * @retval FLASHIF_OK : user flash area successfully erased
 *         FLASHIF_ERASEKO : error occurred
 */
uint32_t FLASH_If_Erase(uint32_t start) {
    uint32_t NbrOfPages = 0;
    uint32_t PageError = 0;
    FLASH_EraseInitTypeDef pEraseInit;
    HAL_StatusTypeDef status = HAL_OK;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    /* Get the sector where start the user flash area */
    if (start < USER_FLASH_BANK1_END_ADDRESS) {
        NbrOfPages = ((USER_FLASH_BANK1_END_ADDRESS + 1) - start) / FLASH_PAGE_SIZE;
        pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
        pEraseInit.PageAddress = start;
        pEraseInit.Banks = FLASH_BANK_1;
        pEraseInit.NbPages = NbrOfPages;
        status = HAL_FLASHEx_Erase(&pEraseInit, &PageError);
    }
    /* Lock the Flash to disable the flash control register access (recommended
       to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock();

    if (status != HAL_OK) {
        /* Error occurred while page erase */
        return FLASHIF_ERASEKO;
    }

    return FLASHIF_OK;
}

/* Firmware image validation -------------------------------------------------*/

/* Validity record stored at FIRMWARE_INFO_ADDRESS (reserved flash page below the
   application). Written only once a complete image has been downloaded, so a
   power loss mid-update leaves the record absent and the board stays in the
   bootloader on the next power cycle instead of jumping into a corrupt image. */
typedef struct
{
    uint32_t magic;    /* FIRMWARE_INFO_MAGIC when a verified image is present */
    uint32_t length;   /* application image length in bytes                    */
    uint32_t crc;      /* Firmware_CRC32 over [FIRMWARE_APP_ADDRESS, +length)  */
    uint32_t reserved; /* pad to a 64-bit doubleword                           */
} FirmwareInfo;

uint32_t Firmware_CRC32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++)
        {
            uint32_t mask = (uint32_t)(-(int32_t)(crc & 1u));
            crc = (crc >> 1) ^ (0xEDB88420u & mask);
        }
    }
    return ~crc;
}

/* Legacy check: the application's initial stack pointer points into SRAM. Cheap,
   but only proves the first word looks plausible - not that the whole image is
   intact. Used as the fallback for images with no validity record (debugger
   flash). */
static bool app_vector_looks_valid(void)
{
    return ((*(const volatile uint32_t *)FIRMWARE_APP_ADDRESS) & 0x2FFE0000u) == 0x20000000u;
}

bool Firmware_Image_Confirmed(void)
{
    const FirmwareInfo *info = (const FirmwareInfo *)FIRMWARE_INFO_ADDRESS;

    /* Only a finalised CAN update writes this record, so a match means the whole
       image was received and its CRC verified - a PROVEN image, not merely a
       plausible one. */
    if (info->magic != FIRMWARE_INFO_MAGIC)
    {
        return false;
    }
    if ((info->length == 0) ||
        (info->length > (USER_FLASH_BANK1_END_ADDRESS - FIRMWARE_APP_ADDRESS)))
    {
        return false;
    }
    if (!app_vector_looks_valid())
    {
        return false;
    }
    return Firmware_CRC32((const uint8_t *)FIRMWARE_APP_ADDRESS, info->length) == info->crc;
}

bool Firmware_Image_Valid(void)
{
    const FirmwareInfo *info = (const FirmwareInfo *)FIRMWARE_INFO_ADDRESS;

    /* A CAN update that started but never finished - never boot it. */
    if (info->magic == FIRMWARE_INFO_INPROGRESS)
    {
        return false;
    }

    /* No validity record at all (erased page). This is a debugger/ST-Link flashed
       image, or a board that predates this scheme - fall back to the legacy
       vector-table check so those can still be booted. */
    if (info->magic != FIRMWARE_INFO_MAGIC)
    {
        return app_vector_looks_valid();
    }

    return Firmware_Image_Confirmed();
}

/* Writes the record page. Returns false if the erase or the write fails - most
   likely because the page is write protected. The record lives in the
   bootloader's own flash region, which is exactly the region a project may choose
   to WRP, so this must never fail silently: without the record the boot check
   degrades to the legacy vector-table test and the anti-brick protection is gone. */
static bool firmware_info_store(const FirmwareInfo *info)
{
    if (FLASH_If_Erase_Page(FIRMWARE_INFO_ADDRESS) != FLASHIF_OK)
    {
        return false;
    }
    /* FLASH_If_Write reads back and compares each word it programs, so an OK
       return also confirms the record actually landed in flash. */
    if (FLASH_If_Write(FIRMWARE_INFO_ADDRESS, (uint32_t *)info, sizeof(*info) / 4) != FLASHIF_OK)
    {
        return false;
    }
    return true;
}

bool Firmware_Image_Invalidate(void)
{
    /* Mark an update as in progress. This is written *before* the application is
       erased, so if the update is interrupted the record stays INPROGRESS and the
       board refuses to boot the half-written image (it stays in the bootloader,
       recoverable, across power cycles). */
    FirmwareInfo info __attribute__((aligned(8)));
    info.magic = FIRMWARE_INFO_INPROGRESS;
    info.length = 0xFFFFFFFFu;
    info.crc = 0xFFFFFFFFu;
    info.reserved = 0xFFFFFFFFu;

    return firmware_info_store(&info);
}

bool Firmware_Image_Finalize(uint32_t length)
{
    FirmwareInfo info __attribute__((aligned(8)));
    info.magic = FIRMWARE_INFO_MAGIC;
    info.length = length;
    info.crc = Firmware_CRC32((const uint8_t *)FIRMWARE_APP_ADDRESS, length);
    info.reserved = 0xFFFFFFFFu;

    return firmware_info_store(&info);
}

/* Public functions ---------------------------------------------------------*/

/**
 * @brief  This function writes a data buffer in flash (data are 32-bit aligned).
 * @note   After writing data buffer, the flash content is checked.
 * @param  destination: start address for target location
 * @param  p_source: pointer on buffer with data to write
 * @param  length: length of data buffer (unit is 32-bit word)
 * @retval uint32_t 0: Data successfully written to Flash memory
 *         1: Error occurred while writing data in Flash memory
 *         2: Written Data in flash memory is different from expected one
 */
uint32_t FLASH_If_Write(uint32_t destination, uint32_t *p_source, uint32_t length) {
    uint32_t i = 0;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    for (i = 0; (i < length) && (destination <= (USER_FLASH_BANK1_END_ADDRESS - 4)); i++) {
        /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
           be done by word */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, destination, *(uint32_t*) (p_source + i)) == HAL_OK) {
            /* Check the written value */
            if (*(uint32_t*) destination != *(uint32_t*) (p_source + i)) {
                /* Flash content doesn't match SRAM content */
                return (FLASHIF_WRITINGCTRL_ERROR);
            }
            /* Increment FLASH destination address */
            destination += 4;
        } else {
            /* Error occurred while writing data in Flash memory */
            return (FLASHIF_WRITING_ERROR);
        }
    }

    /* Lock the Flash to disable the flash control register access (recommended
       to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock();

    return (FLASHIF_OK);
}

/**
 * @brief  Returns the write protection status of application flash area.
 * @param  None
 * @retval If a sector in application area is write-protected returned value is a combinaison
            of the possible values : FLASHIF_PROTECTION_WRPENABLED, FLASHIF_PROTECTION_PCROPENABLED, ...
 *         If no sector is write-protected FLASHIF_PROTECTION_NONE is returned.
 */
uint32_t FLASH_If_GetWriteProtectionStatus(void) {
    uint32_t ProtectedPAGE = FLASHIF_PROTECTION_NONE;
    FLASH_OBProgramInitTypeDef OptionsBytesStruct;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    /* Check if there are write protected sectors inside the user flash area ****/
    HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);

    /* Lock the Flash to disable the flash control register access (recommended
       to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock();

    /* Get pages already write protected ****************************************/
    ProtectedPAGE = ~(OptionsBytesStruct.WRPPage) & FLASH_PAGE_TO_BE_PROTECTED;

    /* Check if desired pages are already write protected ***********************/
    if (ProtectedPAGE != 0) {
        /* Some sectors inside the user flash area are write protected */
        return FLASHIF_PROTECTION_WRPENABLED;
    } else {
        /* No write protected sectors inside the user flash area */
        return FLASHIF_PROTECTION_NONE;
    }
}

/**
 * @brief  Configure the write protection status of user flash area.
 * @param  protectionstate : FLASHIF_WRP_DISABLE or FLASHIF_WRP_ENABLE the protection
 * @retval uint32_t FLASHIF_OK if change is applied.
 */
uint32_t FLASH_If_WriteProtectionConfig(uint32_t protectionstate) {
    uint32_t ProtectedPAGE = 0x0;
    FLASH_OBProgramInitTypeDef config_new, config_old;
    HAL_StatusTypeDef result = HAL_OK;


    /* Get pages write protection status ****************************************/
    HAL_FLASHEx_OBGetConfig(&config_old);

    /* The parameter says whether we turn the protection on or off */
    config_new.WRPState = (protectionstate == FLASHIF_WRP_ENABLE ? OB_WRPSTATE_ENABLE : OB_WRPSTATE_DISABLE);

    /* We want to modify only the Write protection */
    config_new.OptionType = OPTIONBYTE_WRP;

    /* No read protection, keep BOR and reset settings */
    config_new.RDPLevel = OB_RDP_LEVEL_0;
    config_new.USERConfig = config_old.USERConfig;
    /* Get pages already write protected ****************************************/
    ProtectedPAGE = config_old.WRPPage | FLASH_PAGE_TO_BE_PROTECTED;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    /* Unlock the Options Bytes *************************************************/
    HAL_FLASH_OB_Unlock();

    /* Erase all the option Bytes ***********************************************/
    result = HAL_FLASHEx_OBErase();

    if (result == HAL_OK) {
        config_new.WRPPage = ProtectedPAGE;
        result = HAL_FLASHEx_OBProgram(&config_new);
    }

    return (result == HAL_OK ? FLASHIF_OK : FLASHIF_PROTECTION_ERRROR);
}


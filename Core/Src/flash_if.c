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
 * @param  page: page to erase
 * @retval FLASHIF_OK : user flash area successfully erased
 *         FLASHIF_ERASEKO : error occurred
 */
uint32_t FLASH_If_Erase_Page(uint32_t page) {
    uint32_t NbrOfPages = 0;
    uint32_t PageError = 0;
    FLASH_EraseInitTypeDef pEraseInit;
    HAL_StatusTypeDef status = HAL_OK;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    /* Get the sector where start the user flash area */
    if ((USER_FLASH_BANK1_START_ADDRESS + (page * 2048)) < USER_FLASH_BANK1_END_ADDRESS) {
        NbrOfPages = 1;//((USER_FLASH_BANK1_END_ADDRESS + 1) - start) / FLASH_PAGE_SIZE;
        pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
        pEraseInit.PageAddress = USER_FLASH_BANK1_START_ADDRESS + (page * 2048);
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
    if (start < USER_FLASH_BANK1_END_ADDRESS && start >= USER_FLASH_BANK1_START_ADDRESS) {
        NbrOfPages = ((USER_FLASH_BANK1_END_ADDRESS + 1) - start) / FLASH_PAGE_SIZE;
        pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
        pEraseInit.PageAddress = (start - USER_FLASH_BANK1_START_ADDRESS);
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

    for (i = 0; (i < length) && (destination <= (USER_FLASH_BANK1_END_ADDRESS - 4)); i = i + 2) {
        /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
           be done by word */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, destination, *(uint64_t*) (p_source + i)) == HAL_OK) {
            /* Check the written value */
            if (*(uint64_t*) destination != *(uint64_t*) (p_source + i)) {
                /* Flash content doesn't match SRAM content */
                return (FLASHIF_WRITINGCTRL_ERROR);
            }
            /* Increment FLASH destination address */
            destination += 8;
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


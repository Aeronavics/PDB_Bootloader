/*
 * menu.c
 *
 *  Created on: Nov 14, 2024
 *      Author: jmorritt
 */

/* Includes ------------------------------------------------------------------*/

#include "menu.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

uint32_t FlashProtection = 0;
uint8_t aFileName[FILE_NAME_LENGTH];
//relevant factor in deciding if the timeout should occur

/* Private function prototypes -----------------------------------------------*/
void SerialDownload(bool reprogram_flash);
void SerialUpload(void);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Download a file via serial port
 * @param  None
 * @retval None
 */
void SerialDownload(bool reprogram_flash) {
    uint8_t eeprom_boot_value;
    uint8_t eeprom_flash_status;
    uint8_t number[11] = {0};
    uint32_t size = 0;
    COM_StatusTypeDef result;

    Serial_PutString((uint8_t *) "Waiting for the file to be sent ... (press 'a' to abort)\n\r");
    result = Ymodem_Receive(&size);
    if (result == COM_OK) {
        Serial_PutString((uint8_t *) "\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: ");
        Serial_PutString(aFileName);
        Int2Str(number, size);
        Serial_PutString((uint8_t *) "\n\r Size: ");
        Serial_PutString(number);
        Serial_PutString((uint8_t *) " Bytes\r\n");
        Serial_PutString((uint8_t *) "-------------------\n");
        //        if (reprogram_flash) {
        //            //do a normal boot
        //            eeprom_boot_value = NORMAL_BOOT;
        //            i2c_tx_multimaster(&hi2c1, EEPROM_ADDRESS, BOOTLOADER_ADDRESS, 1, &eeprom_boot_value, 1, 10000);
        //        }
        //if we are currently writing, lets just wait a little
        HAL_Delay(10);
        //        eeprom_flash_status = FLASH_SUCCESSFUL;
        //        i2c_tx_multimaster(&hi2c1, EEPROM_ADDRESS, BOOTLOADER_STATUS_ADDRESS, 1, &eeprom_flash_status, 1, 10000);
    } else if (result == COM_LIMIT) {
        Serial_PutString((uint8_t *) "\n\n\rThe image size is higher than the allowed space memory!\n\r");
    } else if (result == COM_DATA) {
        Serial_PutString((uint8_t *) "\n\n\rVerification failed!\n\r");
    } else if (result == COM_ABORT) {
        Serial_PutString((uint8_t *) "\r\n\nAborted by user.\n\r");
    } else {
        Serial_PutString((uint8_t *) "\n\rFailed to receive the file!\n\r");
    }
}

void i2c_tx_multimaster(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    while (HAL_I2C_Mem_Write(hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout) != HAL_OK) {
        HAL_Delay(10);
    }
}

void i2c_rx_multimaster(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    //waits for the device to become ready
    while (HAL_I2C_Mem_Read(hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout) != HAL_OK) {
        HAL_Delay(10);
    }

}

/**
 * @brief  Upload a file via serial port.
 * @param  None
 * @retval None
 */
void SerialUpload(void) {
    uint8_t status = 0;

    Serial_PutString((uint8_t *) "\n\n\rSelect Receive File\n\r");

    HAL_UART_Receive(&UartHandle, &status, 1, RX_TIMEOUT);
    if (status == CRC16) {
        /* Transmit the flash image through ymodem protocol */
        status = Ymodem_Transmit((uint8_t*) APPLICATION_ADDRESS, (const uint8_t*) "UploadedFlashImage.bin", USER_FLASH_SIZE);

        if (status != 0) {
            Serial_PutString((uint8_t *) "\n\rError Occurred while Transmitting File\n\r");
        } else {
            Serial_PutString((uint8_t *) "\n\rFile uploaded successfully \n\r");
        }
    }
}

/**
 * @brief  Display the Main Menu on HyperTerminal
 * @param  None
 * @retval None
 */
void Main_Menu(bool reprogram_flash, bool timeout) {
    uint8_t key = 0;
    /* Test if any sector of Flash memory where user application will be loaded is write protected */
    FlashProtection = FLASH_If_GetWriteProtectionStatus();

    while (1) {

        Serial_PutString((uint8_t *) "\r\n=================== Main Menu ============================\r\n\n");
        Serial_PutString((uint8_t *) "  Download image to the internal Flash ----------------- 1\r\n\n");
        Serial_PutString((uint8_t *) "  Upload image from the internal Flash ----------------- 2\r\n\n");
        Serial_PutString((uint8_t *) "  Execute the loaded application ----------------------- 3\r\n\n");

        /* Clean the input path */
        __HAL_UART_FLUSH_DRREGISTER(&UartHandle);

        /* Receive key */ //5K
        key = 0;
        HAL_UART_Receive(&UartHandle, &key, 1, 5000);
        if (boot_to_application) {
            Set_Boot();
            NVIC_SystemReset();
        }

        switch (key) {
            case '1':
                /* Download user application in the Flash */
                //we have got here - therefore we do not want to boot
                //this is handled in the ISR
                is_timeout_relevant = false;
                //put the bootloader into an unknown flash state.
                //this ensures that if we have not successfully uploaded code, we do not miss the bootloader
                //                eeprom_flash_status = FLASH_UNKNOWN;
                //                i2c_tx_multimaster(&hi2c1, EEPROM_ADDRESS, BOOTLOADER_STATUS_ADDRESS, 1, &eeprom_flash_status, 1, 10000);
                SerialDownload(reprogram_flash);

                break;
            case '2':
                /* Upload user application from the Flash */
                is_timeout_relevant = false;
                SerialUpload();
                break;
            case '3':
                Serial_PutString((uint8_t *) "Start program execution......\r\n\n");
                Set_Boot();
                NVIC_SystemReset();
                break;
            default:
                Serial_PutString((uint8_t *) "Invalid Number ! ==> The number should be either 1, 2, 3 or 4\r");
                break;
        }
    }
}



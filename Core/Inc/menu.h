/*
 * menu.h
 *
 *  Created on: Nov 14, 2024
 *      Author: jmorritt
 */

#ifndef INC_MENU_H_
#define INC_MENU_H_


#ifdef __cplusplus
extern "C" {
#endif

    /* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "flash_if.h"
#include "ymodem.h"
#include "i2c.h"
#include "tim.h"
#include "can.h"
#include <stdbool.h>

    //#include "stm32_iap_bootloader.hpp"
#include "common.h"


    /* Imported variables --------------------------------------------------------*/
    extern uint8_t aFileName[FILE_NAME_LENGTH];
    extern bool is_timeout_relevant;
    extern bool boot_to_application;

    /* Private variables ---------------------------------------------------------*/
    /* Exported types ------------------------------------------------------------*/
    /* Exported constants --------------------------------------------------------*/
    /* Exported macro ------------------------------------------------------------*/
    /* Exported functions ------------------------------------------------------- */
    void Main_Menu(bool reprogram_flash, bool timeout);
//    void Do_Boot(void);
    //    void i2c_rx_multimaster(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
    //    void i2c_tx_multimaster(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);

#ifdef __cplusplus
}
#endif


#endif /* INC_MENU_H_ */

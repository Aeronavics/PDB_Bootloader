/*
 * common.h
 *
 *  Created on: Nov 14, 2024
 *      Author: jmorritt
 */

#ifndef INC_COMMON_H_
#define INC_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

    /* Includes ------------------------------------------------------------------*/
#include "stm32f1xx.h"
#include "usart.h"
#include "can.h"
#include "gpio.h"
#include "tim.h"
#include <stdbool.h>


#define UartHandle huart1
    //#include "stm3210e_eval.h"

    /* Exported types ------------------------------------------------------------*/
    /* Exported constants --------------------------------------------------------*/
    /* Constants used by Serial Command Line Mode */
#define TX_TIMEOUT          ((uint32_t)100)
#define RX_TIMEOUT          HAL_MAX_DELAY

    /* Exported macro ------------------------------------------------------------*/
#define IS_CAP_LETTER(c)    (((c) >= 'A') && ((c) <= 'F'))
#define IS_LC_LETTER(c)     (((c) >= 'a') && ((c) <= 'f'))
#define IS_09(c)            (((c) >= '0') && ((c) <= '9'))
#define ISVALIDHEX(c)       (IS_CAP_LETTER(c) || IS_LC_LETTER(c) || IS_09(c))
#define ISVALIDDEC(c)       IS_09(c)
#define CONVERTDEC(c)       (c - '0')

#define CONVERTHEX_ALPHA(c) (IS_CAP_LETTER(c) ? ((c) - 'A'+10) : ((c) - 'a'+10))
#define CONVERTHEX(c)       (IS_09(c) ? ((c) - '0') : CONVERTHEX_ALPHA(c))
    //defines a option so that we only get into the bootloader once.
    //The bootloader clears this flag and on next boot we will boot into application
#define ONETIME_BOOTLOADER 0x71
    //The bootloader will not clear this flag, the application will be required to reset this. Useful for continuous development
#define MULTI_BOOTLOADER 0x12
    //similar to the multi bootloader  - this however is the default state of EEPROM.
    //If we have an unknown variable in the bootloader eeprom this is set and we just boot to bootloader
    //The difference is that we once programmed, we will set this variable to boot normally
    //All values are assumed UNSET_BOOTLOADER if they do not align to known values.
#define UNSET_BOOTLOADER 0xFF
    //bootloader ignores the IAP bootloader and boots straight to application.
#define NORMAL_BOOT 0xAB
    //if a peripheral has shutdown unexpectedly, then boot into bootloader and timeout if nothing happens
#define UNCLEAN_SHUTDOWN_BOOT 0x34

    //If a flash has occured and is successful - we will boot as normal
#define FLASH_SUCCESSFUL 0x01
    //if a flash has occured and is not successful we will not boot, and wait for reflash
#define FLASH_UNKNOWN 0xBC

#define BOOTLOADER_ADDRESS 0x00
#define BOOTLOADER_STATUS_ADDRESS 0x01
#define EEPROM_ADDRESS (0b10100000)
    /* Exported variables --------------------------------------------------------*/
    /* Exported types ------------------------------------------------------------*/
    /* Exported constants --------------------------------------------------------*/
    /* Exported macro ------------------------------------------------------------*/
    /* Exported functions ------------------------------------------------------- */


#define APPLICATION_ADDRESS     (uint32_t)0x0800C800
#define APPLICATION_OFFSET      (uint32_t)0//0x200 //offset of the actual application code (accounting for the schmoo before)
#define RAM_BOOTLOADER_ACTION_LOCATION 0x20000000
#define RAM_FASTBOOT_BYTE   0xAB //if this is set, it indicates a fastboot (which incidentaly is default)
#define RAM_DO_BOOTLOADER_BYTE   0x22 //if this is set, it indicates we should deliberatlty go into bootloader
    typedef void (*pFunction)(void);
    /* Exported functions ------------------------------------------------------- */
    void Boot(void);
    void Set_Boot(void);
    void Int2Str(uint8_t *p_str, uint32_t intnum);
    uint32_t Str2Int(uint8_t *inputstr, uint32_t *intnum);
    void Serial_PutString(uint8_t *p_string);
    HAL_StatusTypeDef Serial_PutByte(uint8_t param);
    void deinitEverything(void);

typedef void (application_t)(void);

typedef struct
{
	uint32_t		stack_addr;     // Stack Pointer
	application_t*	func_p;        // Program Counter
} JumpStruct;

#ifdef __cplusplus
}
#endif



#endif /* INC_COMMON_H_ */

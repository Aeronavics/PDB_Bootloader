/*
 * common.c
 *
 *  Created on: Nov 14, 2024
 *      Author: jmorritt
 */

#include "common.h"
//#include "stm32_iap_bootloader.hpp"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

void Set_Boot(void) {
    volatile uint8_t *param_values = (volatile uint8_t *)(RAM_BOOTLOADER_ACTION_LOCATION);
    *param_values = RAM_FASTBOOT_BYTE;
}

void Boot(void) {
//    if (((*(__IO uint32_t*) (APPLICATION_ADDRESS + APPLICATION_OFFSET)) & 0x2FFE0000) == 0x20000000) {
//
//        pFunction JumpToApplication;
//        uint32_t JumpAddress;
//        /* Jump to user application */
//        JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + APPLICATION_OFFSET + 4);
//        JumpToApplication = (pFunction) JumpAddress;
//        /* Initialize user application's Stack Pointer */
//        __set_MSP(*(__IO uint32_t*) (APPLICATION_ADDRESS + APPLICATION_OFFSET));
//        JumpToApplication();
//    }
	if (((*(__IO uint32_t*) (APPLICATION_ADDRESS)) & 0x2FFE0000) == 0x20000000) {
		const JumpStruct* vector_p = (JumpStruct*)(APPLICATION_ADDRESS);

		deinitEverything();

		/* let's do The Jump! */
		/* Jump, used asm to avoid stack optimization */
		asm("msr msp, %0; bx %1;" : : "r"(vector_p->stack_addr), "r"(vector_p->func_p));
	}
}

void deinitEverything(void)
{
	HAL_TIM_Base_MspDeInit(&htim1);
	HAL_TIM_Base_MspDeInit(&htim2);

	HAL_UART_MspDeInit(&huart1);

	HAL_CAN_MspDeInit(&hcan);

	HAL_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;
}

/**
 * @brief  Convert an Integer to a string
 * @param  p_str: The string output pointer
 * @param  intnum: The integer to be converted
 * @retval None
 */
void Int2Str(uint8_t *p_str, uint32_t intnum) {
    uint32_t i, divider = 1000000000, pos = 0, status = 0;

    for (i = 0; i < 10; i++) {
        p_str[pos++] = (intnum / divider) + 48;

        intnum = intnum % divider;
        divider /= 10;
        if ((p_str[pos - 1] == '0') & (status == 0)) {
            pos = 0;
        } else {
            status++;
        }
    }
}

/**
 * @brief  Convert a string to an integer
 * @param  p_inputstr: The string to be converted
 * @param  p_intnum: The integer value
 * @retval 1: Correct
 *         0: Error
 */
uint32_t Str2Int(uint8_t *p_inputstr, uint32_t *p_intnum) {
    uint32_t i = 0, res = 0;
    uint32_t val = 0;

    if ((p_inputstr[0] == '0') && ((p_inputstr[1] == 'x') || (p_inputstr[1] == 'X'))) {
        i = 2;
        while ((i < 11) && (p_inputstr[i] != '\0')) {
            if (ISVALIDHEX(p_inputstr[i])) {
                val = (val << 4) + CONVERTHEX(p_inputstr[i]);
            } else {
                /* Return 0, Invalid input */
                res = 0;
                break;
            }
            i++;
        }

        /* valid result */
        if (p_inputstr[i] == '\0') {
            *p_intnum = val;
            res = 1;
        }
    } else /* max 10-digit decimal input */ {
        while ((i < 11) && (res != 1)) {
            if (p_inputstr[i] == '\0') {
                *p_intnum = val;
                /* return 1 */
                res = 1;
            } else if (((p_inputstr[i] == 'k') || (p_inputstr[i] == 'K')) && (i > 0)) {
                val = val << 10;
                *p_intnum = val;
                res = 1;
            } else if (((p_inputstr[i] == 'm') || (p_inputstr[i] == 'M')) && (i > 0)) {
                val = val << 20;
                *p_intnum = val;
                res = 1;
            } else if (ISVALIDDEC(p_inputstr[i])) {
                val = val * 10 + CONVERTDEC(p_inputstr[i]);
            } else {
                /* return 0, Invalid input */
                res = 0;
                break;
            }
            i++;
        }
    }

    return res;
}

/**
 * @brief  Print a string on the HyperTerminal
 * @param  p_string: The string to be printed
 * @retval None
 */
void Serial_PutString(uint8_t *p_string) {
    uint16_t length = 0;

    while (p_string[length] != '\0') {
        length++;
    }
    HAL_UART_Transmit(&UartHandle, p_string, length, TX_TIMEOUT);
}

/**
 * @brief  Transmit a byte to the HyperTerminal
 * @param  param The byte to be sent
 * @retval HAL_StatusTypeDef HAL_OK if OK
 */
HAL_StatusTypeDef Serial_PutByte(uint8_t param) {
    return HAL_UART_Transmit(&UartHandle, &param, 1, TX_TIMEOUT);
}



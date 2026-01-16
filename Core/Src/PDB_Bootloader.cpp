/*
 * PDB_Bootloader.cpp
 *
 *  Created on: Jan 15, 2026
 *      Author: jmorritt
 */

/* Includes ------------------------------------------------------------------*/
#include "PDB_Bootloader.hpp"

bool is_timeout_relevant;
bool boot_to_application;
static volatile uint64_t counter;
//initiate drivers
static volatile bool run_sloppy_100hz = false;
static volatile bool run_sloppy_10hz = false;
static volatile bool run_sloppy_1hz = true;
extern uint8_t bootloader_info_location;

static Driver_module * const drivers[] = {
    static_cast<Driver_module*> (&Libcanard_module::get_driver())
};
static const size_t NUM_DRIVERS = sizeof (drivers) / sizeof (Driver_module*);


/* Private function prototypes -----------------------------------------------*/
static void IAP_Init(void);
void driverhost_broadcast_can(
		CanardRxTransfer const* transfer,
		uint64_t data_type_signature,
		uint16_t data_type_id,
		uint8_t* inout_transfer_id,
		uint8_t priority,
		const void* payload,
		uint16_t payload_len,
#ifdef CANARD_MULTI_IFACE
		uint8_t iface_mask,
#endif
		Driver_module * const except
)
{}
void Do_Bootloader(bool normal_boot_on_flash, bool timeout);

bool first_fire;

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void)
{
    //Check we are meant to go into bootloader mode
    //    volatile uint8_t *param_values = (volatile uint8_t *)(RAM_BOOTLOADER_ACTION_LOCATION);
    //Also check we have had an actual crash
    //if (bootloader_info_location == RAM_DO_BOOTLOADER_BYTE || __HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)
    if (bootloader_info_location == RAM_DO_BOOTLOADER_BYTE)
    {
	    //reset the iwd flag. This can cause trouble and get into an infinite loop of bootloadering
       __HAL_RCC_CLEAR_RESET_FLAGS();
        //	__HAL_RCC_CLEAR_FLAG(RCC_FLAG_IWDGRST);
        //overwrite
        bootloader_info_location = 0;
        //Carry on with bootloader
    }
    else if (bootloader_info_location == RAM_FASTBOOT_BYTE)
    {
        //overwrite
        bootloader_info_location = 0;
        Boot();
    }
    else
    {
        //we default to booting.
        Boot();
    }

    Libcanard_module::get_driver().set_name("dfu.aeronavics.PDB");
    HAL_Init();

    /* Configure the system clock to 72 MHz */
    SystemClock_Config();
    MX_GPIO_Init();

    MX_TIM2_Init();
    //    MX_IWDG_Init();
    //start the interrupt - this should take 5 seconds
    HAL_TIM_Base_Start_IT(&htim2);

    MX_USART1_UART_Init();

    HAL_GPIO_WritePin(ERROR0_GPIO_Port, ERROR0_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(ERROR1_GPIO_Port, ERROR1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(STATUS1_GPIO_Port, STATUS1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(STATUS2_GPIO_Port, STATUS2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(STATUS3_GPIO_Port, STATUS3_Pin, GPIO_PIN_SET);

    //start a uart receive, this will interrupt and cause our code to split into uart bootloader mode or into can bootloader mode
    //(or nothing! which just boots!)

    HAL_Delay(500);

    Serial_PutString((uint8_t *) "  Enter \"ymodem\" to upload code via ymodem, or init a CAN device\r\n\n");
    HAL_UART_Receive_IT(&UartHandle, (uint8_t *)uart_buffer, 1);

    while (1)
    {
            //        HAL_IWDG_Refresh(&hiwdg);

        if (true)
        {
                // Iterate over each of the drivers and run the relevant handler.
            for (size_t i = 0; i < NUM_DRIVERS; i++)
            {
                if (drivers[i] != nullptr) drivers[i]->sync_update_unthrottled();
            }
            if (ymodem_upload)
            {
                Do_Bootloader(true, false);
            }
        }
            // Check if we need to run the sloppy 100Hz tasks.
        if (run_sloppy_100hz)
        {
                // Iterate over each of the drivers and run the relevant handler.
            for (size_t i = 0; i < NUM_DRIVERS; i++)
            {
                if (drivers[i] != nullptr) drivers[i]->sync_update_100Hz();
            }
                // Clear the 100Hz task flag now that the tasks have been run.
            run_sloppy_100hz = false;
        }

            // Check if we need to run the 10Hz tasks.
        if (run_sloppy_10hz)
        {
                // Iterate over each of the drivers and run the relevant handler.
                //            display_uart_status();
            for (size_t i = 0; i < NUM_DRIVERS; i++)
            {
                if (drivers[i] != nullptr) drivers[i]->sync_update_10Hz();
            }
                // Clear the 10Hz task flag now that the tasks have been run.
            run_sloppy_10hz = false;
        }

            // Check if we need to run the 1Hz tasks.
        if (run_sloppy_1hz)
        {
                // // Iterate over each of the drivers and run the relevant handler.
            for (size_t i = 0; i < NUM_DRIVERS; i++)
            {
                if (drivers[i] != nullptr) drivers[i]->sync_update_1Hz();
            }
            HAL_GPIO_TogglePin(STATUS1_GPIO_Port, STATUS1_Pin);

            bool are_drivers_init = false;
            bool are_drivers_normal = false;
            bool are_drivers_error = false;
            for (size_t i = 0; i < NUM_DRIVERS; i++)
            {
                if (drivers[i] != nullptr)
                {
                    if (drivers[i]->get_state() == DRIVER_STATE_INIT) are_drivers_init = true;
                    if (drivers[i]->get_state() == DRIVER_STATE_NORMAL) are_drivers_normal = true;
                    if (drivers[i]->get_state() == DRIVER_STATE_ERROR) are_drivers_error = true;
                }
            }

                // Clear the 1Hz task flag now that the tasks have been run.
            run_sloppy_1hz = false;
        }
    }
}

void Do_Boot(void)
{
    if (((*(__IO uint32_t*) (APPLICATION_ADDRESS + APPLICATION_OFFSET)) & 0x2FFE0000) == 0x20000000)
    {
        Set_Boot();
        NVIC_SystemReset();
    }
    else
    {
        //jump to bootloader if we have nothing programmed
        //if we flash successfully, lets update the boot eeprom value
        Do_Bootloader(true, false);
    }
}

void Do_Bootloader(bool normal_boot_on_flash, bool timeout)
{
    /* Initialise Flash */
    FLASH_If_Init();
    /* Execute the IAP driver in order to reprogram the Flash */
    IAP_Init();
    /* Display main menu */
    Main_Menu(normal_boot_on_flash, timeout);
}

/**
 * @brief  Initialize the IAP: Configure USART.
 * @param  None
 * @retval None
 */
void IAP_Init(void)
{
    /* USART resources configuration (Clock, GPIO pins and USART registers) ----*/
    /* USART configured as follow:
          - BaudRate = 115200 baud
          - Word Length = 8 Bits
          - One Stop Bit
          - No parity
          - Hardware flow control disabled (RTS and CTS signals)
          - Receive and transmit enabled
     */
    MX_USART1_UART_Init();

}

#ifdef USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif

void HAL_UART_RxCpltCallback(UART_HandleTypeDef * huart)
{
    if (huart->Instance == USART1)
    {
        Serial_PutByte(uart_buffer[buffer_ptr]);

        if (strncmp((const char *) uart_buffer, "ymodem", 6) == 0)
        {
            //break the CAN loading
            Libcanard_module::get_driver().kill_loading = true;
            //jump to the main ymodem application
            ymodem_upload = true;
            //do not restart the uart
            return;
        }
        if (buffer_ptr > 10){
            buffer_ptr = 0;
        }

        if (uart_buffer[buffer_ptr] == 'z')
        {
            int i;
            for (int i = 0; i < buffer_ptr; i++)
            {
                uart_buffer[i] = NULL;
            }
            Serial_PutString((uint8_t *) "\r\n");
            buffer_ptr = 0;
        }
        else {
            buffer_ptr++;
        }

        HAL_UART_Receive_IT(&UartHandle, (uint8_t *)uart_buffer + buffer_ptr, 1);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef * htim)
{
    if (htim->Instance == TIM1)
    {
        if (!first_fire)
        {
            first_fire = true;
            return;
        }
        //        HAL_GPIO_TogglePin(GPIO_ALIVE_GPIO_Port, GPIO_ALIVE_Pin);
        //if we are expecting the timeout to do something then lets boot
        //we want to kill the can module from loading if it has not already and get onto booting. If there is nothing to boot this will have no effect
        //check we have an app before we kill the loading
        if (((*(__IO uint32_t*) (APPLICATION_ADDRESS + APPLICATION_OFFSET)) & 0x2FFE0000) == 0x20000000)
        {
            Libcanard_module::get_driver().kill_loading = true;
        }
        if (is_timeout_relevant)
        {
            boot_to_application = true;
            HAL_UART_AbortReceive(&huart1);
        }
    }
    if (htim->Instance == TIM2)
    {
        counter++;
        if (counter % 100 == 0)
        {
            run_sloppy_1hz = true;
        }
        if (counter % 10 == 0)
        {
            run_sloppy_10hz = true;
        }
        run_sloppy_100hz = true;

    }
}

uint64_t driverhost_get_monotonic_time_us(void)
{
    /*
     *     the counter interrupts every 10000 microseconds, so we can extrapolate this and add where we currently are up to in the timer
     */
    return (uint64_t) ((counter * 10000) + (uint64_t) __HAL_TIM_GET_COUNTER(&htim2));
}



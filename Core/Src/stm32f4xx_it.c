/**
 * @file    stm32f4xx_it.c
 * @brief   Interrupt Service Routines.
 */

#include "main.h"
#include "stm32f4xx_it.h"
#include "app_config.h"
#include "FreeRTOS.h"
#include "task.h"

extern UART_HandleTypeDef huart2;

/* Buffer for UART RX */
uint8_t rx_byte;

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
void NMI_Handler(void) { while(1) {} }
void HardFault_Handler(void) { while(1) {} }
void MemManage_Handler(void) { while(1) {} }
void BusFault_Handler(void) { while(1) {} }
void UsageFault_Handler(void) { while(1) {} }
void DebugMon_Handler(void) {}

void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/******************************************************************************/

/**
 * @brief This function handles EXTI line[15:10] interrupts.
 *        Used for the User Button (PC13).
 */
void EXTI15_10_IRQHandler(void)
{
    /* Clear the EXTI pending bit */
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        /* Give semaphore to wake up Sensor Task immediately */
        if (xButtonSemaphore != NULL)
        {
            xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
        }

        /* Context switch if needed */
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief This function handles USART2 global interrupt.
 */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

/**
 * @brief UART RX complete callback (called from HAL_UART_IRQHandler).
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        /* Push received byte into Command Queue */
        if (xCmdQueue != NULL)
        {
            xQueueSendFromISR(xCmdQueue, &rx_byte, &xHigherPriorityTaskWoken);
        }

        /* Re-arm RX for next byte */
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);

        /* Context switch if needed */
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

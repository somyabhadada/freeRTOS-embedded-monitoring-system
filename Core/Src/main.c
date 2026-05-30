/**
 * @file           : main.c
 * @brief          : Main program body
 * @author         : Somya Bhadada
 *
 * Architecture:
 * - FreeRTOS-Based Embedded Monitoring System
 * - Target: STM32F446RE (NUCLEO)
 */

#include "main.h"
#include "app_config.h"

/* Task Modules */
#include "sensor_task.h"
#include "uart_task.h"
#include "spi_task.h"
#include "monitor_task.h"
#include "logger_task.h"

/* Hardware handles */
UART_HandleTypeDef huart2;

/* ---------------------------------------------------------------------------
 * FreeRTOS Object Declarations (Allocated here, externed in app_config.h)
 * --------------------------------------------------------------------------- */
QueueHandle_t     xSensorDataQueue;
QueueHandle_t     xLogQueue;
QueueHandle_t     xCmdQueue;
SemaphoreHandle_t xButtonSemaphore;
SemaphoreHandle_t xUartMutex;

TaskHandle_t xSensorTaskHandle;
TaskHandle_t xUartTaskHandle;
TaskHandle_t xSpiTaskHandle;
TaskHandle_t xMonitorTaskHandle;
TaskHandle_t xLoggerTaskHandle;

/* Private function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

int main(void)
{
    /* 1. MCU Configuration */
    HAL_Init();
    SystemClock_Config();

    /* 2. Initialize Hardware Peripherals */
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    /* 3. Create FreeRTOS Objects (Queues, Semaphores, Mutexes) */
    xSensorDataQueue = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(SensorData_t));
    xLogQueue        = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(LogMessage_t));
    xCmdQueue        = xQueueCreate(CMD_QUEUE_LENGTH, sizeof(uint8_t));

    xButtonSemaphore = xSemaphoreCreateBinary();
    xUartMutex       = xSemaphoreCreateMutex();

    /* Ensure all RTOS objects were created successfully */
    if (xSensorDataQueue == NULL || xLogQueue == NULL || xCmdQueue == NULL ||
        xButtonSemaphore == NULL || xUartMutex == NULL)
    {
        Error_Handler();
    }

    /* 4. Create Tasks */
    xTaskCreate(vLoggerTask,  "Logger",  STACK_SIZE_LOGGER_TASK,  NULL, PRIORITY_LOGGER_TASK,  &xLoggerTaskHandle);
    xTaskCreate(vSensorTask,  "Sensor",  STACK_SIZE_SENSOR_TASK,  NULL, PRIORITY_SENSOR_TASK,  &xSensorTaskHandle);
    xTaskCreate(vSpiTask,     "SPI",     STACK_SIZE_SPI_TASK,     NULL, PRIORITY_SPI_TASK,     &xSpiTaskHandle);
    xTaskCreate(vUartTask,    "UART",    STACK_SIZE_UART_TASK,    NULL, PRIORITY_UART_TASK,    &xUartTaskHandle);
    xTaskCreate(vMonitorTask, "Monitor", STACK_SIZE_MONITOR_TASK, NULL, PRIORITY_MONITOR_TASK, &xMonitorTaskHandle);

    /* 5. Start UART RX Interrupt (for CLI input) */
    extern uint8_t rx_byte; /* Defined in stm32f4xx_it.c */
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);

    /* 6. Start the FreeRTOS Scheduler */
    vTaskStartScheduler();

    /* Should never reach here */
    while (1)
    {
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance        = USART2;
    huart2.Init.BaudRate   = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits   = UART_STOPBITS_1;
    huart2.Init.Parity     = UART_PARITY_NONE;
    huart2.Init.Mode       = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : PC13 (User Button) */
    GPIO_InitStruct.Pin  = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pin : LD2_Pin (PA5) */
    GPIO_InitStruct.Pin   = LD2_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

    /* Enable EXTI Line 13 Interrupt */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 6, 0); /* Priority 6 (Safe for FreeRTOS API) */
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

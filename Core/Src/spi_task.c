/**
 * @file    spi_task.c
 * @brief   SPI Peripheral Task — simulated SPI communication.
 * @author  Somya Bhadada
 *
 * Simulates SPI communication with an external peripheral.
 * Periodically generates mock SPI data and logs it.
 */

#include "spi_task.h"
#include "app_config.h"
#include "main.h"
#include <stdio.h>

/* Global state for CLI access */
SpiPeripheralData_t g_latest_spi_data = {0};
volatile uint8_t    g_spi_data_valid = 0;

void vSpiTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime;
    uint32_t count = 0;

    xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        /* Simulate SPI transaction */
        g_latest_spi_data.device_id = 0xA5;
        g_latest_spi_data.status_reg = 0x01;
        g_latest_spi_data.raw_value = (int16_t)(count % 1024);
        g_latest_spi_data.timestamp_ms = HAL_GetTick();
        g_spi_data_valid = 1;

        count++;

        /* Log the transaction */
        LogMessage_t log_msg;
        snprintf(log_msg.text, sizeof(log_msg.text),
                 "[SPI] Transaction %lu completed. Val: %d",
                 (unsigned long)count, g_latest_spi_data.raw_value);
        xQueueSend(xLogQueue, &log_msg, 0);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(PERIOD_SPI_TASK_MS));
    }
}

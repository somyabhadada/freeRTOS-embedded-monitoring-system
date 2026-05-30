/**
 * @file    sensor_task.c
 * @brief   Sensor Task — periodic temperature acquisition with event-driven reads.
 * @author  Somya Bhadada
 *
 * This module implements the Sensor Task, which:
 *   - Periodically reads the virtual temperature sensor every 1 second.
 *   - Sends sensor data to the UART Task via xSensorDataQueue.
 *   - Sends formatted log messages to the Logger Task via xLogQueue.
 *   - Responds to GPIO button presses (via binary semaphore) for immediate reads.
 *
 * FreeRTOS Concepts Demonstrated:
 *   - xTaskCreate / vTaskDelayUntil (periodic execution)
 *   - xQueueSend (inter-task data transfer)
 *   - xSemaphoreTake (ISR-to-task synchronization)
 *   - Task notifications (receive notification from Monitor Task)
 */

#include "sensor_task.h"
#include "sensor.h"
#include "app_config.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * Private Variables
 * --------------------------------------------------------------------------- */

/** @brief Tracks total number of sensor readings taken since boot. */
static uint32_t s_reading_count = 0;

/* ---------------------------------------------------------------------------
 * Private Helper Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief  Reads the sensor and sends data to queues.
 *
 * @param  triggered_by_button  1 if this read was triggered by GPIO interrupt,
 *                              0 if periodic.
 *
 * Inputs:  Virtual sensor module, HAL tick counter.
 * Outputs: SensorData_t to xSensorDataQueue, LogMessage_t to xLogQueue.
 *
 * Execution:
 *   1. Calls sensor_read_celsius() to get temperature.
 *   2. Packages temperature + timestamp into SensorData_t.
 *   3. Attempts non-blocking send to xSensorDataQueue.
 *   4. Formats a log string and enqueues to xLogQueue.
 */
static void sensor_take_reading(uint8_t triggered_by_button)
{
    SensorData_t data;
    LogMessage_t log_msg;

    /* Step 1: Read temperature from virtual sensor */
    data.temperature_celsius = sensor_read_celsius();
    data.timestamp_ms        = HAL_GetTick();

    s_reading_count++;

    /* Step 2: Send sensor data to UART Task (non-blocking, drop if full) */
    xQueueSend(xSensorDataQueue, &data, 0);

    /* Step 3: Format and send log message to Logger Task */
    if (triggered_by_button)
    {
        snprintf(log_msg.text, sizeof(log_msg.text),
                 "[SENSOR] Button-triggered read #%lu: %.1f C",
                 (unsigned long)s_reading_count, data.temperature_celsius);
    }
    else
    {
        snprintf(log_msg.text, sizeof(log_msg.text),
                 "[SENSOR] Periodic read #%lu: %.1f C",
                 (unsigned long)s_reading_count, data.temperature_celsius);
    }

    xQueueSend(xLogQueue, &log_msg, 0);
}

/* ---------------------------------------------------------------------------
 * Public Task Function
 * --------------------------------------------------------------------------- */

void vSensorTask(void *pvParameters)
{
    (void)pvParameters;

    TickType_t xLastWakeTime;

    /* Initialize the virtual sensor hardware */
    sensor_init();

    /* Record the initial tick for precise periodic execution using
     * vTaskDelayUntil, which prevents timing drift compared to vTaskDelay. */
    xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        /* --- Periodic sensor read --- */
        sensor_take_reading(0);

        /* --- Check for button-triggered immediate read ---
         * xSemaphoreTake with timeout 0: non-blocking check.
         * If the GPIO ISR gave the semaphore, we do an extra read. */
        if (xSemaphoreTake(xButtonSemaphore, 0) == pdTRUE)
        {
            sensor_take_reading(1);
        }

        /* --- Check for task notification from System Monitor ---
         * ulTaskNotifyTake with xClearCountOnExit=pdTRUE clears the
         * notification value. Timeout 0 = non-blocking. */
        uint32_t notification = ulTaskNotifyTake(pdTRUE, 0);
        if (notification > 0)
        {
            LogMessage_t log_msg;
            snprintf(log_msg.text, sizeof(log_msg.text),
                     "[SENSOR] Health check ACK. Total readings: %lu",
                     (unsigned long)s_reading_count);
            xQueueSend(xLogQueue, &log_msg, 0);
        }

        /* --- Wait until next period ---
         * vTaskDelayUntil provides precise periodic execution:
         * it accounts for the time spent in the loop body. */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(PERIOD_SENSOR_TASK_MS));
    }
}

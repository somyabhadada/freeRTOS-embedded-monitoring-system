/**
 * @file    monitor_task.c
 * @brief   System Monitor Task — health checks and diagnostics.
 * @author  Somya Bhadada
 *
 * Periodically checks system health, heap usage, and sends task
 * notifications to verify other tasks are alive.
 */

#include "monitor_task.h"
#include "app_config.h"
#include <stdio.h>

void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        /* 1. Send task notification to Sensor Task (health ping) */
        if (xSensorTaskHandle != NULL)
        {
            xTaskNotifyGive(xSensorTaskHandle);
        }

        /* 2. Log system heap stats */
        LogMessage_t log_msg;
        snprintf(log_msg.text, sizeof(log_msg.text),
                 "[MONITOR] Free Heap: %u bytes",
                 (unsigned int)xPortGetFreeHeapSize());
        xQueueSend(xLogQueue, &log_msg, 0);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(PERIOD_MONITOR_TASK_MS));
    }
}

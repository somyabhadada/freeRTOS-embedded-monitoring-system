/**
 * @file    logger_task.c
 * @brief   Logger Task — central logging consumer.
 * @author  Somya Bhadada
 *
 * Dequeues messages from xLogQueue and prints them safely
 * via the UART Task's printing utility.
 */

#include "logger_task.h"
#include "uart_task.h"
#include "app_config.h"

void vLoggerTask(void *pvParameters)
{
    (void)pvParameters;
    LogMessage_t log_msg;

    for (;;)
    {
        /* Block until a log message is available */
        if (xQueueReceive(xLogQueue, &log_msg, portMAX_DELAY) == pdPASS)
        {
            uart_print("\r\n");
            uart_print(log_msg.text);
            uart_print("\r\n> "); /* Re-print prompt for CLI */
        }
    }
}

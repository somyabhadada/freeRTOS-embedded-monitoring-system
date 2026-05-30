/**
 * @file    uart_task.c
 * @brief   UART Communication Task — CLI handler and serial I/O.
 * @author  Somya Bhadada
 *
 * This module implements the UART Communication Task, which:
 *   - Receives characters from the UART RX ISR via xCmdQueue.
 *   - Provides an interactive CLI with command parsing.
 *   - Displays sensor data and SPI data on demand.
 *   - Is the sole writer to the UART TX line (mutex-protected).
 *
 * FreeRTOS Concepts Demonstrated:
 *   - xQueueReceive (blocking wait for ISR-produced data)
 *   - xSemaphoreTake/Give (mutex for shared UART peripheral)
 *   - Queue peek for latest sensor/SPI data
 */

#include "uart_task.h"
#include "app_config.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

/* ---------------------------------------------------------------------------
 * External HAL Handle (defined in main.c)
 * --------------------------------------------------------------------------- */
extern UART_HandleTypeDef huart2;

/* ---------------------------------------------------------------------------
 * Private Defines
 * --------------------------------------------------------------------------- */

/** @brief ANSI escape code to clear the terminal screen. */
#define ANSI_CLEAR_SCREEN  "\033[2J\033[H"

/** @brief Maximum CLI input line length. */
#define CLI_LINE_MAX       (64)

/* ---------------------------------------------------------------------------
 * Private Function Prototypes
 * --------------------------------------------------------------------------- */
static void cli_handle_command(const char *line);
static void cli_print_help(void);
static void cli_print_status(void);
static void cli_print_version(void);
static void cli_read_sensor(void);
static void cli_read_spi(void);
static void uart_write_char(uint8_t c);

/* ---------------------------------------------------------------------------
 * Public Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief  Thread-safe UART string transmit.
 *
 * @param  msg  Null-terminated string to send via UART2.
 *
 * Inputs:  String pointer, UART handle, UART mutex.
 * Outputs: Characters transmitted on UART2 TX line.
 *
 * When the FreeRTOS scheduler is running, the mutex prevents interleaved
 * output from concurrent tasks. Before the scheduler starts (during boot),
 * the function transmits directly without mutex protection.
 */
void uart_print(const char *msg)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING && xUartMutex != NULL)
    {
        xSemaphoreTake(xUartMutex, portMAX_DELAY);
        HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
        xSemaphoreGive(xUartMutex);
    }
    else
    {
        HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
    }
}

/**
 * @brief  Transmit a single character via UART2 (mutex-protected).
 */
static void uart_write_char(uint8_t c)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING && xUartMutex != NULL)
    {
        xSemaphoreTake(xUartMutex, portMAX_DELAY);
        HAL_UART_Transmit(&huart2, &c, 1, HAL_MAX_DELAY);
        xSemaphoreGive(xUartMutex);
    }
    else
    {
        HAL_UART_Transmit(&huart2, &c, 1, HAL_MAX_DELAY);
    }
}

/* ---------------------------------------------------------------------------
 * Task Entry Point
 * --------------------------------------------------------------------------- */

void vUartTask(void *pvParameters)
{
    (void)pvParameters;

    uint8_t  rx_char;
    char     line_buf[CLI_LINE_MAX];
    uint16_t line_len = 0;

    /* Print boot banner */
    uart_print("\r\n");
    uart_print("=============================================\r\n");
    uart_print("  FreeRTOS Embedded Monitoring System " FW_VERSION_STRING "\r\n");
    uart_print("  Target: STM32F446RE (NUCLEO Board)\r\n");
    uart_print("  Author: Somya Bhadada\r\n");
    uart_print("=============================================\r\n");
    uart_print("\r\nType 'help' for available commands.\r\n");
    uart_print("> ");

    for (;;)
    {
        /* Block until a character arrives from UART RX ISR.
         * The ISR places each received byte into xCmdQueue.
         * Timeout: 100ms allows periodic checks if needed. */
        if (xQueueReceive(xCmdQueue, &rx_char, pdMS_TO_TICKS(100)) == pdPASS)
        {
            /* --- Handle Enter key (CR or LF) --- */
            if (rx_char == '\r' || rx_char == '\n')
            {
                uart_print("\r\n");

                if (line_len > 0)
                {
                    line_buf[line_len] = '\0';
                    cli_handle_command(line_buf);
                    line_len = 0;
                }

                uart_print("> ");
                continue;
            }

            /* --- Handle Backspace (ASCII 0x08 or 0x7F) --- */
            if (rx_char == 0x08 || rx_char == 0x7F)
            {
                if (line_len > 0)
                {
                    line_len--;
                    uart_print("\b \b");  /* Erase character on terminal */
                }
                continue;
            }

            /* --- Accumulate printable characters --- */
            if (line_len < (CLI_LINE_MAX - 1) && rx_char >= 32 && rx_char < 127)
            {
                line_buf[line_len++] = (char)rx_char;
                uart_write_char(rx_char);  /* Echo character */
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * CLI Command Parser
 * --------------------------------------------------------------------------- */

/**
 * @brief  Parse and execute a CLI command line.
 *
 * @param  line  Null-terminated command string from user input.
 *
 * Supported commands:
 *   help     - Display available commands
 *   status   - Show system status summary
 *   version  - Show firmware version and build info
 *   sensor   - Read latest sensor data
 *   spi      - Read latest SPI peripheral data
 *   clear    - Clear the terminal screen
 */
static void cli_handle_command(const char *line)
{
    char cmd[16]  = {0};
    char arg1[16] = {0};

    sscanf(line, "%15s %15s", cmd, arg1);

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0)
    {
        cli_print_help();
    }
    else if (strcmp(cmd, "status") == 0 || strcmp(cmd, "s") == 0)
    {
        cli_print_status();
    }
    else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "v") == 0)
    {
        cli_print_version();
    }
    else if (strcmp(cmd, "sensor") == 0 || strcmp(cmd, "r") == 0)
    {
        cli_read_sensor();
    }
    else if (strcmp(cmd, "spi") == 0)
    {
        cli_read_spi();
    }
    else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0)
    {
        uart_print(ANSI_CLEAR_SCREEN);
    }
    else
    {
        uart_print("Unknown command. Type 'help' for a list.\r\n");
    }
}

/**
 * @brief  Print the help menu listing all available CLI commands.
 */
static void cli_print_help(void)
{
    uart_print(
        "\r\nAvailable Commands:\r\n"
        "  help / h      - Show this help menu\r\n"
        "  status / s    - System status and task info\r\n"
        "  version / v   - Firmware version and build date\r\n"
        "  sensor / r    - Read latest sensor data\r\n"
        "  spi           - Read latest SPI peripheral data\r\n"
        "  clear / cls   - Clear terminal screen\r\n"
    );
}

/**
 * @brief  Print system status including RTOS tick count and heap info.
 */
static void cli_print_status(void)
{
    char buf[128];

    snprintf(buf, sizeof(buf),
             "\r\nSystem Status:\r\n"
             "  Uptime: %lu ms\r\n"
             "  Free Heap: %u bytes\r\n"
             "  Tasks Running: 5\r\n",
             (unsigned long)HAL_GetTick(),
             (unsigned int)xPortGetFreeHeapSize());
    uart_print(buf);
}

/**
 * @brief  Print firmware version and build timestamp.
 */
static void cli_print_version(void)
{
    char buf[128];
    snprintf(buf, sizeof(buf),
             "\r\nFirmware: %s\r\n"
             "Built: %s %s\r\n"
             "Target: STM32F446RE\r\n",
             FW_VERSION_STRING, __DATE__, __TIME__);
    uart_print(buf);
}

/**
 * @brief  Read and display the latest sensor data from xSensorDataQueue.
 *
 * Uses xQueuePeek to read without removing the data, so the Monitor Task
 * can also inspect it independently.
 */
static void cli_read_sensor(void)
{
    SensorData_t data;
    char buf[80];

    if (xQueuePeek(xSensorDataQueue, &data, 0) == pdPASS)
    {
        snprintf(buf, sizeof(buf),
                 "\r\nSensor: %.1f C (at %lu ms)\r\n",
                 data.temperature_celsius,
                 (unsigned long)data.timestamp_ms);
        uart_print(buf);
    }
    else
    {
        uart_print("\r\nNo sensor data available yet.\r\n");
    }
}

/**
 * @brief  Display the latest SPI peripheral data.
 *
 * The SPI Task stores its latest reading in a global variable
 * protected by the task's own execution context.
 */
static void cli_read_spi(void)
{
    /* Import the latest SPI data from spi_task.c */
    extern SpiPeripheralData_t g_latest_spi_data;
    extern volatile uint8_t    g_spi_data_valid;

    char buf[128];

    if (g_spi_data_valid)
    {
        snprintf(buf, sizeof(buf),
                 "\r\nSPI Device ID: 0x%02X\r\n"
                 "  Status Reg:  0x%02X\r\n"
                 "  Raw Value:   %d\r\n"
                 "  Timestamp:   %lu ms\r\n",
                 g_latest_spi_data.device_id,
                 g_latest_spi_data.status_reg,
                 g_latest_spi_data.raw_value,
                 (unsigned long)g_latest_spi_data.timestamp_ms);
        uart_print(buf);
    }
    else
    {
        uart_print("\r\nNo SPI data available yet.\r\n");
    }
}

/**
 * @file    app_config.h
 * @brief   Application-wide configuration and shared type definitions.
 * @author  Somya Bhadada
 *
 * This header centralizes all application constants, shared data types,
 * and extern declarations for FreeRTOS objects used across multiple modules.
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Firmware Version
 * --------------------------------------------------------------------------- */
#define FW_VERSION_MAJOR   2
#define FW_VERSION_MINOR   0
#define FW_VERSION_PATCH   0
#define FW_VERSION_STRING  "v2.0.0"

/* ---------------------------------------------------------------------------
 * Task Priorities (higher number = higher priority)
 *
 * Priority assignment rationale:
 *   - Logger has lowest priority: it only formats and prints, non-critical.
 *   - Sensor and SPI are periodic data producers at normal priority.
 *   - UART/CLI is above normal to ensure responsive user interaction.
 *   - System Monitor has highest priority to detect stuck tasks.
 * --------------------------------------------------------------------------- */
#define PRIORITY_LOGGER_TASK        (1)
#define PRIORITY_SENSOR_TASK        (2)
#define PRIORITY_SPI_TASK           (2)
#define PRIORITY_UART_TASK          (3)
#define PRIORITY_MONITOR_TASK       (4)

/* ---------------------------------------------------------------------------
 * Task Stack Sizes (in words, 1 word = 4 bytes on Cortex-M4)
 * --------------------------------------------------------------------------- */
#define STACK_SIZE_SENSOR_TASK      (256)
#define STACK_SIZE_UART_TASK        (512)
#define STACK_SIZE_SPI_TASK         (256)
#define STACK_SIZE_MONITOR_TASK     (256)
#define STACK_SIZE_LOGGER_TASK      (256)

/* ---------------------------------------------------------------------------
 * Task Periods (milliseconds)
 * --------------------------------------------------------------------------- */
#define PERIOD_SENSOR_TASK_MS       (1000)
#define PERIOD_SPI_TASK_MS          (2000)
#define PERIOD_MONITOR_TASK_MS      (5000)

/* ---------------------------------------------------------------------------
 * Queue Sizes
 * --------------------------------------------------------------------------- */
#define LOG_QUEUE_LENGTH            (16)
#define SENSOR_QUEUE_LENGTH         (8)
#define CMD_QUEUE_LENGTH            (32)

/* ---------------------------------------------------------------------------
 * Shared Data Types
 * --------------------------------------------------------------------------- */

/**
 * @brief Log message structure sent to the Logger Task via queue.
 *
 * Each log message contains a fixed-size text buffer. Producers (Sensor Task,
 * SPI Task, Monitor Task, ISR) format their message into this struct and
 * enqueue it. The Logger Task dequeues and transmits via UART.
 */
typedef struct {
    char text[80];
} LogMessage_t;

/**
 * @brief Sensor data structure sent from Sensor Task to UART Task.
 *
 * Contains the latest sensor reading. The Sensor Task produces this data
 * periodically, and the UART Task can display it on demand.
 */
typedef struct {
    float    temperature_celsius;
    uint32_t timestamp_ms;
} SensorData_t;

/**
 * @brief SPI peripheral data returned from simulated SPI transaction.
 */
typedef struct {
    uint8_t  device_id;
    uint8_t  status_reg;
    int16_t  raw_value;
    uint32_t timestamp_ms;
} SpiPeripheralData_t;

/* ---------------------------------------------------------------------------
 * Extern Declarations for FreeRTOS Objects (defined in main.c)
 *
 * These are shared across task modules so each can access the
 * appropriate queues, semaphores, and mutexes.
 * --------------------------------------------------------------------------- */

/** @brief Queue: Sensor Task -> Logger/UART Task (SensorData_t) */
extern QueueHandle_t xSensorDataQueue;

/** @brief Queue: Any producer -> Logger Task (LogMessage_t) */
extern QueueHandle_t xLogQueue;

/** @brief Queue: UART RX ISR -> UART Task (single bytes) */
extern QueueHandle_t xCmdQueue;

/** @brief Binary Semaphore: GPIO ISR -> Sensor Task (event trigger) */
extern SemaphoreHandle_t xButtonSemaphore;

/** @brief Mutex: Protects UART TX from concurrent access */
extern SemaphoreHandle_t xUartMutex;

/** @brief Task handles for system monitoring */
extern TaskHandle_t xSensorTaskHandle;
extern TaskHandle_t xUartTaskHandle;
extern TaskHandle_t xSpiTaskHandle;
extern TaskHandle_t xMonitorTaskHandle;
extern TaskHandle_t xLoggerTaskHandle;

#endif /* APP_CONFIG_H */

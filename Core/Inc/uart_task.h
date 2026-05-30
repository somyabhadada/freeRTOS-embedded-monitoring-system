/**
 * @file    uart_task.h
 * @brief   UART Communication Task interface — CLI and serial I/O.
 * @author  Somya Bhadada
 *
 * The UART Task is the sole owner of the UART TX line. It handles:
 *   - Interactive command-line interface (CLI) over serial.
 *   - Display of sensor data on user request.
 *   - Thread-safe UART output using a mutex.
 */

#ifndef UART_TASK_H
#define UART_TASK_H

/**
 * @brief  UART Communication Task entry function.
 *
 * @param  pvParameters  Unused (pass NULL).
 *
 * Execution:
 *   1. Prints boot banner and CLI prompt.
 *   2. Enters infinite loop:
 *      a. Waits for characters from xCmdQueue (from UART RX ISR).
 *      b. Builds command line with echo and backspace support.
 *      c. On Enter, parses and executes CLI command.
 *      d. Supports: help, status, sensor, spi, version, clear.
 */
void vUartTask(void *pvParameters);

/**
 * @brief  Thread-safe UART string transmit.
 *
 * @param  msg  Null-terminated string to transmit.
 *
 * Acquires the UART mutex before transmitting to prevent garbled output
 * when multiple tasks attempt to print simultaneously. Falls back to
 * unprotected TX if the scheduler is not yet running.
 */
void uart_print(const char *msg);

#endif /* UART_TASK_H */

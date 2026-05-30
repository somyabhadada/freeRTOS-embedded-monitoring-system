/**
 * @file    sensor_task.h
 * @brief   Sensor Task interface — periodic temperature acquisition.
 * @author  Somya Bhadada
 *
 * The Sensor Task reads the virtual temperature sensor at a fixed interval
 * and sends the data to both the Sensor Data Queue (for UART display) and
 * the Log Queue (for centralized logging). It also listens for a GPIO
 * button-press event via a binary semaphore to perform on-demand reads.
 */

#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

/**
 * @brief  Sensor Task entry function.
 *
 * @param  pvParameters  Unused (pass NULL).
 *
 * Execution:
 *   1. Initializes the virtual sensor hardware.
 *   2. Enters infinite loop:
 *      a. Reads temperature from sensor module.
 *      b. Packages data into SensorData_t struct.
 *      c. Sends data to xSensorDataQueue (non-blocking).
 *      d. Sends formatted log message to xLogQueue.
 *      e. Checks xButtonSemaphore for GPIO-triggered immediate read.
 *      f. Delays for PERIOD_SENSOR_TASK_MS.
 */
void vSensorTask(void *pvParameters);

#endif /* SENSOR_TASK_H */

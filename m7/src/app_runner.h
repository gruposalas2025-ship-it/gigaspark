/*
 * Gigaspark OS - App Runner Header
 * Dynamic app execution with fault recovery and safe termination
 */

#ifndef GIGASPARK_APP_RUNNER_H
#define GIGASPARK_APP_RUNNER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

/* App execution result codes */
#define APP_RESULT_OK           0
#define APP_RESULT_FAULT        (-1)
#define APP_RESULT_LOAD_ERR     (-2)
#define APP_RESULT_MPU_ERR      (-3)
#define APP_RESULT_ABORTED      (-4)

/*
 * Run a user app from a memory buffer.
 */
int app_runner_execute(const uint8_t *app_data, size_t app_size, uint16_t handle);

/*
 * Force exit the currently running app.
 * Called by the navigation bar when Home is pressed.
 * Cleans up cache, disables MPU, aborts the app thread.
 */
void app_force_exit(void);

/*
 * Set up MPU region for app code execution.
 */
int app_runner_setup_mpu(uint32_t app_base, size_t app_size);

/*
 * Disable MPU region for app execution.
 */
void app_runner_disable_mpu(void);

/*
 * Set the thread ID for the app runner thread.
 * Used by app_force_exit() to abort the running app.
 */
void app_runner_set_thread(k_tid_t tid);

/*
 * Check if an app is currently running.
 */
bool app_runner_is_running(void);

#endif /* GIGASPARK_APP_RUNNER_H */

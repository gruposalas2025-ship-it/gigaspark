/*
 * Gigaspark OS - App Runner Header
 * Dynamic app execution with fault recovery
 */

#ifndef GIGASPARK_APP_RUNNER_H
#define GIGASPARK_APP_RUNNER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* App execution result codes */
#define APP_RESULT_OK           0
#define APP_RESULT_FAULT        (-1)
#define APP_RESULT_LOAD_ERR     (-2)
#define APP_RESULT_MPU_ERR      (-3)

/*
 * Run a user app from a memory buffer.
 *
 * Sets up the MPU for app execution, calls setjmp for fault recovery,
 * and invokes app_main(). If the app faults, recovers gracefully.
 *
 * @param app_data    Pointer to app binary in memory.
 * @param app_size    Size of app binary in bytes.
 * @param handle      Memory handle (for cleanup on exit).
 * @return            APP_RESULT_OK or error code.
 */
int app_runner_execute(const uint8_t *app_data, size_t app_size, uint16_t handle);

/*
 * Set up MPU region for app code execution.
 *
 * @param app_base    Base address of app code.
 * @param app_size    Size of app code region.
 * @return            0 on success, negative on error.
 */
int app_runner_setup_mpu(uint32_t app_base, size_t app_size);

/*
 * Disable MPU region for app execution.
 */
void app_runner_disable_mpu(void);

/*
 * Check if an app is currently running.
 */
bool app_runner_is_running(void);

#endif /* GIGASPARK_APP_RUNNER_H */

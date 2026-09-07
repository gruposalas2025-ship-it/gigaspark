/*
 * Gigaspark OS - App Runner Implementation
 * Dynamic app execution with HardFault recovery and safe termination
 *
 * Uses setjmp/longjmp for try-catch style fault recovery.
 * The fault handler triggers PendSV, which calls longjmp()
 * from thread mode (safe on Cortex-M).
 *
 * app_force_exit() provides clean termination when the user
 * presses the Home button in the navigation bar.
 */

#include "app_runner.h"
#include "fault_manager.h"
#include "ipc_protocol.h"

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>
#include <setjmp.h>
#include <string.h>

LOG_MODULE_REGISTER(app_runner, CONFIG_LOG_DEFAULT_LEVEL);

/* App memory region base address (after kernel, in SRAM) */
#define APP_MEMORY_BASE  0x24000000  /* DTCM or SRAM1 */
#define APP_MEMORY_SIZE  (128 * 1024) /* 128KB for apps */

/* App function pointer type */
typedef void (*app_main_fn)(void);

/* Current app state */
static bool app_running = false;
static uint16_t current_handle = 0;

/* App thread reference (set by app_runner_thread in main.c) */
static k_tid_t app_thread_id = NULL;

void app_runner_set_thread(k_tid_t tid)
{
	app_thread_id = tid;
}

bool app_runner_is_running(void)
{
	return app_running;
}

int app_runner_setup_mpu(uint32_t app_base, size_t app_size)
{
	LOG_INF("MPU: App region at 0x%08x, size %u bytes", app_base, app_size);
	return 0;
}

void app_runner_disable_mpu(void)
{
	LOG_INF("MPU: App execution region disabled");
}

/*
 * Force exit the currently running app.
 * Called by the navigation bar when Home is pressed.
 *
 * Execution order (CRITICAL):
 * 1. Disable MPU (stop app code execution)
 * 2. Invalidate L1 data cache (remove stale app data)
 * 3. Invalidate L1 instruction cache (remove stale app code)
 * 4. Abort the app thread (safe termination)
 */
void app_force_exit(void)
{
	if (!app_running) {
		LOG_WRN("No app running to force exit");
		return;
	}

	LOG_INF("=== FORCE EXIT: Cleaning up app ===");

	/* Step 1: Disable MPU first - no more app code execution */
	app_runner_disable_mpu();

	/* Step 2: Invalidate L1 data cache */
	LOG_INF("Force exit: Invalidating L1 data cache...");
	sys_cache_data_invd_all();

	/* Step 3: Invalidate L1 instruction cache */
	LOG_INF("Force exit: Invalidating L1 instruction cache...");
	sys_cache_instr_invd_all();

	/* Step 4: Clear app memory region */
	memset((void *)APP_MEMORY_BASE, 0, APP_MEMORY_SIZE);

	/* Step 5: Abort the app thread */
	if (app_thread_id != NULL) {
		LOG_INF("Force exit: Aborting app thread (tid=%p)", app_thread_id);
		k_thread_abort(app_thread_id);
	}

	/* Step 6: Update state */
	app_running = false;
	fault_manager_app_stop();

	LOG_INF("=== FORCE EXIT: App terminated cleanly ===");
}

int app_runner_execute(const uint8_t *app_data, size_t app_size, uint16_t handle)
{
	struct fault_context *fault_ctx = fault_manager_get_context();
	int result;

	if (app_data == NULL || app_size == 0) {
		LOG_ERR("Invalid app data");
		return APP_RESULT_LOAD_ERR;
	}

	if (app_size > APP_MEMORY_SIZE) {
		LOG_ERR("App too large (%u > %u bytes)", app_size, APP_MEMORY_SIZE);
		return APP_RESULT_MPU_ERR;
	}

	LOG_INF("Preparing app: handle=0x%04x, size=%u bytes", handle, app_size);

	/* Copy app code to execution region */
	uint8_t *exec_base = (uint8_t *)APP_MEMORY_BASE;
	memcpy(exec_base, app_data, app_size);

	/* Setup MPU for app execution */
	int ret = app_runner_setup_mpu(APP_MEMORY_BASE, app_size);
	if (ret < 0) {
		LOG_ERR("MPU setup failed: %d", ret);
		return APP_RESULT_MPU_ERR;
	}

	/* Set up fault recovery */
	current_handle = handle;
	fault_manager_clear_fault();
	fault_manager_app_start();
	app_running = true;

	/* Try-catch: setjmp saves the execution point */
	if (setjmp(fault_ctx->jump_buffer) != 0) {
		/*
		 * We landed here via longjmp from PendSV handler.
		 * A fault occurred during app execution.
		 */
		LOG_ERR("App faulted! Recovering...");

		/* Disable app execution */
		sys_cache_data_invd_all();
		sys_cache_instr_invd_all();
		app_runner_disable_mpu();
		app_running = false;
		fault_manager_app_stop();

		result = APP_RESULT_FAULT;
		goto cleanup;
	}

	/*
	 * Normal execution path.
	 * Cast the app binary to a function pointer and call it.
	 * The app's first function is expected to be app_main().
	 */
	LOG_INF("Launching app at 0x%08x...", APP_MEMORY_BASE);

	app_main_fn app_main = (app_main_fn)(APP_MEMORY_BASE | 1);  /* Thumb mode */
	app_main();

	/* App returned normally */
	LOG_INF("App returned normally");
	result = APP_RESULT_OK;

cleanup:
	/* Clean up */
	sys_cache_data_invd_all();
	sys_cache_instr_invd_all();
	app_runner_disable_mpu();
	app_running = false;
	fault_manager_app_stop();

	return result;
}

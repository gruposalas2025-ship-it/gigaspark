/*
 * Gigaspark OS - Fault Manager Header
 * HardFault recovery mechanism for user apps
 */

#ifndef GIGASPARK_FAULT_MANAGER_H
#define GIGASPARK_FAULT_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <setjmp.h>

/* Fault recovery context */
struct fault_context {
	jmp_buf  jump_buffer;   /* setjmp/longjmp buffer */
	volatile bool app_running;  /* true while app_main is executing */
	volatile bool fault_occurred; /* set by fault handler */
	volatile uint32_t fault_pc;   /* PC at time of fault */
	volatile uint32_t fault_lr;   /* LR at time of fault */
	volatile uint32_t fault_cfsr; /* Configurable Fault Status Register */
};

/*
 * Initialize the fault manager.
 * Installs the custom HardFault handler.
 */
void fault_manager_init(void);

/*
 * Get the current fault context.
 * Used by app_runner to set up setjmp/longjmp.
 */
struct fault_context *fault_manager_get_context(void);

/*
 * Check if a fault occurred during app execution.
 * Returns true if the app faulted and needs recovery.
 */
bool fault_manager_check_fault(void);

/*
 * Clear the fault state after recovery.
 */
void fault_manager_clear_fault(void);

/*
 * Mark that an app is running (faults should trigger recovery).
 */
void fault_manager_app_start(void);

/*
 * Mark that the app has stopped.
 */
void fault_manager_app_stop(void);

#endif /* GIGASPARK_FAULT_MANAGER_H */

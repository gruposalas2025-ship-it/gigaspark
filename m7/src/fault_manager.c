/*
 * Gigaspark OS - Fault Manager Implementation
 * HardFault recovery for user apps on Cortex-M7
 *
 * Strategy: When a HardFault occurs during app execution, the fault
 * handler sets a flag and uses PendSV to defer recovery to thread mode.
 * The PendSV handler then calls longjmp() to return control to the
 * app_runner, which cleans up and returns to the launcher.
 *
 * This avoids the dangerous pattern of longjmp() from ISR context,
 * which is unsafe on Cortex-M due to exception stacking.
 */

#include "fault_manager.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(fault_manager, CONFIG_LOG_DEFAULT_LEVEL);

/* Cortex-M7 system registers */
#define SCB_SHCSR    (*(volatile uint32_t *)0xE000ED24)
#define SCB_CFSR     (*(volatile uint32_t *)0xE000ED28)
#define SCB_HFSR     (*(volatile uint32_t *)0xE000ED2C)
#define SCB_MMAR     (*(volatile uint32_t *)0xE000ED34)
#define SCB_BFAR     (*(volatile uint32_t *)0xE000ED38)

/* PendSV priority (lowest possible) */
#define PENDSV_PRIORITY  0xFF
#define SCB_SHPR3        (*(volatile uint32_t *)0xE000ED20)

/* Fault context */
static struct fault_context ctx;

/* PendSV IRQ number on Cortex-M7 */
#define PENDSV_IRQn  (-2)

void fault_manager_init(void)
{
	memset(&ctx, 0, sizeof(ctx));

	/* Set PendSV to lowest priority so it runs after all other ISRs */
	uint32_t priority = SCB_SHPR3;
	priority &= ~(0xFF << 16);  /* Clear PendSV priority bits */
	priority |= (PENDSV_PRIORITY << 16);  /* Set to lowest */
	SCB_SHPR3 = priority;

	LOG_INF("Fault manager initialized");
}

struct fault_context *fault_manager_get_context(void)
{
	return &ctx;
}

bool fault_manager_check_fault(void)
{
	return ctx.fault_occurred;
}

void fault_manager_clear_fault(void)
{
	ctx.fault_occurred = false;
	ctx.fault_pc = 0;
	ctx.fault_lr = 0;
	ctx.fault_cfsr = 0;
}

void fault_manager_app_start(void)
{
	ctx.app_running = true;
	ctx.fault_occurred = false;
}

void fault_manager_app_stop(void)
{
	ctx.app_running = false;
}

/*
 * PendSV handler - called in thread mode after a fault.
 * This is safe to call longjmp() from because we're in thread mode.
 */
void z_arm_pendsv_isr(void *exc_ptr)
{
	(void)exc_ptr;
	if (!ctx.fault_occurred) {
		return;
	}

	LOG_WRN("PendSV: Recovering from fault (PC=0x%08x, CFSR=0x%08x)",
		ctx.fault_pc, ctx.fault_cfsr);

	/* Clear fault state */
	ctx.app_running = false;

	/* longjmp back to app_runner's setjmp point */
	longjmp(ctx.jump_buffer, 1);
}

/*
 * Called from the fault handler stub (see fault_handler_stub.s).
 * This runs in handler mode (ISR context).
 */
void fault_manager_record_fault(uint32_t *stack_frame)
{
	/* Extract stacked registers from the exception frame */
	uint32_t r0  = stack_frame[0];
	uint32_t r1  = stack_frame[1];
	uint32_t r2  = stack_frame[2];
	uint32_t r3  = stack_frame[3];
	uint32_t r12 = stack_frame[4];
	uint32_t lr  = stack_frame[5];
	uint32_t pc  = stack_frame[6];
	uint32_t psr = stack_frame[7];

	(void)r0; (void)r1; (void)r2; (void)r3; (void)r12;
	(void)lr; (void)psr;

	/* Record fault info */
	ctx.fault_pc = pc;
	ctx.fault_lr = lr;
	ctx.fault_cfsr = SCB_CFSR;

	LOG_ERR("=== HardFault Detected ===");
	LOG_ERR("  PC:  0x%08x", pc);
	LOG_ERR("  LR:  0x%08x", lr);
	LOG_ERR("  CFSR: 0x%08x", SCB_CFSR);
	LOG_ERR("  HFSR: 0x%08x", SCB_HFSR);

	/* Decode CFSR bits */
	if (SCB_CFSR & (1 << 0)) LOG_ERR("  IACCVIOL - Instruction access violation");
	if (SCB_CFSR & (1 << 1)) LOG_ERR("  DACCVIOL - Data access violation");
	if (SCB_CFSR & (1 << 3)) LOG_ERR("  MUNSTKERR - MemManage fault on unstacking");
	if (SCB_CFSR & (1 << 4)) LOG_ERR("  MSTKERR - MemManage fault on stacking");
	if (SCB_CFSR & (1 << 7)) LOG_ERR("  MMARVALID - MemManage Address Valid");
	if (SCB_CFSR & (1 << 8)) LOG_ERR("  IBUSERR - Instruction bus error");
	if (SCB_CFSR & (1 << 9)) LOG_ERR("  PRECISERR - Precise data bus error");
	if (SCB_CFSR & (1 << 10)) LOG_ERR("  IMPRECISERR - Imprecise data bus error");
	if (SCB_CFSR & (1 << 11)) LOG_ERR("  UNSTKERR - Bus fault on unstacking");
	if (SCB_CFSR & (1 << 12)) LOG_ERR("  STKERR - Bus fault on stacking");
	if (SCB_CFSR & (1 << 15)) LOG_ERR("  BFARVALID - Bus Fault Address Valid");
	if (SCB_CFSR & (1 << 16)) LOG_ERR("  UNDEFINSTR - Undefined instruction");
	if (SCB_CFSR & (1 << 17)) LOG_ERR("  INVSTATE - Thumb state violation");
	if (SCB_CFSR & (1 << 18)) LOG_ERR("  INVPC - Invalid PC load");
	if (SCB_CFSR & (1 << 19)) LOG_ERR("  NOCP - No coprocessor");
	if (SCB_CFSR & (1 << 24)) LOG_ERR("  DIVBYZERO - Divide by zero");
	if (SCB_CFSR & (1 << 25)) LOG_ERR("  UNALIGNED - Unaligned access");

	LOG_ERR("============================");

	/* Set fault flag */
	ctx.fault_occurred = true;

	/* Trigger PendSV to recover in thread mode */
	/* Set PendSV pending bit */
	volatile uint32_t *icsr = (volatile uint32_t *)0xE000ED04;
	*icsr = (1 << 28);  /* Set PENDSVSET bit */
}

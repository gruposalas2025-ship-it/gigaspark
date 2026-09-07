/*
 * Gigaspark OS - Gestor de Energia Implementation
 * Modo suspension por inactividad para STM32H747
 *
 * Implementa ahorro de energia a nivel de aplicacion usando
 * las funciones Low Layer (LL) de STM32 directamente.
 * NO usa CONFIG_PM de Zephyr (no soportado en H7).
 *
 * Flujo:
 * 1. Timer de 15 segundos se resetea con cada toque
 * 2. Al expirar: apagar backlight + suspender LVGL + STOP D1
 * 3. Al tocar: WFI retorna automaticamente + reanudar LVGL
 */

#include "power_manager.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stm32h7xx_ll_pwr.h>

LOG_MODULE_REGISTER(power_manager, CONFIG_LOG_DEFAULT_LEVEL);

/* Referencia externa al hilo de LVGL (definido en main.c) */
extern k_tid_t ui_launcher_tid;

/* Forward declaration */
static void inactivity_timeout_handler(struct k_timer *timer);

/* Timer de inactividad */
static K_TIMER_DEFINE(inactivity_timer, inactivity_timeout_handler, NULL);
static volatile bool system_sleeping = false;
static volatile bool power_initialized = false;

/*
 * Callback del timer: el sistema debe dormirse.
 */
static void inactivity_timeout_handler(struct k_timer *timer)
{
	(void)timer;

	if (system_sleeping) {
		return;
	}

	LOG_INF("=== MODO SUSPENSION ===");
	LOG_INF("Inactividad detectada (%d s) - durmiendo...", POWER_SLEEP_TIMEOUT_S);

	/* Paso 1: Suspender hilo de LVGL */
	LOG_INF("[1/3] Suspendiendo hilo UI...");
	k_thread_suspend(ui_launcher_tid);

	/* Paso 2: Poner dominio D1 en STOP */
	LOG_INF("[2/3] Entrando en STOP D1...");
	LL_PWR_CPU_SetD1PowerMode(LL_PWR_CPU_MODE_D1STOP);

	/* Paso 3: Dormir el core M7 */
	LOG_INF("[3/3] M7 durmiendo... (despertar por WFI/interrupcion)");
	system_sleeping = true;

	/* WFI: Wait For Interrupt - el core se detiene aqui
	 * Cuando llegue una interrupcion (touch, timer, etc.)
	 * el core retorna automaticamente a modo RUN.
	 */
	__WFI();

	/* --- El core despierta aqui automaticamente --- */

	LOG_INF("=== M7 DESPIERTO ===");
	system_sleeping = false;
}

/*
 * Inicializar el gestor de energia.
 */
void power_manager_init(void)
{
	LOG_INF("Inicializando gestor de energia...");

	/* Configurar timer de inactividad (15 segundos, periodico) */
	k_timer_start(&inactivity_timer, K_SECONDS(POWER_SLEEP_TIMEOUT_S),
		      K_SECONDS(POWER_SLEEP_TIMEOUT_S));

	power_initialized = true;
	LOG_INF("Gestor de energia listo (timeout: %d s)", POWER_SLEEP_TIMEOUT_S);
}

/*
 * Notificar actividad del usuario (resetear timer).
 */
void power_notify_activity(void)
{
	if (!power_initialized) {
		return;
	}

	/* Si estaba dormido, reanudar */
	if (system_sleeping) {
		power_force_wake();
	}

	/* Resetear timer de inactividad */
	k_timer_start(&inactivity_timer, K_SECONDS(POWER_SLEEP_TIMEOUT_S),
		      K_SECONDS(POWER_SLEEP_TIMEOUT_S));
}

bool power_is_sleeping(void)
{
	return system_sleeping;
}

void power_force_sleep(void)
{
	if (!power_initialized || system_sleeping) {
		return;
	}

	LOG_INF("Forzando suspension...");
	k_timer_stop(&inactivity_timer);
	inactivity_timeout_handler(NULL);
}

void power_force_wake(void)
{
	if (!system_sleeping) {
		return;
	}

	LOG_INF("=== DESPERTANDO SISTEMA ===");

	/* WFI ya retorno, el CPU esta en modo RUN.
	 * Solo necesitamos reanudar el hilo de LVGL.
	 */
	LOG_INF("[1/2] Reanudando hilo UI...");
	k_thread_resume(ui_launcher_tid);

	/* Resetear timer */
	k_timer_start(&inactivity_timer, K_SECONDS(POWER_SLEEP_TIMEOUT_S),
		      K_SECONDS(POWER_SLEEP_TIMEOUT_S));

	system_sleeping = false;
	LOG_INF("[2/2] Sistema operativo");
}

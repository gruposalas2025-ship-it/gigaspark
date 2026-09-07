/*
 * Gigaspark OS - Gestor de Energia Header
 * Modo suspension por inactividad para STM32H747
 */

#ifndef GIGASPARK_POWER_MANAGER_H
#define GIGASPARK_POWER_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

/* Tiempo de inactividad antes de dormir (segundos) */
#define POWER_SLEEP_TIMEOUT_S  15

/*
 * Inicializar el gestor de energia.
 * Configura el timer de inactividad y el callback de input.
 */
void power_manager_init(void);

/*
 * Notificar actividad del usuario (resetear timer).
 * Llamar desde el callback de touch/input.
 */
void power_notify_activity(void);

/*
 * Verificar si el sistema esta dormido.
 * @return true si esta en modo suspension.
 */
bool power_is_sleeping(void);

/*
 * Forzar entrada en modo suspension (para pruebas).
 */
void power_force_sleep(void);

/*
 * Despertar el sistema manualmente.
 */
void power_force_wake(void);

#endif /* GIGASPARK_POWER_MANAGER_H */

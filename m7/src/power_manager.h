/*
 * Gigaspark OS - Gestor de Energia Header
 * Modo suspension por inactividad para STM32H747
 */

#ifndef GIGASPARK_POWER_MANAGER_H
#define GIGASPARK_POWER_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

/* Tiempo de inactividad por defecto (segundos) */
#define POWER_SLEEP_TIMEOUT_DEFAULT_S  15
#define POWER_SLEEP_TIMEOUT_MIN_S      5
#define POWER_SLEEP_TIMEOUT_MAX_S      60

/*
 * Inicializar el gestor de energia.
 * Configura el timer de inactividad con el timeout por defecto.
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

/*
 * Configurar el timeout de inactividad.
 * @param seconds  Tiempo en segundos (5-60). Valores fuera de rango
 *                 se ajustan automaticamente.
 */
void power_set_timeout(uint32_t seconds);

/*
 * Obtener el timeout de inactividad actual.
 * @return Timeout en segundos.
 */
uint32_t power_get_timeout(void);

#endif /* GIGASPARK_POWER_MANAGER_H */

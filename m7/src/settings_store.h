/*
 * Gigaspark OS - Almacenamiento de Configuracion
 * Persistencia de ajustes del usuario en Flash interna (NVS)
 *
 * Guarda y recupera configuraciones entre reinicios:
 * - Timeout de suspension (5-60s)
 * - Brillo de pantalla (0-100)
 * - Red WiFi guardada
 *
 * Usa el subsistema `settings` de Zephyr sobre NVS (Flash interna).
 */

#ifndef GIGASPARK_SETTINGS_STORE_H
#define GIGASPARK_SETTINGS_STORE_H

#include <stdbool.h>
#include <stdint.h>

/* Claves de configuracion en NVS */
#define SETTINGS_KEY_TIMEOUT     "sys/timeout"
#define SETTINGS_KEY_BRIGHTNESS  "sys/brightness"
#define SETTINGS_KEY_WIFI_SSID   "wifi/ssid"
#define SETTINGS_KEY_WIFI_PASS   "wifi/pass"
#define SETTINGS_KEY_MAGIC       "sys/magic"

/* Valor magico para detectar si la Flash tiene datos validos */
#define SETTINGS_MAGIC_VALUE     0x4753 /* "GS" = Gigaspark */

/*
 * Inicializar el sistema de almacenamiento.
 * Carga la configuracion desde la Flash.
 * @return true si encontro configuracion guardada, false si es primera vez.
 */
bool gigaspark_settings_init(void);

/*
 * Guardar el timeout de suspension.
 * @param seconds  Timeout en segundos (5-60).
 */
void settings_store_save_timeout(uint32_t seconds);

/*
 * Cargar el timeout de suspension desde Flash.
 * @return Timeout en segundos, o 0 si no hay valor guardado.
 */
uint32_t settings_store_load_timeout(void);

/*
 * Guardar el brillo de pantalla.
 * @param brightness  Brillo (0-100).
 */
void settings_store_save_brightness(uint8_t brightness);

/*
 * Cargar el brillo de pantalla desde Flash.
 * @return Brillo (0-100), o 100 (maximo) si no hay valor guardado.
 */
uint8_t settings_store_load_brightness(void);

/*
 * Guardar credenciales WiFi.
 * @param ssid  Nombre de red (max 32 chars).
 * @param pass  Contrasena (max 64 chars).
 */
void settings_store_save_wifi(const char *ssid, const char *pass);

/*
 * Cargar credenciales WiFi desde Flash.
 * @param ssid      Buffer de salida para SSID (min 33 bytes).
 * @param pass      Buffer de salida para contrasena (min 65 bytes).
 * @return true si encontro credenciales guardadas.
 */
bool settings_store_load_wifi(char *ssid, char *pass);

/*
 * Borrar toda la configuracion guardada (factory reset).
 */
void settings_store_factory_reset(void);

/*
 * Obtener espacio Used/Total de NVS.
 * @param used  Bytes en uso (o NULL).
 * @param total Bytes totales (o NULL).
 */
void settings_store_get_usage(uint32_t *used, uint32_t *total);

#endif /* GIGASPARK_SETTINGS_STORE_H */

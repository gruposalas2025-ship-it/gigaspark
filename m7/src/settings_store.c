/*
 * Gigaspark OS - Almacenamiento de Configuracion Implementation
 * Persistencia de ajustes del usuario en Flash interna (NVS)
 */

#include "settings_store.h"
#include "power_manager.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <string.h>

LOG_MODULE_REGISTER(settings_store, CONFIG_LOG_DEFAULT_LEVEL);

/* Estado del almacenamiento */
static bool store_initialized = false;
static bool config_found = false;

/* Valores en memoria (cache) */
static uint32_t cached_timeout = 0;
static uint8_t  cached_brightness = 100;
static char     cached_ssid[33] = {0};
static char     cached_pass[65] = {0};
static bool     cached_wifi_valid = false;

/* ---- Callbacks del subsistema settings ---- */

static int timeout_set(const char *key, size_t len,
		       settings_read_cb read_cb, void *cb_arg)
{
	/* Solo nos interesa la clave exacta */
	if (strcmp(key, SETTINGS_KEY_TIMEOUT) != 0) {
		return 0;
	}

	if (len != sizeof(uint32_t)) {
		return -EINVAL;
	}

	ssize_t rc = read_cb(cb_arg, &cached_timeout, sizeof(cached_timeout));
	if (rc != sizeof(cached_timeout)) {
		return -EIO;
	}

	/* Validar rango */
	if (cached_timeout < POWER_SLEEP_TIMEOUT_MIN_S) {
		cached_timeout = POWER_SLEEP_TIMEOUT_MIN_S;
	}
	if (cached_timeout > POWER_SLEEP_TIMEOUT_MAX_S) {
		cached_timeout = POWER_SLEEP_TIMEOUT_MAX_S;
	}

	LOG_INF("Timeout cargado de Flash: %u s", cached_timeout);
	return 0;
}

static int brightness_set(const char *key, size_t len,
			  settings_read_cb read_cb, void *cb_arg)
{
	if (strcmp(key, SETTINGS_KEY_BRIGHTNESS) != 0) {
		return 0;
	}

	if (len != sizeof(uint8_t)) {
		return -EINVAL;
	}

	ssize_t rc = read_cb(cb_arg, &cached_brightness, sizeof(cached_brightness));
	if (rc != sizeof(cached_brightness)) {
		return -EIO;
	}

	if (cached_brightness > 100) {
		cached_brightness = 100;
	}

	LOG_INF("Brillo cargado de Flash: %u%%", cached_brightness);
	return 0;
}

static int wifi_set(const char *key, size_t len,
		    settings_read_cb read_cb, void *cb_arg)
{
	if (strcmp(key, SETTINGS_KEY_WIFI_SSID) == 0) {
		if (len > 32 || len == 0) {
			return -EINVAL;
		}
		ssize_t rc = read_cb(cb_arg, cached_ssid, len);
		cached_ssid[len] = '\0';
		if (rc == (ssize_t)len) {
			LOG_INF("WiFi SSID cargado: %s", cached_ssid);
		}
		return 0;
	}

	if (strcmp(key, SETTINGS_KEY_WIFI_PASS) == 0) {
		if (len > 64 || len == 0) {
			return -EINVAL;
		}
		ssize_t rc = read_cb(cb_arg, cached_pass, len);
		cached_pass[len] = '\0';
		if (rc == (ssize_t)len) {
			cached_wifi_valid = true;
			LOG_INF("WiFi password cargado");
		}
		return 0;
	}

	return 0;
}

/* Subsistema de settings: una entrada por dominio */
static struct settings_handler timeout_handler = {
	.name = "sys",
	.h_set = timeout_set,
};

static struct settings_handler brightness_handler = {
	.name = "sys",
	.h_set = brightness_set,
};

static struct settings_handler wifi_handler = {
	.name = "wifi",
	.h_set = wifi_set,
};

/* ---- API publica ---- */

bool gigaspark_settings_init(void)
{
	int rc;

	if (store_initialized) {
		return config_found;
	}

	LOG_INF("Inicializando almacenamiento de configuracion...");

	/* Registrar handlers de settings */
	rc = settings_register(&timeout_handler);
	if (rc < 0) {
		LOG_WRN("No se pudo registrar handler de timeout: %d", rc);
	}

	rc = settings_register(&brightness_handler);
	if (rc < 0) {
		LOG_WRN("No se pudo registrar handler de brillo: %d", rc);
	}

	rc = settings_register(&wifi_handler);
	if (rc < 0) {
		LOG_WRN("No se pudo registrar handler de WiFi: %d", rc);
	}

	/* Cargar todos los settings desde NVS */
	rc = settings_load();
	if (rc < 0) {
		LOG_WRN("Error cargando settings: %d", rc);
		config_found = false;
	} else {
		/* Si el timeout es 0, no habia datos guardados */
		config_found = (cached_timeout != 0);
		if (config_found) {
			LOG_INF("Configuracion encontrada en Flash");
		} else {
			LOG_INF("Primera vez - usando valores por defecto");
		}
	}

	store_initialized = true;
	return config_found;
}

void settings_store_save_timeout(uint32_t seconds)
{
	if (seconds < POWER_SLEEP_TIMEOUT_MIN_S) {
		seconds = POWER_SLEEP_TIMEOUT_MIN_S;
	}
	if (seconds > POWER_SLEEP_TIMEOUT_MAX_S) {
		seconds = POWER_SLEEP_TIMEOUT_MAX_S;
	}

	cached_timeout = seconds;

	int rc = settings_save_one(SETTINGS_KEY_TIMEOUT, &seconds, sizeof(seconds));
	if (rc < 0) {
		LOG_ERR("Error guardando timeout: %d", rc);
	} else {
		LOG_INF("Timeout guardado: %u s", seconds);
	}
}

uint32_t settings_store_load_timeout(void)
{
	if (!store_initialized) {
		gigaspark_settings_init();
	}
	return cached_timeout;
}

void settings_store_save_brightness(uint8_t brightness)
{
	if (brightness > 100) {
		brightness = 100;
	}

	cached_brightness = brightness;

	int rc = settings_save_one(SETTINGS_KEY_BRIGHTNESS, &brightness, sizeof(brightness));
	if (rc < 0) {
		LOG_ERR("Error guardando brillo: %d", rc);
	} else {
		LOG_INF("Brillo guardado: %u%%", brightness);
	}
}

uint8_t settings_store_load_brightness(void)
{
	if (!store_initialized) {
		gigaspark_settings_init();
	}
	return cached_brightness;
}

void settings_store_save_wifi(const char *ssid, const char *pass)
{
	if (!ssid || !pass) {
		return;
	}

	size_t ssid_len = strlen(ssid);
	size_t pass_len = strlen(pass);

	if (ssid_len > 32 || pass_len > 64) {
		LOG_ERR("Credenciales WiFi demasiado largas");
		return;
	}

	/* Guardar SSID */
	int rc = settings_save_one(SETTINGS_KEY_WIFI_SSID, ssid, ssid_len);
	if (rc < 0) {
		LOG_ERR("Error guardando SSID: %d", rc);
		return;
	}

	/* Guardar password */
	rc = settings_save_one(SETTINGS_KEY_WIFI_PASS, pass, pass_len);
	if (rc < 0) {
		LOG_ERR("Error guardando password: %d", rc);
		return;
	}

	/* Actualizar cache */
	strncpy(cached_ssid, ssid, sizeof(cached_ssid) - 1);
	cached_ssid[32] = '\0';
	strncpy(cached_pass, pass, sizeof(cached_pass) - 1);
	cached_pass[64] = '\0';
	cached_wifi_valid = true;

	LOG_INF("WiFi guardado: SSID=%s", ssid);
}

bool settings_store_load_wifi(char *ssid, char *pass)
{
	if (!store_initialized) {
		gigaspark_settings_init();
	}

	if (!cached_wifi_valid || cached_ssid[0] == '\0') {
		return false;
	}

	if (ssid) {
		strncpy(ssid, cached_ssid, 33);
	}
	if (pass) {
		strncpy(pass, cached_pass, 65);
	}

	return true;
}

void settings_store_factory_reset(void)
{
	LOG_INF("=== FACTORY RESET ===");

	/* Borrar todas las claves */
	settings_delete(SETTINGS_KEY_TIMEOUT);
	settings_delete(SETTINGS_KEY_BRIGHTNESS);
	settings_delete(SETTINGS_KEY_WIFI_SSID);
	settings_delete(SETTINGS_KEY_WIFI_PASS);

	/* Resetear cache */
	cached_timeout = 0;
	cached_brightness = 100;
	memset(cached_ssid, 0, sizeof(cached_ssid));
	memset(cached_pass, 0, sizeof(cached_pass));
	cached_wifi_valid = false;

	LOG_INF("Configuracion borrada - valores por defecto se usaran en proximo boot");
}

void settings_store_get_usage(uint32_t *used, uint32_t *total)
{
	/* NVS no expone uso directamente via settings API.
	 * Esto es un stub que se puede expandir con flash_map.
	 */
	if (used) {
		*used = 0;
	}
	if (total) {
		*total = 0;
	}
}

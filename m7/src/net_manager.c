/*
 * Gigaspark OS - Network Manager Implementation
 * Basic network support for Arduino Giga R1
 *
 * Note: Full WiFi support requires CYW43439 firmware blobs
 * which are not available in this Zephyr version.
 * This provides a stub implementation for future development.
 */

#include "net_manager.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(net_manager, CONFIG_LOG_DEFAULT_LEVEL);

/* Connection state */
static bool net_initialized = false;
static bool net_connected = false;
static char ip_str[16] = "0.0.0.0";

int net_manager_init(void)
{
	LOG_INF("Initializing network manager...");

	/*
	 * In a full implementation, this would:
	 * 1. Get the WiFi interface (net_if_get_wifi_sta())
	 * 2. Register event handlers for WiFi connect/disconnect
	 * 3. Register DHCP event handlers
	 *
	 * For now, we provide a stub that allows the system to boot
	 * without WiFi firmware blobs.
	 */

	net_initialized = true;
	LOG_INF("Network manager initialized (stub mode)");
	LOG_INF("Note: WiFi requires CYW43439 firmware blobs");
	return 0;
}

int net_connect_wifi(const char *ssid, const char *psk)
{
	(void)ssid;
	(void)psk;

	LOG_WRN("WiFi connect not available (firmware missing)");
	LOG_INF("Would connect to: %s", ssid ? ssid : "(null)");
	return -ENOTSUP;
}

int net_wait_ip(int32_t timeout_ms)
{
	(void)timeout_ms;

	LOG_WRN("IP wait not available (WiFi not connected)");
	return -ENODEV;
}

bool net_is_connected(void)
{
	return net_connected;
}

int net_get_ip_str(char *buf, size_t buf_len)
{
	if (!net_initialized || buf == NULL || buf_len == 0) {
		return -ENODEV;
	}

	strncpy(buf, ip_str, buf_len - 1);
	buf[buf_len - 1] = '\0';
	return 0;
}

int net_disconnect(void)
{
	net_connected = false;
	LOG_INF("Network disconnected");
	return 0;
}

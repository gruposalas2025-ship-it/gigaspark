/*
 * Gigaspark OS - App Downloader Implementation
 * HTTP client stub for downloading apps
 *
 * Note: Full HTTP support requires network stack.
 * This provides a stub implementation for future development.
 */

#include "app_downloader.h"
#include "ipc_config.h"
#include "ipc_protocol.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_REGISTER(app_downloader, CONFIG_LOG_DEFAULT_LEVEL);

/* Shared buffer for file writes */
extern uint8_t g_file_write_buf[FILE_WRITE_CHUNK_MAX];

/* Send a command to M4 and wait for response */
extern int send_ipc_cmd(struct ipc_msg *cmd, struct ipc_msg *resp);

int download_app_from_url(const char *host, uint16_t port,
			  const char *url, const char *app_name)
{
	(void)host;
	(void)port;
	(void)url;
	(void)app_name;

	LOG_WRN("App download not available (network not ready)");
	LOG_INF("Would download from: http://%s:%d%s", host ? host : "", port, url ? url : "");
	LOG_INF("Would save as: %s", app_name ? app_name : "");
	return -ENOTSUP;
}

int download_app_from_url_string(const char *full_url, const char *app_name)
{
	(void)full_url;
	(void)app_name;

	LOG_WRN("App download not available (network not ready)");
	LOG_INF("Would download from: %s", full_url ? full_url : "");
	LOG_INF("Would save as: %s", app_name ? app_name : "");
	return -ENOTSUP;
}

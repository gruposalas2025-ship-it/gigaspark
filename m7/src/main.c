/*
 * Gigaspark OS - M7 Primary Core
 * STM32H747XI Cortex-M7 @ 480MHz
 *
 * Phase 10: IPC + App Runner + WiFi Downloader
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log.h>

#include "ipc_config.h"
#include "ipc_protocol.h"
#include "fault_manager.h"
#include "app_runner.h"
#include "net_manager.h"
#include "app_downloader.h"

LOG_MODULE_REGISTER(gigaspark_m7, CONFIG_LOG_DEFAULT_LEVEL);

/* SDK init */
extern void gigaspark_sdk_init(void);

/* Green LED = PJ13 (alias led1) */
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static K_SEM_DEFINE(bound_sem, 0, 1);
static K_SEM_DEFINE(data_sem, 0, 1);

static struct ipc_ept ept;
static volatile bool ept_ready;

/* Last response from M4 */
static struct ipc_msg last_resp;

/* Shared buffer for file writes */
uint8_t g_file_write_buf[FILE_WRITE_CHUNK_MAX];

static void ept_bound(void *priv)
{
	ept_ready = true;
	k_sem_give(&bound_sem);
	LOG_INF("Endpoint bound to M4");
}

static void ept_received(const void *data, size_t len, void *priv)
{
	if (len < sizeof(struct ipc_msg)) {
		return;
	}

	memcpy(&last_resp, data, sizeof(last_resp));
	k_sem_give(&data_sem);
}

static struct ipc_ept_cfg ept_cfg = {
	.name = IPC_ENDPOINT_NAME,
	.cb = {
		.bound    = ept_bound,
		.received = ept_received,
	},
};

/* Send a command to M4 and wait for response */
int send_ipc_cmd(struct ipc_msg *cmd, struct ipc_msg *resp)
{
	int ret;

	ret = ipc_service_send(&ept, cmd, sizeof(*cmd));
	if (ret < 0) {
		LOG_ERR("Send failed: %d", ret);
		return ret;
	}

	k_sem_take(&data_sem, K_FOREVER);
	memcpy(resp, &last_resp, sizeof(*resp));
	return 0;
}

int main(void)
{
	const struct device *ipc_instance;
	int ret;

	LOG_INF("Gigaspark OS M7 Primary Core starting...");

	/* Initialize GPIO for green LED */
	if (!gpio_is_ready_dt(&led_green)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure LED: %d", ret);
		return ret;
	}

	/* Initialize fault manager */
	fault_manager_init();

	/* Initialize SDK */
	gigaspark_sdk_init();

	/* Initialize network */
	LOG_INF("Initializing network...");
	ret = net_manager_init();
	if (ret < 0) {
		LOG_WRN("Network init failed: %d", ret);
	}

	/* Initialize IPC */
	ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc_instance);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to open IPC instance: %d", ret);
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc_instance, &ept, &ept_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to register endpoint: %d", ret);
		return ret;
	}

	LOG_INF("Waiting for M4 endpoint...");
	k_sem_take(&bound_sem, K_FOREVER);

	/* === WiFi Connection === */
	LOG_INF("=== Connecting to WiFi ===");

	ret = net_connect_wifi("Gigaspark-AP", "gigaspark123");
	if (ret < 0) {
		LOG_WRN("WiFi connection failed: %d", ret);
	} else {
		LOG_INF("Waiting for IP address...");
		ret = net_wait_ip(10000);  /* 10 second timeout */
		if (ret < 0) {
			LOG_WRN("Failed to get IP: %d", ret);
		} else {
			char ip[16];
			net_get_ip_str(ip, sizeof(ip));
			LOG_INF("Connected! IP: %s", ip);

			/* Try to download an app */
			LOG_INF("=== Downloading app from server ===");
			ret = download_app_from_url(
				"192.168.1.100", 80,
				"/apps/clicker.bin", "clicker.bin"
			);
			if (ret < 0) {
				LOG_WRN("Download failed: %d", ret);
			} else {
				LOG_INF("App downloaded successfully!");
			}
		}
	}

	/* === App Manager === */
	LOG_INF("=== Gigaspark OS App Manager ===");

	/* Get app list from M4 */
	struct ipc_msg list_cmd = {
		.cmd = CMD_GET_APP_LIST,
		.handle = 0,
		.size = 0,
		.status = 0,
	};
	struct ipc_msg resp;

	ret = send_ipc_cmd(&list_cmd, &resp);
	if (ret < 0) {
		LOG_ERR("Failed to get app list: %d", ret);
	} else if (resp.status == STATUS_OK) {
		int app_count = resp.size;
		LOG_INF("Found %d app(s) on SD card", app_count);

		/* Load and execute first app if available */
		if (app_count > 0) {
			LOG_INF("--- Loading app 0 ---");

			struct ipc_msg load_cmd = {
				.cmd = CMD_LOAD_APP,
				.handle = 0,
				.size = 0,
				.status = 0,
			};

			ret = send_ipc_cmd(&load_cmd, &resp);
			if (ret == 0 && resp.status == STATUS_OK) {
				LOG_INF("App loaded: handle=0x%04x, size=%u bytes",
					resp.handle, resp.size);

				/* Execute app with fault recovery */
				LOG_INF("Executing app with fault recovery...");
				int app_result = app_runner_execute(
					(const uint8_t *)(uintptr_t)resp.handle,
					resp.size,
					resp.handle
				);

				if (app_result == APP_RESULT_FAULT) {
					LOG_WRN("App faulted - recovered, returning to launcher");
				} else if (app_result == APP_RESULT_OK) {
					LOG_INF("App exited normally");
				} else {
					LOG_ERR("App execution error: %d", app_result);
				}
			}
		}
	}

	/* Turn ON green LED - system ready */
	gpio_pin_set_dt(&led_green, 1);

	LOG_INF("====================================================");
	LOG_INF(" GIGASPARK OS PHASE 10 COMPLETE");
	LOG_INF(" WiFi + App Downloader + Fault Recovery");
	LOG_INF(" M7 @ 480MHz | M4 @ 240MHz | IPC OK");
	LOG_INF("====================================================");

	return 0;
}

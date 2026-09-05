/*
 * Gigaspark OS - M7 Primary Core
 * STM32H747XI Cortex-M7 @ 480MHz
 *
 * Phase 2: Sends CMD_ALLOC/CMD_FREE to M4 memory engine via IPC.
 * Verifies handle-based memory management works end-to-end.
 * Uses Zephyr IPC Service with OpenAMP static vrings.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log.h>

#include "ipc_config.h"
#include "ipc_protocol.h"

LOG_MODULE_REGISTER(gigaspark_m7, CONFIG_LOG_DEFAULT_LEVEL);

/* Green LED = PJ13 (alias led1) */
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static K_SEM_DEFINE(bound_sem, 0, 1);
static K_SEM_DEFINE(data_sem, 0, 1);

static struct ipc_ept ept;
static volatile bool ept_ready;

/* Last response from M4 */
static struct ipc_msg last_resp;

static void ept_bound(void *priv)
{
	ept_ready = true;
	k_sem_give(&bound_sem);
	LOG_INF("Endpoint bound to M4");
}

static void ept_received(const void *data, size_t len, void *priv)
{
	if (len < sizeof(struct ipc_msg)) {
		LOG_WRN("Received %zu bytes, expected %zu. Ignoring.",
			len, sizeof(struct ipc_msg));
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
static int send_cmd(struct ipc_msg *cmd, struct ipc_msg *resp)
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

	/* === Phase 2: Memory Handle Test === */
	LOG_INF("--- Phase 2: Memory Handle Test ---");

	/* Step 1: Allocate 128 bytes from M4 */
	struct ipc_msg alloc_cmd = {
		.cmd = CMD_ALLOC,
		.size = 128,
		.handle = 0,
		.status = 0,
	};
	struct ipc_msg resp;

	LOG_INF("Sending CMD_ALLOC (128 bytes)...");
	ret = send_cmd(&alloc_cmd, &resp);
	if (ret < 0) {
		LOG_ERR("Alloc command failed: %d", ret);
		return ret;
	}

	if (resp.status != STATUS_OK) {
		LOG_ERR("Alloc failed with status: %d", resp.status);
		return -ENOMEM;
	}

	LOG_INF("Alloc OK! Handle = 0x%04x", resp.handle);
	uint16_t my_handle = resp.handle;

	/* Step 2: Free the handle */
	struct ipc_msg free_cmd = {
		.cmd = CMD_FREE,
		.handle = my_handle,
		.size = 0,
		.status = 0,
	};

	LOG_INF("Sending CMD_FREE (handle 0x%04x)...", my_handle);
	ret = send_cmd(&free_cmd, &resp);
	if (ret < 0) {
		LOG_ERR("Free command failed: %d", ret);
		return ret;
	}

	if (resp.status != STATUS_OK) {
		LOG_ERR("Free failed with status: %d", resp.status);
		return -EIO;
	}

	LOG_INF("Free OK!");

	/* Step 3: Turn ON green LED - system verified */
	gpio_pin_set_dt(&led_green, 1);

	LOG_INF("========================================");
	LOG_INF(" GIGASPARK OS PHASE 2 COMPLETE");
	LOG_INF(" Memory engine: ALLOC/FREE via handles");
	LOG_INF(" M7 @ 480MHz | M4 @ 240MHz | IPC OK");
	LOG_INF("========================================");

	return 0;
}

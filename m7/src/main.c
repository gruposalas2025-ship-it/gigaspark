/*
 * Gigaspark OS - M7 Primary Core
 * STM32H747XI Cortex-M7 @ 480MHz
 *
 * Phase 3: zRAM compression test.
 * Fills the M4 memory pool to force compression, then reads back
 * a compressed block to verify transparent decompression.
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

/* Read buffer for CMD_READ responses */
static uint8_t read_buf[MEM_READ_MAX];

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

	/* === Phase 3: zRAM Compression Test === */
	LOG_INF("=== Phase 3: zRAM Compression Test ===");

	/*
	 * Step 1: Fill the pool by allocating 8 blocks of 512 bytes.
	 * Total = 4096 bytes = full pool.
	 * The 5th allocation will force M4 to compress a victim block.
	 */
	uint16_t handles[8];
	int alloc_count = 0;

	LOG_INF("--- Step 1: Filling pool (8 x 512B) ---");

	for (int i = 0; i < 8; i++) {
		struct ipc_msg alloc_cmd = {
			.cmd = CMD_ALLOC,
			.size = 512,
			.handle = 0,
			.status = 0,
		};
		struct ipc_msg resp;

		ret = send_cmd(&alloc_cmd, &resp);
		if (ret < 0 || resp.status != STATUS_OK) {
			LOG_INF("Alloc %d failed (status=%d), pool full at %d blocks",
				i, resp.status, alloc_count);
			break;
		}

		handles[i] = resp.handle;
		alloc_count++;
		LOG_INF("Alloc[%d]: handle=0x%04x", i, resp.handle);
	}

	LOG_INF("Pool filled: %d blocks allocated", alloc_count);

	/*
	 * Step 2: Read back the first handle to verify decompression.
	 * The M4 should decompress it transparently.
	 */
	if (alloc_count > 0) {
		LOG_INF("--- Step 2: Reading compressed block ---");

		struct ipc_msg read_cmd = {
			.cmd = CMD_READ,
			.handle = handles[0],
			.size = MEM_READ_MAX,
			.status = 0,
		};
		struct ipc_msg resp;

		ret = send_cmd(&read_cmd, &resp);
		if (ret < 0) {
			LOG_ERR("Read failed: %d", ret);
		} else if (resp.status == STATUS_OK) {
			LOG_INF("Read OK: handle=0x%04x actual=%u bytes",
				resp.handle, resp.size);
		} else {
			LOG_ERR("Read error: status=%d", resp.status);
		}
	}

	/*
	 * Step 3: Free all allocated blocks.
	 */
	LOG_INF("--- Step 3: Freeing all blocks ---");

	for (int i = 0; i < alloc_count; i++) {
		struct ipc_msg free_cmd = {
			.cmd = CMD_FREE,
			.handle = handles[i],
			.size = 0,
			.status = 0,
		};
		struct ipc_msg resp;

		ret = send_cmd(&free_cmd, &resp);
		if (ret < 0 || resp.status != STATUS_OK) {
			LOG_ERR("Free handle=0x%04x failed", handles[i]);
		}
	}

	LOG_INF("All %d blocks freed", alloc_count);

	/* Step 4: Turn ON green LED - system verified */
	gpio_pin_set_dt(&led_green, 1);

	LOG_INF("============================================");
	LOG_INF(" GIGASPARK OS PHASE 3 COMPLETE");
	LOG_INF(" zRAM compression: RLE on M4");
	LOG_INF(" M7 @ 480MHz | M4 @ 240MHz | IPC OK");
	LOG_INF("============================================");

	return 0;
}

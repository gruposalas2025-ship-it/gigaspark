/*
 * Gigaspark OS - M4 Remote Core
 * STM32H747XI Cortex-M4 @ 240MHz
 *
 * Phase 4: 3-tier memory engine with SD swap.
 * Tier 1: 4KB main pool (live data)
 * Tier 2: 2KB compressed pool (RLE blobs)
 * Tier 3: SD card sectors (evicted compressed blobs)
 *
 * Receives CMD_ALLOC/CMD_FREE/CMD_READ/CMD_WRITE from M7 via IPC.
 * Returns opaque handles, never exposes real addresses to M7.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log.h>

#include "ipc_config.h"
#include "ipc_protocol.h"
#include "mem_engine.h"

LOG_MODULE_REGISTER(gigaspark_m4, CONFIG_LOG_DEFAULT_LEVEL);

static K_SEM_DEFINE(bound_sem, 0, 1);
static K_SEM_DEFINE(data_sem, 0, 1);

static struct ipc_ept ept;
static volatile bool ept_ready;

/* Buffer for CMD_READ responses */
static uint8_t read_buf[MEM_IO_MAX];

/* Process an incoming IPC command and prepare the response */
static void process_command(const struct ipc_msg *req, struct ipc_msg *resp)
{
	resp->cmd = CMD_RESPONSE;

	switch (req->cmd) {
	case CMD_ALLOC: {
		uint16_t handle = mem_alloc(req->size);

		resp->status = (handle != MEM_HANDLE_INVALID)
			       ? STATUS_OK : STATUS_ERR_NOMEM;
		resp->handle = handle;
		resp->size = req->size;

		LOG_INF("CMD_ALLOC size=%u -> handle=0x%04x status=%d",
			req->size, handle, resp->status);
		break;
	}
	case CMD_FREE: {
		int ret = mem_free(req->handle);

		resp->status = (ret == 0) ? STATUS_OK : STATUS_ERR_BAD_HANDLE;
		resp->handle = req->handle;
		resp->size = 0;

		LOG_INF("CMD_FREE handle=0x%04x -> status=%d",
			req->handle, resp->status);
		break;
	}
	case CMD_READ: {
		size_t to_read = req->size;
		if (to_read > MEM_IO_MAX) {
			to_read = MEM_IO_MAX;
		}

		size_t actual = mem_read(req->handle, read_buf, to_read);

		resp->handle = req->handle;
		resp->size = actual;

		if (actual > 0) {
			resp->status = STATUS_OK;
		} else {
			resp->status = STATUS_ERR_READ_FAILED;
		}

		LOG_INF("CMD_READ handle=0x%04x size=%u -> actual=%u status=%d",
			req->handle, to_read, actual, resp->status);
		break;
	}
	case CMD_WRITE: {
		/*
		 * For Phase 4, CMD_WRITE uses the size field to carry
		 * a simple pattern byte that the M4 uses to fill the block.
		 * The actual data payload is not sent via IPC to keep
		 * the message fixed-size.
		 */
		int ret = mem_write(req->handle, &req->size, sizeof(req->size));

		resp->status = (ret == 0) ? STATUS_OK : STATUS_ERR_WRITE_FAILED;
		resp->handle = req->handle;
		resp->size = 0;

		LOG_INF("CMD_WRITE handle=0x%04x -> status=%d",
			req->handle, resp->status);
		break;
	}
	default:
		LOG_WRN("Unknown cmd: 0x%02x", req->cmd);
		resp->status = STATUS_ERR_BAD_HANDLE;
		resp->handle = MEM_HANDLE_INVALID;
		resp->size = 0;
		break;
	}
}

static void ept_bound(void *priv)
{
	ept_ready = true;
	k_sem_give(&bound_sem);
	LOG_INF("Endpoint bound to M7");
}

static void ept_received(const void *data, size_t len, void *priv)
{
	if (len < sizeof(struct ipc_msg)) {
		LOG_WRN("Received %zu bytes, expected %zu. Ignoring.",
			len, sizeof(struct ipc_msg));
		return;
	}

	const struct ipc_msg *req = (const struct ipc_msg *)data;
	struct ipc_msg resp;
	int ret;

	process_command(req, &resp);

	ret = ipc_service_send(&ept, &resp, sizeof(resp));
	if (ret < 0) {
		LOG_ERR("Failed to send response: %d", ret);
	}

	k_sem_give(&data_sem);
}

static struct ipc_ept_cfg ept_cfg = {
	.name = IPC_ENDPOINT_NAME,
	.cb = {
		.bound    = ept_bound,
		.received = ept_received,
	},
};

int main(void)
{
	const struct device *ipc_instance;
	int ret;

	LOG_INF("Gigaspark OS M4 Remote Core starting...");

	/* Initialize memory engine (includes SD swap init) */
	mem_init();

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

	LOG_INF("Waiting for M7 handshake...");
	k_sem_take(&bound_sem, K_FOREVER);
	LOG_INF("M4 3-tier memory engine ready. Waiting for commands...");

	/* Wait forever - commands are processed in ept_received callback */
	while (1) {
		k_sem_take(&data_sem, K_FOREVER);
	}

	return 0;
}

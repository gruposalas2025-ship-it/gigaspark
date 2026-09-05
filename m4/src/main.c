/*
 * Gigaspark OS - M4 Remote Core
 * STM32H747XI Cortex-M4 @ 240MHz
 *
 * Memory engine: receives CMD_ALLOC/CMD_FREE from M7 via IPC,
 * manages a static 4KB pool, returns opaque handles.
 * Uses Zephyr IPC Service with OpenAMP static vrings.
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

	/* Initialize memory engine */
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
	LOG_INF("M4 memory engine ready. Waiting for commands...");

	/* Wait forever - commands are processed in ept_received callback */
	while (1) {
		k_sem_take(&data_sem, K_FOREVER);
	}

	return 0;
}

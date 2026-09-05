/*
 * Gigaspark OS - M4 Remote Core
 * STM32H747XI Cortex-M4 @ 240MHz
 *
 * Phase 5: File system manager + 3-tier memory engine.
 * Serves app list and app binaries to M7 via IPC.
 * Memory engine: RAM -> compressed_pool -> SD swap.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log.h>

#include "ipc_config.h"
#include "ipc_protocol.h"
#include "mem_engine.h"
#include "fs_manager.h"

LOG_MODULE_REGISTER(gigaspark_m4, CONFIG_LOG_DEFAULT_LEVEL);

static K_SEM_DEFINE(bound_sem, 0, 1);
static K_SEM_DEFINE(data_sem, 0, 1);

static struct ipc_ept ept;
static volatile bool ept_ready;

/* Buffers for IPC responses */
static uint8_t io_buf[MEM_IO_MAX];
static uint8_t app_buf[APP_BINARY_MAX_SIZE];

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
		LOG_INF("CMD_ALLOC size=%u -> handle=0x%04x", req->size, handle);
		break;
	}
	case CMD_FREE: {
		int ret = mem_free(req->handle);
		resp->status = (ret == 0) ? STATUS_OK : STATUS_ERR_BAD_HANDLE;
		resp->handle = req->handle;
		resp->size = 0;
		LOG_INF("CMD_FREE handle=0x%04x -> %d", req->handle, ret);
		break;
	}
	case CMD_READ: {
		size_t to_read = req->size;
		if (to_read > MEM_IO_MAX) {
			to_read = MEM_IO_MAX;
		}
		size_t actual = mem_read(req->handle, io_buf, to_read);
		resp->handle = req->handle;
		resp->size = actual;
		resp->status = (actual > 0) ? STATUS_OK : STATUS_ERR_READ_FAILED;
		LOG_INF("CMD_READ handle=0x%04x -> %u bytes", req->handle, actual);
		break;
	}
	case CMD_WRITE: {
		int ret = mem_write(req->handle, &req->size, sizeof(req->size));
		resp->status = (ret == 0) ? STATUS_OK : STATUS_ERR_WRITE_FAILED;
		resp->handle = req->handle;
		resp->size = 0;
		LOG_INF("CMD_WRITE handle=0x%04x -> %d", req->handle, ret);
		break;
	}
	case CMD_GET_APP_LIST: {
		/*
		 * Respond with app count in size field.
		 * The actual app list is sent in subsequent messages
		 * to avoid exceeding RPMsg buffer size.
		 */
		int count = fs_get_app_count();
		resp->status = (count > 0) ? STATUS_OK : STATUS_ERR_NO_APPS;
		resp->size = count;
		resp->handle = 0;
		LOG_INF("CMD_GET_APP_LIST -> %d apps", count);
		break;
	}
	case CMD_LOAD_APP: {
		/*
		 * Load app binary into memory and return handle.
		 * req->handle contains the app id to load.
		 */
		uint8_t app_id = (uint8_t)(req->handle & 0xFF);
		size_t loaded = fs_load_app(app_id, app_buf, sizeof(app_buf));

		if (loaded == 0) {
			resp->status = STATUS_ERR_APP_NOT_FOUND;
			resp->handle = MEM_HANDLE_INVALID;
			resp->size = 0;
			LOG_INF("CMD_LOAD_APP id=%u -> NOT FOUND", app_id);
		} else {
			/* Allocate memory in the engine for the app */
			uint16_t mem_handle = mem_alloc(loaded);
			if (mem_handle == MEM_HANDLE_INVALID) {
				resp->status = STATUS_ERR_NOMEM;
				resp->handle = MEM_HANDLE_INVALID;
				resp->size = 0;
				LOG_INF("CMD_LOAD_APP id=%u -> NO MEMORY for %u bytes",
					app_id, loaded);
			} else {
				/* Copy app data into allocated memory */
				mem_write(mem_handle, app_buf, loaded);
				resp->status = STATUS_OK;
				resp->handle = mem_handle;
				resp->size = loaded;
				LOG_INF("CMD_LOAD_APP id=%u -> handle=0x%04x size=%u",
					app_id, mem_handle, loaded);
			}
		}
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

	/* Initialize filesystem manager (mount SD, scan apps) */
	ret = fs_init();
	if (ret < 0) {
		LOG_WRN("Filesystem init failed: %d (SD not available)", ret);
	} else {
		LOG_INF("Filesystem ready, %d app(s) found", fs_get_app_count());
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

	LOG_INF("Waiting for M7 handshake...");
	k_sem_take(&bound_sem, K_FOREVER);
	LOG_INF("M4 ready. Waiting for commands...");

	while (1) {
		k_sem_take(&data_sem, K_FOREVER);
	}

	return 0;
}

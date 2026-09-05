/*
 * Gigaspark OS - M4 Remote Core
 * STM32H747XI Cortex-M4 @ 240MHz
 *
 * Waits for BOOT_GIGASPARK from M7, responds M4_KERNEL_READY.
 * Uses Zephyr IPC Service with OpenAMP static vrings.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/sys/printk.h>

#include "ipc_config.h"

static K_SEM_DEFINE(bound_sem, 0, 1);
static K_SEM_DEFINE(data_sem, 0, 1);

static volatile bool ept_ready;
static char recv_buf[IPC_BUFFER_SIZE];
static volatile size_t recv_len;

static void ept_bound(void *priv)
{
	ept_ready = true;
	k_sem_give(&bound_sem);
	printk("[M4] Endpoint bound to M7\n");
}

static void ept_received(const void *data, size_t len, void *priv)
{
	size_t copy_len = (len < sizeof(recv_buf) - 1) ? len : sizeof(recv_buf) - 1;

	memcpy(recv_buf, data, copy_len);
	recv_buf[copy_len] = '\0';
	recv_len = copy_len;

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
	struct ipc_ept ept;
	int ret;

	printk("[M4] Gigaspark OS Remote Core starting...\n");

	ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc_instance);
	if (ret < 0 && ret != -EALREADY) {
		printk("[M4] Failed to open IPC instance: %d\n", ret);
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc_instance, &ept, &ept_cfg);
	if (ret < 0) {
		printk("[M4] Failed to register endpoint: %d\n", ret);
		return ret;
	}

	printk("[M4] Waiting for M7 handshake...\n");
	k_sem_take(&bound_sem, K_FOREVER);

	while (1) {
		k_sem_take(&data_sem, K_FOREVER);

		printk("[M4] Received: \"%s\"\n", recv_buf);

		if (strcmp(recv_buf, MSG_BOOT_REQUEST) == 0) {
			printk("[M4] Boot request recognized, sending response...\n");

			ret = ipc_service_send(&ept, MSG_BOOT_RESPONSE,
					      sizeof(MSG_BOOT_RESPONSE));
			if (ret < 0) {
				printk("[M4] Send failed: %d\n", ret);
				return ret;
			}

			printk("[M4] Response sent. M4 kernel ready.\n");
			break;
		}
	}

	return 0;
}

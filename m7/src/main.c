/*
 * Gigaspark OS - M7 Primary Core
 * STM32H747XI Cortex-M7 @ 480MHz
 *
 * Sends BOOT_GIGASPARK to M4, waits for M4_KERNEL_READY.
 * On success: green LED ON, system ready.
 * Uses Zephyr IPC Service with OpenAMP static vrings.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
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
	printk("[M7] Endpoint bound to M4\n");
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
	const struct device *gpio_dev;
	struct ipc_ept ept;
	int ret;

	printk("[M7] Gigaspark OS Primary Core starting...\n");

	/* --- Initialize GPIO for green LED (led1 = PJ13) --- */
	gpio_dev = DEVICE_DT_GET(DT_ALIAS(led1));
	if (!device_is_ready(gpio_dev)) {
		printk("[M7] GPIO device not ready\n");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(gpio_pin_dt_spec_get(DT_ALIAS(led1)),
				    GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("[M7] Failed to configure LED: %d\n", ret);
		return ret;
	}

	/* --- Initialize IPC --- */
	ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc_instance);
	if (ret < 0 && ret != -EALREADY) {
		printk("[M7] Failed to open IPC instance: %d\n", ret);
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc_instance, &ept, &ept_cfg);
	if (ret < 0) {
		printk("[M7] Failed to register endpoint: %d\n", ret);
		return ret;
	}

	printk("[M7] Waiting for M4 endpoint...\n");
	k_sem_take(&bound_sem, K_FOREVER);

	/* --- Send boot request to M4 --- */
	printk("[M7] Sending boot request to M4...\n");

	ret = ipc_service_send(&ept, MSG_BOOT_REQUEST, sizeof(MSG_BOOT_REQUEST));
	if (ret < 0) {
		printk("[M7] Send failed: %d\n", ret);
		return ret;
	}

	/* --- Wait for M4 response --- */
	printk("[M7] Waiting for M4 response...\n");
	k_sem_take(&data_sem, K_FOREVER);

	printk("[M7] Received: \"%s\"\n", recv_buf);

	if (strcmp(recv_buf, MSG_BOOT_RESPONSE) == 0) {
		printk("[M7] M4 handshake OK!\n");

		/* Turn ON green LED */
		gpio_pin_set_dt(gpio_pin_dt_spec_get(DT_ALIAS(led1)), 1);

		printk("[M7] ========================================\n");
		printk("[M7]  GIGASPARK OS BASE SYSTEM READY\n");
		printk("[M7]  M7 @ 480MHz | M4 @ 240MHz | IPC OK\n");
		printk("[M7] ========================================\n");
	} else {
		printk("[M7] Unexpected response from M4: \"%s\"\n", recv_buf);
		return -EIO;
	}

	return 0;
}

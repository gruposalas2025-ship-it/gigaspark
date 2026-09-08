/*
 * Gigaspark OS - Driver de Herramientas RF Implementation
 * CC1101 + PN532 NFC + IR TX/RX
 *
 * NOTA: Estos son stubs funcionales. Los modulos reales
 * requieren los dispositivos habilitados en el overlay DTS.
 */

#include "rf_tools.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(rf_tools, CONFIG_LOG_DEFAULT_LEVEL);

static bool cc1101_ready = false;
static bool pn532_ready = false;
static bool ir_ready = false;

/* CC1101 Sub-1GHz */
int rf_cc1101_init(uint16_t freq_mhz)
{
	LOG_INF("CC1101: %d MHz", freq_mhz);

#if defined(CONFIG_SPI)
	LOG_INF("SPI disponible - modo hardware");
	cc1101_ready = true;
#else
	LOG_WRN("SPI no habilitado - modo stub");
	cc1101_ready = false;
#endif

	return cc1101_ready ? 0 : -ENODEV;
}

int rf_send_433(const uint8_t *data, size_t len)
{
	if (!cc1101_ready) return -ENODEV;
	if (!data || len == 0) return -EINVAL;

	LOG_INF("CC1101 TX: %d bytes @ 433MHz", (int)len);
	/* Pasos reales: CS low -> TXFIFO write -> STX -> wait GDO0 -> CS high */
	return 0;
}

int rf_receive_433(uint8_t *buf, size_t max_len)
{
	if (!cc1101_ready) return -ENODEV;
	if (!buf || max_len == 0) return -EINVAL;

	/* Pasos reales: check GDO0 -> read RXFIFO -> verify CRC */
	return 0;
}

/* PN532 NFC */
int rf_nfc_init(void)
{
	LOG_INF("PN532 NFC: I2C");

#if defined(CONFIG_I2C)
	LOG_INF("I2C disponible - modo hardware");
	pn532_ready = true;
#else
	LOG_WRN("I2C no habilitado - modo stub");
	pn532_ready = false;
#endif

	return pn532_ready ? 0 : -ENODEV;
}

int rf_nfc_read_card(uint8_t *uid, uint8_t *uid_len)
{
	if (!pn532_ready) return -ENODEV;
	if (!uid || !uid_len) return -EINVAL;

	/* Stub: sin tarjeta */
	*uid_len = 0;
	return 0;
}

int rf_nfc_write_block(uint8_t block_num, const uint8_t *data)
{
	if (!pn532_ready) return -ENODEV;
	if (!data || block_num > 63) return -EINVAL;

	LOG_INF("NFC: write block %d", block_num);
	/* Pasos: auth block -> write 16 bytes -> verify ACK */
	return 0;
}

/* IR Infrarrojo */
int rf_ir_init(void)
{
	LOG_INF("IR TX/RX: 38kHz NEC");
	ir_ready = true;
	return 0;
}

int rf_ir_send_nec(uint8_t address, uint8_t command)
{
	if (!ir_ready) return -ENODEV;

	LOG_INF("IR TX: NEC 0x%02X 0x%02X", address, command);
	/* Pasos: 9ms pulse + 4.5ms space + 32 bits (addr + ~addr + cmd + ~cmd) */
	return 0;
}

int rf_ir_receive(uint8_t *address, uint8_t *command)
{
	if (!ir_ready) return -ENODEV;
	if (!address || !command) return -EINVAL;

	/* Stub: sin datos */
	return 1;
}

/* API Publica */
int rf_init_all(void)
{
	int failures = 0;

	LOG_INF("Inicializando modulos RF...");

	if (rf_cc1101_init(RF_FREQ_433MHZ) < 0) failures++;
	if (rf_nfc_init() < 0) failures++;
	if (rf_ir_init() < 0) failures++;

	LOG_INF("RF: %d/3 modulos listos", 3 - failures);
	return (failures == 3) ? -ENODEV : 0;
}

bool rf_is_ready(uint8_t module)
{
	switch (module) {
	case RF_MODULE_CC1101: return cc1101_ready;
	case RF_MODULE_PN532:  return pn532_ready;
	case RF_MODULE_IR:     return ir_ready;
	default:               return false;
	}
}

/*
 * Gigaspark OS - Driver de Fuente de Poder Implementation
 *
 * MCP41010: 8-bit digital pot via SPI, controla XL4015 Buck
 * Feedback: ADC lee voltaje real (div 91k/10k) y corriente (ACS712)
 *
 * NOTA: Requiere SPI1 y ADC habilitados en overlay DTS.
 */

#include "power_supply.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(power_supply, CONFIG_LOG_DEFAULT_LEVEL);

static uint8_t current_mode = PS_MODE_INDEPENDENT;
static float current_voltage[2] = { 0.0f, 0.0f };
static bool supply_ready = false;

int power_supply_init(void)
{
	LOG_INF("Inicializando fuente de poder...");

#if defined(CONFIG_SPI) && defined(CONFIG_ADC)
	LOG_INF("SPI+ADC habilitados - modo hardware");
	LOG_INF("CH1: D4=CS, A4=Vfb, A6=Ifb, D6=ReleSerie");
	LOG_INF("CH2: D5=CS, A5=Vfb, A7=Ifb, D7=ReleParalelo");
	supply_ready = true;
#else
	LOG_WRN("SPI/ADC no habilitados - modo stub");
	LOG_INF("Para habilitar, agregar al prj.conf:");
	LOG_INF("  CONFIG_SPI=y");
	LOG_INF("  CONFIG_ADC=y");
	supply_ready = false;
#endif

	return 0;
}

int power_set_voltage(uint8_t channel, float voltage)
{
	if (!supply_ready) {
		return -ENODEV;
	}

	if (channel > PS_CH2 || voltage < 0.0f || voltage > PS_VMAX) {
		return -EINVAL;
	}

	/* MCP41010: wiper = (voltage / 30) * 255 */
	float normalized = voltage / PS_VMAX;
	uint8_t wiper = (uint8_t)(normalized * 255.0f);

	LOG_INF("CH%d: %.1fV (wiper=%d)", channel + 1, voltage, wiper);

	/*
	 * Implementacion real SPI:
	 * uint8_t tx[2] = { 0x00, wiper };
	 * struct spi_buf buf = { .buf = tx, .len = 2 };
	 * gpio_pin_set_dt(&cs, 1);
	 * spi_write(spi_dev, &cfg, &(struct spi_buf_set){&buf, 1});
	 * gpio_pin_set_dt(&cs, 0);
	 */

	current_voltage[channel] = voltage;
	return 0;
}

int power_get_readings(uint8_t channel, float *v_out, float *i_out)
{
	if (!supply_ready) {
		return -ENODEV;
	}

	if (channel > PS_CH2) {
		return -EINVAL;
	}

	/*
	 * Implementacion real ADC:
	 * 1. adc_read canal A4/A5 -> voltaje feedback
	 * 2. adc_read canal A6/A7 -> corriente feedback
	 * 3. Escalar: Vreal = Vadc * 10.1 (divisor)
	 * 4. Escalar: Ireal = (Vadc - 1.65) / 0.185 (ACS712)
	 */

	if (v_out) *v_out = current_voltage[channel];
	if (i_out) *i_out = 0.0f;

	return 0;
}

int power_set_mode(uint8_t mode)
{
	if (mode > PS_MODE_PARALLEL) {
		return -EINVAL;
	}

	LOG_INF("Modo: %s",
		mode == 0 ? "Independiente" :
		mode == 1 ? "Serie (60V)" : "Paralelo (10A)");

	/*
	 * Implementacion real GPIO:
	 * gpio_pin_set_dt(&rele_serie, (mode == 1) ? 1 : 0);
	 * gpio_pin_set_dt(&rele_paralelo, (mode == 2) ? 1 : 0);
	 */

	current_mode = mode;
	return 0;
}

uint8_t power_get_mode(void)
{
	return current_mode;
}

void power_off_all(void)
{
	power_set_voltage(PS_CH1, 0.0f);
	power_set_voltage(PS_CH2, 0.0f);
	LOG_INF("Fuente apagada");
}

bool power_is_ready(void)
{
	return supply_ready;
}

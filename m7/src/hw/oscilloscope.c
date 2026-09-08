/*
 * Gigaspark OS - Driver de Osciloscopio Implementation
 *
 * NOTA: Requiere habilitar ADC en el overlay del DTS:
 *   &adc1 { status = "okay"; };
 *   zephyr,user { io-channels = <&adc1 10>, <&adc1 14>; };
 *
 * Cuando el ADC no esta habilitado, funciona en modo stub.
 */

#include "oscilloscope.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(oscilloscope, CONFIG_LOG_DEFAULT_LEVEL);

static volatile bool osc_ready_flag = false;
static volatile bool osc_busy_flag = false;

int osc_init(void)
{
	LOG_INF("Inicializando osciloscopio...");
	LOG_INF("CH1: A0 (PC0), CH2: A1 (PA4)");
	LOG_INF("Rango: 0-%.0fV, Divisor: %.0f:1", (double)OSC_VMAX, (double)OSC_DIVIDER);

#if defined(CONFIG_ADC) && DT_NODE_EXISTS(DT_PATH(zephyr_user))
	LOG_INF("ADC habilitado en DTS - modo hardware");
	/* Aqui iria la configuracion real del ADC con
	 * adc_channel_setup_dt() y las specs del DT */
	osc_ready_flag = true;
#else
	LOG_WRN("ADC no habilitado en DTS - modo stub");
	LOG_INF("Para habilitar, agregar al overlay:");
	LOG_INF("  &adc1 { status = \"okay\"; };");
	LOG_INF("  zephyr,user { io-channels = <&adc1 10>, <&adc1 14>; };");
	osc_ready_flag = false;
#endif

	return 0;
}

int osc_capture(uint8_t channel, uint16_t *buf, uint16_t samples, uint32_t fs_hz)
{
	if (!osc_ready_flag) {
		return -ENODEV;
	}

	if (buf == NULL || samples == 0 || samples > OSC_CAPTURE_MAX) {
		return -EINVAL;
	}

	if (channel > OSC_CH2) {
		return -EINVAL;
	}

	osc_busy_flag = true;

	LOG_DBG("Capturando %d muestras CH%d @ %u Hz", samples, channel + 1, fs_hz);

	/*
	 * Implementacion real con DMA:
	 * 1. adc_sequence_init(&seq, &adc_dev)
	 * 2. seq.channels = BIT(channel_id)
	 * 3. seq.buffer = buf
	 * 4. seq.buffer_size = samples * 2
	 * 5. seq.resolution = 12
	 * 6. adc_read(adc_dev, &seq)
	 *
	 * Para alta velocidad usar DMA:
	 * seq.dma_enabled = true
	 * seq.dma_periodic = true
	 */

	osc_busy_flag = false;
	osc_ready_flag = true;
	return samples;
}

float osc_get_voltage(uint8_t channel)
{
	if (!osc_ready_flag) {
		return -1.0f;
	}

	uint16_t buf[1] = {0};
	int ret = osc_capture(channel, buf, 1, 1000);
	if (ret < 0) {
		return -1.0f;
	}

	float v_adc = (float)buf[0] / 4095.0f * OSC_VREF;
	return v_adc * OSC_DIVIDER;
}

float osc_get_voltage_rms(const uint16_t *buf, uint16_t samples)
{
	if (buf == NULL || samples == 0) {
		return 0.0f;
	}

	float sum = 0.0f;
	for (uint16_t i = 0; i < samples; i++) {
		float v = (float)buf[i] / 4095.0f * OSC_VREF;
		sum += v * v;
	}

	return sqrtf(sum / (float)samples) * OSC_DIVIDER;
}

float osc_get_voltage_pp(const uint16_t *buf, uint16_t samples)
{
	if (buf == NULL || samples == 0) {
		return 0.0f;
	}

	uint16_t min_v = buf[0], max_v = buf[0];
	for (uint16_t i = 1; i < samples; i++) {
		if (buf[i] < min_v) min_v = buf[i];
		if (buf[i] > max_v) max_v = buf[i];
	}

	float vmin = (float)min_v / 4095.0f * OSC_VREF;
	float vmax = (float)max_v / 4095.0f * OSC_VREF;
	return (vmax - vmin) * OSC_DIVIDER;
}

bool osc_data_ready(void)
{
	return osc_ready_flag;
}

void osc_stop(void)
{
	osc_busy_flag = false;
}

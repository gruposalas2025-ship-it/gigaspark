/*
 * Gigaspark OS - Driver de Osciloscopio
 * Captura ADC a alta velocidad
 *
 * CH1: A0 (PC0) - Osciloscopio canal 1
 * CH2: A1 (PA4) - Osciloscopio canal 2
 * Rango: 0-30V (divisor 10:1 hardware)
 */

#ifndef GIGASPARK_OSCILLOSCOPE_H
#define GIGASPARK_OSCILLOSCOPE_H

#include <stdint.h>
#include <stdbool.h>

#define OSC_CH1  0
#define OSC_CH2  1
#define OSC_CAPTURE_MAX  4096
#define OSC_VREF         3.3f
#define OSC_DIVIDER      10.1f
#define OSC_VMAX         30.0f

int osc_init(void);
int osc_capture(uint8_t channel, uint16_t *buf, uint16_t samples, uint32_t fs_hz);
float osc_get_voltage(uint8_t channel);
float osc_get_voltage_rms(const uint16_t *buf, uint16_t samples);
float osc_get_voltage_pp(const uint16_t *buf, uint16_t samples);
bool osc_data_ready(void);
void osc_stop(void);

#endif

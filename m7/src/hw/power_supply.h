/*
 * Gigaspark OS - Driver de Fuente de Poder
 * MCP41010 SPI + Feedback ADC
 *
 * CH1: D4=CS MCP41010, A4=Feedback V, A6=Feedback I, D6=Rele Serie
 * CH2: D5=CS MCP41010, A5=Feedback V, A7=Feedback I, D7=Rele Paralelo
 */

#ifndef GIGASPARK_POWER_SUPPLY_H
#define GIGASPARK_POWER_SUPPLY_H

#include <stdint.h>
#include <stdbool.h>

#define PS_CH1  0
#define PS_CH2  1
#define PS_MODE_INDEPENDENT  0
#define PS_MODE_SERIES       1
#define PS_MODE_PARALLEL     2
#define PS_VMAX  30.0f
#define PS_IMAX  5.0f

int power_supply_init(void);
int power_set_voltage(uint8_t channel, float voltage);
int power_get_readings(uint8_t channel, float *v_out, float *i_out);
int power_set_mode(uint8_t mode);
uint8_t power_get_mode(void);
void power_off_all(void);
bool power_is_ready(void);

#endif

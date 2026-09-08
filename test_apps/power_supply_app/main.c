/*
 * Gigaspark OS - App Fuente de Laboratorio
 * Control de voltaje/corriente con feedback en pantalla
 *
 * Muestra dos botones (+/-) para ajustar el voltaje de
 * salida del Canal 1 (0.0V a 30.0V en pasos de 0.5V).
 * Muestra voltaje y corriente reales leidos del feedback.
 *
 * Uso: Seleccionar desde el Launcher en la pantalla tactil.
 */

#include "gigaspark_api.h"
#include "../../m7/src/hw/power_supply.h"

/* Configuracion */
#define VOLTAGE_STEP   0.5f
#define VOLTAGE_MIN    0.0f
#define VOLTAGE_MAX    30.0f
#define UPDATE_MS      500

/* Estado de la app */
static float target_voltage = 0.0f;

/*
 * Punto de entrada de la app de fuente de laboratorio.
 */
void app_main(void)
{
	giga_display_init();
	giga_display_clear(0x0000);

	/* Titulo */
	giga_display_draw_text(10, 10, "FUENTE LAB - Canal 1",
			       0xFFFF, 0x0000, 16);

	/* Inicializar fuente de poder */
	power_supply_init();

	/* Botones de voltaje */
	giga_button_t btn_minus = {
		.x = 20, .y = 60,
		.width = 80, .height = 50,
		.text = "-",
		.color = 0xF800,  /* Rojo */
		.text_color = 0xFFFF,
	};

	giga_button_t btn_plus = {
		.x = 220, .y = 60,
		.width = 80, .height = 50,
		.text = "+",
		.color = 0x07E0,  /* Verde */
		.text_color = 0xFFFF,
	};

	giga_button_t btn_onoff = {
		.x = 120, .y = 180,
		.width = 80, .height = 40,
		.text = "ON/OFF",
		.color = 0xFFE0,  /* Amarillo */
		.text_color = 0x0000,
	};

	giga_button_draw(&btn_minus);
	giga_button_draw(&btn_plus);
	giga_button_draw(&btn_onoff);

	/* Loop principal */
	while (1) {
		/* Dibujar voltaje objetivo */
		char msg[32];
		snprintf(msg, sizeof(msg), "Vset: %.1fV", target_voltage);
		giga_display_draw_text(90, 70, msg, 0xFFFF, 0x0000, 20);

		/* Leer y mostrar voltaje/corriente reales */
		float v_real = 0.0f, i_real = 0.0f;
		power_get_readings(PS_CH1, &v_real, &i_real);

		snprintf(msg, sizeof(msg), "V: %.2fV", v_real);
		giga_display_draw_text(10, 130, msg, 0x07FF, 0x0000, 16);

		snprintf(msg, sizeof(msg), "I: %.2fA", i_real);
		giga_display_draw_text(170, 130, msg, 0x07FF, 0x0000, 16);

		/* Potencia */
		float power = v_real * i_real;
		snprintf(msg, sizeof(msg), "P: %.1fW", power);
		giga_display_draw_text(100, 155, msg, 0xFFE0, 0x0000, 14);

		/* Verificar botones tocados */
		giga_touch_event_t touch;
		if (giga_touch_read(&touch)) {
			if (giga_button_hit_test(&btn_minus, &touch)) {
				target_voltage -= VOLTAGE_STEP;
				if (target_voltage < VOLTAGE_MIN) {
					target_voltage = VOLTAGE_MIN;
				}
				power_set_voltage(PS_CH1, target_voltage);
			}

			if (giga_button_hit_test(&btn_plus, &touch)) {
				target_voltage += VOLTAGE_STEP;
				if (target_voltage > VOLTAGE_MAX) {
					target_voltage = VOLTAGE_MAX;
				}
				power_set_voltage(PS_CH1, target_voltage);
			}

			if (giga_button_hit_test(&btn_onoff, &touch)) {
				if (target_voltage > 0.0f) {
					target_voltage = 0.0f;
					power_off_all();
				} else {
					target_voltage = 5.0f;
					power_set_voltage(PS_CH1, 5.0f);
				}
			}
		}

		k_msleep(UPDATE_MS);
	}
}

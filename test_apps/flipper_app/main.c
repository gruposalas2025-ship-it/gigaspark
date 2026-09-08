/*
 * Gigaspark OS - App Flipper / Herramientas RF
 * NFC, IR, y Sub-1GHz en una sola interfaz
 *
 * Menu con 3 opciones:
 *   1. Leer NFC - Muestra UID de tarjetas Mifare/NTAG
 *   2. Enviar IR - Comando NEC predefinido
 *   3. Enviar 433MHz - Packet de prueba CC1101
 *
 * Uso: Seleccionar desde el Launcher en la pantalla tactil.
 */

#include "gigaspark_api.h"
#include "../../m7/src/hw/rf_tools.h"

/* IDs de menu */
#define MENU_NFC     0
#define MENU_IR      1
#define MENU_433     2
#define MENU_COUNT   3

/* Estado */
static int selected_menu = 0;
static bool showing_result = false;

/* Texto del resultado */
static char result_text[128] = "Selecciona una opcion";

/*
 * Dibujar el menu principal del Flipper
 */
static void draw_menu(void)
{
	giga_display_clear(0x0000);

	/* Titulo */
	giga_display_draw_text(10, 10, "HERRAMIENTAS RF",
			       0xFFFF, 0x0000, 16);

	/* Opciones del menu */
	const char *options[] = {
		"1. Leer NFC",
		"2. Enviar IR (NEC)",
		"3. Enviar 433MHz",
	};

	for (int i = 0; i < MENU_COUNT; i++) {
		uint16_t color = (i == selected_menu) ? 0x07E0 : 0xFFFF;
		giga_display_draw_text(20, 50 + i * 40, options[i],
				       color, 0x0000, 16);
	}

	/* Resultado */
	giga_display_draw_text(10, 200, result_text,
			       0x07FF, 0x0000, 12);
}

/*
 * Ejecutar la accion seleccionada
 */
static void execute_action(void)
{
	switch (selected_menu) {
	case MENU_NFC: {
		/* Leer tarjeta NFC */
		uint8_t uid[7];
		uint8_t uid_len = 0;

		int ret = rf_nfc_read_card(uid, &uid_len);

		if (ret == 0 && uid_len > 0) {
			/* Formatear UID como hex */
			char hex[24];
			int pos = 0;
			for (int i = 0; i < uid_len && pos < 22; i++) {
				pos += snprintf(hex + pos, sizeof(hex) - pos,
						"%02X ", uid[i]);
			}
			snprintf(result_text, sizeof(result_text),
				 "NFC: %s", hex);
		} else {
			snprintf(result_text, sizeof(result_text),
				 "NFC: Sin tarjeta detectada");
		}
		break;
	}

	case MENU_IR: {
		/* Enviar comando IR NEC */
		int ret = rf_ir_send_nec(0x00, 0xFF);
		if (ret == 0) {
			snprintf(result_text, sizeof(result_text),
				 "IR: NEC enviado (0x00, 0xFF)");
		} else {
			snprintf(result_text, sizeof(result_text),
				 "IR: Error %d", ret);
		}
		break;
	}

	case MENU_433: {
		/* Enviar packet de prueba 433MHz */
		uint8_t test_data[] = { 0xDE, 0xAD, 0xBE, 0xEF };
		int ret = rf_send_433(test_data, sizeof(test_data));
		if (ret == 0) {
			snprintf(result_text, sizeof(result_text),
				 "433MHz: DE AD BE EF enviado");
		} else {
			snprintf(result_text, sizeof(result_text),
				 "433MHz: Error %d", ret);
		}
		break;
	}
	}

	showing_result = true;
}

/*
 * Punto de entrada de la app Flipper.
 */
void app_main(void)
{
	giga_display_init();

	/* Inicializar modulos RF */
	rf_init_all();

	draw_menu();

	/* Loop principal */
	while (1) {
		/* Leer toque */
		giga_touch_event_t touch;
		if (giga_touch_read(&touch)) {
			int tx = touch.x;
			int ty = touch.y;

			/* Verificar si toco una opcion del menu */
			for (int i = 0; i < MENU_COUNT; i++) {
				int opt_y = 50 + i * 40;
				if (tx >= 20 && tx <= 300 &&
				    ty >= opt_y && ty <= opt_y + 30) {
					selected_menu = i;
					showing_result = false;
					draw_menu();
					break;
				}
			}

			/* Si toco el area de resultado, ejecutar accion */
			if (ty >= 190 && ty <= 210 && showing_result == false) {
				execute_action();
				draw_menu();
			}

			/* Toque en pantalla para volver al menu */
			if (ty > 220) {
				showing_result = false;
				draw_menu();
			}
		}

		/* Si hay resultado mostrado y se toca, volver al menu */
		if (showing_result) {
			k_msleep(100);
		}
	}
}

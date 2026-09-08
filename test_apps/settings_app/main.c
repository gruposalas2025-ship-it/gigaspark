/*
 * Gigaspark OS - App de Ajustes y Monitor de Recursos
 *
 * Muestra el estado del hardware y permite configurar el OS.
 * Usa el SDK (gigaspark_api.h) para todas las interacciones.
 *
 * Secciones:
 * - Monitor de Memoria: uso del buddy allocator M4
 * - Bateria y Energia: nivel de bateria (stub por ahora)
 * - Ajustes de Suspension: tiempo de inactividad (5-60s)
 *
 * Ejecuta en el M7 via app_runner.
 */

#include "../sdk/gigaspark_api.h"

/* ---- Layout ---- */
#define MARGIN_X    20
#define CONTENT_W   440
#define ROW_H       36
#define SEC_GAP     12

/* Botones +/- */
#define BTN_SIZE    24
#define BTN_GAP     16

/* Colores */
#define BG_COLOR    GIGA_BLACK
#define TITLE_COLOR GIGA_CYAN
#define LABEL_COLOR GIGA_WHITE
#define VALUE_COLOR GIGA_GREEN
#define BTN_BG      GIGA_BLUE
#define BTN_FG      GIGA_WHITE
#define GRAY_COLOR  GIGA_GRAY

/* ---- Estado global ---- */
static uint32_t current_timeout = 15;
static int btn_minus_y;
static int btn_plus_y;

/* Entero a string (buffer minimo 12 bytes) */
static void utoa(uint32_t val, char *buf)
{
	char tmp[12];
	int i = 0;

	if (val == 0) {
		buf[0] = '0';
		buf[1] = '\0';
		return;
	}
	while (val > 0) {
		tmp[i++] = '0' + (val % 10);
		val /= 10;
	}
	for (int j = 0; j < i; j++) {
		buf[j] = tmp[i - 1 - j];
	}
	buf[i] = '\0';
}

/* Dibujar titulo de seccion, retorna Y siguiente */
static int draw_section(int y, const char *title)
{
	giga_fill_rect(MARGIN_X, y, CONTENT_W, 26, BG_COLOR);
	giga_draw_text(MARGIN_X, y + 4, title, TITLE_COLOR);
	return y + 26 + 4;
}

/* Dibujar fila label:valor, retorna Y siguiente */
static int draw_row(int y, const char *label, const char *value)
{
	giga_fill_rect(MARGIN_X, y, CONTENT_W, ROW_H, BG_COLOR);
	giga_draw_text(MARGIN_X + 10, y + 8, label, LABEL_COLOR);
	giga_draw_text(MARGIN_X + 220, y + 8, value, VALUE_COLOR);
	return y + ROW_H;
}

/* Dibujar botones +/- al lado del valor de timeout */
static void draw_buttons(int y)
{
	int by = y + 6;

	/* Boton - */
	giga_fill_rect(MARGIN_X + 320, by, BTN_SIZE, BTN_SIZE, BTN_BG);
	giga_draw_rect(MARGIN_X + 320, by, BTN_SIZE, BTN_SIZE, GRAY_COLOR);
	giga_draw_text(MARGIN_X + 328, by + 4, "-", BTN_FG);
	btn_minus_y = by;

	/* Boton + */
	giga_fill_rect(MARGIN_X + 320 + BTN_SIZE + BTN_GAP, by,
		       BTN_SIZE, BTN_SIZE, BTN_BG);
	giga_draw_rect(MARGIN_X + 320 + BTN_SIZE + BTN_GAP, by,
		       BTN_SIZE, BTN_SIZE, GRAY_COLOR);
	giga_draw_text(MARGIN_X + 320 + BTN_SIZE + BTN_GAP + 8, by + 4,
		       "+", BTN_FG);
	btn_plus_y = by;
}

/* Redibujar toda la pantalla */
static void redraw(void)
{
	char vbuf[16];

	giga_fill_rect(0, 0, GIGA_SCREEN_W, GIGA_SCREEN_H, BG_COLOR);

	/* Titulo */
	giga_draw_text(MARGIN_X, 8, "AJUSTES GIGASPARK", GIGA_CYAN);
	giga_draw_text(MARGIN_X, 28, "Monitor y Configuracion", GRAY_COLOR);

	int y = 56;

	/* Seccion: Memoria */
	y = draw_section(y, "MEMORIA");
	uint8_t usage = giga_mem_get_usage();
	utoa(usage, vbuf);
	char mem_str[20] = "Uso: ";
	int ml = 5;
	for (int i = 0; vbuf[i]; i++) {
		mem_str[ml++] = vbuf[i];
	}
	mem_str[ml++] = '%';
	mem_str[ml] = '\0';
	y = draw_row(y, "Buddy Allocator M4:", mem_str);

	y += SEC_GAP;

	/* Seccion: Bateria */
	y = draw_section(y, "BATERIA");
	uint8_t batt = giga_get_battery_pct();
	utoa(batt, vbuf);
	char batt_str[20] = "Nivel: ";
	int bl = 7;
	for (int i = 0; vbuf[i]; i++) {
		batt_str[bl++] = vbuf[i];
	}
	batt_str[bl++] = '%';
	batt_str[bl] = '\0';
	y = draw_row(y, "Bateria:", batt_str);
	y = draw_row(y, "Fuente:", "USB/DC directa");

	y += SEC_GAP;

	/* Seccion: Suspension */
	y = draw_section(y, "SUSPENSION");
	current_timeout = giga_get_screen_timeout();
	utoa(current_timeout, vbuf);
	char tstr[20] = "Timeout: ";
	int tl = 9;
	for (int i = 0; vbuf[i]; i++) {
		tstr[tl++] = vbuf[i];
	}
	tstr[tl++] = 's';
	tstr[tl] = '\0';
	y = draw_row(y, "Inactividad:", tstr);
	draw_buttons(y);

	y += ROW_H + 30;
	giga_draw_text(MARGIN_X, y, "Toca HOME para salir", GRAY_COLOR);

	giga_update();
}

/* ---- App Main ---- */
void app_main(void)
{
	giga_log_info("Settings app starting...");
	redraw();
	giga_log_info("Settings: Main loop");

	while (1) {
		int tx, ty;

		if (giga_get_touch(&tx, &ty)) {
			bool changed = false;
			int bx_minus = MARGIN_X + 320;
			int bx_plus = MARGIN_X + 320 + BTN_SIZE + BTN_GAP;
			int bxe = bx_plus + BTN_SIZE;

			/* Boton - */
			if (tx >= bx_minus && tx < bx_minus + BTN_SIZE &&
			    ty >= btn_minus_y && ty < btn_minus_y + BTN_SIZE) {
				if (current_timeout > 5) {
					current_timeout -= 5;
					giga_set_screen_timeout(current_timeout);
					changed = true;
				}
			}

			/* Boton + */
			if (tx >= bx_plus && tx < bxe &&
			    ty >= btn_plus_y && ty < btn_plus_y + BTN_SIZE) {
				if (current_timeout < 60) {
					current_timeout += 5;
					giga_set_screen_timeout(current_timeout);
					changed = true;
				}
			}

			if (changed) {
				redraw();
			}
		}

		giga_sleep_ms(50);
	}
}

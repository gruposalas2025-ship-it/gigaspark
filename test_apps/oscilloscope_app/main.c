/*
 * Gigaspark OS - App Osciloscopio
 * Captura y visualizacion de waveform en tiempo real
 *
 * Muestra la forma de onda del Canal 1 (A0) en la pantalla
 * con 320 puntos (uno por columna de 320px del display).
 * Calcula y muestra voltaje RMS y pico-pico.
 *
 * Uso: Seleccionar desde el Launcher en la pantalla tactil.
 */

/* Incluir SDK del Mega-Dispositivo */
#include "gigaspark_api.h"

/* Headers de hardware */
#include "../../m7/src/hw/oscilloscope.h"

/* Buffer de captura: 320 muestras (1 por columna de pantalla) */
#define CAPTURE_SAMPLES  320
#define DISPLAY_WIDTH    320
#define DISPLAY_HEIGHT   240

static uint16_t capture_buf[CAPTURE_SAMPLES];

/*
 * Punto de entrada de la app de osciloscopio.
 * Se ejecuta en el hilo app_runner del M7.
 */
void app_main(void)
{
	/* Inicializar display via SDK */
	giga_display_init();

	/* Limpiar pantalla (fondo negro) */
	giga_display_clear(0x0000);

	/* Titulo */
	giga_display_draw_text(10, 10, "OSCILLOSCOPIO CH1",
			       0xFFFF, 0x0000, 16);

	/* Inicializar osciloscopio */
	osc_init();

	/* Loop principal de captura y dibujo */
	while (1) {
		/* Capturar 320 muestras a 360kHz (1 MSPS / 3) */
		int captured = osc_capture(OSC_CH1, capture_buf,
					   CAPTURE_SAMPLES, 1000000);

		if (captured > 0) {
			/* Limpiar area de grafica */
			giga_display_fill_rect(0, 30, DISPLAY_WIDTH,
					       DISPLAY_HEIGHT - 40, 0x0000);

			/* Dibujar grilla (lineas verticales) */
			for (int x = 0; x < DISPLAY_WIDTH; x += 40) {
				giga_display_draw_line(x, 30, x,
						      DISPLAY_HEIGHT - 10,
						      0x2104);  /* Gris oscuro */
			}

			/* Dibujar grilla (lineas horizontales) */
			for (int y = 30; y < DISPLAY_HEIGHT - 10; y += 40) {
				giga_display_draw_line(0, y, DISPLAY_WIDTH,
						      y, 0x2104);
			}

			/* Dibujar waveform (lineas conectando puntos) */
			for (int i = 0; i < captured - 1; i++) {
				/* Escalar de 12 bits a alto de pantalla */
				int y1 = DISPLAY_HEIGHT - 10 -
					 (capture_buf[i] * (DISPLAY_HEIGHT - 50) / 4095);
				int y2 = DISPLAY_HEIGHT - 10 -
					 (capture_buf[i + 1] * (DISPLAY_HEIGHT - 50) / 4095);

				giga_display_draw_line(i, y1, i + 1, y2,
						      0x07E0);  /* Verde */
			}

			/* Calcular y mostrar voltaje RMS */
			float v_rms = osc_get_voltage_rms(capture_buf,
							   CAPTURE_SAMPLES);
			float v_pp = osc_get_voltage_pp(capture_buf,
							 CAPTURE_SAMPLES);

			char msg[64];
			snprintf(msg, sizeof(msg), "RMS: %.2fV  PP: %.2fV",
				 v_rms, v_pp);
			giga_display_draw_text(10, DISPLAY_HEIGHT - 25, msg,
					       0xFFE0, 0x0000, 12);
		}

		/* Esperar 33ms (~30 FPS) */
		k_msleep(33);
	}
}

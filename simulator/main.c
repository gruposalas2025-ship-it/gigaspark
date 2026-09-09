/*
 * Gigaspark OS - Simulador Nativo (SDL2)
 *
 * Ejecuta el OS de Gigaspark en una ventana de PC usando SDL2.
 * Simula la pantalla 480x272 y el touch via raton.
 *
 * Compilar: cd simulator && mkdir build && cd build && cmake .. && make
 * Ejecutar: ./gigaspark_sim
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Incluir header del SDK para la tabla de API */
#include "gigaspark_api.h"

/* ============================================================
 * CONSTANTES
 * ============================================================ */

#define SCREEN_W        480
#define SCREEN_H        272
#define WINDOW_SCALE    2      /* Ventana 960x544 (2x) */
#define WINDOW_W        (SCREEN_W * WINDOW_SCALE)
#define WINDOW_H        (SCREEN_H * WINDOW_SCALE)
#define WINDOW_TITLE    "Gigaspark OS - Simulador"
#define TARGET_FPS      30
#define FRAME_MS        (1000 / TARGET_FPS)

/* ============================================================
 * FRAMEBUFFER Y ESTADO
 * ============================================================ */

/* Framebuffer RGB565 (del SDK) */
extern uint16_t sim_fb[SCREEN_W * SCREEN_H];

/* Touch desde SDK */
extern void sim_set_touch(int x, int y, bool pressed);

/* API table (simulada en memoria) */
static giga_api_t *api = (giga_api_t *)GIGA_API_TABLE_ADDR;

/* ============================================================
 * Launcher Menu (simula el menu del Arduino Giga)
 * ============================================================ */

#define MAX_APPS    8
#define APP_NAME_LEN 32

typedef struct {
	char name[APP_NAME_LEN];
	char desc[64];
} app_entry_t;

static app_entry_t app_list[MAX_APPS];
static int app_count = 0;
static int selected_app = 0;

static void init_app_list(void)
{
	/* Apps del sistema (simuladas) */
	snprintf(app_list[0].name, APP_NAME_LEN, "Clicker");
	snprintf(app_list[0].desc, 64, "Demo de touch interactiva");
	snprintf(app_list[1].name, APP_NAME_LEN, "Settings");
	snprintf(app_list[1].desc, 64, "Ajustes y monitor de recursos");
	snprintf(app_list[2].name, APP_NAME_LEN, "Oscilloscope");
	snprintf(app_list[2].desc, 64, "Osciloscopio 2 canales");
	snprintf(app_list[3].name, APP_NAME_LEN, "Power Supply");
	snprintf(app_list[3].desc, 64, "Fuente de poder dual");
	snprintf(app_list[4].name, APP_NAME_LEN, "Flipper");
	snprintf(app_list[4].desc, 64, "Herramientas RF/IR/NFC");
	app_count = 5;
}

/* Dibujar un caracter simple (font 8x16 bitmap) */
static void draw_char(int x, int y, char c, uint16_t color)
{
	/* Font bitmap simple 8x16 para ASCII 32-127 */
	/* Solo dibujamos '#' como placeholder para cada caracter visible */
	if (c < 32 || c > 126) return;

	for (int row = 0; row < 14; row++) {
		for (int col = 0; col < 8; col++) {
			/* Patron: todos los caracteres visibles son bloques solidos */
			int px = x + col;
			int py = y + row;
			if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H) {
				sim_fb[py * SCREEN_W + px] = color;
			}
		}
	}
}

/* Dibujar string (font 8x14) */
static void draw_string(int x, int y, const char *str, uint16_t color)
{
	int cx = x;
	while (*str) {
		if (*str == '\n') {
			cx = x;
			y += 16;
		} else {
			draw_char(cx, y, *str, color);
			cx += 8;
		}
		str++;
	}
}

/* Dibujar rectangulo relleno */
static void fill_rect(int x, int y, int w, int h, uint16_t color)
{
	for (int row = y; row < y + h && row < SCREEN_H; row++) {
		for (int col = x; col < x + w && col < SCREEN_W; col++) {
			if (col >= 0 && row >= 0) {
				sim_fb[row * SCREEN_W + col] = color;
			}
		}
	}
}

/* Dibujar rectangulo con borde */
static void draw_rect(int x, int y, int w, int h, uint16_t color)
{
	for (int i = 0; i < w; i++) {
		if (x+i >= 0 && x+i < SCREEN_W) {
			if (y >= 0 && y < SCREEN_H)
				sim_fb[y * SCREEN_W + (x+i)] = color;
			if (y+h-1 >= 0 && y+h-1 < SCREEN_H)
				sim_fb[(y+h-1) * SCREEN_W + (x+i)] = color;
		}
	}
	for (int j = 0; j < h; j++) {
		if (y+j >= 0 && y+j < SCREEN_H) {
			if (x >= 0 && x < SCREEN_W)
				sim_fb[(y+j) * SCREEN_W + x] = color;
			if (x+w-1 >= 0 && x+w-1 < SCREEN_W)
				sim_fb[(y+j) * SCREEN_W + (x+w-1)] = color;
		}
	}
}

/* Dibujar barra de navegacion */
static void draw_nav_bar(void)
{
	int bar_y = SCREEN_H - 40;

	/* Fondo de la barra */
	fill_rect(0, bar_y, SCREEN_W, 40, 0x4208);  /* Gris oscuro */

	/* Linea separadora */
	fill_rect(0, bar_y, SCREEN_W, 2, 0x7BEF);  /* Gris */

	/* Boton Home (centro) */
	int btn_x = 190;
	int btn_w = 100;
	int btn_h = 30;
	int btn_y = bar_y + 5;

	fill_rect(btn_x, btn_y, btn_w, btn_h, 0x001F);  /* Azul */
	draw_rect(btn_x, btn_y, btn_w, btn_h, 0xFFFF);  /* Borde blanco */
	draw_string(btn_x + 30, btn_y + 8, "HOME", 0xFFFF);
}

/* Dibujar el launcher menu */
static void draw_launcher(void)
{
	/* Fondo negro */
	fill_rect(0, 0, SCREEN_W, SCREEN_H, 0x0000);

	/* Titulo */
	draw_string(20, 10, "GIGASPARK OS v1.0", 0x07FF);  /* Cyan */
	draw_string(20, 30, "Simulador Nativo - SDL2", 0x7BEF);  /* Gris */

	/* Lista de apps */
	int y = 60;
	for (int i = 0; i < app_count; i++) {
		int item_h = 36;
		uint16_t bg = (i == selected_app) ? 0x001F : 0x2104;  /* Azul seleccion / gris */
		uint16_t fg = (i == selected_app) ? 0xFFFF : 0x7BEF;

		/* Fondo del item */
		fill_rect(20, y, SCREEN_W - 40, item_h, bg);

		/* Borde */
		draw_rect(20, y, SCREEN_W - 40, item_h, 0x7BEF);

		/* Indicador de seleccion */
		if (i == selected_app) {
			draw_string(30, y + 10, ">", 0xFFE0);  /* Amarillo */
		}

		/* Nombre de la app */
		draw_string(50, y + 4, app_list[i].name, fg);

		/* Descripcion */
		draw_string(50, y + 20, app_list[i].desc, 0x7BEF);

		y += item_h + 4;
	}

	/* Barra de navegacion */
	draw_nav_bar();

	/* Instrucciones */
	draw_string(20, SCREEN_H - 55, "Click para seleccionar", 0x7BEF);
}

/* ============================================================
 * MAIN
 * ============================================================ */

int main(int argc, char *argv[])
{
	(void)argc; (void)argv;

	printf("========================================\n");
	printf("  Gigaspark OS - Simulador Nativo\n");
	printf("  Pantalla: %dx%d (ventana %dx%d)\n", SCREEN_W, SCREEN_H, WINDOW_W, WINDOW_H);
	printf("  FPS: %d\n", TARGET_FPS);
	printf("  Controles:\n");
	printf("    Mouse click = Touch\n");
	printf("    Flechas     = Navegar menu\n");
	printf("    Enter       = Seleccionar app\n");
	printf("    Escape      = Volver al launcher\n");
	printf("    q           = Salir\n");
	printf("========================================\n");

	/* Inicializar SDL2 */
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Error inicializando SDL2: %s\n", SDL_GetError());
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow(
		WINDOW_TITLE,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_W, WINDOW_H,
		SDL_WINDOW_SHOWN
	);
	if (!window) {
		fprintf(stderr, "Error creando ventana: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	}
	if (!renderer) {
		fprintf(stderr, "Error creando renderer: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	/* Textura para el framebuffer */
	SDL_Texture *texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGB565,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_W, SCREEN_H
	);
	if (!texture) {
		fprintf(stderr, "Error creando textura: %s\n", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	/* Limpiar framebuffer */
	memset(sim_fb, 0, sizeof(sim_fb));

	/* Inicializar API table */
	printf("Inicializando SDK API...\n");
	/* La tabla ya esta en la variable global api */
	(void)api;

	/* Inicializar lista de apps */
	init_app_list();

	/* Estado del launcher */
	bool running = true;
	bool in_launcher = true;
	SDL_Event event;

	printf("Simulador listo. Ventana abierta.\n");

	/* ============================================================
	 * MAIN LOOP
	 * ============================================================ */
	while (running) {
		Uint32 frame_start = SDL_GetTicks();

		/* Procesar eventos SDL2 */
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				running = false;
				break;

			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
					if (!in_launcher) {
						in_launcher = true;
						printf("[Sim] Volviendo al launcher\n");
					}
					break;
				case SDLK_q:
					running = false;
					break;
				case SDLK_UP:
					if (in_launcher && selected_app > 0) {
						selected_app--;
					}
					break;
				case SDLK_DOWN:
					if (in_launcher && selected_app < app_count - 1) {
						selected_app++;
					}
					break;
				case SDLK_RETURN:
					if (in_launcher) {
						printf("[Sim] App seleccionada: %s\n",
						       app_list[selected_app].name);
						in_launcher = false;
					}
					break;
				default:
					break;
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == SDL_BUTTON_LEFT) {
					int mx = event.button.x / WINDOW_SCALE;
					int my = event.button.y / WINDOW_SCALE;
					sim_set_touch(mx, my, true);

					/* Detectar click en la lista del launcher */
					if (in_launcher) {
						int item_y = 60;
						for (int i = 0; i < app_count; i++) {
							if (mx >= 20 && mx < SCREEN_W - 20 &&
							    my >= item_y && my < item_y + 36) {
								selected_app = i;
								printf("[Sim] App seleccionada: %s\n",
								       app_list[i].name);
								in_launcher = false;
								break;
							}
							item_y += 40;
						}

						/* Detectar click en boton Home */
						if (my >= SCREEN_H - 35 && my < SCREEN_H - 5 &&
						    mx >= 190 && mx < 290) {
							in_launcher = true;
							printf("[Sim] Home pressed\n");
						}
					}
				}
				break;

			case SDL_MOUSEBUTTONUP:
				if (event.button.button == SDL_BUTTON_LEFT) {
					sim_set_touch(0, 0, false);
				}
				break;

			case SDL_MOUSEMOTION:
				if (event.motion.state & SDL_BUTTON_LMASK) {
					int mx = event.motion.x / WINDOW_SCALE;
					int my = event.motion.y / WINDOW_SCALE;
					sim_set_touch(mx, my, true);
				}
				break;
			}
		}

		/* ---- Renderizar ---- */

		/* Dibujar segun estado */
		if (in_launcher) {
			draw_launcher();
		} else {
			/* Pantalla de app en ejecucion */
			fill_rect(0, 0, SCREEN_W, SCREEN_H, 0x0000);
			draw_string(20, 20, "Ejecutando:", 0x07FF);
			draw_string(20, 40, app_list[selected_app].name, 0xFFFF);
			draw_string(20, 70, app_list[selected_app].desc, 0x7BEF);
			draw_string(20, 110, "Presiona ESC para volver", 0x7BEF);
			draw_nav_bar();
		}

		/* Enviar framebuffer a SDL2 */
		SDL_UpdateTexture(texture, NULL, sim_fb, SCREEN_W * sizeof(uint16_t));
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		/* Control de FPS */
		Uint32 frame_time = SDL_GetTicks() - frame_start;
		if (frame_time < FRAME_MS) {
			SDL_Delay(FRAME_MS - frame_time);
		}
	}

	/* Limpiar */
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	printf("Simulador cerrado.\n");
	return 0;
}

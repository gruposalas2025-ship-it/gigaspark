/*
 * Gigaspark OS - SDK Stubs para Simulador PC
 *
 * Implementaciones de las funciones del SDK que funcionan
 * en una PC con SDL2, sin hardware STM32 real.
 */

#include "gigaspark_api.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>

/* Framebuffer del simulador (global, accedido por main.c) */
uint16_t sim_fb[GIGA_SCREEN_W * GIGA_SCREEN_H];

/* Estado del touch simulado (mouse) */
static int sim_touch_x = 0;
static int sim_touch_y = 0;
static bool sim_touch_pressed = false;

/* ---- Memory ---- */
static void *sim_malloc(size_t size) { return malloc(size); }
static void  sim_free(void *ptr) { free(ptr); }

/* ---- Display ---- */
static void sim_fill_rect(int x, int y, int w, int h, uint16_t color)
{
	if (x < 0 || y < 0 || x + w > GIGA_SCREEN_W || y + h > GIGA_SCREEN_H) return;
	for (int row = y; row < y + h; row++) {
		for (int col = x; col < x + w; col++) {
			sim_fb[row * GIGA_SCREEN_W + col] = color;
		}
	}
}

static void sim_draw_rect(int x, int y, int w, int h, uint16_t color)
{
	for (int i = 0; i < w; i++) {
		if (x+i >= 0 && x+i < GIGA_SCREEN_W) {
			if (y >= 0 && y < GIGA_SCREEN_H)
				sim_fb[y * GIGA_SCREEN_W + (x+i)] = color;
			if (y+h-1 >= 0 && y+h-1 < GIGA_SCREEN_H)
				sim_fb[(y+h-1) * GIGA_SCREEN_W + (x+i)] = color;
		}
	}
	for (int j = 0; j < h; j++) {
		if (y+j >= 0 && y+j < GIGA_SCREEN_H) {
			if (x >= 0 && x < GIGA_SCREEN_W)
				sim_fb[(y+j) * GIGA_SCREEN_W + x] = color;
			if (x+w-1 >= 0 && x+w-1 < GIGA_SCREEN_W)
				sim_fb[(y+j) * GIGA_SCREEN_W + (x+w-1)] = color;
		}
	}
}

static void sim_draw_text(int x, int y, const char *text, uint16_t color)
{
	/* Stub: solo log en modo verbose */
	(void)x; (void)y; (void)text; (void)color;
}

static void sim_update(void)
{
	/* El framebuffer se actualiza en el loop de SDL2 (main.c) */
}

/* ---- Touch ---- */
static bool sim_get_touch(int *x, int *y)
{
	if (x) *x = sim_touch_x;
	if (y) *y = sim_touch_y;
	return sim_touch_pressed;
}

/* ---- Buttons ---- */
static void *sim_btn_create(void *parent) { (void)parent; return NULL; }
static void  sim_btn_set_text(void *btn, const char *txt) { (void)btn; (void)txt; }
static void  sim_btn_set_pos(void *btn, int x, int y) { (void)btn; (void)x; (void)y; }
static void  sim_btn_set_size(void *btn, int w, int h) { (void)btn; (void)w; (void)h; }
static bool  sim_btn_hit(void *btn, int x, int y) { (void)btn; (void)x; (void)y; return false; }
static uint16_t sim_btn_get_state(void *btn) { (void)btn; return 0; }

/* ---- Timing ---- */
static void sim_sleep_ms(uint32_t ms) { (void)ms; /* SDL_Delay en main.c */ }
static uint32_t sim_get_tick_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* ---- Logging ---- */
static void sim_log_info(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	printf("[INFO] ");
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
}

static void sim_log_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "[ERROR] ");
	vprintf(fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

/* ---- IPC (stubs) ---- */
static int sim_ipc_send(uint8_t cmd, const void *data, size_t len)
{
	(void)cmd; (void)data; (void)len;
	return 0;
}
static int sim_ipc_recv(void *data, size_t len, uint32_t timeout_ms)
{
	(void)data; (void)len; (void)timeout_ms;
	return 0;
}

/* ---- Text ---- */
static void sim_set_font(uint8_t font_id) { (void)font_id; }
static void sim_get_text_size(const char *text, int *w, int *h)
{
	if (text) {
		if (w) *w = (int)strlen(text) * 8;
		if (h) *h = 16;
	} else {
		if (w) *w = 0;
		if (h) *h = 0;
	}
}

/* ---- Red (stubs) ---- */
static int sim_http_get(const char *url, uint8_t *buf, size_t buf_len, size_t *out_len)
{
	(void)url; (void)buf; (void)buf_len;
	if (out_len) *out_len = 0;
	return -ENOSYS;
}
static int sim_http_post(const char *url, const char *content_type,
			  const uint8_t *data, size_t data_len,
			  uint8_t *buf, size_t buf_len, size_t *out_len)
{
	(void)url; (void)content_type; (void)data; (void)data_len;
	(void)buf; (void)buf_len;
	if (out_len) *out_len = 0;
	return -ENOSYS;
}

/* ---- Memoria avanzada (stubs) ---- */
static uint32_t sim_mem_alloc_buddy(uint32_t size) { (void)size; return 0; }
static void sim_mem_free_buddy(uint32_t handle) { (void)handle; }
static uint8_t sim_mem_get_usage(void) { return 42; }

/* ---- zRAM (stubs) ---- */
static size_t sim_zram_store(uint32_t h, const uint8_t *d, size_t l)
{ (void)h; (void)d; (void)l; return 0; }
static size_t sim_zram_load(uint32_t h, uint8_t *b, size_t l)
{ (void)h; (void)b; (void)l; return 0; }
static void sim_zram_get_stats(uint8_t *u, uint8_t *t, uint32_t *c)
{ (void)u; (void)t; (void)c; }

/* ---- Sistema (Fase 20) ---- */
static uint8_t sim_get_battery_pct(void) { return 100; }
static uint32_t sim_get_screen_timeout(void) { return 15; }
static void sim_set_screen_timeout(uint32_t s) { (void)s; }

/* ---- Tabla de API ---- */
static const giga_api_t sim_api = {
	.version          = 0x0100,
	.malloc           = sim_malloc,
	.free             = sim_free,
	.fill_rect        = sim_fill_rect,
	.draw_rect        = sim_draw_rect,
	.draw_text        = sim_draw_text,
	.update           = sim_update,
	.get_touch        = sim_get_touch,
	.btn_create       = sim_btn_create,
	.btn_set_text     = sim_btn_set_text,
	.btn_set_pos      = sim_btn_set_pos,
	.btn_set_size     = sim_btn_set_size,
	.btn_hit          = sim_btn_hit,
	.btn_get_state    = sim_btn_get_state,
	.sleep_ms         = sim_sleep_ms,
	.get_tick_ms      = sim_get_tick_ms,
	.log_info         = sim_log_info,
	.log_error        = sim_log_error,
	.ipc_send         = sim_ipc_send,
	.ipc_recv         = sim_ipc_recv,
	.set_font         = sim_set_font,
	.get_text_size    = sim_get_text_size,
	.http_get         = sim_http_get,
	.http_post        = sim_http_post,
	.mem_alloc_buddy  = sim_mem_alloc_buddy,
	.mem_free_buddy   = sim_mem_free_buddy,
	.mem_get_usage    = sim_mem_get_usage,
	.zram_store       = sim_zram_store,
	.zram_load        = sim_zram_load,
	.zram_get_stats   = sim_zram_get_stats,
	.get_battery_pct  = sim_get_battery_pct,
	.get_screen_timeout = sim_get_screen_timeout,
	.set_screen_timeout = sim_set_screen_timeout,
};

/* Funciones para main.c */
void sim_set_touch(int x, int y, bool pressed)
{
	sim_touch_x = x;
	sim_touch_y = y;
	sim_touch_pressed = pressed;
}

void sim_api_init(void)
{
	/* Copiar tabla a la direccion fija (simulada) */
	giga_api_t *dst = (giga_api_t *)GIGA_API_TABLE_ADDR;
	/* En PC no podemos escribir a 0xC0001000, asi que usamos una variable global */
	memcpy(dst, &sim_api, sizeof(sim_api));
}

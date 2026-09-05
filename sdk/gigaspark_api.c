/*
 * Gigaspark OS - SDK API Implementation
 * Runtime support for user apps
 *
 * This file provides the actual implementations of SDK functions
 * and populates the export table at 0xC0001000.
 */

#include "gigaspark_api.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(gigaspark_sdk, CONFIG_LOG_DEFAULT_LEVEL);

/* ---- Memory ---- */

static void *giga_malloc_impl(size_t size)
{
	return k_malloc(size);
}

static void giga_free_impl(void *ptr)
{
	k_free(ptr);
}

/* ---- Display (stub - writes to framebuffer) ---- */

static uint16_t fb[GIGA_SCREEN_W * GIGA_SCREEN_H] __attribute__((aligned(4)));

static void giga_fill_rect_impl(int x, int y, int w, int h, uint16_t color)
{
	if (x < 0 || y < 0 || x + w > GIGA_SCREEN_W || y + h > GIGA_SCREEN_H) {
		return;
	}
	for (int row = y; row < y + h; row++) {
		for (int col = x; col < x + w; col++) {
			fb[row * GIGA_SCREEN_W + col] = color;
		}
	}
}

static void giga_draw_rect_impl(int x, int y, int w, int h, uint16_t color)
{
	/* Draw border only */
	for (int i = 0; i < w; i++) {
		if (x + i >= 0 && x + i < GIGA_SCREEN_W) {
			if (y >= 0 && y < GIGA_SCREEN_H)
				fb[y * GIGA_SCREEN_W + (x + i)] = color;
			if (y + h - 1 >= 0 && y + h - 1 < GIGA_SCREEN_H)
				fb[(y + h - 1) * GIGA_SCREEN_W + (x + i)] = color;
		}
	}
	for (int j = 0; j < h; j++) {
		if (y + j >= 0 && y + j < GIGA_SCREEN_H) {
			if (x >= 0 && x < GIGA_SCREEN_W)
				fb[(y + j) * GIGA_SCREEN_W + x] = color;
			if (x + w - 1 >= 0 && x + w - 1 < GIGA_SCREEN_W)
				fb[(y + j) * GIGA_SCREEN_W + (x + w - 1)] = color;
		}
	}
}

static void giga_draw_text_impl(int x, int y, const char *text, uint16_t color)
{
	/* Stub: log the text being drawn */
	if (text) {
		LOG_DBG("draw_text(%d, %d, '%s', 0x%04x)", x, y, text, color);
	}
}

static void giga_update_impl(void)
{
	/* TODO: Send framebuffer to LTDC/DMA2D */
	LOG_DBG("Display update");
}

/* ---- Touch ---- */

static bool giga_get_touch_impl(int *x, int *y)
{
	/* TODO: Read from LVGL indev */
	if (x) *x = 0;
	if (y) *y = 0;
	return false;
}

/* ---- Buttons (LVGL) ---- */

static void *giga_btn_create_impl(void *parent)
{
	/* TODO: Call lv_btn_create(parent) */
	LOG_DBG("btn_create(parent=%p)", parent);
	return NULL;
}

static void giga_btn_set_text_impl(void *btn, const char *txt)
{
	/* TODO: Call lv_label_set_text() on button's label */
	LOG_DBG("btn_set_text(btn=%p, '%s')", btn, txt);
}

static void giga_btn_set_pos_impl(void *btn, int x, int y)
{
	/* TODO: Call lv_obj_set_pos() */
	LOG_DBG("btn_set_pos(btn=%p, %d, %d)", btn, x, y);
}

static void giga_btn_set_size_impl(void *btn, int w, int h)
{
	/* TODO: Call lv_obj_set_size() */
	LOG_DBG("btn_set_size(btn=%p, %d, %d)", btn, w, h);
}

static bool giga_btn_hit_impl(void *btn, int x, int y)
{
	/* TODO: Check if (x,y) is within button bounds */
	(void)btn; (void)x; (void)y;
	return false;
}

static uint16_t giga_btn_get_state_impl(void *btn)
{
	/* TODO: Return lv_obj_get_state(btn) */
	(void)btn;
	return 0;
}

/* ---- Timing ---- */

static void giga_sleep_ms_impl(uint32_t ms)
{
	k_msleep(ms);
}

static uint32_t giga_get_tick_ms_impl(void)
{
	return k_uptime_get_32();
}

/* ---- Logging ---- */

static void giga_log_info_impl(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[256];
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	LOG_INF("[APP] %s", buf);
}

static void giga_log_error_impl(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[256];
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	LOG_ERR("[APP] %s", buf);
}

/* ---- IPC ---- */

static int giga_ipc_send_impl(uint8_t cmd, const void *data, size_t len)
{
	/* TODO: Send via IPC to M4 */
	(void)cmd; (void)data; (void)len;
	return 0;
}

static int giga_ipc_recv_impl(void *data, size_t len, uint32_t timeout_ms)
{
	/* TODO: Receive via IPC from M4 */
	(void)data; (void)len; (void)timeout_ms;
	return 0;
}

/* ---- Text Rendering ---- */

static void giga_set_font_impl(uint8_t font_id)
{
	(void)font_id;
	LOG_DBG("set_font(%d)", font_id);
}

static void giga_get_text_size_impl(const char *text, int *w, int *h)
{
	/* Rough estimate: 8px per char, 16px height */
	if (text) {
		if (w) *w = strlen(text) * 8;
		if (h) *h = 16;
	} else {
		if (w) *w = 0;
		if (h) *h = 0;
	}
}

/* ---- Export Table ---- */

static const giga_api_t giga_api_table = {
	.version       = GIGA_API_VERSION,
	.malloc        = giga_malloc_impl,
	.free          = giga_free_impl,
	.fill_rect     = giga_fill_rect_impl,
	.draw_rect     = giga_draw_rect_impl,
	.draw_text     = giga_draw_text_impl,
	.update        = giga_update_impl,
	.get_touch     = giga_get_touch_impl,
	.btn_create    = giga_btn_create_impl,
	.btn_set_text  = giga_btn_set_text_impl,
	.btn_set_pos   = giga_btn_set_pos_impl,
	.btn_set_size  = giga_btn_set_size_impl,
	.btn_hit       = giga_btn_hit_impl,
	.btn_get_state = giga_btn_get_state_impl,
	.sleep_ms      = giga_sleep_ms_impl,
	.get_tick_ms   = giga_get_tick_ms_impl,
	.log_info      = giga_log_info_impl,
	.log_error     = giga_log_error_impl,
	.ipc_send      = giga_ipc_send_impl,
	.ipc_recv      = giga_ipc_recv_impl,
	.set_font      = giga_set_font_impl,
	.get_text_size = giga_get_text_size_impl,
};

/*
 * Initialize the SDK export table.
 * Copies the table to the known address (0xC0001000)
 * so apps can access it.
 */
void gigaspark_sdk_init(void)
{
	LOG_INF("Initializing SDK export table at 0x%08x", GIGA_API_TABLE_ADDR);

	/*
	 * In a production system, you'd set up an MPU region for 0xC0001000
	 * that maps to a RAM region containing the table.
	 * For now, we'll use a linker symbol approach.
	 *
	 * The table is placed at the beginning of a dedicated memory section.
	 * Apps access it via the fixed address.
	 */
	LOG_INF("SDK v0x%04x initialized", GIGA_API_VERSION);
}

/* Export the table symbol for linker */
const giga_api_t *gigaspark_get_api(void)
{
	return &giga_api_table;
}

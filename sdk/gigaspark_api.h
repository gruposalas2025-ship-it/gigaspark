/*
 * Gigaspark OS - SDK API Header
 * Public API for user apps running on Gigaspark OS
 *
 * This header is provided to app developers.
 * Apps link against this via the export table.
 */

#ifndef GIGASPARK_API_H
#define GIGASPARK_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Display dimensions */
#define GIGA_SCREEN_W  480
#define GIGA_SCREEN_H  272

/* API function table version */
#define GIGA_API_VERSION  0x0100

/*
 * API function table - placed at known address (0xC0001000).
 * Apps use indices into this table to call OS functions.
 * The OS populates this table at boot time.
 */
typedef struct giga_api {
	/* Version */
	uint32_t version;

	/* Memory */
	void *(*malloc)(size_t size);
	void  (*free)(void *ptr);

	/* Display */
	void  (*fill_rect)(int x, int y, int w, int h, uint16_t color);
	void  (*draw_rect)(int x, int y, int w, int h, uint16_t color);
	void  (*draw_text)(int x, int y, const char *text, uint16_t color);
	void  (*update)(void);

	/* Touch */
	bool  (*get_touch)(int *x, int *y);

	/* Buttons (LVGL-based) */
	void *(*btn_create)(void *parent);
	void  (*btn_set_text)(void *btn, const char *txt);
	void  (*btn_set_pos)(void *btn, int x, int y);
	void  (*btn_set_size)(void *btn, int w, int h);
	bool  (*btn_hit)(void *btn, int x, int y);
	uint16_t (*btn_get_state)(void *btn);

	/* Timing */
	void  (*sleep_ms)(uint32_t ms);
	uint32_t (*get_tick_ms)(void);

	/* Logging */
	void  (*log_info)(const char *fmt, ...);
	void  (*log_error)(const char *fmt, ...);

	/* IPC */
	int   (*ipc_send)(uint8_t cmd, const void *data, size_t len);
	int   (*ipc_recv)(void *data, size_t len, uint32_t timeout_ms);

	/* Text rendering */
	void  (*set_font)(uint8_t font_id);
	void  (*get_text_size)(const char *text, int *w, int *h);

} giga_api_t;

/* API table address (linker symbol) */
#define GIGA_API_TABLE_ADDR  0xC0001000

/* Access the API table from app code */
#define GIGA_API  ((giga_api_t *)GIGA_API_TABLE_ADDR)

/* Convenience macros */
#define giga_malloc(size)        GIGA_API->malloc(size)
#define giga_free(ptr)           GIGA_API->free(ptr)
#define giga_fill_rect(x,y,w,h,c) GIGA_API->fill_rect(x,y,h,c)
#define giga_draw_rect(x,y,w,h,c) GIGA_API->draw_rect(x,y,h,c)
#define giga_draw_text(x,y,t,c)  GIGA_API->draw_text(x,y,t,c)
#define giga_update()            GIGA_API->update()
#define giga_get_touch(x,y)      GIGA_API->get_touch(x,y)
#define giga_btn_create(p)       GIGA_API->btn_create(p)
#define giga_btn_set_text(b,t)   GIGA_API->btn_set_text(b,t)
#define giga_btn_set_pos(b,x,y)  GIGA_API->btn_set_pos(b,x,y)
#define giga_btn_set_size(b,w,h) GIGA_API->btn_set_size(b,w,h)
#define giga_btn_hit(b,x,y)      GIGA_API->btn_hit(b,x,y)
#define giga_btn_get_state(b)    GIGA_API->btn_get_state(b)
#define giga_sleep_ms(ms)        GIGA_API->sleep_ms(ms)
#define giga_get_tick_ms()       GIGA_API->get_tick_ms()
#define giga_log_info(fmt,...)   GIGA_API->log_info(fmt,##__VA_ARGS__)
#define giga_log_error(fmt,...)  GIGA_API->log_error(fmt,##__VA_ARGS__)

/* Colors (RGB565) */
#define GIGA_BLACK    0x0000
#define GIGA_WHITE    0xFFFF
#define GIGA_RED      0xF800
#define GIGA_GREEN    0x07E0
#define GIGA_BLUE     0x001F
#define GIGA_YELLOW   0xFFE0
#define GIGA_CYAN     0x07FF
#define GIGA_MAGENTA  0xF81F
#define GIGA_ORANGE   0xFD20
#define GIGA_GRAY     0x7BEF

/* Font IDs */
#define GIGA_FONT_SMALL   0
#define GIGA_FONT_NORMAL  1
#define GIGA_FONT_LARGE   2

#endif /* GIGASPARK_API_H */

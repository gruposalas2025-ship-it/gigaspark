/*
 * Gigaspark OS - SDK API Header
 * API publica para apps de usuario en Gigaspark OS.
 *
 * Este header se provee a desarrolladores de apps.
 * Las apps acceden a esta tabla via la direccion fija 0xC0001000.
 */

#ifndef GIGASPARK_API_H
#define GIGASPARK_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** Ancho de pantalla en pixeles. */
#define GIGA_SCREEN_W  480

/** Alto de pantalla en pixeles. */
#define GIGA_SCREEN_H  272

/** Version de la tabla de API (Major << 8 | Minor). */
#define GIGA_API_VERSION  0x0100

/**
 * @brief Tabla de funciones de la API del sistema operativo.
 *
 * Ubicada en la direccion fija 0xC0001000. Las apps usan indices
 * en esta tabla para llamar funciones del OS. El OS pobla esta
 * tabla al iniciar.
 */
typedef struct giga_api {
	/* ---- Version ---- */
	uint32_t version;   /**< Version de la API */

	/* ---- Memoria ---- */

	/**
	 * @brief Asignar memoria dinamica.
	 * @param size  Tamano en bytes a asignar.
	 * @return Puntero a la memoria asignada, o NULL si falla.
	 */
	void *(*malloc)(size_t size);

	/**
	 * @brief Liberar memoria dinamica.
	 * @param ptr  Puntero previamente asignado con malloc.
	 */
	void  (*free)(void *ptr);

	/* ---- Pantalla ---- */

	/**
	 * @brief Dibujar un rectangulo relleno.
	 * @param x      Coordenada X superior izquierda.
	 * @param y      Coordenada Y superior izquierda.
	 * @param w      Ancho en pixeles.
	 * @param h      Alto en pixeles.
	 * @param color  Color RGB565.
	 */
	void  (*fill_rect)(int x, int y, int w, int h, uint16_t color);

	/**
	 * @brief Dibujar el borde de un rectangulo.
	 * @param x      Coordenada X superior izquierda.
	 * @param y      Coordenada Y superior izquierda.
	 * @param w      Ancho en pixeles.
	 * @param h      Alto en pixeles.
	 * @param color  Color RGB565.
	 */
	void  (*draw_rect)(int x, int y, int w, int h, uint16_t color);

	/**
	 * @brief Dibujar texto en pantalla.
	 * @param x      Coordenada X.
	 * @param y      Coordenada Y.
	 * @param text   Cadena de texto terminada en nulo.
	 * @param color  Color RGB565 del texto.
	 */
	void  (*draw_text)(int x, int y, const char *text, uint16_t color);

	/**
	 * @brief Actualizar la pantalla (enviar framebuffer al display).
	 * Debe llamarse despues de dibujar para que los cambios sean visibles.
	 */
	void  (*update)(void);

	/* ---- Touch ---- */

	/**
	 * @brief Leer el estado del touchscreen.
	 * @param x  Puntero para coordenada X (o NULL).
	 * @param y  Puntero para coordenada Y (o NULL).
	 * @return true si hay toque activo, false si no.
	 */
	bool  (*get_touch)(int *x, int *y);

	/* ---- Botones LVGL ---- */

	/**
	 * @brief Crear un boton LVGL.
	 * @param parent  Objeto padre (NULL para pantalla completa).
	 * @return Puntero al boton creado, o NULL si falla.
	 */
	void *(*btn_create)(void *parent);

	/**
	 * @brief Establecer el texto de un boton.
	 * @param btn  Puntero al boton.
	 * @param txt  Cadena de texto.
	 */
	void  (*btn_set_text)(void *btn, const char *txt);

	/**
	 * @brief Establecer la posicion de un boton.
	 * @param btn  Puntero al boton.
	 * @param x    Coordenada X.
	 * @param y    Coordenada Y.
	 */
	void  (*btn_set_pos)(void *btn, int x, int y);

	/**
	 * @brief Establecer el tamano de un boton.
	 * @param btn  Puntero al boton.
	 * @param w    Ancho en pixeles.
	 * @param h    Alto en pixeles.
	 */
	void  (*btn_set_size)(void *btn, int w, int h);

	/**
	 * @brief Verificar si un toque esta dentro de un boton.
	 * @param btn  Puntero al boton.
	 * @param x    Coordenada X del toque.
	 * @param y    Coordenada Y del toque.
	 * @return true si el toque esta dentro del boton.
	 */
	bool  (*btn_hit)(void *btn, int x, int y);

	/**
	 * @brief Obtener el estado de un boton.
	 * @param btn  Puntero al boton.
	 * @return Estado del boton (0=inactivo, otro=activo).
	 */
	uint16_t (*btn_get_state)(void *btn);

	/* ---- Tiempo ---- */

	/**
	 * @brief Dormir el hilo actual por un tiempo.
	 * @param ms  Milisegundos a dormir.
	 */
	void  (*sleep_ms)(uint32_t ms);

	/**
	 * @brief Obtener el tiempo actual en milisegundos.
	 * @return Milisegundos desde el arranque del sistema.
	 */
	uint32_t (*get_tick_ms)(void);

	/* ---- Logging ---- */

	/**
	 * @brief Registrar un mensaje informativo.
	 * @param fmt  Formato printf.
	 */
	void  (*log_info)(const char *fmt, ...);

	/**
	 * @brief Registrar un mensaje de error.
	 * @param fmt  Formato printf.
	 */
	void  (*log_error)(const char *fmt, ...);

	/* ---- IPC (Comunicacion entre nucleos) ---- */

	/**
	 * @brief Enviar un mensaje al nucleo M4 via IPC.
	 * @param cmd   Codigo de comando.
	 * @param data  Puntero a los datos (o NULL).
	 * @param len   Longitud de los datos.
	 * @return 0 si ok, negativo si falla.
	 */
	int   (*ipc_send)(uint8_t cmd, const void *data, size_t len);

	/**
	 * @brief Recibir un mensaje del nucleo M4 via IPC.
	 * @param data         Buffer de salida.
	 * @param len          Tamano del buffer.
	 * @param timeout_ms   Tiempo maximo de espera (0=infinite).
	 * @return Bytes recibidos, o negativo si falla/timeout.
	 */
	int   (*ipc_recv)(void *data, size_t len, uint32_t timeout_ms);

	/* ---- Texto ---- */

	/**
	 * @brief Seleccionar la fuente activa.
	 * @param font_id  ID de la fuente (GIGA_FONT_SMALL/NORMAL/LARGE).
	 */
	void  (*set_font)(uint8_t font_id);

	/**
	 * @brief Obtener el tamano estimado de un texto.
	 * @param text  Cadena de texto.
	 * @param w     Puntero para ancho estimado (o NULL).
	 * @param h     Puntero para alto estimado (o NULL).
	 */
	void  (*get_text_size)(const char *text, int *w, int *h);

	/* ---- Red (Fase 16) ---- */

	/**
	 * @brief Realizar una peticion HTTP GET.
	 * @param url      URL completa.
	 * @param buf      Buffer para la respuesta.
	 * @param buf_len  Tamano del buffer.
	 * @param out_len  Bytes recibidos (o NULL).
	 * @return 0 si ok, -ENOSYS si no implementado.
	 */
	int   (*http_get)(const char *url, uint8_t *buf, size_t buf_len,
			  size_t *out_len);

	/**
	 * @brief Realizar una peticion HTTP POST.
	 * @param url           URL completa.
	 * @param content_type  Tipo de contenido (ej: "application/json").
	 * @param data          Datos a enviar.
	 * @param data_len      Longitud de los datos.
	 * @param buf           Buffer para la respuesta.
	 * @param buf_len       Tamano del buffer.
	 * @param out_len       Bytes recibidos (o NULL).
	 * @return 0 si ok, -ENOSYS si no implementado.
	 */
	int   (*http_post)(const char *url, const char *content_type,
			   const uint8_t *data, size_t data_len,
			   uint8_t *buf, size_t buf_len, size_t *out_len);

	/* ---- Memoria Avanzada (Fase 16) ---- */

	/**
	 * @brief Asignar memoria via buddy allocator (M4).
	 * @param size  Tamano en bytes a asignar.
	 * @return Handle del bloque asignado (1-64), o 0 si falla.
	 */
	uint32_t (*mem_alloc_buddy)(uint32_t size);

	/**
	 * @brief Liberar un bloque del buddy allocator.
	 * @param handle  Handle previamente asignado.
	 */
	void     (*mem_free_buddy)(uint32_t handle);

	/**
	 * @brief Obtener el porcentaje de uso de la memoria M4.
	 * @return Porcentaje de uso (0-100).
	 */
	uint8_t  (*mem_get_usage)(void);

	/* ---- zRAM LRU (Fase 16) ---- */

	/**
	 * @brief Comprimir y almacenar un bloque en zRAM.
	 * @param handle  Handle del bloque original.
	 * @param data    Datos fuente.
	 * @param len     Longitud de datos.
	 * @return Tamano comprimido, o 0 si falla.
	 */
	size_t (*zram_store)(uint32_t handle, const uint8_t *data, size_t len);

	/**
	 * @brief Recuperar y descomprimir un bloque de zRAM.
	 * @param handle  Handle del bloque.
	 * @param buf     Buffer de salida.
	 * @param buf_len Tamano del buffer.
	 * @return Tamano descomprimido, o 0 si no existe.
	 */
	size_t (*zram_load)(uint32_t handle, uint8_t *buf, size_t buf_len);

	/**
	 * @brief Obtener estadisticas del zRAM.
	 * @param used     Puntero para bloques en uso (o NULL).
	 * @param total    Puntero para bloques totales (o NULL).
	 * @param counter  Puntero para contador LRU global (o NULL).
	 */
	void   (*zram_get_stats)(uint8_t *used, uint8_t *total,
				 uint32_t *counter);

	/* ---- Sistema (Fase 20) ---- */

	/**
	 * @brief Obtener el porcentaje de bateria.
	 * @return Porcentaje (0-100), o 0 si no hay bateria detectada.
	 */
	uint8_t (*get_battery_pct)(void);

	/**
	 * @brief Obtener el timeout de suspension actual.
	 * @return Timeout en segundos.
	 */
	uint32_t (*get_screen_timeout)(void);

	/**
	 * @brief Configurar el timeout de suspension.
	 * @param seconds  Timeout en segundos (5-60).
	 */
	void     (*set_screen_timeout)(uint32_t seconds);

} giga_api_t;

/** Direccion de la tabla de API en memoria (symbol del linker). */
#define GIGA_API_TABLE_ADDR  0xC0001000

/** Acceder a la tabla de API desde codigo de app. */
#define GIGA_API  ((giga_api_t *)GIGA_API_TABLE_ADDR)

/* ---- Macros de conveniencia ---- */

#define giga_malloc(size)          GIGA_API->malloc(size)
#define giga_free(ptr)             GIGA_API->free(ptr)
#define giga_fill_rect(x,y,w,h,c) GIGA_API->fill_rect(x,y,w,h,c)
#define giga_draw_rect(x,y,w,h,c) GIGA_API->draw_rect(x,y,w,h,c)
#define giga_draw_text(x,y,t,c)   GIGA_API->draw_text(x,y,t,c)
#define giga_update()              GIGA_API->update()
#define giga_get_touch(x,y)       GIGA_API->get_touch(x,y)
#define giga_btn_create(p)        GIGA_API->btn_create(p)
#define giga_btn_set_text(b,t)    GIGA_API->btn_set_text(b,t)
#define giga_btn_set_pos(b,x,y)   GIGA_API->btn_set_pos(b,x,y)
#define giga_btn_set_size(b,w,h)  GIGA_API->btn_set_size(b,w,h)
#define giga_btn_hit(b,x,y)       GIGA_API->btn_hit(b,x,y)
#define giga_btn_get_state(b)     GIGA_API->btn_get_state(b)
#define giga_sleep_ms(ms)         GIGA_API->sleep_ms(ms)
#define giga_get_tick_ms()        GIGA_API->get_tick_ms()
#define giga_log_info(fmt,...)    GIGA_API->log_info(fmt,##__VA_ARGS__)
#define giga_log_error(fmt,...)   GIGA_API->log_error(fmt,##__VA_ARGS__)
#define giga_get_battery_pct()    GIGA_API->get_battery_pct()
#define giga_get_screen_timeout() GIGA_API->get_screen_timeout()
#define giga_set_screen_timeout(s) GIGA_API->set_screen_timeout(s)

/* ---- Colores (RGB565) ---- */
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

/* ---- IDs de fuente ---- */
#define GIGA_FONT_SMALL   0
#define GIGA_FONT_NORMAL  1
#define GIGA_FONT_LARGE   2

#endif /* GIGASPARK_API_H */

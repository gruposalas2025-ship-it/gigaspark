# Gigaspark OS SDK - Documentacion de la API

## Resumen

El SDK de Gigaspark OS provee una API completa para desarrollar apps que corren en el Arduino Giga R1 (STM32H747). Las apps acceden a las funciones del sistema via una tabla de API en memoria fija (`0xC0001000`).

## Arquitectura

```
┌─────────────────────────────────────┐
│         App de Usuario              │
│   (usa macros giga_*)               │
├─────────────────────────────────────┤
│     Tabla de API (0xC0001000)       │
│   giga_api_t con 30+ funciones      │
├─────────────────────────────────────┤
│     Gigaspark OS (M7 + M4)          │
│   Zephyr RTOS + Drivers             │
├─────────────────────────────────────┤
│     Hardware STM32H747              │
└─────────────────────────────────────┘
```

## Inicio Rapido

```c
#include "gigaspark_api.h"

void app_main(void) {
    // Dibujar fondo negro
    giga_fill_rect(0, 0, GIGA_SCREEN_W, GIGA_SCREEN_H, GIGA_BLACK);

    // Escribir texto
    giga_draw_text(20, 20, "Mi App!", GIGA_CYAN);

    // Actualizar pantalla
    giga_update();

    // Loop principal
    while (1) {
        int x, y;
        if (giga_get_touch(&x, &y)) {
            giga_log_info("Touch en %d, %d", x, y);
        }
        giga_sleep_ms(50);
    }
}
```

## Categorias de la API

### Pantalla
- `giga_fill_rect()` - Rectangulo relleno
- `giga_draw_rect()` - Borde de rectangulo
- `giga_draw_text()` - Texto en pantalla
- `giga_update()` - Actualizar display

### Touch
- `giga_get_touch()` - Leer estado del touchscreen

### Memoria
- `giga_malloc()` / `giga_free()` - Memoria dinamica
- `giga_mem_alloc_buddy()` - Buddy allocator (M4)
- `giga_mem_free_buddy()` - Liberar bloque buddy

### zRAM
- `giga_zram_store()` - Comprimir y almacenar
- `giga_zram_load()` - Descomprimir y recuperar
- `giga_zram_get_stats()` - Estadisticas

### Red
- `giga_http_get()` - Peticion HTTP GET
- `giga_http_post()` - Peticion HTTP POST

### Sistema
- `giga_get_battery_pct()` - Nivel de bateria
- `giga_get_screen_timeout()` - Timeout actual
- `giga_set_screen_timeout()` - Configurar timeout
- `giga_sleep_ms()` - Dormir hilo
- `giga_get_tick_ms()` - Tiempo actual

### IPC
- `giga_ipc_send()` - Enviar a M4
- `giga_ipc_recv()` - Recibir de M4

### Logging
- `giga_log_info()` - Log informativo
- `giga_log_error()` - Log de error

## Colores (RGB565)

```c
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
```

## Ejemplos

Ver `test_apps/` para apps de ejemplo:
- `clicker/main.c` - Demo de touch
- `oscilloscope_app/main.c` - Osciloscopio
- `power_supply_app/main.c` - Fuente de laboratorio
- `settings_app/main.c` - Ajustes del sistema
- `flipper_app/main.c` - Herramientas RF

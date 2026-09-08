/*
 * Gigaspark OS - Motor de Memoria M4 (Reescritura Fase 16)
 *
 * Reemplaza el asignador first-fit con un Buddy Allocator O(log N).
 * Implementa LRU eviction para zRAM compression.
 * Pool total: 4KB (0x20000000) dividido en bloques binarios.
 *
 * Organizacion del pool:
 *   Nivel 0: 1 bloque de 4096 bytes (order 0 = 2^12)
 *   Nivel 1: 2 bloques de 2048 bytes (order 1 = 2^11)
 *   Nivel 2: 4 bloques de 1024 bytes (order 2 = 2^10)
 *   Nivel 3: 8 bloques de 512 bytes  (order 3 = 2^9)
 *   Nivel 4: 16 bloques de 256 bytes (order 4 = 2^8)
 *   Nivel 5: 32 bloques de 128 bytes (order 5 = 2^7)
 *   Nivel 6: 64 bloques de 64 bytes  (order 6 = 2^6)
 *   Nivel 7: 128 bloques de 32 bytes (order 7 = 2^5)
 *
 * Referencia: Knuth, "The Art of Computer Programming" Vol 1
 */

#include "mem_engine.h"

#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mem_engine, CONFIG_LOG_DEFAULT_LEVEL);

/* ============================================================
 * BUDDY ALLOCATOR
 * ============================================================ */

#define BUDDY_POOL_SIZE    4096    /* 4KB pool total */
#define BUDDY_MIN_ORDER    5      /* 2^5 = 32 bytes minimo */
#define BUDDY_MAX_ORDER    12     /* 2^12 = 4096 bytes maximo */
#define BUDDY_NUM_ORDERS   (BUDDY_MAX_ORDER - BUDDY_MIN_ORDER + 1)

/* Pool de memoria (4KB alineado a 32 bytes) */
static uint8_t __attribute__((aligned(32))) buddy_pool[BUDDY_POOL_SIZE];

/* Bitmap de ocupacion: 1 bit por bloque de 32 bytes */
/* 4096 / 32 = 128 bloques = 16 bytes de bitmap */
static uint8_t buddy_bitmap[16];

/* Arbol de buddies: para cada nivel, un array de "esta libre?" */
#define BUDDY_BLOCKS(order) (1 << (BUDDY_MAX_ORDER - order))

static uint8_t buddy_free_lists[BUDDY_NUM_ORDERS][BUDDY_BLOCKS(BUDDY_MIN_ORDER)];

/* Contador de bloques libres por nivel */
static uint16_t free_count[BUDDY_NUM_ORDERS];

/* Tabla de handles: indice = handle - 1 */
#define MAX_HANDLES  64

struct buddy_handle {
	uint32_t address;     /* Offset dentro del pool */
	uint32_t size;        /* Tamano en bytes */
	uint8_t  order;       /* Orden del buddy (2^order) */
	uint8_t  in_use;      /* 1 si esta asignado */
};

static struct buddy_handle handle_table[MAX_HANDLES];
static uint8_t handle_bitmap[MAX_HANDLES / 8];

/* ============================================================
 * FUNCIONES INTERNAS DEL BUDDY ALLOCATOR
 * ============================================================ */

/*
 * Inicializa el buddy allocator.
 * Todo el pool comienza libre en el nivel mas alto (4096 bytes).
 */
static void buddy_init_internal(void)
{
	memset(buddy_bitmap, 0, sizeof(buddy_bitmap));
	memset(free_count, 0, sizeof(free_count));
	memset(handle_table, 0, sizeof(handle_table));
	memset(handle_bitmap, 0, sizeof(handle_bitmap));

	/* Marcar todo el pool como libre */
	for (int i = 0; i < BUDDY_BLOCKS(BUDDY_MIN_ORDER); i++) {
		buddy_bitmap[i / 8] &= ~(1 << (i % 8));
	}

	/* Un solo bloque libre de orden maximo */
	buddy_free_lists[BUDDY_NUM_ORDERS - 1][0] = 1;
	free_count[BUDDY_NUM_ORDERS - 1] = 1;

	/* Marcar todo el bitmap como ocupado excepto el bloque libre */
	memset(buddy_bitmap, 0xFF, sizeof(buddy_bitmap));
	/* Primer bloque libre (4096 bytes = 128 bloques de 32) */
	for (int i = 0; i < 128; i++) {
		buddy_bitmap[i / 8] &= ~(1 << (i % 8));
	}
}

/*
 * Marca un rango de bloques de 32 bytes como ocupado/libre en el bitmap.
 */
static void buddy_mark_range(uint32_t offset, uint32_t size, int used)
{
	int start = offset / 32;
	int count = size / 32;

	for (int i = start; i < start + count; i++) {
		if (used) {
			buddy_bitmap[i / 8] |= (1 << (i % 8));
		} else {
			buddy_bitmap[i / 8] &= ~(1 << (i % 8));
		}
	}
}

/*
 * Divide recursivamente un bloque hasta alcanzar el orden deseado.
 * Retorna la direccion del bloque asignado, o -1 si no hay espacio.
 */
static int buddy_split(int current_order, int target_order, int index)
{
	if (current_order == target_order) {
		/* Encontramos el bloque del tamano correcto */
		buddy_free_lists[current_order][index] = 0;
		free_count[current_order]--;
		return index;
	}

	/* Si no hay bloques libres en este nivel, dividir de arriba */
	if (free_count[current_order] == 0) {
		return -1;
	}

	/* Buscar un bloque libre en este nivel */
	int found = -1;
	for (int i = 0; i < BUDDY_BLOCKS(current_order); i++) {
		if (buddy_free_lists[current_order][i]) {
			found = i;
			break;
		}
	}

	if (found < 0) {
		return -1;
	}

	/* Marcar como ocupado en este nivel */
	buddy_free_lists[current_order][found] = 0;
	free_count[current_order]--;

	/* Dividir en dos hijos */
	int child_index = found * 2;
	int next_order = current_order - 1;

	/* El hijo izquierdo queda libre */
	buddy_free_lists[next_order][child_index] = 1;
	free_count[next_order]++;

	/* Recursivamente dividir el hijo izquierdo */
	return buddy_split(next_order, target_order, child_index);
}

/*
 * Combina dos buddies libres en un bloque mas grande.
 * (Coalescing)
 */
static void buddy_coalesce(int order, int index)
{
	if (order >= BUDDY_NUM_ORDERS - 1) {
		return;  /* Ya estamos en el nivel maximo */
	}

	int buddy_index = (index % 2 == 0) ? index + 1 : index - 1;

	/* Verificar si el buddy esta libre */
	if (buddy_index >= BUDDY_BLOCKS(order)) {
		return;
	}

	if (buddy_free_lists[order][buddy_index] == 0) {
		return;  /* Buddy no esta libre */
	}

	/* Ambos buddies estan libres, combinar */
	buddy_free_lists[order][index] = 0;
	buddy_free_lists[order][buddy_index] = 0;
	free_count[order] -= 2;

	/* Crear bloque padre */
	int parent_index = index / 2;
	buddy_free_lists[order + 1][parent_index] = 1;
	free_count[order + 1]++;

	/* Intentar combinar mas arriba */
	buddy_coalesce(order + 1, parent_index);
}

/*
 * Busca un handle libre en la tabla.
 */
static int find_free_handle(void)
{
	for (int i = 0; i < MAX_HANDLES; i++) {
		if (handle_table[i].in_use == 0) {
			return i;
		}
	}
	return -1;
}

/* ============================================================
 * FUNCIONES PUBLICAS (API del motor de memoria)
 * ============================================================ */

void mem_engine_init(void)
{
	buddy_init_internal();
	LOG_INF("Buddy Allocator inicializado: 4KB pool, 7 niveles (32-4096 bytes)");
}

/*
 * Asigna memoria usando el buddy allocator.
 * Retorna un handle (1-64) o 0 si falla.
 */
uint32_t mem_alloc(uint32_t size)
{
	if (size == 0 || size > BUDDY_POOL_SIZE) {
		return 0;
	}

	/* Encontrar el orden minimo que contiene el tamano solicitado */
	int target_order = BUDDY_MIN_ORDER;
	while (((uint32_t)1 << target_order) < size) {
		target_order++;
	}

	/* Asignar handle */
	int handle_idx = find_free_handle();
	if (handle_idx < 0) {
		return 0;  /* Sin handles disponibles */
	}

	/* Buscar espacio libre en el buddy allocator */
	int block_index = -1;

	/* Buscar en el nivel exacto primero */
	if (free_count[target_order - BUDDY_MIN_ORDER] > 0) {
		for (int i = 0; i < BUDDY_BLOCKS(target_order); i++) {
			if (buddy_free_lists[target_order - BUDDY_MIN_ORDER][i]) {
				block_index = i;
				buddy_free_lists[target_order - BUDDY_MIN_ORDER][i] = 0;
				free_count[target_order - BUDDY_MIN_ORDER]--;
				break;
			}
		}
	}

	/* Si no hay en el nivel exacto, dividir de arriba */
	if (block_index < 0) {
		block_index = buddy_split(BUDDY_NUM_ORDERS - 1,
					  target_order - BUDDY_MIN_ORDER,
					  0);
	}

	if (block_index < 0) {
		return 0;  /* Sin memoria */
	}

	/* Calcular direccion */
	uint32_t offset = block_index * (1 << target_order);

	/* Marcar en bitmap */
	buddy_mark_range(offset, (1 << target_order), 1);

	/* Llenar handle */
	handle_table[handle_idx].address = offset;
	handle_table[handle_idx].size = (1 << target_order);
	handle_table[handle_idx].order = target_order - BUDDY_MIN_ORDER;
	handle_table[handle_idx].in_use = 1;

	/* Retornar handle (1-indexed) */
	return (uint32_t)(handle_idx + 1);
}

/*
 * Libera memoria y retorna al buddy allocator.
 * Retorna 0 si ok, -1 si error.
 */
int mem_free(uint32_t handle)
{
	if (handle == 0 || handle > MAX_HANDLES) {
		return -1;
	}

	int idx = handle - 1;
	if (handle_table[idx].in_use == 0) {
		return -1;
	}

	/* Marcar como libre */
	uint32_t offset = handle_table[idx].address;
	uint32_t size = handle_table[idx].size;
	int order = handle_table[idx].order;

	handle_table[idx].in_use = 0;

	/* Limpiar bitmap */
	buddy_mark_range(offset, size, 0);

	/* Restaurar en el buddy allocator */
	int block_index = offset / (1 << (order + BUDDY_MIN_ORDER));
	buddy_free_lists[order][block_index] = 1;
	free_count[order]++;

	/* Coalescing con buddy */
	buddy_coalesce(order, block_index);

	return 0;
}

/*
 * Obtiene informacion de un handle.
 */
int mem_get_info(uint32_t handle, uint32_t *addr, uint32_t *size)
{
	if (handle == 0 || handle > MAX_HANDLES) {
		return -1;
	}

	int idx = handle - 1;
	if (handle_table[idx].in_use == 0) {
		return -1;
	}

	if (addr) {
		*addr = (uint32_t)&buddy_pool[handle_table[idx].address];
	}
	if (size) {
		*size = handle_table[idx].size;
	}

	return 0;
}

/*
 * Retorna el porcentaje de memoria usada.
 */
uint8_t mem_get_usage(void)
{
	uint32_t used = 0;

	for (int i = 0; i < MAX_HANDLES; i++) {
		if (handle_table[i].in_use) {
			used += handle_table[i].size;
		}
	}

	return (uint8_t)((used * 100) / BUDDY_POOL_SIZE);
}

/*
 * Lee datos de un bloque a un buffer.
 * Retorna numero de bytes leidos, o 0 si error.
 */
size_t mem_read(uint32_t handle, void *buf, size_t buf_size)
{
	if (handle == 0 || handle > MAX_HANDLES || buf == NULL) {
		return 0;
	}

	int idx = handle - 1;
	if (handle_table[idx].in_use == 0) {
		return 0;
	}

	uint32_t block_addr = (uint32_t)&buddy_pool[handle_table[idx].address];
	uint32_t block_size = handle_table[idx].size;

	/* Copiar datos (tamano minimo entre buffer y bloque) */
	size_t copy_len = (buf_size < block_size) ? buf_size : block_size;
	memcpy(buf, (void *)block_addr, copy_len);

	return copy_len;
}

/*
 * Escribe datos en un bloque existente.
 * Retorna 0 si ok, -1 si error.
 */
int mem_write(uint32_t handle, const void *data, size_t data_len)
{
	if (handle == 0 || handle > MAX_HANDLES || data == NULL) {
		return -1;
	}

	int idx = handle - 1;
	if (handle_table[idx].in_use == 0) {
		return -1;
	}

	uint32_t block_addr = (uint32_t)&buddy_pool[handle_table[idx].address];
	uint32_t block_size = handle_table[idx].size;

	/* No escribir mas que el tamano del bloque */
	size_t copy_len = (data_len < block_size) ? data_len : block_size;
	memcpy((void *)block_addr, data, copy_len);

	return 0;
}

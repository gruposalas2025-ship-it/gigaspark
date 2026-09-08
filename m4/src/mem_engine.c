/*
 * Gigaspark OS - Motor de Memoria M4 (Fase 16: Buddy Allocator)
 *
 * Asignador buddy O(log N) con coalescing automatico.
 * Pool total: 4KB dividido en bloques binarios (32 a 4096 bytes).
 */

#include "mem_engine.h"

#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mem_engine, CONFIG_LOG_DEFAULT_LEVEL);

/* ============================================================
 * BUDDY ALLOCATOR
 * ============================================================ */

#define POOL_SIZE    4096
#define MIN_ORDER    5      /* 2^5 = 32 bytes (bloque minimo) */
#define MAX_ORDER    12     /* 2^12 = 4096 bytes (pool completo) */
#define NUM_ORDERS   (MAX_ORDER - MIN_ORDER + 1)  /* 8 niveles */

#define BLOCK_SIZE(order)   (1U << (order))
#define BLOCKS_AT(order)    (POOL_SIZE >> (order))
#define IDX(order)          ((order) - MIN_ORDER)

/* Pool de memoria */
static uint8_t __attribute__((aligned(32))) pool[POOL_SIZE];

/* free_lists[idx][index]: 1 = libre, 0 = ocupado
 * idx = order - MIN_ORDER
 * Indices por nivel:
 *   idx 0 (order 5,  32B):  128 bloques, indices 0..127
 *   idx 1 (order 6,  64B):   64 bloques, indices 0..63
 *   idx 2 (order 7, 128B):   32 bloques, indices 0..31
 *   idx 3 (order 8, 256B):   16 bloques, indices 0..15
 *   idx 4 (order 9, 512B):    8 bloques, indices 0..7
 *   idx 5 (order 10, 1KB):    4 bloques, indices 0..3
 *   idx 6 (order 11, 2KB):    2 bloques, indices 0..1
 *   idx 7 (order 12, 4KB):    1 bloque,  indice 0
 */
static uint8_t free_lists[NUM_ORDERS][128];

/* Tabla de handles */
#define MAX_HANDLES  64

struct handle_entry {
	uint32_t offset;
	uint32_t size;
	uint8_t  order;
	uint8_t  in_use;
};

static struct handle_entry handles[MAX_HANDLES];

/* ============================================================
 * FUNCIONES INTERNAS
 * ============================================================ */

static void buddy_init(void)
{
	memset(handles, 0, sizeof(handles));
	memset(free_lists, 0, sizeof(free_lists));

	/* Un solo bloque libre de orden maximo (4096B) */
	free_lists[IDX(MAX_ORDER)][0] = 1;
}

/*
 * Busca el bloque libre mas pequeno >= min_order.
 * Retorna 0 si encuentra, -1 si no hay.
 * En *order e *index devuelve la posicion en free_lists (valores reales).
 */
static int find_free(int min_order, int *out_order, int *out_index)
{
	for (int o = min_order; o <= MAX_ORDER; o++) {
		int idx = IDX(o);
		int max_idx = BLOCKS_AT(o);
		for (int i = 0; i < max_idx; i++) {
			if (free_lists[idx][i]) {
				*out_order = o;
				*out_index = i;
				return 0;
			}
		}
	}
	return -1;
}

/*
 * Divide recursivamente un bloque desde from_order hasta to_order.
 * Deja el bloque resultante marcado como ocupado.
 * Los bloques hermanos en cada nivel intermedio quedan libres.
 */
static int split_block(int from_order, int to_order, int index)
{
	for (int o = from_order; o > to_order; o--) {
		int idx = IDX(o);
		free_lists[idx][index] = 0;

		/* Crear dos hijos en el nivel inferior */
		int child_idx = IDX(o - 1);
		free_lists[child_idx][index * 2] = 1;
		free_lists[child_idx][index * 2 + 1] = 1;

		/* Bajar por el hijo izquierdo */
		index = index * 2;
	}

	/* Marcar el bloque final como ocupado */
	free_lists[IDX(to_order)][index] = 0;
	return index;
}

/*
 * Combina bloques hermanos libres向上 (coalescing).
 */
static void merge_block(int order, int index)
{
	while (order < MAX_ORDER) {
		int idx = IDX(order);
		int buddy = (index & 1) ? index - 1 : index + 1;
		int max_idx = BLOCKS_AT(order);

		if (buddy >= max_idx) break;
		if (!free_lists[idx][buddy]) break;

		/* Ambos libres: combinar */
		free_lists[idx][index] = 0;
		free_lists[idx][buddy] = 0;

		order++;
		index >>= 1;
		free_lists[IDX(order)][index] = 1;
	}
}

static int alloc_handle(void)
{
	for (int i = 0; i < MAX_HANDLES; i++) {
		if (!handles[i].in_use) return i;
	}
	return -1;
}

/* ============================================================
 * API PUBLICA
 * ============================================================ */

void mem_engine_init(void)
{
	buddy_init();
	LOG_INF("Buddy Allocator: 4KB pool, %d niveles (%u-%u bytes)",
		NUM_ORDERS, BLOCK_SIZE(MIN_ORDER), BLOCK_SIZE(MAX_ORDER));
}

uint32_t mem_alloc(uint32_t size)
{
	if (size == 0 || size > POOL_SIZE) return 0;

	/* Calcular orden minimo necesario */
	int target = MIN_ORDER;
	while (BLOCK_SIZE(target) < size) target++;
	if (target > MAX_ORDER) return 0;

	/* Buscar bloque libre */
	int fo, fi;
	if (find_free(target, &fo, &fi) < 0) return 0;

	/* Dividir hasta el orden deseado */
	int bi = split_block(fo, target, fi);
	uint32_t off = bi * BLOCK_SIZE(target);

	/* Asignar handle */
	int h = alloc_handle();
	if (h < 0) return 0;

	handles[h].offset = off;
	handles[h].size = BLOCK_SIZE(target);
	handles[h].order = target;
	handles[h].in_use = 1;

	return (uint32_t)(h + 1);
}

int mem_free(uint32_t handle)
{
	if (handle == 0 || handle > MAX_HANDLES) return -1;
	int idx = handle - 1;
	if (!handles[idx].in_use) return -1;

	int order = handles[idx].order;
	int bi = handles[idx].offset / handles[idx].size;
	handles[idx].in_use = 0;

	free_lists[IDX(order)][bi] = 1;
	merge_block(order, bi);
	return 0;
}

int mem_get_info(uint32_t handle, uint32_t *addr, uint32_t *size)
{
	if (handle == 0 || handle > MAX_HANDLES) return -1;
	int idx = handle - 1;
	if (!handles[idx].in_use) return -1;
	if (addr) *addr = (uint32_t)&pool[handles[idx].offset];
	if (size) *size = handles[idx].size;
	return 0;
}

uint8_t mem_get_usage(void)
{
	uint32_t used = 0;
	for (int i = 0; i < MAX_HANDLES; i++) {
		if (handles[i].in_use) used += handles[i].size;
	}
	return (uint8_t)((used * 100) / POOL_SIZE);
}

size_t mem_read(uint32_t handle, void *buf, size_t buf_size)
{
	if (handle == 0 || handle > MAX_HANDLES || !buf) return 0;
	int idx = handle - 1;
	if (!handles[idx].in_use) return 0;
	size_t len = (buf_size < handles[idx].size) ? buf_size : handles[idx].size;
	memcpy(buf, &pool[handles[idx].offset], len);
	return len;
}

int mem_write(uint32_t handle, const void *data, size_t data_len)
{
	if (handle == 0 || handle > MAX_HANDLES || !data) return -1;
	int idx = handle - 1;
	if (!handles[idx].in_use) return -1;
	size_t len = (data_len < handles[idx].size) ? data_len : handles[idx].size;
	memcpy(&pool[handles[idx].offset], data, len);
	return 0;
}

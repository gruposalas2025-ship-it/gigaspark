/*
 * Gigaspark OS - zRAM Compression (Fase 16: LRU Eviction)
 *
 * Compresion RLE con politica de eviccion LRU (Least Recently Used).
 * Cada bloque comprimido lleva un timestamp de uso.
 * Cuando el zRAM se llena, se expulsa el bloque menos reciente.
 *
 * Formato RLE: pares [count][byte].
 *   - count: 1-255 (uint8_t)
 *   - byte:  el byte de datos
 *   - Un run de 300 bytes identicos = [255][byte] + [45][byte]
 *   - Un byte sin repeticion = [1][byte]
 *
 * Peor caso: datos sin runs -> expansion 2x.
 * Mejor caso: datos altamente repetitivos -> compresion ~1:255.
 */

#include "zram_comp.h"

#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zram_comp, CONFIG_LOG_DEFAULT_LEVEL);

/* ============================================================
 * TABLA LRU - Timestamps de uso por bloque
 * ============================================================ */

#define ZRAM_MAX_BLOCKS    32     /* Maximo 32 bloques comprimidos */
#define ZRAM_BLOCK_SIZE    512    /* Tamano maximo de cada bloque comprimido */
#define ZRAM_TOTAL_SIZE    (ZRAM_MAX_BLOCKS * ZRAM_BLOCK_SIZE)  /* 16KB total */

/* Entrada en la tabla LRU */
struct zram_entry {
	uint32_t handle;        /* Handle del bloque original */
	uint32_t size_orig;     /* Tamano original */
	uint32_t size_comp;     /* Tamano comprimido */
	uint32_t lru_counter;   /* Contador de uso (mayor = mas reciente) */
	uint8_t  in_use;        /* 1 si la entrada esta ocupada */
	uint8_t  data[ZRAM_BLOCK_SIZE];  /* Datos comprimidos */
};

/* Tabla de entradas zRAM */
static struct zram_entry zram_table[ZRAM_MAX_BLOCKS];

/* Contador global LRU (se incrementa en cada acceso) */
static uint32_t lru_global_counter = 0;

/* Contador de bloques usados */
static uint8_t zram_used_count = 0;

/* ============================================================
 * FUNCIONES LRU
 * ============================================================ */

/*
 * Busca un bloque por su handle.
 * Retorna el indice en zram_table o -1 si no existe.
 */
static int lru_find(uint32_t handle)
{
	for (int i = 0; i < ZRAM_MAX_BLOCKS; i++) {
		if (zram_table[i].in_use && zram_table[i].handle == handle) {
			return i;
		}
	}
	return -1;
}

/*
 * Marca un bloque como "recien usado" (actualiza LRU counter).
 */
static void lru_touch(int index)
{
	zram_table[index].lru_counter = ++lru_global_counter;
}

/*
 * Encuentra la victima LRU (el bloque menos recientemente usado).
 * Retorna el indice de la victima, o -1 si no hay bloques.
 */
static int lru_find_victim(void)
{
	int victim = -1;
	uint32_t min_counter = UINT32_MAX;

	for (int i = 0; i < ZRAM_MAX_BLOCKS; i++) {
		if (zram_table[i].in_use) {
			if (zram_table[i].lru_counter < min_counter) {
				min_counter = zram_table[i].lru_counter;
				victim = i;
			}
		}
	}

	return victim;
}

/*
 * Expulsa (evict) el bloque menos recientemente usado.
 * Retorna el handle del bloque expulsado, o 0 si no hubo eviccion.
 */
static uint32_t lru_evict(void)
{
	int victim = lru_find_victim();

	if (victim < 0) {
		return 0;  /* No hay bloques para expulsar */
	}

	uint32_t evicted_handle = zram_table[victim].handle;

	LOG_WRN("LRU evict: handle=%u, size=%u, lru=%u",
		evicted_handle,
		zram_table[victim].size_comp,
		zram_table[victim].lru_counter);

	/* Liberar la entrada */
	zram_table[victim].in_use = 0;
	zram_used_count--;

	/* Notificar al motor de memoria que el bloque fue evictado */
	/* El caller debe usar mem_free() en el handle expulsado */
	return evicted_handle;
}

/*
 * Inserta un bloque comprimido en la tabla LRU.
 * Si la tabla esta llena, expulsa la victima primero.
 * Retorna 0 si ok, -1 si falla.
 */
static int lru_insert(uint32_t handle, const uint8_t *data,
		      uint32_t size_orig, uint32_t size_comp)
{
	/* Si ya existe, solo actualizar */
	int existing = lru_find(handle);
	if (existing >= 0) {
		memcpy(zram_table[existing].data, data, size_comp);
		zram_table[existing].size_orig = size_orig;
		zram_table[existing].size_comp = size_comp;
		lru_touch(existing);
		return 0;
	}

	/* Si esta lleno, expulsar victima */
	if (zram_used_count >= ZRAM_MAX_BLOCKS) {
		uint32_t evicted = lru_evict();
		if (evicted == 0) {
			return -1;  /* No se pudo expulsar */
		}
	}

	/* Buscar entrada libre */
	for (int i = 0; i < ZRAM_MAX_BLOCKS; i++) {
		if (!zram_table[i].in_use) {
			zram_table[i].handle = handle;
			zram_table[i].size_orig = size_orig;
			zram_table[i].size_comp = size_comp;
			memcpy(zram_table[i].data, data, size_comp);
			lru_touch(i);
			zram_used_count++;
			return 0;
		}
	}

	return -1;  /* No deberia llegar aqui */
}

/* ============================================================
 * FUNCIONES DE COMPRESION/DECOMPRESION RLE
 * ============================================================ */

size_t zram_compress(const uint8_t *src, size_t src_len,
		     uint8_t *dst, size_t dst_len)
{
	size_t si = 0;  /* source index */
	size_t di = 0;  /* destination index */

	while (si < src_len) {
		uint8_t current = src[si];
		uint8_t count = 1;

		/* Contar bytes identicos consecutivos */
		while (si + count < src_len &&
		       src[si + count] == current &&
		       count < 255) {
			count++;
		}

		/* Necesitamos 2 bytes para este run */
		if (di + 2 > dst_len) {
			return 0;  /* buffer overflow */
		}

		dst[di++] = count;
		dst[di++] = current;

		si += count;
	}

	return di;
}

size_t zram_decompress(const uint8_t *src, size_t src_len,
		       uint8_t *dst, size_t dst_len)
{
	size_t si = 0;  /* source index */
	size_t di = 0;  /* destination index */

	while (si + 1 < src_len) {
		uint8_t count = src[si];
		uint8_t byte  = src[si + 1];

		si += 2;

		/* Verificar espacio en buffer de salida */
		if (di + count > dst_len) {
			return 0;  /* buffer overflow */
		}

		/* Escribir 'count' copias de 'byte' */
		for (uint8_t i = 0; i < count; i++) {
			dst[di++] = byte;
		}
	}

	return di;
}

/* ============================================================
 * FUNCIONES PUBLICAS (API LRU + COMPRESION)
 * ============================================================ */

/*
 * Comprime un bloque y lo almacena en la tabla LRU.
 * Retorna el tamano comprimido, o 0 si falla.
 */
size_t zram_compress_and_store(uint32_t handle,
			       const uint8_t *src, size_t src_len)
{
	if (src_len > ZRAM_BLOCK_SIZE * 2) {
		return 0;  /* Bloque demasiado grande para RLE worst-case */
	}

	/* Buffer temporal para compresion */
	uint8_t compressed[ZRAM_BLOCK_SIZE];

	size_t comp_len = zram_compress(src, src_len, compressed, sizeof(compressed));

	if (comp_len == 0) {
		return 0;  /* Fallo la compresion o buffer overflow */
	}

	/* Almacenar en la tabla LRU */
	if (lru_insert(handle, compressed, src_len, comp_len) < 0) {
		return 0;  /* Fallo la insercion */
	}

	LOG_INF("zRAM store: handle=%u orig=%u comp=%u (%u%%)",
		handle, src_len, comp_len,
		(uint32_t)((comp_len * 100) / src_len));

	return comp_len;
}

/*
 * Recupera un bloque comprimido de la tabla LRU.
 * Actualiza el timestamp LRU.
 * Retorna el tamano descomprimido, o 0 si no existe.
 */
size_t zram_retrieve(uint32_t handle, uint8_t *dst, size_t dst_len)
{
	int idx = lru_find(handle);

	if (idx < 0) {
		return 0;  /* Bloque no encontrado en zRAM */
	}

	/* Actualizar LRU (fue recien usado) */
	lru_touch(idx);

	/* Descomprimir */
	size_t decomp_len = zram_decompress(zram_table[idx].data,
					    zram_table[idx].size_comp,
					    dst, dst_len);

	LOG_INF("zRAM retrieve: handle=%u comp=%u decomp=%u",
		handle, zram_table[idx].size_comp, decomp_len);

	return decomp_len;
}

/*
 * Expulsa explicitamente un bloque del zRAM.
 * Retorna el handle expulsado, o 0 si no se pudo.
 */
uint32_t zram_evict_block(uint32_t handle)
{
	int idx = lru_find(handle);

	if (idx < 0) {
		return 0;  /* No existe */
	}

	uint32_t evicted = zram_table[idx].handle;
	zram_table[idx].in_use = 0;
	zram_used_count--;

	return evicted;
}

/*
 * Retorna estadisticas del zRAM.
 */
void zram_get_stats(uint8_t *used, uint8_t *total, uint32_t *counter)
{
	if (used) {
		*used = zram_used_count;
	}
	if (total) {
		*total = ZRAM_MAX_BLOCKS;
	}
	if (counter) {
		*counter = lru_global_counter;
	}
}

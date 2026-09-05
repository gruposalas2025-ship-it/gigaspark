/*
 * Gigaspark OS - Memory Engine Implementation
 * First-fit block allocator with zRAM compression
 *
 * Manages a static 4KB main pool. When full, compresses inactive
 * blocks into a 2KB compressed_pool. Returns handles, not pointers.
 *
 * Compression decision: when no free block exists in main_pool,
 * the engine finds the first allocated block that is NOT already
 * compressed, compresses it to compressed_pool, and reclaims its
 * main_pool slot for the new allocation.
 */

#include "mem_engine.h"
#include "zram_comp.h"
#include "ipc_protocol.h"

#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(mem_engine, CONFIG_LOG_DEFAULT_LEVEL);

/* Block alignment for allocations */
#define BLOCK_ALIGN     16
#define MAX_BLOCKS      (MEM_POOL_SIZE / BLOCK_ALIGN)

/* Block states */
#define STATE_FREE      0
#define STATE_LIVE      1   /* allocated, data in main pool */
#define STATE_COMPRESSED 2  /* allocated, data in compressed pool */

struct block_desc {
	uint16_t handle;       /* unique handle for this block */
	uint32_t size;         /* original requested size */
	uint32_t aligned_size; /* aligned block size */
	uint8_t  state;        /* STATE_FREE, STATE_LIVE, STATE_COMPRESSED */
	uint16_t comp_offset;  /* offset in compressed_pool (if compressed) */
	uint16_t comp_size;    /* compressed size (if compressed) */
};

/* Static memory pools */
static uint8_t mem_pool[MEM_POOL_SIZE] __aligned(BLOCK_ALIGN);
static uint8_t compressed_pool[COMPRESSED_POOL_SIZE] __aligned(BLOCK_ALIGN);

/* Block descriptor table */
static struct block_desc blocks[MAX_BLOCKS];
static uint16_t next_handle = 1;

/* Compressed pool bookkeeping */
static uint16_t comp_next_free = 0;  /* next free offset in compressed_pool */

void mem_init(void)
{
	memset(mem_pool, 0, sizeof(mem_pool));
	memset(compressed_pool, 0, sizeof(compressed_pool));
	memset(blocks, 0, sizeof(blocks));
	comp_next_free = 0;

	LOG_INF("Memory engine initialized: main=%d, compressed=%d, blocks=%d",
		MEM_POOL_SIZE, COMPRESSED_POOL_SIZE, MAX_BLOCKS);
}

/*
 * Find a free descriptor slot for a new allocation.
 */
static int find_free_slot(void)
{
	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (blocks[i].state == STATE_FREE) {
			return i;
		}
	}
	return -1;
}

/*
 * Find a live (non-compressed) block to compress.
 * Strategy: pick the first LIVE block (simple first-fit eviction).
 * A more advanced implementation could use LRU or access counters.
 */
static int find_victim_block(void)
{
	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (blocks[i].state == STATE_LIVE) {
			return i;
		}
	}
	return -1;
}

/*
 * Compress a block from main_pool to compressed_pool.
 * Returns 0 on success, -1 on failure.
 */
static int compress_block(int slot)
{
	struct block_desc *blk = &blocks[slot];
	uint8_t *src = &mem_pool[slot * BLOCK_ALIGN];
	uint8_t *dst = &compressed_pool[comp_next_free];
	size_t max_comp = COMPRESSED_POOL_SIZE - comp_next_free;

	size_t comp_size = zram_compress(src, blk->aligned_size, dst, max_comp);
	if (comp_size == 0) {
		LOG_WRN("compress: block 0x%04x too large for compressed pool",
			blk->handle);
		return -1;
	}

	blk->comp_offset = comp_next_free;
	blk->comp_size = comp_size;
	blk->state = STATE_COMPRESSED;

	comp_next_free += comp_size;

	LOG_INF("compress: handle=0x%04x %u->%u bytes (offset=%u)",
		blk->handle, blk->aligned_size, comp_size, blk->comp_offset);

	return 0;
}

uint16_t mem_alloc(uint32_t size)
{
	if (size == 0 || size > MEM_MAX_ALLOC) {
		LOG_WRN("alloc: invalid size %u", size);
		return MEM_HANDLE_INVALID;
	}

	uint32_t aligned_size = (size + BLOCK_ALIGN - 1) & ~(BLOCK_ALIGN - 1);

	/* Try to find a free slot */
	int slot = find_free_slot();

	/* If no free slot, compress a victim block */
	if (slot < 0) {
		int victim = find_victim_block();
		if (victim < 0) {
			LOG_WRN("alloc: no compressible blocks for %u bytes", size);
			return MEM_HANDLE_INVALID;
		}
		if (compress_block(victim) < 0) {
			LOG_WRN("alloc: compression failed for victim 0x%04x",
				blocks[victim].handle);
			return MEM_HANDLE_INVALID;
		}
		/* The victim slot is now freed for reuse */
		slot = victim;
	}

	/* Initialize the block */
	uint16_t handle = next_handle++;

	blocks[slot].handle = handle;
	blocks[slot].size = size;
	blocks[slot].aligned_size = aligned_size;
	blocks[slot].state = STATE_LIVE;
	blocks[slot].comp_offset = 0;
	blocks[slot].comp_size = 0;

	LOG_INF("alloc: handle=0x%04x size=%u (aligned=%u) slot=%d",
		handle, size, aligned_size, slot);

	return handle;
}

int mem_free(uint16_t handle)
{
	if (handle == MEM_HANDLE_INVALID) {
		return -1;
	}

	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (blocks[i].handle == handle && blocks[i].state != STATE_FREE) {
			LOG_INF("free: handle=0x%04x state=%d comp_size=%u",
				handle, blocks[i].state, blocks[i].comp_size);

			/* If compressed, reclaim compressed_pool space */
			if (blocks[i].state == STATE_COMPRESSED) {
				/*
				 * Simple reclaim: shift compressed_pool data left.
				 * This is O(n) but acceptable for small pool sizes.
				 */
				uint16_t removed_offset = blocks[i].comp_offset;
				uint16_t removed_size = blocks[i].comp_size;

				if (removed_offset + removed_size < comp_next_free) {
					memmove(&compressed_pool[removed_offset],
						&compressed_pool[removed_offset + removed_size],
						comp_next_free - removed_offset - removed_size);

					/* Update offsets of all compressed blocks after this one */
					for (int j = 0; j < MAX_BLOCKS; j++) {
						if (blocks[j].state == STATE_COMPRESSED &&
						    blocks[j].comp_offset > removed_offset) {
							blocks[j].comp_offset -= removed_size;
						}
					}
				}
				comp_next_free -= removed_size;
			}

			/* Mark slot as free */
			blocks[i].handle = 0;
			blocks[i].size = 0;
			blocks[i].aligned_size = 0;
			blocks[i].state = STATE_FREE;
			blocks[i].comp_offset = 0;
			blocks[i].comp_size = 0;

			return 0;
		}
	}

	LOG_WRN("free: bad handle 0x%04x", handle);
	return -2;
}

size_t mem_read(uint16_t handle, void *buf, size_t buf_size)
{
	if (handle == MEM_HANDLE_INVALID || buf == NULL || buf_size == 0) {
		return 0;
	}

	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (blocks[i].handle == handle && blocks[i].state != STATE_FREE) {
			size_t copy_size = blocks[i].size;
			if (copy_size > buf_size) {
				copy_size = buf_size;
			}

			if (blocks[i].state == STATE_COMPRESSED) {
				/* Decompress from compressed_pool */
				size_t dec_size = zram_decompress(
					&compressed_pool[blocks[i].comp_offset],
					blocks[i].comp_size,
					(uint8_t *)buf,
					copy_size);

				LOG_INF("read: handle=0x%04x DECOMPRESSED %u->%u bytes",
					handle, blocks[i].comp_size, dec_size);
				return dec_size;
			} else {
				/* Live block: copy directly from main pool */
				memcpy(buf, &mem_pool[i * BLOCK_ALIGN], copy_size);

				LOG_INF("read: handle=0x%04x LIVE %u bytes",
					handle, copy_size);
				return copy_size;
			}
		}
	}

	LOG_WRN("read: bad handle 0x%04x", handle);
	return 0;
}

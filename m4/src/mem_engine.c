/*
 * Gigaspark OS - Memory Engine Implementation
 * 3-tier block allocator: RAM -> compressed_pool -> SD swap
 *
 * Tier 1 (fastest): mem_pool[4KB] - live data, direct access
 * Tier 2 (medium):  compressed_pool[2KB] - RLE compressed blobs
 * Tier 3 (slowest): SD card sectors - evicted compressed blobs
 *
 * Eviction policy: when a tier fills up, the engine evicts
 * the oldest (lowest index) block from that tier to make room.
 * This is simple O(n) but acceptable for small pool sizes.
 */

#include "mem_engine.h"
#include "zram_comp.h"
#include "sd_swap.h"
#include "ipc_protocol.h"

#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(mem_engine, CONFIG_LOG_DEFAULT_LEVEL);

/* Block alignment for allocations */
#define BLOCK_ALIGN     16
#define MAX_BLOCKS      (MEM_POOL_SIZE / BLOCK_ALIGN)

/* Block states */
#define STATE_FREE       0
#define STATE_LIVE       1   /* allocated, data in main pool */
#define STATE_COMPRESSED 2   /* allocated, data in compressed pool */
#define STATE_SWAPPED    3   /* allocated, data on SD card */

struct block_desc {
	uint16_t handle;       /* unique handle for this block */
	uint32_t size;         /* original requested size */
	uint32_t aligned_size; /* aligned block size */
	uint8_t  state;        /* STATE_FREE, STATE_LIVE, STATE_COMPRESSED, STATE_SWAPPED */
	uint16_t comp_offset;  /* offset in compressed_pool */
	uint16_t comp_size;    /* compressed size */
};

/* Static memory pools */
static uint8_t mem_pool[MEM_POOL_SIZE] __aligned(BLOCK_ALIGN);
static uint8_t compressed_pool[COMPRESSED_POOL_SIZE] __aligned(BLOCK_ALIGN);

/* Block descriptor table */
static struct block_desc blocks[MAX_BLOCKS];
static uint16_t next_handle = 1;

/* Compressed pool bookkeeping */
static uint16_t comp_next_free = 0;

/* Temporary buffer for decompression during mem_read */
static uint8_t decomp_buf[MEM_MAX_ALLOC] __aligned(BLOCK_ALIGN);

void mem_init(void)
{
	memset(mem_pool, 0, sizeof(mem_pool));
	memset(compressed_pool, 0, sizeof(compressed_pool));
	memset(blocks, 0, sizeof(blocks));
	comp_next_free = 0;

	LOG_INF("Memory engine initialized: main=%d, compressed=%d, blocks=%d",
		MEM_POOL_SIZE, COMPRESSED_POOL_SIZE, MAX_BLOCKS);

	/* Initialize SD swap (non-fatal if SD not present) */
	int ret = swap_init();
	if (ret < 0) {
		LOG_WRN("SD swap unavailable (ret=%d), operating in zRAM-only mode", ret);
	} else {
		LOG_INF("SD swap available, 3-tier mode active");
	}
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
 * Find the oldest LIVE block to compress (eviction from tier 1).
 * Uses first-fit: picks the block with the lowest descriptor index.
 */
static int find_victim_live(void)
{
	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (blocks[i].state == STATE_LIVE) {
			return i;
		}
	}
	return -1;
}

/*
 * Find the oldest COMPRESSED block to evict to SD (eviction from tier 2).
 * Uses first-fit: picks the block with the lowest comp_offset.
 */
static int find_victim_compressed(void)
{
	int best = -1;
	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (blocks[i].state == STATE_COMPRESSED) {
			if (best < 0 || blocks[i].comp_offset < blocks[best].comp_offset) {
				best = i;
			}
		}
	}
	return best;
}

/*
 * Evict a compressed block to SD card.
 * Returns 0 on success, -1 on failure.
 */
static int evict_to_sd(int slot)
{
	struct block_desc *blk = &blocks[slot];

	if (swap_is_available() < 0) {
		LOG_WRN("evict_to_sd: SD not available");
		return -1;
	}

	/* Read compressed data from compressed_pool */
	const uint8_t *comp_data = &compressed_pool[blk->comp_offset];
	int sector = swap_write_blob(blk->handle, comp_data, blk->comp_size);

	if (sector < 0) {
		LOG_WRN("evict_to_sd: SD write failed for handle=0x%04x", blk->handle);
		return -1;
	}

	/* Reclaim compressed_pool space */
	uint16_t removed_offset = blk->comp_offset;
	uint16_t removed_size = blk->comp_size;

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

	blk->state = STATE_SWAPPED;
	blk->comp_offset = 0;
	blk->comp_size = 0;

	LOG_INF("evict_to_sd: handle=0x%04x -> sector=%d", blk->handle, sector);
	return 0;
}

/*
 * Compress a block from main_pool to compressed_pool.
 * If compressed_pool is full, evicts to SD first.
 */
static int compress_block(int slot)
{
	struct block_desc *blk = &blocks[slot];
	uint8_t *src = &mem_pool[slot * BLOCK_ALIGN];

	/* Check if compressed_pool has space */
	size_t max_comp = COMPRESSED_POOL_SIZE - comp_next_free;

	if (max_comp < blk->aligned_size) {
		/* compressed_pool might be full, try evicting to SD */
		int victim = find_victim_compressed();
		if (victim >= 0) {
			if (evict_to_sd(victim) < 0) {
				LOG_WRN("compress: cannot evict to SD, pool full");
				return -1;
			}
			/* Recalculate after eviction */
			max_comp = COMPRESSED_POOL_SIZE - comp_next_free;
		} else {
			LOG_WRN("compress: no compressed blocks to evict");
			return -1;
		}
	}

	uint8_t *dst = &compressed_pool[comp_next_free];
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
		int victim = find_victim_live();
		if (victim < 0) {
			LOG_WRN("alloc: no compressible blocks for %u bytes", size);
			return MEM_HANDLE_INVALID;
		}
		if (compress_block(victim) < 0) {
			LOG_WRN("alloc: compression failed for victim 0x%04x",
				blocks[victim].handle);
			return MEM_HANDLE_INVALID;
		}
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
			LOG_INF("free: handle=0x%04x state=%d", handle, blocks[i].state);

			/* If compressed, reclaim compressed_pool space */
			if (blocks[i].state == STATE_COMPRESSED) {
				uint16_t removed_offset = blocks[i].comp_offset;
				uint16_t removed_size = blocks[i].comp_size;

				if (removed_offset + removed_size < comp_next_free) {
					memmove(&compressed_pool[removed_offset],
						&compressed_pool[removed_offset + removed_size],
						comp_next_free - removed_offset - removed_size);

					for (int j = 0; j < MAX_BLOCKS; j++) {
						if (blocks[j].state == STATE_COMPRESSED &&
						    blocks[j].comp_offset > removed_offset) {
							blocks[j].comp_offset -= removed_size;
						}
					}
				}
				comp_next_free -= removed_size;
			}

			/* If swapped, remove from SD */
			if (blocks[i].state == STATE_SWAPPED) {
				swap_remove_blob(handle);
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

			switch (blocks[i].state) {
			case STATE_LIVE: {
				/* Direct copy from main pool */
				memcpy(buf, &mem_pool[i * BLOCK_ALIGN], copy_size);
				LOG_INF("read: handle=0x%04x LIVE %u bytes",
					handle, copy_size);
				return copy_size;
			}
			case STATE_COMPRESSED: {
				/* Decompress from compressed_pool */
				size_t dec_size = zram_decompress(
					&compressed_pool[blocks[i].comp_offset],
					blocks[i].comp_size,
					(uint8_t *)buf,
					copy_size);

				LOG_INF("read: handle=0x%04x DECOMPRESSED %u->%u bytes",
					handle, blocks[i].comp_size, dec_size);
				return dec_size;
			}
			case STATE_SWAPPED: {
				/* Read from SD, then decompress */
				if (swap_is_available() < 0) {
					LOG_ERR("read: handle=0x%04x is SWAPPED but SD unavailable",
						handle);
					return 0;
				}

				size_t sd_read = swap_read_blob(handle, decomp_buf,
								sizeof(decomp_buf));
				if (sd_read == 0) {
					LOG_ERR("read: SD read failed for handle=0x%04x", handle);
					return 0;
				}

				/* Decompress from decomp_buf into user buffer */
				size_t dec_size = zram_decompress(decomp_buf, sd_read,
								  (uint8_t *)buf, copy_size);

				LOG_INF("read: handle=0x%04x SWAPPED SD->DECOMP %u->%u bytes",
					handle, sd_read, dec_size);
				return dec_size;
			}
			default:
				return 0;
			}
		}
	}

	LOG_WRN("read: bad handle 0x%04x", handle);
	return 0;
}

int mem_write(uint16_t handle, const void *data, size_t data_len)
{
	if (handle == MEM_HANDLE_INVALID || data == NULL || data_len == 0) {
		return -1;
	}

	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (blocks[i].handle == handle && blocks[i].state != STATE_FREE) {
			if (data_len > blocks[i].size) {
				LOG_WRN("write: data too large (%u > %u)", data_len, blocks[i].size);
				return -1;
			}

			/* If block is compressed or swapped, we need to bring it back to LIVE */
			if (blocks[i].state == STATE_COMPRESSED) {
				/* Decompress into main pool */
				zram_decompress(
					&compressed_pool[blocks[i].comp_offset],
					blocks[i].comp_size,
					&mem_pool[i * BLOCK_ALIGN],
					blocks[i].aligned_size);

				/* Reclaim compressed_pool space */
				uint16_t removed_offset = blocks[i].comp_offset;
				uint16_t removed_size = blocks[i].comp_size;

				if (removed_offset + removed_size < comp_next_free) {
					memmove(&compressed_pool[removed_offset],
						&compressed_pool[removed_offset + removed_size],
						comp_next_free - removed_offset - removed_size);

					for (int j = 0; j < MAX_BLOCKS; j++) {
						if (blocks[j].state == STATE_COMPRESSED &&
						    blocks[j].comp_offset > removed_offset) {
							blocks[j].comp_offset -= removed_size;
						}
					}
				}
				comp_next_free -= removed_size;

				blocks[i].comp_offset = 0;
				blocks[i].comp_size = 0;
				blocks[i].state = STATE_LIVE;

				LOG_INF("write: handle=0x%04x promoted from COMPRESSED", handle);
			} else if (blocks[i].state == STATE_SWAPPED) {
				/* Read from SD into main pool, then decompress */
				if (swap_is_available() >= 0) {
					size_t sd_read = swap_read_blob(handle, decomp_buf,
									sizeof(decomp_buf));
					if (sd_read > 0) {
						zram_decompress(decomp_buf, sd_read,
								&mem_pool[i * BLOCK_ALIGN],
								blocks[i].aligned_size);
					}
					swap_remove_blob(handle);
				}

				blocks[i].state = STATE_LIVE;
				LOG_INF("write: handle=0x%04x promoted from SWAPPED", handle);
			}

			/* Now write the data into main pool */
			memcpy(&mem_pool[i * BLOCK_ALIGN], data, data_len);

			LOG_INF("write: handle=0x%04x %u bytes", handle, data_len);
			return 0;
		}
	}

	LOG_WRN("write: bad handle 0x%04x", handle);
	return -2;
}

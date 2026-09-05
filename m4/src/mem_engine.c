/*
 * Gigaspark OS - Memory Engine Implementation
 * Simple first-fit block allocator on M4
 *
 * Manages a static 4KB pool. Each allocation returns a handle (index),
 * not a pointer. The M7 never sees real addresses.
 */

#include "mem_engine.h"
#include "ipc_protocol.h"

#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(mem_engine, CONFIG_LOG_DEFAULT_LEVEL);

/* Memory pool descriptor */
#define BLOCK_ALIGN     16   /* 16-byte alignment for all blocks */
#define MAX_BLOCKS      (MEM_POOL_SIZE / BLOCK_ALIGN)

struct block_desc {
	uint16_t handle;    /* unique handle for this block */
	uint32_t size;      /* usable size in bytes */
	uint8_t  used;      /* 1 = allocated, 0 = free */
};

/* Static memory pool */
static uint8_t mem_pool[MEM_POOL_SIZE] __aligned(BLOCK_ALIGN);

/* Block descriptor table (one entry per aligned slot) */
static struct block_desc blocks[MAX_BLOCKS];
static uint16_t next_handle = 1;

void mem_init(void)
{
	memset(mem_pool, 0, sizeof(mem_pool));
	memset(blocks, 0, sizeof(blocks));

	LOG_INF("Memory engine initialized: %d bytes pool, %d max blocks",
		MEM_POOL_SIZE, MAX_BLOCKS);
}

uint16_t mem_alloc(uint32_t size)
{
	if (size == 0 || size > MEM_MAX_ALLOC) {
		LOG_WRN("alloc: invalid size %u", size);
		return MEM_HANDLE_INVALID;
	}

	/* Round up to block alignment */
	uint32_t aligned_size = (size + BLOCK_ALIGN - 1) & ~(BLOCK_ALIGN - 1);

	/* First-fit search through descriptor table */
	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (!blocks[i].used && blocks[i].size == 0) {
			/* Free entry with enough space (or first fit) */
			uint16_t handle = next_handle++;

			blocks[i].handle = handle;
			blocks[i].size = aligned_size;
			blocks[i].used = 1;

			LOG_INF("alloc: handle=0x%04x size=%u (aligned=%u) offset=%d",
				handle, size, aligned_size, i * BLOCK_ALIGN);

			return handle;
		}
	}

	LOG_WRN("alloc: no free block for %u bytes", size);
	return MEM_HANDLE_INVALID;
}

int mem_free(uint16_t handle)
{
	if (handle == MEM_HANDLE_INVALID) {
		return -1;
	}

	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (blocks[i].used && blocks[i].handle == handle) {
			LOG_INF("free: handle=0x%04x size=%u",
				handle, blocks[i].size);

			blocks[i].used = 0;
			blocks[i].handle = 0;
			/* Keep size so next_alloc can find the slot */
			return 0;
		}
	}

	LOG_WRN("free: bad handle 0x%04x", handle);
	return -2;
}

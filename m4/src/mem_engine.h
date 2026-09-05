/*
 * Gigaspark OS - Memory Engine Header
 * Block allocator with zRAM compression on M4
 */

#ifndef GIGASPARK_MEM_ENGINE_H
#define GIGASPARK_MEM_ENGINE_H

#include <stdint.h>
#include <stddef.h>

/*
 * Initialize the memory engine.
 * Must be called once before any alloc/free operations.
 */
void mem_init(void);

/*
 * Allocate a block of memory.
 *
 * If the main pool is full, the engine will automatically compress
 * an inactive block to the compressed_pool and reclaim its space.
 *
 * @param size  Number of bytes to allocate (must be > 0 and <= MEM_MAX_ALLOC).
 * @return      Handle (opaque ID) on success, or MEM_HANDLE_INVALID on failure.
 */
uint16_t mem_alloc(uint32_t size);

/*
 * Free a previously allocated block.
 *
 * If the block was compressed, it is removed from compressed_pool.
 *
 * @param handle  Handle returned by mem_alloc().
 * @return        0 on success, negative error code on failure.
 */
int mem_free(uint16_t handle);

/*
 * Read data from a block into a buffer.
 *
 * If the block is compressed, it is decompressed first.
 * If the block is live (not compressed), data is copied directly.
 *
 * @param handle  Handle returned by mem_alloc().
 * @param buf     Destination buffer.
 * @param buf_size  Size of destination buffer.
 * @return        Number of bytes actually read, or 0 on error.
 */
size_t mem_read(uint16_t handle, void *buf, size_t buf_size);

#endif /* GIGASPARK_MEM_ENGINE_H */

/*
 * Gigaspark OS - Memory Engine Header
 * 3-tier block allocator: RAM -> compressed_pool -> SD swap
 */

#ifndef GIGASPARK_MEM_ENGINE_H
#define GIGASPARK_MEM_ENGINE_H

#include <stdint.h>
#include <stddef.h>

/*
 * Initialize the memory engine and SD swap subsystem.
 * Must be called once before any alloc/free operations.
 */
void mem_init(void);

/*
 * Allocate a block of memory.
 *
 * If the main pool is full, compresses an inactive block.
 * If compressed_pool is full, evicts oldest compressed block to SD.
 *
 * @param size  Number of bytes to allocate (must be > 0 and <= MEM_MAX_ALLOC).
 * @return      Handle (opaque ID) on success, or MEM_HANDLE_INVALID on failure.
 */
uint16_t mem_alloc(uint32_t size);

/*
 * Free a previously allocated block.
 *
 * Handles all states: LIVE, COMPRESSED, SWAPPED.
 *
 * @param handle  Handle returned by mem_alloc().
 * @return        0 on success, negative error code on failure.
 */
int mem_free(uint16_t handle);

/*
 * Read data from a block into a buffer.
 *
 * Transparently reads from RAM, compressed_pool, or SD swap.
 *
 * @param handle    Handle returned by mem_alloc().
 * @param buf       Destination buffer.
 * @param buf_size  Size of destination buffer.
 * @return          Number of bytes actually read, or 0 on error.
 */
size_t mem_read(uint16_t handle, void *buf, size_t buf_size);

/*
 * Write data into an existing block.
 *
 * @param handle    Handle returned by mem_alloc().
 * @param data      Source data to write.
 * @param data_len  Number of bytes to write.
 * @return          0 on success, negative error code on failure.
 */
int mem_write(uint16_t handle, const void *data, size_t data_len);

#endif /* GIGASPARK_MEM_ENGINE_H */

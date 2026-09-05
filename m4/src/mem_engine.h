/*
 * Gigaspark OS - Memory Engine Header
 * Simple block allocator running on M4
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
 * @param size  Number of bytes to allocate (must be > 0 and <= MEM_MAX_ALLOC).
 * @return      Handle (opaque ID) on success, or MEM_HANDLE_INVALID on failure.
 *
 * The returned handle is NOT a pointer. The M7 never sees real addresses.
 */
uint16_t mem_alloc(uint32_t size);

/*
 * Free a previously allocated block.
 *
 * @param handle  Handle returned by mem_alloc().
 * @return        0 on success, negative error code on failure.
 */
int mem_free(uint16_t handle);

#endif /* GIGASPARK_MEM_ENGINE_H */

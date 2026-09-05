/*
 * Gigaspark OS - SD Swap Header
 * Block-level swap to SD card for compressed memory blobs
 */

#ifndef GIGASPARK_SD_SWAP_H
#define GIGASPARK_SD_SWAP_H

#include <stdint.h>
#include <stddef.h>

/*
 * Initialize the SD swap subsystem.
 *
 * Attempts to open the SD card via Zephyr disk_access API.
 * If no SD card is present, the module enters graceful degradation
 * mode (swap_enabled = false) and all operations return errors.
 *
 * @return  0 on success, -ENODEV if no SD card found.
 */
int swap_init(void);

/*
 * Write a compressed blob to the SD card.
 *
 * @param handle     The memory handle identifying this blob.
 * @param data       Compressed data to write.
 * @param data_len   Length of compressed data in bytes.
 * @return           Sector index on success, -1 on failure.
 */
int swap_write_blob(uint16_t handle, const uint8_t *data, uint16_t data_len);

/*
 * Read a compressed blob from the SD card.
 *
 * @param handle     The memory handle identifying this blob.
 * @param buf        Destination buffer.
 * @param buf_size   Size of destination buffer.
 * @return           Number of bytes read, or 0 on failure.
 */
size_t swap_read_blob(uint16_t handle, uint8_t *buf, size_t buf_size);

/*
 * Remove a blob from the SD swap table.
 *
 * @param handle  The memory handle to remove.
 * @return        0 on success, -1 if not found.
 */
int swap_remove_blob(uint16_t handle);

/*
 * Check if the SD swap subsystem is available.
 *
 * @return  true if SD card is initialized and ready.
 */
int swap_is_available(void);

#endif /* GIGASPARK_SD_SWAP_H */

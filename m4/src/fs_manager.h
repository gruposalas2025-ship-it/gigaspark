/*
 * Gigaspark OS - File System Manager Header
 * SD card access via SPI + FAT32 filesystem
 */

#ifndef GIGASPARK_FS_MANAGER_H
#define GIGASPARK_FS_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include "ipc_protocol.h"

/*
 * Initialize the filesystem manager.
 * Mounts the SD card at /SD: and scans /apps/ for .bin files.
 *
 * @return  0 on success, negative error code on failure.
 */
int fs_init(void);

/*
 * Get the number of apps found on the SD card.
 *
 * @return  Number of apps (0 if none found or SD not available).
 */
int fs_get_app_count(void);

/*
 * Get the list of apps found on the SD card.
 *
 * @param out   Pointer to array of app_info structs.
 * @param max   Maximum number of apps to return.
 * @return      Number of apps copied to out.
 */
int fs_get_app_list(struct app_info *out, int max);

/*
 * Load an app binary into a buffer.
 *
 * @param id    App id (0-based index).
 * @param buf   Destination buffer.
 * @param buf_size  Size of destination buffer.
 * @return      Number of bytes loaded, or 0 on error.
 */
size_t fs_load_app(uint8_t id, uint8_t *buf, size_t buf_size);

/*
 * Check if the filesystem is ready.
 *
 * @return  0 if ready, negative error code otherwise.
 */
int fs_is_ready(void);

/*
 * Open a file for writing.
 *
 * @param filename  Name of the file (relative to /SD:/apps/).
 * @return          File descriptor (>= 0) or negative error code.
 */
int fs_file_open(const char *filename);

/*
 * Write data to an open file.
 *
 * @param fd    File descriptor from fs_file_open.
 * @param data  Data to write.
 * @param len   Number of bytes to write.
 * @return      Number of bytes written, or negative error code.
 */
int fs_file_write(int fd, const uint8_t *data, size_t len);

/*
 * Close an open file.
 *
 * @param fd    File descriptor from fs_file_open.
 * @return      0 on success, negative error code.
 */
int fs_file_close(int fd);

#endif /* GIGASPARK_FS_MANAGER_H */

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

/* ---- Media Read API ---- */

/*
 * Abrir archivo multimedia para lectura.
 *
 * @param filename  Nombre del archivo (relativo a /SD:/media/).
 * @return          File descriptor (>= 0) o error negativo.
 */
int fs_media_open(const char *filename);

/*
 * Leer chunk de archivo multimedia.
 * Los datos se leen alineados a 32 bytes para DMA/DCACHE.
 *
 * @param fd         File descriptor de fs_media_open.
 * @param buf        Buffer de destino.
 * @param buf_size   Tamano maximo a leer.
 * @param bytes_read Puntero donde almacenar bytes leidos.
 * @return           0 en exito, error negativo.
 */
int fs_media_read(int fd, uint8_t *buf, size_t buf_size, size_t *bytes_read);

/*
 * Mover cursor en archivo multimedia.
 *
 * @param fd      File descriptor.
 * @param offset  Offset en bytes.
 * @param whence  0=SET, 1=CUR, 2=END.
 * @return        0 en exito, error negativo.
 */
int fs_media_seek(int fd, int32_t offset, int whence);

/*
 * Cerrar archivo multimedia.
 *
 * @param fd  File descriptor.
 * @return    0 en exito, error negativo.
 */
int fs_media_close(int fd);

#endif /* GIGASPARK_FS_MANAGER_H */

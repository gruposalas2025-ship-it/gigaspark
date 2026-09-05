/*
 * Gigaspark OS - zRAM Compression Header
 * Simple RLE (Run-Length Encoding) compressor/decompressor
 * No external dependencies, pure C, designed for embedded.
 */

#ifndef GIGASPARK_ZRAM_COMP_H
#define GIGASPARK_ZRAM_COMP_H

#include <stdint.h>
#include <stddef.h>

/*
 * Compress data using RLE encoding.
 *
 * Format: [count][byte] pairs. count is 1-255.
 * If a run exceeds 255, it is split into multiple pairs.
 * Literal (non-repeated) bytes are stored as [1][byte].
 *
 * @param src       Source data to compress.
 * @param src_len   Length of source data in bytes.
 * @param dst       Destination buffer for compressed data.
 * @param dst_len   Size of destination buffer in bytes.
 * @return          Number of bytes written to dst, or 0 on error
 *                  (dst buffer too small).
 */
size_t zram_compress(const uint8_t *src, size_t src_len,
		     uint8_t *dst, size_t dst_len);

/*
 * Decompress RLE-encoded data.
 *
 * @param src       Compressed data.
 * @param src_len   Length of compressed data in bytes.
 * @param dst       Destination buffer for decompressed data.
 * @param dst_len   Size of destination buffer in bytes.
 * @return          Number of bytes written to dst, or 0 on error
 *                  (dst buffer too small or corrupt data).
 */
size_t zram_decompress(const uint8_t *src, size_t src_len,
		       uint8_t *dst, size_t dst_len);

#endif /* GIGASPARK_ZRAM_COMP_H */

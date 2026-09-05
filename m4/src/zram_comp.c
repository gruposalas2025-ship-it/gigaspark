/*
 * Gigaspark OS - zRAM Compression Implementation
 * Run-Length Encoding (RLE) compressor/decompressor
 *
 * RLE format: sequences of [count][byte] pairs.
 *   - count: 1-255 (uint8_t)
 *   - byte:  the data byte
 *   - A run of 300 identical bytes becomes [255][byte] + [45][byte]
 *   - A non-repeated byte becomes [1][byte]
 *
 * Worst case: input with no runs at all -> 2x expansion.
 * Best case: highly repetitive data -> near 1:255 compression.
 *
 * This is intentionally simple and fast for an MCU without FPU.
 */

#include "zram_comp.h"

size_t zram_compress(const uint8_t *src, size_t src_len,
		     uint8_t *dst, size_t dst_len)
{
	size_t si = 0;  /* source index */
	size_t di = 0;  /* destination index */

	while (si < src_len) {
		uint8_t current = src[si];
		uint8_t count = 1;

		/* Count how many identical bytes follow */
		while (si + count < src_len &&
		       src[si + count] == current &&
		       count < 255) {
			count++;
		}

		/* Need 2 bytes for this run */
		if (di + 2 > dst_len) {
			return 0;  /* buffer overflow */
		}

		dst[di++] = count;
		dst[di++] = current;

		si += count;
	}

	return di;
}

size_t zram_decompress(const uint8_t *src, size_t src_len,
		       uint8_t *dst, size_t dst_len)
{
	size_t si = 0;  /* source index */
	size_t di = 0;  /* destination index */

	while (si + 1 < src_len) {
		uint8_t count = src[si];
		uint8_t byte  = src[si + 1];

		si += 2;

		/* Check if output buffer has space */
		if (di + count > dst_len) {
			return 0;  /* buffer overflow */
		}

		/* Write 'count' copies of 'byte' */
		for (uint8_t i = 0; i < count; i++) {
			dst[di++] = byte;
		}
	}

	return di;
}

/*
 * Gigaspark OS - Decodificador de Video JPEG por Hardware
 * Usa el acelerador JPEG integrado del STM32H747
 */

#ifndef GIGASPARK_VIDEO_DECODER_H
#define GIGASPARK_VIDEO_DECODER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Dimensiones maximas de imagen (360p = 640x360) */
#define VIDEO_MAX_WIDTH   640
#define VIDEO_MAX_HEIGHT  360

/* Tamano del buffer RGB565 (2 bytes por pixel) */
#define VIDEO_RGB565_SIZE  (VIDEO_MAX_WIDTH * VIDEO_MAX_HEIGHT * 2)

/*
 * Inicializar el decodificador JPEG hardware.
 *
 * @return  0 en exito, error negativo.
 */
int video_decoder_init(void);

/*
 * Decodificar un frame JPEG a RGB565.
 * Usa el acelerador JPEG del STM32H747 por DMA.
 *
 * @param jpeg_data   Datos JPEG de entrada.
 * @param jpeg_size   Tamano de los datos JPEG.
 * @param rgb565_out  Buffer de salida RGB565 (debe ser >= VIDEO_RGB565_SIZE).
 * @param width       Puntero donde almacenar ancho de imagen decodificada.
 * @param height      Puntero donde almacenar alto de imagen decodificada.
 * @return            Numero de bytes RGB565 generados, o error negativo.
 */
int video_decode_frame(const uint8_t *jpeg_data, uint32_t jpeg_size,
		       uint8_t *rgb565_out, uint32_t *width, uint32_t *height);

/*
 * Verificar si el decodificador esta listo.
 *
 * @return  true si inicializado correctamente.
 */
bool video_decoder_is_ready(void);

#endif /* GIGASPARK_VIDEO_DECODER_H */

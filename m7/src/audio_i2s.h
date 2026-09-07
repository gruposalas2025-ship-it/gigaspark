/*
 * Gigaspark OS - Motor de Audio I2S
 * Reproduccion PCM 16-bit estereo via I2S + DMA
 */

#ifndef GIGASPARK_AUDIO_I2S_H
#define GIGASPARK_AUDIO_I2S_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Configuracion de audio por defecto */
#define AUDIO_SAMPLE_RATE    44100
#define AUDIO_BITS_PER_SAMPLE 16
#define AUDIO_CHANNELS       2

/* Tamano del buffer de audio (frames) */
#define AUDIO_BUFFER_FRAMES  1024

/*
 * Inicializar el motor de audio I2S.
 *
 * @return  0 en exito, error negativo.
 */
int audio_i2s_init(void);

/*
 * Reproducir un chunk de audio PCM.
 * La operacion es non-blocking (usa DMA interno).
 *
 * @param pcm_data  Datos PCM 16-bit estereo.
 * @param size      Tamano en bytes.
 * @return          0 en exito, error negativo.
 */
int audio_play_chunk(const uint8_t *pcm_data, uint32_t size);

/*
 * Detener reproduccion de audio.
 */
void audio_stop(void);

/*
 * Verificar si el audio esta reproduciendo.
 *
 * @return  true si esta reproduciendo.
 */
bool audio_is_playing(void);

/*
 * Obtener el nivel de buffer (para sync con video).
 *
 * @return  Porcentaje del buffer lleno (0-100).
 */
int audio_get_buffer_level(void);

#endif /* GIGASPARK_AUDIO_I2S_H */

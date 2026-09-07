/*
 * Gigaspark OS - Motor de Audio I2S Implementation
 * Reproduccion PCM 16-bit estereo via I2S + DMA
 *
 * Usa el driver I2S de Zephyr con DMA para no bloquear la CPU.
 * Los chunks de audio se envian al DAC externo (o integrado)
 * mientras el M7 continua procesando video frames.
 *
 * NOTA: El Arduino Giga R1 no expose I2S por defecto en el DTS.
 * Cuando se habilite el periferico I2S en el overlay, se activara
 * automaticamente el codigo de hardware via CONFIG_I2S.
 */

#include "audio_i2s.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(audio_i2s, CONFIG_LOG_DEFAULT_LEVEL);

/* Estado del audio */
static volatile bool audio_playing = false;
static volatile bool audio_initialized = false;

#if defined(CONFIG_I2S) && defined(CONFIG_SOC_STM32H747XX)

/* Cuando I2S este habilitado y el SoC lo soporte, usar driver nativo */
#include <zephyr/drivers/i2s.h>

static const struct device *i2s_dev;

#define AUDIO_DMA_BUF_COUNT  4
#define AUDIO_DMA_BUF_SIZE   (AUDIO_BUFFER_FRAMES * AUDIO_CHANNELS * (AUDIO_BITS_PER_SAMPLE / 8))

static int32_t __aligned(32) audio_dma_buf[AUDIO_DMA_BUF_COUNT]
	[AUDIO_BUFFER_FRAMES * AUDIO_CHANNELS];

static int current_buf = 0;

int audio_i2s_init(void)
{
	LOG_INF("Inicializando motor de audio I2S...");

	/* Intentar obtener dispositivo I2S del DTS */
	i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s2));

	if (!device_is_ready(i2s_dev)) {
		LOG_WRN("I2S2 no disponible en DTS - modo stub");
		return -ENODEV;
	}

	struct i2s_config i2s_cfg = {
		.word_size = AUDIO_BITS_PER_SAMPLE,
		.channels = AUDIO_CHANNELS,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
		.frame_clk_freq = AUDIO_SAMPLE_RATE,
		.mem_slab = NULL,
		.block_size = AUDIO_DMA_BUF_SIZE,
		.timeout = 1000,
	};

	int ret = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);
	if (ret < 0) {
		LOG_ERR("I2S configure fallido: %d", ret);
		return ret;
	}

	audio_initialized = true;
	LOG_INF("Motor de audio I2S listo (%d Hz, %d-bit, %d canales)",
		AUDIO_SAMPLE_RATE, AUDIO_BITS_PER_SAMPLE, AUDIO_CHANNELS);
	return 0;
}

int audio_play_chunk(const uint8_t *pcm_data, uint32_t size)
{
	if (!audio_initialized || !pcm_data || size == 0) {
		return -ENODEV;
	}

	uint32_t copy_size = (size > AUDIO_DMA_BUF_SIZE) ? AUDIO_DMA_BUF_SIZE : size;
	memcpy(audio_dma_buf[current_buf], pcm_data, copy_size);

	int ret = i2s_write(i2s_dev, audio_dma_buf[current_buf], copy_size);
	if (ret < 0) {
		LOG_ERR("I2S write fallido: %d", ret);
		return ret;
	}

	current_buf = (current_buf + 1) % AUDIO_DMA_BUF_COUNT;

	if (!audio_playing) {
		audio_playing = true;
		LOG_INF("Audio reproduciendo...");
	}
	return 0;
}

void audio_stop(void)
{
	if (audio_playing) {
		audio_playing = false;
		LOG_INF("Audio detenido");
	}
}

bool audio_is_playing(void) { return audio_playing; }
int audio_get_buffer_level(void) { return audio_playing ? 50 : 0; }

#else /* Stub */

int audio_i2s_init(void)
{
	LOG_WRN("I2S no habilitado - modo stub");
	return -ENODEV;
}

int audio_play_chunk(const uint8_t *pcm_data, uint32_t size)
{
	(void)pcm_data; (void)size;
	return -ENODEV;
}

void audio_stop(void) { }
bool audio_is_playing(void) { return false; }
int audio_get_buffer_level(void) { return 0; }

#endif

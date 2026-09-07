/*
 * Gigaspark OS - Decodificador de Video JPEG por Hardware
 * Usa el acelerador JPEG integrado del STM32H747 via HAL
 *
 * El STM32H747 tiene un bloque JPEG dedicado que puede decodificar
 * imagenes JPEG a formato RGB565 usando DMA, sin cargar la CPU.
 *
 * Flujo:
 * 1. Inicializar el periferico JPEG con HAL_JPEG_Init
 * 2. Configurar buffers de entrada/salida
 * 3. Ejecutar HAL_JPEG_Decode (bloquea hasta completar)
 * 4. El resultado RGB565 queda listo para enviar al display
 */

#include "video_decoder.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(video_decoder, CONFIG_LOG_DEFAULT_LEVEL);

/* Incluir HAL de STM32 solo si esta habilitado */
#if defined(CONFIG_USE_STM32_HAL_JPEG)

#include <stm32h7xx_hal.h>
#include <stm32h7xx_hal_jpeg.h>

/* Handle del JPEG hardware */
static JPEG_HandleTypeDef hjpeg;

/* Buffer de trabajo para el decoder (en SDRAM si esta disponible) */
#if defined(CONFIG_SOC_STM32H747XX)
static uint32_t __attribute__((section(".sdram"))) jpeg_work_buf[1024];
#else
static uint32_t jpeg_work_buf[1024];
#endif

/* Buffer interno para resultado RGB565 */
static uint8_t __attribute__((section(".sdram"))) rgb565_framebuf[VIDEO_RGB565_SIZE];

/* Callback de finalizacion */
static volatile bool jpeg_decode_complete;
static volatile int jpeg_decode_result;

/*
 * Callback del decoder JPEG - llamada al finalizar decode
 */
static void JPEG_DecodeCompleteCallback(JPEG_HandleTypeDef *hjpeg)
{
	(void)hjpeg;
	jpeg_decode_complete = true;
	jpeg_decode_result = 0;
	LOG_DBG("JPEG decode completado");
}

/*
 * Callback de error
 */
static void JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpeg)
{
	(void)hjpeg;
	jpeg_decode_complete = true;
	jpeg_decode_result = -EIO;
	LOG_ERR("JPEG decode error");
}

int video_decoder_init(void)
{
	LOG_INF("Inicializando decodificador JPEG hardware...");

	/* Limpiar handle */
	memset(&hjpeg, 0, sizeof(hjpeg));

	/* Configurar el periferico JPEG */
	hjpeg.Instance = JPEG;
	hjpeg.Init.Encoding = JPEG_CODEC_MODE_DECODE;
	hjpeg.Init.DataWidthMode = JPEG_DATAWIDTH_24B;
	hjpeg.Init.HeaderParsing = JPEG_HEADER_PARSING_DISABLE;
	hjpeg.Init.Prosessing = JPEG_OPT_PROCESSING_DEFAULT;
	hjpeg.Init.Resolution = JPEG_RESOLUTION_128x128;
	hjpeg.Init.ColorSpace = JPEG_COLORSPACE_CMYK;

	/* Callbacks */
	hjpeg.Init.Callbacks = JPEG_DecodeCompleteCallback;
	hjpeg.Init.ErrorCallback = JPEG_ErrorCallback;

	HAL_StatusTypeDef status = HAL_JPEG_Init(&hjpeg);
	if (status != HAL_OK) {
		LOG_ERR("JPEG HAL init fallido: %d", status);
		return -EIO;
	}

	/* Descargar tabla de Huffman por defecto */
	// HAL_JPEG_Decode_Init(&hjpeg);

	LOG_INF("Decodificador JPEG hardware listo");
	return 0;
}

int video_decode_frame(const uint8_t *jpeg_data, uint32_t jpeg_size,
		       uint8_t *rgb565_out, uint32_t *width, uint32_t *height)
{
	if (!video_decoder_is_ready()) {
		return -ENODEV;
	}

	if (jpeg_data == NULL || jpeg_size == 0) {
		return -EINVAL;
	}

	if (rgb565_out == NULL || width == NULL || height == NULL) {
		return -EINVAL;
	}

	jpeg_decode_complete = false;
	jpeg_decode_result = 0;

	LOG_DBG("Decodificando frame JPEG (%u bytes)...", jpeg_size);

	/* Configurar decodificacion */
	JPEG_DecodeConfTypeDef decode_conf;
	decode_conf.InputDataLength = jpeg_size;
	decode_conf.OutputBuffer = (uint32_t *)rgb565_out;
	decode_conf.OutputBufferSize = VIDEO_RGB565_SIZE;

	/* Ejecutar decode (bloquea hasta completar via DMA interno) */
	HAL_StatusTypeDef status = HAL_JPEG_Decode(&hjpeg,
		(uint8_t *)jpeg_data, jpeg_size,
		rgb565_out, VIDEO_RGB565_SIZE,
		JPEG_DecodeCompleteCallback, JPEG_ErrorCallback);

	if (status != HAL_OK) {
		LOG_ERR("JPEG decode fallido: %d", status);
		return -EIO;
	}

	/* Obtener dimensiones de la imagen decodificada */
	JPEG_ConfTypeDef conf;
	HAL_JPEG_GetInfo(&hjpeg, &conf);

	*width = conf.ImageWidth;
	*height = conf.ImageHeight;

	LOG_DBG("Frame decodificado: %ux%u RGB565", *width, *height);

	return (int)(*width * *height * 2);
}

bool video_decoder_is_ready(void)
{
	return (hjpeg.Instance == JPEG);
}

#else /* !CONFIG_USE_STM32_HAL_JPEG */

/* Stub cuando JPEG HAL no esta habilitado */
int video_decoder_init(void)
{
	LOG_WRN("JPEG hardware no habilitado (CONFIG_USE_STM32_HAL_JPEG=n)");
	return -ENODEV;
}

int video_decode_frame(const uint8_t *jpeg_data, uint32_t jpeg_size,
		       uint8_t *rgb565_out, uint32_t *width, uint32_t *height)
{
	(void)jpeg_data; (void)jpeg_size; (void)rgb565_out;
	(void)width; (void)height;
	return -ENODEV;
}

bool video_decoder_is_ready(void)
{
	return false;
}

#endif /* CONFIG_USE_STM32_HAL_JPEG */

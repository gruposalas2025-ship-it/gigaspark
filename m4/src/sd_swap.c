/*
 * Gigaspark OS - SD Swap Implementation
 * Simulated swap to SD card via RAM buffer
 *
 * The Arduino Giga R1's SD card is connected to M7's SDMMC1,
 * so the M4 cannot access it directly. This module simulates
 * the swap using a RAM buffer, proving the 3-tier architecture.
 *
 * To use real SD: implement swap_init() to communicate with M7
 * via IPC for SD sector read/write operations.
 */

#include "sd_swap.h"
#include "ipc_protocol.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(sd_swap, CONFIG_LOG_DEFAULT_LEVEL);

/* Simulated SD sectors in RAM */
#define SD_SECTOR_SIZE      512
#define SD_MAX_SECTORS      64
#define SD_DATA_MAX         (SD_SECTOR_SIZE - 4)  /* 508 bytes usable per sector */

/* Swap table entry */
struct swap_entry {
	uint16_t handle;
	uint16_t sector;
	uint8_t  sectors_used;
	uint8_t  valid;
};

/* Sector header */
struct __packed swap_sector_header {
	uint16_t handle;
	uint16_t data_len;
};

/* Module state */
static uint8_t sd_sim[SD_MAX_SECTORS][SD_SECTOR_SIZE] __aligned(4);
static struct swap_entry swap_table[SD_MAX_SECTORS];
static uint16_t next_free_sector = 0;
static bool swap_ready = false;

int swap_init(void)
{
	memset(swap_table, 0, sizeof(swap_table));
	memset(sd_sim, 0, sizeof(sd_sim));
	next_free_sector = 0;
	swap_ready = true;

	LOG_INF("SD swap initialized (simulated, %d sectors x %d bytes)",
		SD_MAX_SECTORS, SD_SECTOR_SIZE);

	return 0;
}

int swap_is_available(void)
{
	return swap_ready ? 0 : -ENODEV;
}

static struct swap_entry *find_entry(uint16_t handle)
{
	for (int i = 0; i < SD_MAX_SECTORS; i++) {
		if (swap_table[i].valid && swap_table[i].handle == handle) {
			return &swap_table[i];
		}
	}
	return NULL;
}

static struct swap_entry *find_free_entry(void)
{
	for (int i = 0; i < SD_MAX_SECTORS; i++) {
		if (!swap_table[i].valid) {
			return &swap_table[i];
		}
	}
	return NULL;
}

int swap_write_blob(uint16_t handle, const uint8_t *data, uint16_t data_len)
{
	if (!swap_ready) {
		return -ENODEV;
	}

	/* Check if already exists -> overwrite */
	struct swap_entry *existing = find_entry(handle);
	if (existing) {
		uint16_t sector = existing->sector;
		uint8_t sectors_needed = (data_len + 4 + SD_DATA_MAX - 1) / SD_DATA_MAX;

		/* Write first sector with header */
		memset(sd_sim[sector], 0, SD_SECTOR_SIZE);
		struct swap_sector_header *hdr = (struct swap_sector_header *)sd_sim[sector];
		hdr->handle = handle;
		hdr->data_len = data_len;

		uint16_t to_copy = (data_len > SD_DATA_MAX) ? SD_DATA_MAX : data_len;
		memcpy(sd_sim[sector] + 4, data, to_copy);

		/* Write additional sectors */
		uint16_t offset = to_copy;
		for (uint8_t s = 1; s < sectors_needed && offset < data_len; s++) {
			memset(sd_sim[sector + s], 0, SD_SECTOR_SIZE);
			to_copy = (data_len - offset > SD_DATA_MAX)
				  ? SD_DATA_MAX : (data_len - offset);
			memcpy(sd_sim[sector + s], data + offset, to_copy);
			offset += to_copy;
		}

		existing->sectors_used = sectors_needed;
		LOG_INF("swap_write: handle=0x%04x sector=%u sectors=%u len=%u",
			handle, sector, sectors_needed, data_len);
		return sector;
	}

	/* Allocate new sectors */
	uint8_t sectors_needed = (data_len + 4 + SD_DATA_MAX - 1) / SD_DATA_MAX;

	if (next_free_sector + sectors_needed > SD_MAX_SECTORS) {
		LOG_WRN("swap_write: SD full, need %u sectors", sectors_needed);
		return -1;
	}

	struct swap_entry *entry = find_free_entry();
	if (!entry) {
		LOG_WRN("swap_write: swap table full");
		return -1;
	}

	uint16_t sector = next_free_sector;
	next_free_sector += sectors_needed;

	/* Write first sector with header */
	memset(sd_sim[sector], 0, SD_SECTOR_SIZE);
	struct swap_sector_header *hdr = (struct swap_sector_header *)sd_sim[sector];
	hdr->handle = handle;
	hdr->data_len = data_len;

	uint16_t to_copy = (data_len > SD_DATA_MAX) ? SD_DATA_MAX : data_len;
	memcpy(sd_sim[sector] + 4, data, to_copy);

	/* Write additional sectors */
	uint16_t offset = to_copy;
	for (uint8_t s = 1; s < sectors_needed && offset < data_len; s++) {
		memset(sd_sim[sector + s], 0, SD_SECTOR_SIZE);
		to_copy = (data_len - offset > SD_DATA_MAX)
			  ? SD_DATA_MAX : (data_len - offset);
		memcpy(sd_sim[sector + s], data + offset, to_copy);
		offset += to_copy;
	}

	entry->handle = handle;
	entry->sector = sector;
	entry->sectors_used = sectors_needed;
	entry->valid = 1;

	LOG_INF("swap_write: handle=0x%04x -> sector=%u sectors=%u len=%u",
		handle, sector, sectors_needed, data_len);
	return sector;
}

size_t swap_read_blob(uint16_t handle, uint8_t *buf, size_t buf_size)
{
	if (!swap_ready) {
		return 0;
	}

	struct swap_entry *entry = find_entry(handle);
	if (!entry) {
		LOG_WRN("swap_read: handle 0x%04x not in swap table", handle);
		return 0;
	}

	struct swap_sector_header *hdr =
		(struct swap_sector_header *)sd_sim[entry->sector];
	uint16_t data_len = hdr->data_len;

	if (data_len > buf_size) {
		LOG_WRN("swap_read: buffer too small (%zu < %u)", buf_size, data_len);
		return 0;
	}

	/* Copy first sector data */
	uint16_t to_copy = (data_len > SD_DATA_MAX) ? SD_DATA_MAX : data_len;
	memcpy(buf, sd_sim[entry->sector] + 4, to_copy);

	/* Read additional sectors */
	uint16_t offset = to_copy;
	for (uint8_t s = 1; s < entry->sectors_used && offset < data_len; s++) {
		to_copy = (data_len - offset > SD_DATA_MAX)
			  ? SD_DATA_MAX : (data_len - offset);
		memcpy(buf + offset, sd_sim[entry->sector + s], to_copy);
		offset += to_copy;
	}

	LOG_INF("swap_read: handle=0x%04x -> %u bytes from sector %u",
		handle, data_len, entry->sector);
	return data_len;
}

int swap_remove_blob(uint16_t handle)
{
	struct swap_entry *entry = find_entry(handle);
	if (!entry) {
		return -1;
	}

	LOG_INF("swap_remove: handle=0x%04x sector=%u", handle, entry->sector);

	/* Zero out sectors */
	for (uint8_t s = 0; s < entry->sectors_used; s++) {
		memset(sd_sim[entry->sector + s], 0, SD_SECTOR_SIZE);
	}

	/* Reclaim if last allocation */
	if (entry->sector + entry->sectors_used == next_free_sector) {
		next_free_sector = entry->sector;
	}

	entry->valid = 0;
	entry->handle = 0;
	entry->sector = 0;
	entry->sectors_used = 0;

	return 0;
}

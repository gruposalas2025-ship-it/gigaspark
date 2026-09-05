/*
 * Gigaspark OS - File System Manager Implementation
 * SD card access via SPI + FAT32 filesystem
 *
 * Mounts the SD card at /SD: and scans /apps/ directory
 * for .bin application files. Provides an API to list and
 * load apps for the M7 launcher.
 */

#include "fs_manager.h"
#include "ipc_protocol.h"

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(fs_manager, CONFIG_LOG_DEFAULT_LEVEL);

/* Mount point */
#define SD_MOUNT_POINT  "/SD:"

/* Apps directory */
#define APPS_DIR        "/SD:/apps"

/* Forward declaration */
static int fs_scan_apps(void);

/* Static app list */
static struct app_info app_list[APP_MAX_COUNT];
static int app_count = 0;
static bool fs_mounted = false;

int fs_init(void)
{
	int ret;

	memset(app_list, 0, sizeof(app_list));
	app_count = 0;

	/* Mount the SD card FAT32 filesystem */
	static struct fs_mount_t mount_point = {
		.type = FS_FATFS,
		.mnt_point = SD_MOUNT_POINT,
		.fs_data = NULL,
		.storage_dev = NULL,
		.flags = 0,
	};

	ret = fs_mount(&mount_point);
	if (ret < 0) {
		LOG_WRN("SD mount failed: %d (no SD card?)", ret);
		fs_mounted = false;
		return ret;
	}

	fs_mounted = true;
	LOG_INF("SD card mounted at %s", SD_MOUNT_POINT);

	/* Scan /apps/ directory for .bin files */
	ret = fs_scan_apps();
	if (ret < 0) {
		LOG_WRN("App scan failed: %d", ret);
	}

	return 0;
}

static int fs_scan_apps(void)
{
	int ret;
	struct fs_dir_t dir_entry;
	struct fs_dirent entry;

	if (!fs_mounted) {
		return -ENODEV;
	}

	memset(&dir_entry, 0, sizeof(dir_entry));
	app_count = 0;

	ret = fs_opendir(&dir_entry, APPS_DIR);
	if (ret < 0) {
		LOG_WRN("Cannot open %s: %d", APPS_DIR, ret);
		return ret;
	}

	LOG_INF("Scanning %s for apps...", APPS_DIR);

	while (1) {
		ret = fs_readdir(&dir_entry, &entry);
		if (ret < 0) {
			LOG_WRN("readdir error: %d", ret);
			break;
		}

		/* End of directory */
		if (entry.name[0] == '\0') {
			break;
		}

		/* Skip directories */
		if (entry.type != FS_DIR_ENTRY_FILE) {
			continue;
		}

		/* Check for .bin extension */
		size_t name_len = strlen(entry.name);
		if (name_len < 5) {
			continue;
		}

		if (strcmp(&entry.name[name_len - 4], ".bin") != 0) {
			continue;
		}

		/* Found a .bin file - add to app list */
		if (app_count >= APP_MAX_COUNT) {
			LOG_WRN("Too many apps, skipping %s", entry.name);
			break;
		}

		app_list[app_count].id = app_count;

		/* Copy name without .bin extension */
		size_t copy_len = name_len - 4;
		if (copy_len >= APP_NAME_MAX) {
			copy_len = APP_NAME_MAX - 1;
		}
		memcpy(app_list[app_count].name, entry.name, copy_len);
		app_list[app_count].name[copy_len] = '\0';

		LOG_INF("  App[%d]: %s", app_count, app_list[app_count].name);
		app_count++;
	}

	fs_closedir(&dir_entry);
	LOG_INF("Found %d app(s)", app_count);

	return 0;
}

int fs_get_app_count(void)
{
	return app_count;
}

int fs_get_app_list(struct app_info *out, int max)
{
	if (!fs_mounted || out == NULL || max <= 0) {
		return 0;
	}

	int count = (app_count < max) ? app_count : max;
	memcpy(out, app_list, count * sizeof(struct app_info));
	return count;
}

size_t fs_load_app(uint8_t id, uint8_t *buf, size_t buf_size)
{
	if (!fs_mounted || id >= app_count || buf == NULL || buf_size == 0) {
		return 0;
	}

	/* Build full path */
	char path[64];
	snprintf(path, sizeof(path), "%s/%s.bin", APPS_DIR, app_list[id].name);

	struct fs_file_t file;
	memset(&file, 0, sizeof(file));

	int ret = fs_open(&file, path, FS_O_READ);
	if (ret < 0) {
		LOG_ERR("Cannot open %s: %d", path, ret);
		return 0;
	}

	/* Get file size */
	struct fs_dirent entry;
	ret = fs_stat(path, &entry);
	if (ret < 0) {
		LOG_ERR("Cannot stat %s: %d", path, fs_close(&file));
		return 0;
	}

	size_t file_size = entry.size;
	if (file_size > buf_size) {
		LOG_WRN("File %s too large (%zu > %zu)", path, file_size, buf_size);
		file_size = buf_size;
	}

	/* Read the file */
	ssize_t bytes_read = fs_read(&file, buf, file_size);
	fs_close(&file);

	if (bytes_read < 0) {
		LOG_ERR("Read error for %s: %d", path, (int)bytes_read);
		return 0;
	}

	LOG_INF("Loaded %s: %d bytes", path, (int)bytes_read);
	return (size_t)bytes_read;
}

int fs_is_ready(void)
{
	return fs_mounted ? 0 : -ENODEV;
}

/*
 * Gigaspark OS - App Downloader Header
 * HTTP client for downloading apps from a local server
 */

#ifndef GIGASPARK_APP_DOWNLOADER_H
#define GIGASPARK_APP_DOWNLOADER_H

#include <stdint.h>
#include <stddef.h>

/* Download result codes */
#define DOWNLOAD_OK             0
#define DOWNLOAD_ERR_SOCKET    (-1)
#define DOWNLOAD_ERR_CONNECT   (-2)
#define DOWNLOAD_ERR_HTTP      (-3)
#define DOWNLOAD_ERR_TIMEOUT   (-4)
#define DOWNLOAD_ERR_IO        (-5)
#define DOWNLOAD_ERR_NO_SD     (-6)
#define DOWNLOAD_ERR_M4        (-7)

/*
 * Download an app from a remote HTTP server and save it to SD card.
 *
 * The app is downloaded in chunks and sent to M4 via IPC
 * for writing to /SD:/apps/<app_name>.bin.
 *
 * @param host       Hostname or IP of the HTTP server.
 * @param port       TCP port (typically 80).
 * @param url        URL path to the .bin file (e.g., "/apps/clicker.bin").
 * @param app_name   Name to save as on SD (e.g., "clicker.bin").
 * @return           DOWNLOAD_OK on success, negative error code.
 */
int download_app_from_url(const char *host, uint16_t port,
			  const char *url, const char *app_name);

/*
 * Download an app using a full URL string.
 *
 * Parses "http://host:port/path" format.
 *
 * @param full_url   Full URL string.
 * @param app_name   Name to save as on SD.
 * @return           DOWNLOAD_OK on success, negative error code.
 */
int download_app_from_url_string(const char *full_url, const char *app_name);

#endif /* GIGASPARK_APP_DOWNLOADER_H */

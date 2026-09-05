/*
 * Gigaspark OS - Launcher Header
 * Application launcher menu for LVGL display
 */

#ifndef GIGASPARK_LAUNCHER_H
#define GIGASPARK_LAUNCHER_H

#include <zephyr/ipc/ipc_service.h>

/*
 * Initialize the launcher.
 * Requests the app list from M4 via IPC and creates a button
 * menu on the LVGL display.
 *
 * @param ept  IPC endpoint for communication with M4.
 * @return     0 on success, negative error code on failure.
 */
int launcher_init(struct ipc_ept *ept);

/*
 * Handle an IPC response from M4.
 * Called by the main IPC received callback when a response arrives.
 *
 * @param resp  Pointer to the received IPC message.
 */
void launcher_handle_response(const void *resp);

#endif /* GIGASPARK_LAUNCHER_H */

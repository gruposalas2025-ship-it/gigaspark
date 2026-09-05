/*
 * Gigaspark OS - UI Manager Header
 * LVGL initialization and display/touch management
 */

#ifndef GIGASPARK_UI_MANAGER_H
#define GIGASPARK_UI_MANAGER_H

/*
 * Initialize the UI manager.
 * Sets up LVGL, display driver, touch input, and starts the
 * LVGL handler thread.
 *
 * @return  0 on success, negative error code on failure.
 */
int ui_init(void);

/*
 * Get a reference to the default LVGL display.
 * Must be called after ui_init().
 *
 * @return  Pointer to the LVGL display object.
 */
void *ui_get_display(void);

#endif /* GIGASPARK_UI_MANAGER_H */

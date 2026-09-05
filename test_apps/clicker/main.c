/*
 * Gigaspark OS - Clicker App
 * Interactive touch button demo
 *
 * This is a user app that runs on the M7 via the app_runner.
 * It uses the SDK API (gigaspark_api.h) for all interactions.
 *
 * Features:
 * - Creates a button on screen
 * - Counts touches
 * - Updates display with click count
 * - Demonstrates fault recovery (try accessing bad address)
 */

#include "../sdk/gigaspark_api.h"

/* Button state */
static int clicks = 0;
static char click_text[32] = "Clicks: 0";

/* Button area (centered on screen) */
#define BTN_X      160
#define BTN_Y      100
#define BTN_W      160
#define BTN_H      60
#define BTN_COLOR  GIGA_BLUE
#define BTN_TEXT_COLOR GIGA_WHITE
#define BG_COLOR   GIGA_BLACK

/* Font size for button text */
#define FONT_H  24

void app_main(void)
{
	giga_log_info("Clicker app starting...");

	/* Clear screen */
	giga_fill_rect(0, 0, GIGA_SCREEN_W, GIGA_SCREEN_H, BG_COLOR);

	/* Draw title */
	giga_draw_text(10, 10, "Gigaspark OS - Touch Demo", GIGA_CYAN);
	giga_draw_text(10, 30, "Touch the button to count!", GIGA_WHITE);

	/* Draw initial button */
	giga_fill_rect(BTN_X, BTN_Y, BTN_W, BTN_H, BTN_COLOR);
	giga_draw_rect(BTN_X, BTN_Y, BTN_W, BTN_H, GIGA_WHITE);
	giga_draw_text(BTN_X + 30, BTN_Y + 20, click_text, BTN_TEXT_COLOR);

	/* Update display */
	giga_update();

	giga_log_info("Clicker: Main loop starting");

	/* Main loop */
	while (1) {
		int tx, ty;

		/* Check touch */
		if (giga_get_touch(&tx, &ty)) {
			/* Check if touch is within button bounds */
			if (tx >= BTN_X && tx < BTN_X + BTN_W &&
			    ty >= BTN_Y && ty < BTN_Y + BTN_H) {
				clicks++;

				/* Update text */
				int len = 0;
				const char *prefix = "Clicks: ";
				for (const char *p = prefix; *p; p++) {
					click_text[len++] = *p;
				}

				/* Simple integer to string */
				char num[12];
				int n = clicks;
				int i = 0;

				if (n == 0) {
					num[i++] = '0';
				} else {
					char temp[12];
					int j = 0;
					while (n > 0) {
						temp[j++] = '0' + (n % 10);
						n /= 10;
					}
					for (int k = j - 1; k >= 0; k--) {
						num[i++] = temp[k];
					}
				}
				num[i] = '\0';

				/* Append number */
				for (int k = 0; k < i; k++) {
					click_text[len++] = num[k];
				}
				click_text[len] = '\0';

				/* Redraw */
				giga_fill_rect(BTN_X, BTN_Y, BTN_W, BTN_H, BTN_COLOR);
				giga_draw_rect(BTN_X, BTN_Y, BTN_W, BTN_H, GIGA_WHITE);

				/* Center text on button */
				int text_w = len * 8;
				int text_x = BTN_X + (BTN_W - text_w) / 2;
				giga_draw_text(text_x, BTN_Y + 20, click_text, BTN_TEXT_COLOR);

				giga_update();
				giga_log_info("Click! Total: %d", clicks);
			}
		}

		/* Small delay to prevent CPU hogging */
		giga_sleep_ms(50);
	}
}

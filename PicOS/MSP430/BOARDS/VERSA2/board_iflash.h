#include "board_pins.h"

#define	if_start_up	if_start_up_flash ()

static inline void if_start_up_flash () {

	int i;

	if (VERSA2_RESET_KEY_PRESSED) {
		for (i = 0; i < 4; i++) {
			leds (0, 1); leds (2, 1); leds (3, 1);
			mdelay (256);
			leds (0, 0); leds (2, 0); leds (3, 0);
			mdelay (256);
		}
		if_erase (-1);
		while (VERSA2_RESET_KEY_PRESSED);
	}
}

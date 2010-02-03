// LEDs are emulated as the three RF symbols on the LCD

#include "ez430_lcd.h"

#undef	LED0_ON
#undef	LED1_ON
#undef	LED2_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	LED2_OFF
#undef	leds_save
#undef	leds_off
#undef	leds_restore

// ============================================================================

#define	leds(a,b)	do { \
				if ((a) >= 0 && (a) <= 2) \
					ezlcd_item (LCD_ICON_RADIO0 + (a), \
					(b) == LED_OFF ? LCD_MODE_CLEAR : \
						((b) == LED_ON ? \
							LCD_MODE_SET : \
							LCD_MODE_BLINK)); \
			} while (0)

// We don't need these as our "LEDs" are low power and they don't go down on
// powerdown
#define	leds_save()	(0)
#define	leds_off()	CNOP
#define	leds_restore(w)	CNOP

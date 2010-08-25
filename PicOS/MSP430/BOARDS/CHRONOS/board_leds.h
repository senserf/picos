// LEDs are emulated as the three RF symbols on the LCD

#include "ez430_lcd.h"

// ============================================================================

#define	leds(a,b)	do { \
				if ((a) >= 0 && (a) <= 2) \
					ezlcd_item (LCD_ICON_RADIO0 + (a), \
					(b) == LED_OFF ? LCD_MODE_CLEAR : \
						((b) == LED_ON ? \
							LCD_MODE_SET : \
							LCD_MODE_BLINK)); \
			} while (0)

#define	leds_all(b)	do { \
			 if ((b) == LED_ON) { \
			  ezlcd_item (LCD_ICON_RADIO0 + 0, LCD_MODE_SET); \
			  ezlcd_item (LCD_ICON_RADIO0 + 1, LCD_MODE_SET); \
			  ezlcd_item (LCD_ICON_RADIO0 + 2, LCD_MODE_SET); \
			 } else if ((b) == LED_OFF) { \
			  ezlcd_item (LCD_ICON_RADIO0 + 0, LCD_MODE_CLEAR); \
			  ezlcd_item (LCD_ICON_RADIO0 + 1, LCD_MODE_CLEAR); \
			  ezlcd_item (LCD_ICON_RADIO0 + 2, LCD_MODE_CLEAR); \
			 } else { \
			  ezlcd_item (LCD_ICON_RADIO0 + 0, LCD_MODE_BLINK); \
			  ezlcd_item (LCD_ICON_RADIO0 + 1, LCD_MODE_BLINK); \
			  ezlcd_item (LCD_ICON_RADIO0 + 2, LCD_MODE_BLINK); \
			 } \
			} while (0)

#define	all_leds_blink	do { \
			    ezlcd_item (LCD_ICON_RADIO0 + 0, LCD_MODE_BLINK); \
			    ezlcd_item (LCD_ICON_RADIO0 + 1, LCD_MODE_BLINK); \
			    ezlcd_item (LCD_ICON_RADIO0 + 2, LCD_MODE_BLINK); \
			} while (0)

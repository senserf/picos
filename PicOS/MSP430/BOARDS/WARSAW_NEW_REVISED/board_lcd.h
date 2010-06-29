/*
 * Pin assignment (DOG-ME-162, ST7036)
 *
 *  ST7036		        MSP430
 *  =================================
 *  RS				P4.0		->
 *  CS				P4.4		->
 *  CLK				P4.5		->
 *  SI				P4.6 		->
 */

#include "pins.h"

#define	LCD_LINE_LENGTH		16
#define	LCD_N_LINES		2

#define	lcd_bring_up	do { \
				_BIS (P4OUT, 0x30); \
				_BIS (P4DIR, 0x71); \
				cswitch_on (CSWITCH_LCD); \
				mdelay (1); \
			} while (0)

#define	lcd_bring_down	do { \
				cswitch_off (CSWITCH_LCD); \
				mdelay (1); \
				_BIC (P4DIR, 0x71); \
			} while (0)

#define	lcd_start	_BIC (P4OUT, 0x10)
#define	lcd_stop	_BIS (P4OUT, 0x10)
#define	lcd_select_data	_BIS (P4OUT, 0x01)
#define	lcd_select_cmd	_BIC (P4OUT, 0x01)
#define	lcd_outh	_BIS (P4OUT, 0x40)
#define	lcd_outl	_BIC (P4OUT, 0x40)
#define	lcd_clkh	_BIS (P4OUT, 0x20)
#define	lcd_clkl	_BIC (P4OUT, 0x20)

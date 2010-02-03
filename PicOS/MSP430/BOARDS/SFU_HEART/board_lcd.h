/*
 * Pin assignment (DOG-ME-162, ST7036)
 *
 *  ST7036		        MSP430
 *  =================================
 *  CS				P5.0		->
 *  SI				P5.1 		->
 *  RS				P5.2		->
 *  CLK				P5.3		->
 */

#include "pins.h"

#define	LCD_LINE_LENGTH		16
#define	LCD_N_LINES		2

#define	lcd_bring_up	do { \
				_BIS (P5OUT, 0x09); \
				_BIS (P5DIR, 0x0F); \
			} while (0)

#define	lcd_bring_down	CNOP

#define	lcd_start	_BIC (P5OUT, 0x01)
#define	lcd_stop	_BIS (P5OUT, 0x01)
#define	lcd_select_data	_BIS (P5OUT, 0x04)
#define	lcd_select_cmd	_BIC (P5OUT, 0x04)
#define	lcd_outh	_BIS (P5OUT, 0x02)
#define	lcd_outl	_BIC (P5OUT, 0x02)
#define	lcd_clkh	_BIS (P5OUT, 0x08)
#define	lcd_clkl	_BIC (P5OUT, 0x08)

// Needed for DIAG_IMPLEMENTATION == 2
#include "lcd_st7036.h"

#define	lcd_diag_start		lcd_clear (0, 0)
#define	lcd_diag_wchar(c)	lcd_putchar (c)
#define	lcd_diag_wait		CNOP
#define	lcd_diag_stop		CNOP

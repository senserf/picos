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

#include "board_pins.h"

#define	LCD_LINE_LENGTH		16
#define	LCD_N_LINES		2

#define	lcd_ini_regs	do { \
				_BIS (P5OUT, 0x09); \
				_BIS (P5DIR, 0x0F); \
			} while (0)

#define	lcd_start	_BIC (P5OUT, 0x01)
#define	lcd_stop	_BIS (P5OUT, 0x01)
#define	lcd_select_data	_BIS (P5OUT, 0x04)
#define	lcd_select_cmd	_BIC (P5OUT, 0x04)
#define	lcd_outh	_BIS (P5OUT, 0x02)
#define	lcd_outl	_BIC (P5OUT, 0x02)
#define	lcd_clkh	_BIS (P5OUT, 0x08)
#define	lcd_clkl	_BIC (P5OUT, 0x08)

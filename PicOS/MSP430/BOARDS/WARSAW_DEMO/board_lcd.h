/*
 * Pin assignment (DOG-ME-162, ST7036)
 *
 *  ST7036		        MSP430
 *  =================================
 *  RS				P6.3		->
 *  CS				P6.4		->
 *  CLK				P6.5		->
 *  SI				P6.6 		->
 */

#include "board_pins.h"

#define	LCD_LINE_LENGTH		16
#define	LCD_N_LINES		2

#define	lcd_ini_regs	do { \
				_BIS (P6OUT, 0x30); \
				_BIS (P6DIR, 0x78); \
			} while (0)

#define	lcd_start	_BIC (P6OUT, 0x10)
#define	lcd_stop	_BIS (P6OUT, 0x10)
#define	lcd_select_data	_BIS (P6OUT, 0x08)
#define	lcd_select_cmd	_BIC (P6OUT, 0x08)
#define	lcd_outh	_BIS (P6OUT, 0x40)
#define	lcd_outl	_BIC (P6OUT, 0x40)
#define	lcd_clkh	_BIS (P6OUT, 0x20)
#define	lcd_clkl	_BIC (P6OUT, 0x20)

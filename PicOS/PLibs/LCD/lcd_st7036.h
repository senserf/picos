#ifndef	__pg_lcd_st036_h__
#define	__pg_lcd_st036_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ lcd_st7036.c

void	lcd_on (word);
void	lcd_off ();
void	lcd_clear (word, word);
void	lcd_write (word, const char*);
void	lcd_putchar (char);
void	lcd_setp (word);

#define	LCD_CURSOR_ON		0x0001
#define	LCD_N_CHARS		(LCD_LINE_LENGTH * LCD_N_LINES)

#endif

#ifndef	__pg_ez430_lcd_h__
#define __pg_ez430_lcd_h__

// Headers for accessing the LCD on ez430_chronos

//+++ "ez430_lcd.c"

// ============================================================================
// Object ordinals ============================================================
// ============================================================================

// Line1
#define LCD_SYMB_AM				0
#define LCD_SYMB_PM				1
#define LCD_SYMB_ARROW_UP			2
#define LCD_SYMB_ARROW_DOWN			3
#define LCD_SYMB_PERCENT			4

// Symbols for Line2
#define LCD_SYMB_TOTAL				5
#define LCD_SYMB_AVERAGE			6
#define LCD_SYMB_MAX				7
#define LCD_SYMB_BATTERY			8

// Units for Line1
#define LCD_UNIT_L1_FT				9
#define LCD_UNIT_L1_K				10
#define LCD_UNIT_L1_M				11
#define LCD_UNIT_L1_I				12
#define LCD_UNIT_L1_PER_S			13
#define LCD_UNIT_L1_PER_H			14
#define LCD_UNIT_L1_DEGREE			15

// Units for Line2
#define LCD_UNIT_L2_KCAL			16
#define LCD_UNIT_L2_KM				17
#define LCD_UNIT_L2_MI				18

// Icons
#define LCD_ICON_HEART				19
#define LCD_ICON_STOPWATCH			20
#define LCD_ICON_RECORD				21
#define LCD_ICON_ALARM				22
#define LCD_ICON_RADIO0				23
#define LCD_ICON_RADIO1				24
#define LCD_ICON_RADIO2				25

// Line1 objects
#define LCD_SEG_L1_COL				26
#define LCD_SEG_L1_DP1				27
#define LCD_SEG_L1_DP0				28

// Line2 objects
#define LCD_SEG_L2_COL1				29
#define LCD_SEG_L2_COL0				30
#define LCD_SEG_L2_DP				31

#define	LCD_SEGMENTS_START			32

// Line 1
#define LCD_SEG_L1_0				32
#define LCD_SEG_L1_1				33
#define LCD_SEG_L1_2				34
#define LCD_SEG_L1_3				35

// Line 2
#define LCD_SEG_L2_0				36
#define LCD_SEG_L2_1				37
#define LCD_SEG_L2_2				38
#define LCD_SEG_L2_3				39
#define LCD_SEG_L2_4				40
#define LCD_SEG_L2_5				41

#define	LCD_SEGMENTS_END			41

#define	LCD_MODE_CLEAR				0x200
#define	LCD_MODE_BLINK				0x100
#define	LCD_MODE_SET				0x000

// ============================================================================

void ezlcd_brate (byte);
void ezlcd_item (word, word);
void ezlcd_init ();

#define	ezlcd_on()	LCDBCTL0 |= LCDON
#define	ezlcd_off()	LCDBCTL0 &= ^LCDON


#endif

#include "sysio.h"
#include "ez430_lcd.h"
#include "lcd_sys.h"

// ============================================================================

typedef struct {
//
// Describes one object
//
	byte offset, mask;

} dobject_t;

#define	LCD_N_OBJECTS	42

static const dobject_t dobjects [LCD_N_OBJECTS] = {

	{  0, 0x03 },	// AM
	{  0, 0x01 },	// PM
	{  0, 0x04 },	// ARR UP
	{  0, 0x08 },	// ARR DN
	{  4, 0x10 },	// PERC

	{ 10, 0x80 },	// TOTAL
	{  9, 0x80 },	// AVERAGE
	{  7, 0x80 },	// MAX
	{  6, 0x80 },	// BATT

	{  4, 0x20 },	// FT
	{  4, 0x40 },	// K
	{  6, 0x02 },	// M
	{  6, 0x01 },	// I
	{  4, 0x08 },	// /S
	{  6, 0x04 },	// /H
	{  4, 0x02 },	// DEGREE

	{  6, 0x10 },	// KCAL
	{  6, 0x20 },	// KM
	{  6, 0x40 },	// MI

	{  1, 0x08 },	// HEART
	{  2, 0x08 },	// STOPW
	{  0, 0x80 },	// REC
	{  3, 0x08 },	// ALARM
	{  4, 0x08 },	// RADIO0
	{  5, 0x08 },	// RADIO1
	{  6, 0x08 },	// RADIO2

	{  0, 0x20 },	// SEG L1 COL
	{  0, 0x40 },	// SEG L1 DP1
	{  4, 0x04 },	// SEG L1 DP0

	{  0, 0x10 },	// SEG L2 COL1
	{  4, 0x01 },	// SEG L2 COL0
	{  8, 0x80 },	// SEG L2 DP

	{  5, 0xF7 },	// SEG L1 0
	{  3, 0xF7 },	// SEG L1 1
	{  2, 0xF7 },	// SEG L1 2
	{  1, 0xF7 },	// SEG L1 3

	{  7, 0x7F },	// SEG L2 0
	{  8, 0x7F },	// SEG L2 1
	{  9, 0x7F },	// SEG L2 2
	{ 10, 0x7F },	// SEG L2 3
	{ 11, 0x7F },	// SEG L2 4
	{ 11, 0x80 } 	// SEG L2 5
};

// Segment names for character objects (this, as well as ccodes [font] has been
// copied from SportsWatch)
#define SEG_A                	0x10
#define SEG_B                	0x20
#define SEG_C                	0x40
#define SEG_D                	0x80
#define SEG_E                	0x04
#define SEG_F                	0x01
#define SEG_G                	0x02

static const ccodes [] = {

  SEG_A+SEG_B+SEG_C+SEG_D+SEG_E+SEG_F      ,     // "0"
        SEG_B+SEG_C                        ,     // "1"
  SEG_A+SEG_B+      SEG_D+SEG_E+      SEG_G,     // "2"
  SEG_A+SEG_B+SEG_C+SEG_D+            SEG_G,     // "3"
        SEG_B+SEG_C+            SEG_F+SEG_G,     // "4"
  SEG_A+      SEG_C+SEG_D+      SEG_F+SEG_G,     // "5"
  SEG_A+      SEG_C+SEG_D+SEG_E+SEG_F+SEG_G,     // "6"
  SEG_A+SEG_B+SEG_C                        ,     // "7"
  SEG_A+SEG_B+SEG_C+SEG_D+SEG_E+SEG_F+SEG_G,     // "8"
  SEG_A+SEG_B+SEG_C+SEG_D+      SEG_F+SEG_G,     // "9"
  0                                        ,     // " "
  0                                        ,     // " "
  0                                        ,     // " "
  0                                        ,     // " "
  0                                        ,     // " "
                   SEG_D+SEG_E+       SEG_G,     // "c"
  0                                        ,     // " "
  SEG_A+SEG_B+SEG_C+      SEG_E+SEG_F+SEG_G,     // "A"
              SEG_C+SEG_D+SEG_E+SEG_F+SEG_G,     // "b"
  SEG_A+            SEG_D+SEG_E+SEG_F      ,     // "C"
        SEG_B+SEG_C+SEG_D+SEG_E+      SEG_G,     // "d"
  SEG_A+           +SEG_D+SEG_E+SEG_F+SEG_G,     // "E"
  SEG_A+                  SEG_E+SEG_F+SEG_G,     // "F"
  SEG_A+SEG_B+SEG_C+SEG_D+      SEG_F+SEG_G,     // "g"
        SEG_B+SEG_C+      SEG_E+SEG_F+SEG_G,     // "H"
                          SEG_E+SEG_F      ,     // "I"
  SEG_A+SEG_B+SEG_C+SEG_D                  ,     // "J"
                    SEG_D+SEG_E+SEG_F+SEG_G,     // "k"
                    SEG_D+SEG_E+SEG_F      ,     // "L"
  SEG_A+SEG_B+SEG_C+      SEG_E+SEG_F      ,     // "M"
              SEG_C+      SEG_E+      SEG_G,     // "n"
              SEG_C+SEG_D+SEG_E+      SEG_G,     // "o"
  SEG_A+SEG_B+            SEG_E+SEG_F+SEG_G,     // "P"
  SEG_A+SEG_B+SEG_C+SEG_D+SEG_E+SEG_F      ,     // "Q"
                          SEG_E+      SEG_G,     // "r"
  SEG_A+      SEG_C+SEG_D+      SEG_F+SEG_G,     // "S"
                    SEG_D+SEG_E+SEG_F+SEG_G,     // "t"
              SEG_C+SEG_D+SEG_E            ,     // "u"
              SEG_C+SEG_D+SEG_E            ,     // "u"
                                      SEG_G,     // "-"
        SEG_B+SEG_C+     +SEG_E+SEG_F+SEG_G,     // "X"
        SEG_B+SEG_C+SEG_D+      SEG_F+SEG_G,     // "Y"
  SEG_A+SEG_B+      SEG_D+SEG_E+      SEG_G      // "Z"

};

// ============================================================================

static inline byte code_char (word ch) {

	if (ch == 0x2d)
		// Exception - not in set
		return 0x02;
	ch = (ch & 0xff) - (word) '0';
	return ch >= sizeof (ccodes) ? 0 : ccodes [ch];
}

// ============================================================================

void ezlcd_init () {

	// Clear entire display memory
	LCDBMEMCTL |= LCDCLRBM + LCDCLRM;

	// LCD_FREQ = ACLK/16/8 = 256Hz 
	// Frame frequency = 256Hz/4 = 64Hz, LCD mux 4, LCD on
	LCDBCTL0 = (LCDDIV0 + LCDDIV1 + LCDDIV2 + LCDDIV3) |
		(LCDPRE0 + LCDPRE1) | LCD4MUX | LCDON;

	// LCB_BLK_FREQ = ACLK/8/4096 = 1Hz
	LCDBBLKCTL = LCDBLKPRE0 | LCDBLKPRE1 | LCDBLKDIV0 | LCDBLKDIV1 |
		LCDBLKDIV2 | LCDBLKMOD0; 

	// Activate LCD output
	LCDBPCTL0 = 0xFFFF;	// Select LCD segments S0-S15
	LCDBPCTL1 = 0x00FF;	// Select LCD segments S16-S22

	// Charge pump voltage generated internally;
	// internal bias (V2-V4) generation
	// LCDBVCTL = LCDCPEN | VLCD_2_72;
}

void ezlcd_item (word it, word mode) {
//
// Display an item
//
	byte *ma, *mb;
	byte ms;

	if (it > LCD_N_OBJECTS)
		// Should we abort?
		return;

	ma = LCD_MEM_BASE + dobjects [it] . offset;
	mb = LCD_BLN_BASE + dobjects [it] . offset;
	ms = dobjects [it] . mask;

	// Start by clearing the whole thing
	*ma &= ~ms;
	*mb &= ~ms;

	if (mode >= LCD_MODE_CLEAR)
		// Erase -> nothing more to do
		return;

	// We will display the thing, check if a character item
	if (it >= LCD_SEGMENTS_START) {
		// Display character
		ms = code_char (mode);
		if (it >= LCD_SEG_L2_0) {
			if (it == LCD_SEG_L2_5 && ms != 0)
				// There is just one segment in this one
				ms = 0x80;
			else
				// Swap nibbles
				ms = (ms << 4) | ((ms & 0xf0) >> 4);
		}
	}
	*ma |= ms;

	if (mode >= LCD_MODE_BLINK)
		// Set blinking
		*mb |= ms;
}

void ezlcd_brate (byte rate) {
//
// Set blink rate (3 most significant bits)
//
	LCDBBLKCTL = (LCDBBLKCTL & 0x1F) | (rate << 5);
}

#if DIAG_MESSAGES
#if DIAG_IMPLEMENTATION == 2

// Poor man's diag on the LCD

#define	N_DIAG_CHARS	9

static const byte txline [N_DIAG_CHARS] = { 
					LCD_SEG_L1_3,
					LCD_SEG_L1_2,
					LCD_SEG_L1_1,
					LCD_SEG_L1_0,

					LCD_SEG_L2_4,
					LCD_SEG_L2_3,
					LCD_SEG_L2_2,
					LCD_SEG_L2_1,
					LCD_SEG_L2_0
				};
byte	dptr;

void ezlcd_diag_start () {

	word i;

	// This is idempotent
	ezlcd_init ();

	// Erase all line segments
	for (i = 0; i <= LCD_SEGMENTS_END; i++)
		ezlcd_item (i, LCD_MODE_CLEAR);

	dptr = 0;

	ezlcd_item (LCD_ICON_HEART, LCD_MODE_BLINK);
}

void ezlcd_diag_char (char c) {

	if (dptr >= N_DIAG_CHARS)
		return;

	if (c >= 'a' && c <= 'z')
		c -= ('a' - 'A');

	ezlcd_item (txline [dptr], (word) (c) | LCD_MODE_SET);
	dptr++;
	mdelay (256);
}

#endif
#endif

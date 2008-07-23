/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "lcdg_n6100p.h"

// Controller commands
#define	LNOP		0x00 // nop
#define	SWRESET		0x01 // software reset
#define	BSTROFF		0x02 // booster voltage OFF
#define	BSTRON		0x03 // booster voltage ON
#define	RDDIDIF		0x04 // read display identification
#define	RDDST		0x09 // read display status
#define	SLEEPIN		0x10 // sleep in
#define	SLEEPOUT	0x11 // sleep out
#define	PTLON		0x12 // partial display mode
#define	NORON		0x13 // display normal mode
#define	INVOFF		0x20 // inversion OFF
#define	INVON		0x21 // inversion ON
#define	DALO		0x22 // all pixel OFF
#define	DAL		0x23 // all pixel ON
#define	SETCON		0x25 // write contrast
#define	DISPOFF		0x28 // display OFF
#define	DISPON		0x29 // display ON
#define	CASET		0x2A // column address set
#define	PASET		0x2B // page address set
#define	RAMWR		0x2C // memory write
#define	RGBSET		0x2D // colour set
#define	PTLAR		0x30 // partial area
#define	VSCRDEF		0x33 // vertical scrolling definition
#define	TEOFF		0x34 // test mode
#define	TEON		0x35 // test mode
#define	MADCTL		0x36 // memory access control
#define	SEP		0x37 // vertical scrolling start address
#define	IDMOFF		0x38 // idle mode OFF
#define	IDMON		0x39 // idle mode ON
#define	COLMOD		0x3A // interface pixel format
#define	SETVOP		0xB0 // set Vop
#define	BRS		0xB4 // bottom row swap
#define	TRS		0xB6 // top row swap
#define	DISCTR		0xB9 // display control
#define	DOR		0xBA // data order
#define	TCDFE		0xBD // enable/disable DF temperature compensation
#define	TCVOPE		0xBF // enable/disable Vop temp comp
#define	EC		0xC0 // internal or external oscillator
#define	SETMUL		0xC2 // set multiplication factor
#define	TCVOPAB		0xC3 // set TCVOP slopes A and B
#define	TCVOPCD		0xC4 // set TCVOP slopes c and d
#define	TCDF		0xC5 // set divider frequency
#define	DF8COLOR	0xC6 // set divider frequency 8-color mode
#define	SETBS		0xC7 // set bias system
#define	RDTEMP		0xC8 // temperature read back
#define	NLI		0xC9 // n-line inversion
#define	RDID1		0xDA // read ID1
#define	RDID2		0xDB // read ID2
#define	RDID3		0xDC // read ID3

// A few standard colors (12-bit)
#define	WHITE_12		0xFFF
#define	BLACK_12		0x000
#define	RED_12			0xF00
#define	GREEN_12		0x0F0
#define	BLUE_12			0x00F

// And 8 bit versions
#define	WHITE_8			0xFF
#define	BLACK_8			0x00
#define	RED_8			0xE0
#define	GREEN_8			0x1C
#define	BLUE_8			0x03

static const word ctable12 [] = {
    WHITE_12, BLACK_12, RED_12, GREEN_12, BLUE_12
};

static const byte ctable8 [] = {
    WHITE_8, BLACK_8, RED_8, GREEN_8, BLUE_8
};

static const byte rgbtable [] = {
	// RGB table for 8bpp
		0,	 // red 000 value
		2,	 // red 001 value
		5,	 // red 010 value
		7,	 // red 011 value
		9,	 // red 100 value
		11,	 // red 101 value
		14,	 // red 110 value
		16,	 // red 111 value
		0,	 // green 000 value
		2,	 // green 001 value
		5,	 // green 010 value
		7,	 // green 011 value
		9,	 // green 100 value
		11,	 // green 101 value
		14,	 // green 110 value
		16,	 // green 111 value
		0,	 // blue 000 value
		6,	 // blue 001 value
		11,	 // blue 010 value
		15	 // blue 011 value
};

#define	N_COLORS	(sizeof (rgbtable) / sizeof (word))

// ============================================================================

static byte X_org  = LCDG_XOFF,
	    Y_org  = LCDG_YOFF,
	    X_last = LCDG_MAXX,
	    Y_last = LCDG_MAXY,
	    ColF   = 1,		// Foreground color
	    ColB   = 0,		// Background color
	    D_mod  = 0;

// ============================================================================

static void sc (byte stuff) {

	int i;

	nlcd_data_down;
	nlcd_clk_up;
	nlcd_clk_down;

	for (i = 0; i < 8; i++) {
		if (stuff & 0x80)
			nlcd_data_up;
		else
			nlcd_data_down;
		nlcd_clk_up;
		nlcd_clk_down;
		stuff <<= 1;
	}
}

void sd (byte stuff) {

	int i;

	nlcd_data_up;
	nlcd_clk_up;
	nlcd_clk_down;

	for (i = 0; i < 8; i++) {
		if (stuff & 0x80)
			nlcd_data_up;
		else
			nlcd_data_down;
		nlcd_clk_up;
		nlcd_clk_down;
		stuff <<= 1;
	}
}

// ============================================================================

void lcdg_reset () {

	word i;

	nlcd_cs_down;
	sc (SLEEPOUT);
	// Color Interface Pixel Format (command 0x3A)
	sc (COLMOD);
	sd (0x03);	// 0x03 = 12 bits-per-pixel
	sc (MADCTL);
	sd (0x88);	// Mirror y, rgb
	sc (SETCON);	// Contrast
	sd (0x37);
	mdelay (10);
	// Set up the RGB table for 8bpp display
	sc (RGBSET);
	for (i = 0; i < sizeof (rgbtable); i++)
		sd (rgbtable [i]);
	nlcd_cs_up;
}

void zz_lcdg_init () {

	nlcd_clk_down;
	nlcd_cs_up;
	mdelay (1);
	nlcd_rst_down;
	mdelay (20);
	nlcd_rst_up;
	mdelay (20);

	lcdg_reset ();
}

void lcdg_cmd (byte cmd, byte *arg, byte n) {
//
// Issue a raw command (debugging only)
//
	if (cmd == BNONE) {
		lcdg_reset ();
		return;
	}
	nlcd_cs_down;
	sc (cmd);
	while (n--)
		sd (*arg++);
	nlcd_cs_up;
}

void lcdg_on (byte con) {
//
	nlcd_cs_down;
	// sc (DISPOFF);
	// mdelay (10);
	if (con) {
		sc (SETCON);	// Change contrast
		sd (con);
		// mdelay (10);
	}
	sc (DISPON);
	// mdelay (10);
	nlcd_cs_up;
}

void lcdg_off () {
	nlcd_cs_down;
	sc (DISPOFF);
	nlcd_cs_up;
}

void lcdg_set (byte x0, byte y0, byte x, byte y, byte flags) {
//
// Set up for rendering or filling
//
	X_org = x0 + LCDG_XOFF;
	Y_org = y0 + LCDG_YOFF;

	if (x == 0)
		X_last = LCDG_MAXXP;
	else
		X_last = X_org + x - 1;

	if (y == 0)
		Y_last = LCDG_MAXYP;
	else
		Y_last = Y_org + y - 1;

	if (X_last < X_org || Y_last < Y_org || X_last > LCDG_MAXXP ||
		Y_last > LCDG_MAXYP)
			syserror (EREQPAR, "lcdg_set");

	D_mod = flags;

	nlcd_cs_down;
	sc (COLMOD);
	sd ((flags & LCDGF_8BPP) ? 0x02 : 0x03);
	nlcd_cs_up;
}

void lcdg_setc (byte bg, byte fg) {
//
// Set background and foreground color
//
	if (bg >= N_COLORS)
		bg = COLOR_WHITE;
	if (fg >= N_COLORS)
		fg = COLOR_BLACK;
	ColF = fg;
	ColB = bg;
}

void lcdg_clear (byte col) {
//
// Clear the 'set' rectangle
//
	word w, h;
	byte a, b, c;

	if (col >= N_COLORS)
		col = ColB;

	nlcd_cs_down;
	// The row
	sc (PASET);
	sd (Y_org);
	sd (Y_last);
	sc (CASET);
	sd (X_org);
	sd (X_last);

	sc (RAMWR);

	w = (Y_last - Y_org + 1) * (X_last - X_org + 1);

	if ((D_mod & LCDGF_8BPP)) {
		a = ctable8 [col];
		while (w--)
			sd (a);
	} else {
		w = (w + 1) >> 1;
		// 12 bit colors
		a = (byte)(ctable12 [col] >> 4);
		b = (byte)((ctable12 [col] << 4) | ((ctable12 [col] >> 8) &
			0xf));
		c = (byte)ctable12 [col];
		while (w--) {
			sd (a); sd (b); sd (c);
		}
	}
	nlcd_cs_up;
}

void lcdg_render (byte cs, byte rs, byte *pix, word n) {
//
// Display pixels; note, we can do it faster if we know that the pixels
// are coming in order. This version is supposed to work with individual
// chunks, possibly containing holes, arriving from the network. I thought
// we would need a second version of this function for displaying images
// directly from EEPROM, but, as it turns out, images stored in EEPROM may
// also arrive from the network, and their chunks need not be ordered.
//
	word ys, xs;
	byte a, b, c, pturn;

	if (n == 0)
		return;

	// Actual first row of the chunk
	ys = (word) Y_org + rs;
	if (ys > Y_last)
		// Out of screen, illegal
		return;

	// This is where the first pixel goes
	xs = (word) X_org + cs;
	if (xs > X_last)
		// This is illegal as well, ignore the whole chunk
		return;

	// Pixel turn
	pturn = 0;

	nlcd_cs_down;

	// Special treatment for the first row
	if (cs) {

		// Row
		sc (PASET);
		sd ((byte)ys);	// Single row only
		sd ((byte)ys);
		sc (CASET);
		sd ((byte)xs);
		sd (X_last);
		sc (RAMWR);

		// The max number of pixels to be written to device in this row
		xs = X_last - xs + 1;

		if (xs > n)
			// Do not exceed the specified number of pixels
			xs = n;

		n -= xs;

		if (D_mod & LCDGF_8BPP) {
			// The easy way
			while (xs--)
				sd (*pix++);
		} else {
			// 12bpp
			while (xs--) {
				if (pturn) {
					// Write three bytes on even pixel
					sd (*pix++); sd (*pix++); sd (*pix++);
					pturn = 0;
				} else {
					// And skip the odd one
					pturn++;
				}
			}
			if (pturn) {
				// We had an odd number of pixels
				sd (*pix++);
				// Only 1/2 of this one is used, which leaves
				// us in a desperate state. Note: this will be
				// avoided, if the row size is always even
				// (note that the chunks is an even number of
				// pixels)
				sd (*pix);
			}
		}
		// Next row
		if (n == 0 || ++ys > Y_last)
			goto Done;
	}

	// Continue with the remaining pixels
	sc (PASET);
	sd ((byte)ys);
	sd (Y_last);
	sc (CASET);
	sd (X_org);
	sd (X_last);
	sc (RAMWR);

	if (pturn) {
		// The tricky way (12bpp only)
		while (n--) {
			if (pturn) {
				// Skip even turns, we are out of phase
				pturn = 0;
			} else {
				pturn++;
				a = (*pix) << 4; pix++;
				a |= (*pix) >> 4;
				b = (*pix) << 4; pix++;
				b |= (*pix) >> 4;
				c = (*pix) << 4; pix++;
				c |= (*pix) >> 4;
				sd (a); sd (b); sd (c);
			}
		}

		if (pturn == 0) {
			a = (*pix) << 4; pix++;
			a |= (*pix) >> 4;
			b = (*pix) << 4; pix++;
			sd (a); sd (b);
		}

	} else if (D_mod & LCDGF_8BPP) {
		// The easiest of them all
		while (n--)
			sd (*pix++);
	} else {
		// Even 12 bpp case (we know pturn == 0)
		while (n--) {
			if (pturn) {
				// Write three bytes on even pixel
				sd (*pix++); sd (*pix++); sd (*pix++);
				pturn = 0;
			} else {
				// And skip the odd one
				pturn++;
			}
		}
		if (pturn) {
			sd (*pix++);
			sd (*pix);
		}
	}
			
Done:
	nlcd_cs_up;
}

#ifdef EEPROM_PRESENT

word lcdg_text (byte fn, const char *st) {
//
// Render text
//
	word fbase;
	byte *fbuf, *cp;
	byte fp [3], cx, cs, cm, rn, cn, a, b, c;

	// This is the largest character block at present
	if ((fbuf = (byte*)umalloc (32)) == NULL)
		// Failure
		return 1;

	// Read the font block from page zero
	fbuf [0] = 0;
	ee_read ((lword)0, fbuf, 32);
	if (((word*)fbuf) [0] != 0x7f01) {
		// This is not a font page
ERet:
		ufree (fbuf);
		return 2;
	}

	// Offset to our font
	
	if (((word*)fbuf) [1] <= fn)
		// Not that many fonts
		goto ERet;

	fbase = ((word*)fbuf) [2 + fn];

	// Read the font header
	ee_read ((lword)fbase, fp, 3); // width, height, blocksize

	// Skip the first block
	fbase += fp [2];

	// This can only be 8 or 16 (at least for now): prepare the
	// shift count for block size
	cs = (fp [2] == 8) ? 3 : 4;

	cx = X_org;

	while (*st != '\0') {

		// Loop for next character

		if (cx + fp [0] > X_last)
			// No more
			break;

		rn = (byte)(*st);
		if (rn < 0x20 || rn > 0x7f)
			// blank
			rn = 0;
		else
			rn -= 0x20;

		// Read the block
		ee_read ((lword)(fbase + ((word)rn << cs)), fbuf, fp [2]);
		
		nlcd_cs_down;
		sc (PASET);
		sd (Y_org);
		sd (Y_org + fp [1] - 1);
		sc (CASET);
		sd (cx);
		sd (cx + fp [0] - 1);
		sc (RAMWR);

		cp = fbuf + fp [1];
		for (rn = 0; rn < fp [1]; rn++) {
			cp--;
			cm = 0x80;
			if ((D_mod & LCDGF_8BPP)) {
				for (cn = 0; cn < fp [0]; cn++) {
					a = ctable8 [(*cp & cm) ? ColF : ColB];
					sd (a);
					cm >>= 1;
				}
			} else {
				for (cn = 0; cn < fp [0]; cn += 2) {
					b = (*cp & cm) ? ColF : ColB;
					cm >>= 1;
					c = (*cp & cm) ? ColF : ColB;
					cm >>= 1;
					a = (byte)(ctable12 [b] >> 4);
					b = (byte)((ctable12 [b] << 4) |
						(ctable12 [c] >> 8));
					c = (byte) ctable12 [c];
					sd (a); sd (b); sd (c);
				}
			}
		}
		nlcd_cs_up;
		cx += fp [0];
		st++;
	}
	ufree (fbuf);
	return 0;
}

#endif

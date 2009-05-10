/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "pins.h"
#include "lcdg_n6100p.h"

#ifdef LCDG_FONT_BASE
// Need to access EEPROM
#include "storage.h"
#endif

#ifndef	LCDG_N6100P_EPSON
#define	LCDG_N6100P_EPSON	0
#endif

#if LCDG_N6100P_EPSON

// Line/column offsets
#define	LCDG_XOFF	0
#define	LCDG_YOFF	2

// ============================================================================
#define DISON     	0xAF // Display on 
#define DISOFF    	0xAE // Display off 
#define DISNOR    	0xA6 // Normal display 
#define DISINV    	0xA7 // Inverse display 
#define COMSCN    	0xBB // Common scan direction 
#define DISCTL    	0xCA // Display control 
#define SLPIN     	0x95 // Sleep in 
#define SLPOUT    	0x94 // Sleep out 
#define PASET     	0x75 // Page address set 
#define CASET     	0x15 // Column address set 
#define DATCTL    	0xBC // Data scan direction, etc. 
#define RGBSET8   	0xCE // 256-color position set 
#define RAMWR     	0x5C // Writing to memory 
#define RAMRD     	0x5D // Reading from memory 
#define PTLIN     	0xA8 // Partial display in 
#define PTLOUT    	0xA9 // Partial display out 
#define RMWIN     	0xE0 // Read and modify write 
#define RMWOUT    	0xEE // End 
#define ASCSET    	0xAA // Area scroll set 
#define SCSTART   	0xAB // Scroll start set 
#define OSCON     	0xD1 // Internal oscillation on 
#define OSCFF    	0xD2 // Internal oscillation off 
#define PWRCTR    	0x20 // Power control 
#define VOLCTR    	0x81 // Electronic volume control 
#define VOLUP     	0xD6 // Increment electronic control by 1 
#define VOLDOWN   	0xD7 // Decrement electronic control by 1 
#define TMPGRD    	0x82 // Temperature gradient set 
#define EPCTIN    	0xCD // Control EEPROM 
#define EPCOUT    	0xCC // Cancel EEPROM control 
#define EPMWR     	0xFC // Write into EEPROM 
#define EPMRD     	0xFD // Read from EEPROM 
#define EPSRRD1   	0x7C // Read register 1 
#define EPSRRD2   	0x7D // Read register 2 
#define NOP       	0x25 // NOP instruction 

#else	/* PHILIPS */
// ============================================================================

#define	LCDG_XOFF	1
#define	LCDG_YOFF	1

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
#define	DALO		0x22 // all pixels OFF
#define	DAL		0x23 // all pixels ON
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

#endif	/* PHILIPS or EPSON */
// ============================================================================

#define	LCDG_MAXXP	(LCDG_XOFF+LCDG_MAXX)
#define	LCDG_MAXYP	(LCDG_YOFF+LCDG_MAXY)

// 12-bit color definitions 
#define WHITE         /* 0x000*/	0xFFF 
#define BLACK         /* 0xFFF*/	0x000 
#define RED           /* 0x0FF*/	0xF00 
#define GREEN         /* 0xF0F*/	0x0F0 
#define BLUE          /* 0xFF0*/	0x00F 
#define CYAN          /* 0xF00*/	0x0FF 
#define MAGENTA       /* 0x0F0*/	0xF0F 
#define YELLOW        /* 0x00F*/	0xFF0 
#define BROWN         /* 0x8DD*/	0xB22 
#define ORANGE        /* 0x05F*/	0xFA0 
#define PINK          /* 0x095*/	0xF6A 

static
#ifndef	LCDG_SETTABLE_CTABLE
const
#endif
word ctable12 [] = {
    WHITE, BLACK, RED, GREEN, BLUE, CYAN, MAGENTA, YELLOW, BROWN, ORANGE, PINK
};

#if 0
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
#endif

#define	N_COLORS	(sizeof (ctable12) / sizeof (word))

// ============================================================================

static byte X_org  = LCDG_XOFF,
	    Y_org  = LCDG_YOFF,
	    X_last = LCDG_MAXX,
	    Y_last = LCDG_MAXY,
	    ColF   = 0,		// Foreground color
	    ColB   = 0,		// Background color

// Preassembled standard colors (BG, FG) + crossovers F-B and B-F

	    KAB, KBB, KCB, KAF, KBF, KCF, KXF, KXB;

#ifdef LCDG_FONT_BASE

#ifndef	EEPROM_PRESENT
#error "LCDG_FONT_BASE requires EEPROM_PRESENT"
#endif

static byte fpar [4],		// Font parameters: cols, rows, bsize, bshift
	    fbuf [32];		// Font buffer

static word fbase;		// Font base offset in EEPROM

#endif /* LCDG_FONT_BASE */

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

static void sd (byte stuff) {

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

	nlcd_cs_down;

#if LCDG_N6100P_EPSON
// ==================

#if 0
	// This seems to be the default, so why wasting code

        sc (DISCTL); 
        sd (0x00); // P1: 0x00 = 2 divisions, switching period=8 (default) 
        sd (0x20); // P2: 0x20 = nlines/4 - 1 = 132/4 - 1 = 32) 
        sd (0x00); // P3: 0x00 = no inversely highlighted lines 
 
        // COM scan 
        sc (COMSCN); 
        sd (1);    // P1: 0x01 = Scan 1->80, 160<-81 
        // Internal oscilator ON 
        sc (OSCON); 

#endif
 
        // Sleep out 
        sc (SLPOUT); 
 
        // Power control 
        sc (PWRCTR); 
        sd (0x0f);   // reference voltage regulator on,
 
        // Inverse display (does nothing, not needed)
        // sc (DISINV); 
        // Data control 
        sc (DATCTL); 
        sd (0x00); // Do not invert anything, column address normal
        sd (0x00); // RGB sequence (default value) 
        sd (0x02); // Grayscale -> 16 (selects 12-bit color, type A) 
 
        // Voltage control (contrast setting) 
        sc (VOLCTR); 
        sd (37);   // Looks like a decent default
        sd (3);    // Resistance ratio - whatever that is

#else	/* PHILIPS */
// ==================

	sc (SLEEPOUT);
	// Color Interface Pixel Format (command 0x3A)
	sc (COLMOD);
	sd (0x03);	// 0x03 = 12 bits-per-pixel
	sc (MADCTL);
	sd (0x00);	// Don't mirror y, rgb
	sc (SETCON);	// Contrast
	sd (0x37);
	mdelay (10);

#endif
// ==================

	nlcd_cs_up;
	lcdg_setc (COLOR_BLACK, COLOR_WHITE);
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

void lcdg_cmd (byte cmd, const byte *arg, byte n) {
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

#if LCDG_N6100P_EPSON
// ==================

	if (con) {
		sc (VOLCTR);	// Change contrast
		sd (con);
		sd (3);
		// mdelay (10);
	}
	// Tune it later
	mdelay (10);
	sc (DISON);

#else	/* PHILIPS */
// =====================

	// sc (DISPOFF);
	// mdelay (10);
	if (con) {
		sc (SETCON);	// Change contrast
		sd (con);
		// mdelay (10);
	}
	sc (DISPON);
	// mdelay (10);
#endif
// =====================

	nlcd_cs_up;
}

void lcdg_off () {
	nlcd_cs_down;

#if LCDG_N6100P_EPSON
	sc (DISOFF);
#else
	sc (DISPOFF);
#endif
	nlcd_cs_up;
}

void lcdg_set (byte xl, byte yl, byte xh, byte yh) {
//
// Set up for rendering or filling
//
	X_org = xl + LCDG_XOFF;
	Y_org = yl + LCDG_YOFF;

	X_last = xh + LCDG_XOFF;
	Y_last = yh + LCDG_YOFF;

	if (X_last < X_org || Y_last < Y_org || X_last > LCDG_MAXXP ||
		Y_last > LCDG_MAXYP)
			syserror (EREQPAR, "lcdg_set");
}

void lcdg_get (byte *XL, byte *YL, byte *XH, byte *YH) {
//
// Get the current bounding rectangle
//
	if (XL != NULL) *XL = X_org;
	if (YL != NULL) *YL = Y_org;
	if (XH != NULL) *XH = X_last;
	if (YH != NULL) *YH = Y_last;
}

#ifdef LCDG_SETTABLE_CTABLE

void lcdg_setct (byte co, word val) {

	if (co < N_COLORS)
		ctable12 [co] = val;
}

#endif

void lcdg_setc (byte bg, byte fg) {
//
// Set background and foreground color
//
	word k;
	byte u, d;
	
	if (bg >= N_COLORS)
		bg = ColB;
	if (fg >= N_COLORS)
		fg = ColF;

	ColF = fg;
	k = ctable12 [ColF];
	KAF = (byte)( k >> 4 );
	KXF = (byte)( k << 4 );
	KXB = (byte)( k >> 8 );
	KBF = (byte)( KXF | KXB);
	KCF = (byte)  k;

	ColB = bg;
	k = ctable12 [ColB];
	KAB = (byte)( k >> 4 );
	u   = (byte)( k << 4 );
	d   = (byte)( k >> 8 );
	KBB = (u | d);
	KXF |= d;
	KXB |= u;
	KCB = (byte)  k;
}

void lcdg_clear () {
//
// Clear the 'set' rectangle
//
	word w;

	nlcd_cs_down;
	// The row
	sc (PASET);
	sd (Y_org);
	sd (Y_last);
	sc (CASET);
	sd (X_org);
	sd (X_last);

	sc (RAMWR);

	// The number of pixels
	w = ((Y_last - Y_org + 1) * (X_last - X_org + 1) + 1) >> 1;

	while (w--) {
		sd (KAB); sd (KBB); sd (KCB);
	}

	nlcd_cs_up;
}

void lcdg_render (byte cs, byte rs, const byte *pix, word n) {
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
		// Off screen, illegal
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
			// (note that the chunk is an even number of
			// pixels)
			sd (*pix);
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
	} else {
		// Even 12 bpp case (pturn == 0)
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

#ifdef LCDG_FONT_BASE

word lcdg_font (byte fn) {
//
// Set the font
//
	fbuf [0] = fpar [0] = fpar [1] = 0;
	// Read the header
	ee_read ((lword)LCDG_FONT_BASE, fbuf, 32);
	if (((word*)fbuf) [0] != 0x7f01)
		// This is not a font page
		return ERROR;

	// Offset to our font
	
	if (((word*)fbuf) [1] <= fn)
		return ERROR;

	fbase = ((word*)fbuf) [2 + fn];

	// Read the font header: width, height, blocksize
	ee_read ((lword)LCDG_FONT_BASE + fbase, fpar, 3);

	// Skip the first block
	fbase += fpar [2];

	// Shift count for font block
	fpar [3] = fpar [2] == 8 ? 3 : 4;

	return 0;
}

byte lcdg_cwidth () {
//
// Return character width (based on the current font)
//
	return fpar [0];
}

byte lcdg_cheight () {
//
// Character height
//
	return fpar [1];
}

word lcdg_sett (byte x, byte y, byte nc, byte nl) {
//
// Set text area: corner + number of columns, number of lines
//
	word x0, y0, x1, y1;

	if (fpar [0] == 0)
		// No font
		return ERROR;

	x0 = (word) x + LCDG_XOFF;
	x1 = x0 + (word) fpar [0] * nc - 1;
	y0 = (word) y + LCDG_YOFF;
	y1 = y0 + (word) fpar [1] * nl - 1;

	if (x1 < x0 || y1 < y0 || x1 > LCDG_MAXXP || y1 > LCDG_MAXYP)
		// Don't touch anything and return ERROR
		return ERROR;

	// Set the area
	X_org = x0;
	Y_org = y0;
	X_last = x1;
	Y_last = y1;

	return 0;
}

void lcdg_ec (byte cx, byte cy, byte nc) {
//
// Erase nc character positions starting from column cx, row cy
//
	word ex;

	if (fpar [0] == 0)
		return;

	cx = X_org + cx * fpar [0];
	if (cx > X_last)
		return;
	cy = Y_org + cy * fpar [1];
	if (cy > Y_last)
		return;

	ex = cx + nc * fpar [0] - 1;
	if (ex > X_last)
		ex = X_last;

	nlcd_cs_down;

	sc (PASET);
	sd (cy);
	sd (cy + fpar [1] - 1);
	sc (CASET);
	sd (cx);
	sd ((byte)ex);
	sc (RAMWR);

	ex = ((word) (fpar [1]) * (ex - cx + 1) + 1) >> 1;

	while (ex--) {
		sd (KAB); sd (KBB); sd (KCB);
	}

	nlcd_cs_up;
}
	
void lcdg_el (byte cy, byte nl) {
//
// Erase nl lines starting from line ro
//
	word ex;

	if (fpar [0] == 0)
		return;

	cy = Y_org + cy * fpar [1];
	if (cy > Y_last)
		return;

	nlcd_cs_down;
	sc (PASET);
	sd (cy);
	sd (Y_last);
	sc (CASET);
	sd (X_org);
	sd (X_last);
	sc (RAMWR);

	ex = ((word) (Y_last - cy + 1) * (X_last - X_org + 1) + 1) >> 1;

	while (ex--) {
		sd (KAB); sd (KBB); sd (KCB);
	}

	nlcd_cs_up;
}

void lcdg_wl (const char *st, word sh, byte cx, byte cy) {
//
// Write line in the text area starting from column cx, row cy; sh is
// the shift, i.e., how many initial characters from the line should be
// ignored (used for horizontal scrolling).
//
	byte *cp, rn, cn, cm;

	if (fpar [0] == 0)
		return;
	
	// Starting coordinates
	cx = X_org + cx * fpar [0];
	if (cx >= X_last)
		return;
	cy = Y_org + cy * fpar [1];

	// Ignore sh initial characters
	while (sh) {
		if (*st == '\0')
			return;
		st++;
		sh--;
	}

	while (*st != '\0') {

		// More characters in this line
		if (cx >= X_last)
			// No more characters will fit
			return;

		rn = (byte)(*st);
		if (rn < 0x20 || rn > 0x7f)
			// blank
			rn = 0;
		else
			rn -= 0x20;

		// Read the block from the font file
		ee_read ((lword)LCDG_FONT_BASE + fbase +
			(((word)rn << fpar [3])), fbuf, fpar [2]);
		nlcd_cs_down;
		sc (PASET);
		sd (cy);
		sd (cy + fpar [1] - 1);
		sc (CASET);
		sd (cx);
		sd (cx + fpar [0] - 1);
		sc (RAMWR);

		// Render the character
		cp = fbuf;
		for (rn = 0; rn < fpar [1]; rn++) {
			// Position mask
			cm = 6;
			for (cn = 0; cn < fpar [0]; cn += 2) {

				switch ((*cp >> cm) & 0x03) {

					case 0:		// BB

						sd (KAB); sd (KBB); sd (KCB);
						break;
	
					case 1:		// BF

						sd (KAB); sd (KXB); sd (KCF);
						break;

					case 2:		// FB

						sd (KAF); sd (KXF); sd (KCB);
						break;

					default:	// FF
						sd (KAF); sd (KBF); sd (KCF);
				}

				cm -= 2;
			}
			cp++;
		}
		cx += fpar [0];
		st++;
	}
	return;
}

#endif

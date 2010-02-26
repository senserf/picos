/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef __lcdg_n6100p_c__
#define __lcdg_n6100p_c__

#include "lcdg_n6100p_driver.h"

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

static const word ctable12 [] = {
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

#ifdef	__SMURPH__

// ============================================================================
// === VUEE ===================================================================
// ============================================================================

#define	nlcd_cs_down	CNOP

// Use this to flush pixel updates; also, use a special update to tell
// UDAEMON that that's the end
#define	nlcd_cs_up	update_flush ()
#define	_lcdgm_(a)	LCDG::m_ ## a

LCDG::LCDG () {

	// Socket only
	IN.init (XTRN_OMODE_SOCKET);

	memset (canvas, 0, sizeof (canvas));

	ONStat = NO;

	X_org = LCDG_XOFF;
	Y_org = LCDG_YOFF;
	X_last = LCDG_MAXX;
	Y_last = LCDG_MAXY;

	ColF = 0;
	ColB = 0;

	UHead = NULL;

	// This is the default contrast for Philips
	Contrast = 0x37;

	// No updates
	updp = 0;

	SPC = 0;
	LP = WNONE;
}

void LCDG::queue () {

	lcdg_update_t *cu;
	int nb;

	if (updp == 0)
		return;

	nb = (updp << 1);
	cu = (lcdg_update_t*) new byte [(sizeof (lcdg_update_t) + nb)];

	cu->Next = NULL;
	cu->Size = updp;

	memcpy (cu->Buf, updbuf, nb);

	if (UHead == NULL) {
		UHead = UTail = cu;
		IN.OT->signal (NULL);
	} else
		UTail = (UTail->Next = cu);

	updp = 0;
}

void LCDG::set_to_render (byte y0, byte y1, byte x0, byte x1) {

	if (IN.OT) {
		update_fpx ();
		update (LCDG_NOT_SET);
		update (x0);
		update (x1);
		update (y0);
		update (y1);
	}

	XC = X0 = x0;
	YC = Y0 = y0;
	X1 = x1;
	Y1 = y1;
	PC = 0;
}

void LCDG::update_fpx () {
//
// Flush any pending pixel repeat
//
	if (SPC) {
		// Pixel count pending
		if (updp == LCDG_OUTPUT_BUFSIZE)
			queue ();

		updbuf [updp++] = (0x8000 | (SPC - 1));
		SPC = 0;
	}

	LP = WNONE;
}

void LCDG::update_flush () {

	if (IN.OT == NULL)
		// Have to check because we can be called via the
		// nlcd_cs_up hack
		return;

	update_fpx ();

	// Insert an UPD command

	if (updp == LCDG_OUTPUT_BUFSIZE)
		queue ();

	updbuf [updp++] = LCDG_NOT_UPD;
	queue ();
}

void LCDG::init_connection () {

	assert (UHead == NULL, "LCDG->init_connection: queue nonempty");

	updbuf = new word [LCDG_OUTPUT_BUFSIZE];
	updp = 0;
	dump_screen ();
}

void LCDG::close_connection () {

	lcdg_update_t *c;

	delete [] updbuf;
	// Deallocate any pending updates
	while (UHead != NULL) {
		UHead = (c = UHead)->Next;
		free (c);
	}
}

void LCDG::update (word val) {
//
// A non-pixel item
//
	if (updp == LCDG_OUTPUT_BUFSIZE)
		queue ();
	updbuf [updp++] = val;
}

void LCDG::dump_screen () {
//
// Sends the entire screen as an update
//
	int i;

	if (IN.OT == NULL)
		return;

	update_fpx ();

	update (LCDG_NOT_SET);
	update (0);
	update (LCDG_MAXX);
	update (0);
	update (LCDG_MAXY);

	for (i = 0; i < LCDG_CANVAS_SIZE; i++)
		send_pix (canvas [i]);

	if (ONStat) {
		update_fpx ();
		update (LCDG_NOT_ON | Contrast);
	}
	update_flush ();
}

void LCDG::m_lcdg_on (byte con) {
//
// Switch it on
//
	if (con)
		Contrast = con;

	ONStat = YES;

	if (IN.OT) {
		update_fpx ();
		update (LCDG_NOT_ON | Contrast);
		update_flush ();
	}
}

void LCDG::m_lcdg_off () {
//
// Switch it on
//
	ONStat = NO;

	if (IN.OT) {
		update_fpx ();
		update (LCDG_NOT_OFF);
		update_flush ();
	}
}

void LCDG::sd (byte hp) {
//
// One third of a two-pixel update
//
	if (PC == 0) {
		// The first byte - just store it
		SB = hp;
		PC = 1;
		return;
	}
	if (PC == 1) {
		// Assemble the first pixel
		update_pix ((SB << 4) | (hp >> 4));
		SB = ((word)hp << 8) & 0x0F00;
		PC = 2;
		return;
	}
	update_pix (SB | hp);
	PC = 0;
}

#else

// ============================================================================
// === PICOS ==================================================================
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

	// This seems to be the default, so why wasting code
#if 0
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

#define	set_to_render(y0,y1,x0,x1)	do { \
						sc (PASET); \
						sd (y0); \
						sd (y1); \
						sc (CASET); \
						sd (x0); \
						sd (x1); \
						sc (RAMWR); \
					} while (0)

// Used to turn functions into methods in VUEE
#define	_lcdgm_(a)	a

#endif	/* VUEE or PICOS */

// ============================================================================
// === COMMON =================================================================
// ============================================================================

void _lcdgm_(lcdg_set) (byte xl, byte yl, byte xh, byte yh) {
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

void _lcdgm_(lcdg_get) (byte *XL, byte *YL, byte *XH, byte *YH) {
//
// Get the current bounding rectangle
//
	if (XL != NULL) *XL = X_org;
	if (YL != NULL) *YL = Y_org;
	if (XH != NULL) *XH = X_last;
	if (YH != NULL) *YH = Y_last;
}

void _lcdgm_(lcdg_setc) (byte bg, byte fg) {
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

void _lcdgm_(lcdg_clear) () {
//
// Clear the 'set' rectangle
//
	word w;

	nlcd_cs_down;
	// The row
	set_to_render (Y_org, Y_last, X_org, X_last);

	// The number of pixels
	w = ((Y_last - Y_org + 1) * (X_last - X_org + 1) + 1) >> 1;

	while (w--) {
		sd (KAB); sd (KBB); sd (KCB);
	}

	nlcd_cs_up;
}

void _lcdgm_(lcdg_render) (byte cs, byte rs, const byte *pix, word n) {
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
		set_to_render ((byte)ys, (byte)ys, (byte)xs, X_last);

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
	set_to_render ((byte)ys, Y_last, X_org, X_last);

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
			
Done:	CNOP;
#ifndef __SMURPH__
// To speed up VUEE emulation of this, we only flush on lcdg_end
	nlcd_cs_up;
#endif
}

#ifdef LCDG_FONT_BASE

word _lcdgm_(lcdg_font) (byte fn) {
//
// Set the font
//
	fbuf [0] = fpar [0] = fpar [1] = 0;
	// Read the header
	ee_read ((lword)LCDG_FONT_BASE, fbuf, 32);
	if (((word*)fbuf) [0] != 0x7f01)
		// This is not a font page
		return (word) ERROR;

	// Offset to our font
	
	if (((word*)fbuf) [1] <= fn)
		return (word) ERROR;

	fbase = ((word*)fbuf) [2 + fn];

	// Read the font header: width, height, blocksize
	ee_read ((lword)LCDG_FONT_BASE + fbase, fpar, 3);

	// Skip the first block
	fbase += fpar [2];

	// Shift count for font block
	fpar [3] = fpar [2] == 8 ? 3 : 4;

	return 0;
}

byte _lcdgm_(lcdg_cwidth) () {
//
// Return character width (based on the current font)
//
	return fpar [0];
}

byte _lcdgm_(lcdg_cheight) () {
//
// Character height
//
	return fpar [1];
}

word _lcdgm_(lcdg_sett) (byte x, byte y, byte nc, byte nl) {
//
// Set text area: corner + number of columns, number of lines
//
	word x0, y0, x1, y1;

	if (fpar [0] == 0)
		// No font
		return (word) ERROR;

	x0 = (word) x + LCDG_XOFF;
	x1 = x0 + (word) fpar [0] * nc - 1;
	y0 = (word) y + LCDG_YOFF;
	y1 = y0 + (word) fpar [1] * nl - 1;

	if (x1 < x0 || y1 < y0 || x1 > LCDG_MAXXP || y1 > LCDG_MAXYP)
		// Don't touch anything and return ERROR
		return (word) ERROR;

	// Set the area
	X_org = x0;
	Y_org = y0;
	X_last = x1;
	Y_last = y1;

	return 0;
}

void _lcdgm_(lcdg_ec) (byte cx, byte cy, byte nc) {
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

	set_to_render (cy, cy + fpar [1] - 1, cx, (byte)ex);

	ex = ((word) (fpar [1]) * (ex - cx + 1) + 1) >> 1;

	while (ex--) {
		sd (KAB); sd (KBB); sd (KCB);
	}

	nlcd_cs_up;
}
	
void _lcdgm_(lcdg_el) (byte cy, byte nl) {
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

	set_to_render (cy, Y_last, X_org, X_last);

	ex = ((word) (Y_last - cy + 1) * (X_last - X_org + 1) + 1) >> 1;

	while (ex--) {
		sd (KAB); sd (KBB); sd (KCB);
	}

	nlcd_cs_up;
}

void _lcdgm_(lcdg_wl) (const char *st, word sh, byte cx, byte cy) {
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
			break;

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

		set_to_render (cy, cy + fpar [1] - 1, cx, cx + fpar [0] - 1);

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
	nlcd_cs_up;
}

#endif	/* LCDG_FONT_BASE */

#endif	/* This file inclusion */

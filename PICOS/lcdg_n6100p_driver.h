#ifndef __lcdg_h__
#define	__lcdg_h__

#define	LCDG_MAXX	129
#define	LCDG_MAXY	129

#define	LCDG_XOFF	0
#define	LCDG_YOFF	0

#define LCDG_CANVAS_SIZE	((LCDG_MAXX + 1) * (LCDG_MAXY + 1))

#define	COLOR_WHITE	0
#define COLOR_BLACK	1
#define COLOR_RED	2
#define COLOR_GREEN	3
#define COLOR_BLUE	4
#define COLOR_CYAN	5
#define COLOR_MAGENTA	6
#define COLOR_YELLOW	7
#define COLOR_BROWN	8
#define COLOR_ORANGE	9
#define COLOR_PINK	10

#define	LCDG_NOT_ON	0x1100
#define	LCDG_NOT_OFF	0x1200
#define	LCDG_NOT_SET	0x1300
#define	LCDG_NOT_UPD	0x1400

#define	LCDG_FONT_BASE	0

#include "agent.h"

// Make it in words (do I have to subtract one?)
#define	LCDG_OUTPUT_BUFSIZE	(LCDG_OUTPUT_BUFLEN/2)

class LCDG {
//
// Nokia display
//
	friend	class LcdgHandler;

	lcdg_update_t *UHead, *UTail;

	Process *OutputThread;

	word canvas [LCDG_CANVAS_SIZE];

	word fbase,		// Font base offset in EEPROM
	     SP,		// Saved pixel for reassembly
	     LP,		// Last pixel (for compression)
	     SPC;		// Same pixel count

	// The format of update messages (words in network format):
	//
	// 0x0xxx - a pixel (inserted according to the last rectangle)
	// 0x1xxx - command (identifies a new update)
	// 0x1000 - NOP
	// 0x8xxx - repeat count for last pixel (covers clear)
	word *updbuf;

	word updp,		// The pointer
	     SB;		// Saved half-pixel

	byte ONStat,		// ON/OFF status
	     Contrast,		// Contrast
	     X_org,
	     Y_org,
	     X_last,
	     Y_last,
	     ColF,		// Foreground color
	     ColB,		// Background color

	// Preassembled standard colors (BG, FG) + crossovers F-B and B-F

	     KAB, KBB, KCB, KAF, KBF, KCF, KXF, KXB,

	     fpar [4],		// Font parameters: cols, rows, bsize, bshift
	     fbuf [32];		// Font buffer

	byte	X0, X1, Y0, Y1,	// Current rectangle for rendering
		XC, YC,		// ... and the current position
		PC;		// Counter modulo 3

	inline void send_pix (word pix) {

		if (pix == LP && SPC != 0x8000) {
			// The same pixel value
			SPC++;
			return;
		}

		// Not same

		if (SPC) {
			// Need to write the run length
			if (updp == LCDG_OUTPUT_BUFSIZE)
				queue ();

			updbuf [updp++] = (0x8000 | (SPC - 1));
			SPC = 0;
		}

		LP = pix;

		if (updp == LCDG_OUTPUT_BUFSIZE)
			queue ();

		updbuf [updp++] = pix;
	};

	inline void update_pix (word pix) {

		canvas [YC * (LCDG_MAXX + 1) + XC] = pix;

		if (XC == X1) {
			if (YC == Y1)
				YC = Y0;
			else
				YC++;
			XC = X0;
		} else
			XC++;

		if (OutputThread != NULL)
			send_pix (pix);
	};

	void queue ();
	void set_to_render (byte, byte, byte, byte);
	void update_fpx ();
	void update_flush ();
	void update (word);
	void dump_screen ();
	void sd (byte b);	// Handle one element of a triplet
	void init_connection (Process*);
	void close_connection ();

	public:

	void m_lcdg_on (byte);
	void m_lcdg_off ();
	void m_lcdg_set (byte, byte, byte, byte);
	void m_lcdg_get (byte*, byte*, byte*, byte*);
	void m_lcdg_setc (byte, byte);
	void m_lcdg_clear ();
	void m_lcdg_render (byte, byte, const byte*, word);
	void m_lcdg_end () { update_flush (); }

	word m_lcdg_font (byte);
	byte m_lcdg_cwidth (void);
	byte m_lcdg_cheight (void);
	word m_lcdg_sett (byte, byte, byte, byte);
	void m_lcdg_ec (byte, byte, byte);
	void m_lcdg_el (byte, byte);
	void m_lcdg_wl (const char*, word, byte, byte);

	LCDG ();
};

#endif

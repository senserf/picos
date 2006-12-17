#ifndef __node_h__
#define	__node_h__

#include "board.h"
#include "chan_shadow.h"
#include "plug_null.h"

station	Node : NNode {

	/*
	 * Session (application) specific data
	 */
	word	_na_a, _na_w, _na_len, _na_bs, _na_nt, _na_sl, _na_ss, _na_dcnt;
	int	_na_b;
	lword	_na_lw;
	byte	_na_str [129], *_na_blk;
	char	_na_ibuf [132];

	/*
	 * Application starter
	 */
	void appStart ();

	void setup (
		word mem,
		double	X,		// Coordinates
		double  Y,
		double	XP,		// Power
		double	RP,
		Long	BCmin,		// Backoff
		Long	BCmax,
		Long	LBTDel, 	// LBT delay (ms) and threshold (dBm)
		double	LBTThs,
		RATE	rate,
		Long	eesz,		// EEPROM size
		Long	ifsz,		// IFLASH size
		Long	ifps,		// IFLASH page size
		Long	PRE,		// Preamble
		Long	UMODE,		// UART mode
		Long	UBS,		// UART buffer size
		Long	USP,		// UART rate
		char	*UIDV,		// Input device for UART
		char	*UODV		// Output device for UART
	);

	void reset ();
	void init ();
};

#endif

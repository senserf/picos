#ifndef __node_h__
#define	__node_h__

#include "board.h"
#include "chan_shadow.h"
#include "plug_null.h"

station	Node : NNode {

	/*
	 * Session (application) specific data
	 */

	int	sfd;
	lword	last_snt, last_rcv, lost;
	word	nodeid;
	Boolean	XMTon, RCVon, rkillflag, tkillflag;

	int rcv_start ();
	int rcv_stop ();
	int snd_start (int);
	int snd_stop ();

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

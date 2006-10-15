#include "common.h"
#include "board.cc"
#include "diag.h"

void app_diag (const word level, const char *fmt, ... ) {

	char 		*buf;
	va_list		ap;
	va_start	(ap, fmt);

	if (app_dl < level)
		return;

	// compiled out if both levels are constant?
	// if not, go by #if DIAG_MESSAGES as well

	buf = TheNode->vform (NULL, fmt, ap);
	diag ("app_diag: %s", buf);
	ufree (buf);
}

void net_diag (const word level, const char * fmt, ...) {
	char * buf;
	va_list	ap;

	if (net_dl < level)
		return;

	va_start (ap, fmt);

	// compiled out if both levels are constant?

	buf = TheNode->vform (NULL, fmt, ap);
	diag ("net_diag: %s", buf);
	ufree (buf);
}

process Root : BoardRoot {

	void buildNode (
		const char *tp,		// Type
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
	) {

		if (strcmp (tp, "tag") == 0)

		    buildTagNode (
			mem,			// Memory
			X,
			Y,
			XP, RP,
			BCmin, BCmax,
			LBTDel, LBTThs,
			rate,
			PRE,
			UMODE,
			UBS,
			USP,
			UIDV,
			UODV
		    );

		else if (strcmp (tp, "peg") == 0)

		    buildPegNode (
			mem,			// Memory
			X,
			Y,
			XP, RP,
			BCmin, BCmax,
			LBTDel, LBTThs,
			rate,
			PRE,
			UMODE,
			UBS,
			USP,
			UIDV,
			UODV
		    );

		else
			excptn ("Root: illegal node type: %s", tp);
	};
};

#include "node.h"

#include "board.cc"

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
		Long	eesz,		// EEPROM size
		Long	ifsz,		// IFLASH size
		Long	ifps,		// IFLASH page size
		Long	UMODE,		// UART mode
		Long	UBS,		// UART buffer size
		Long	USP,		// UART rate
		char	*UIDV,		// Input device for UART
		char	*UODV		// Output device for UART
	) {
		create Node (
			mem,			// Memory
			X,
			Y,
			XP, RP,
			BCmin, BCmax,
			LBTDel, LBTThs,
			rate,
			PRE,
			eesz,
			ifsz,
			ifps,
			UMODE,
			UBS,
			USP,
			UIDV,
			UODV
		    );
	};
};

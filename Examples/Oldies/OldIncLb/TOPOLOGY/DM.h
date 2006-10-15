/* ----------------------------- */
/* Dumpling topology (segmented) */
/* ----------------------------- */

	// By defining XMITPORT you can chose a variant with a separate
	// port for transmission. Then you can set the length of the link
	// connecting this port to the "Left" (outgoing) port.

station DMPORTS virtual {

    Port *ULPort,       // Upper left
	 *URPort,       // Upper right
	 *LLPort,       // Lower left
	 *LRPort;       // Lower right

#ifdef	XMITPORT
    Port *UXPort,	// Upper transmission port
	 *LXPort;	// Lower transmission port
#endif

    void mkPorts (RATE r) {
	 URPort = create Port, "UR", (r);
	 ULPort = create Port, "UL", (r);
#ifdef	XMITPORT
	 UXPort = create Port, "UX", (r);
#endif

	 LRPort = create Port, "LR", (r);
	 LLPort = create Port, "LL", (r);
#ifdef	XMITPORT
	 LXPort = create Port, "LX", (r);
#endif
    };
};

#ifndef LINKTYPE
#define LINKTYPE PLink
#endif

LINKTYPE   **USegments, // Upper segments (array)
	   **LSegments, // Lower segments (array)
	   *UConnect,   // Upper connecting segment
	   *LConnect;   // Lower connecting segment

void    initDMTopology (DISTANCE, DISTANCE,
#ifdef	XMITPORT
                                       DISTANCE,
#endif
						RATE, TIME);
inline void initDMTopology (DISTANCE b, DISTANCE c, RATE r) {
	initDMTopology (b, c, 
#ifdef	XMITPORT
			DISTANCE_0,
#endif
				r, TIME_0);
};

#ifdef	XMITPORT
inline void initDMTopology (DISTANCE b, DISTANCE c, DISTANCE d, RATE r) {
	initDMTopology (b, c, d, r, TIME_0);
};
#endif

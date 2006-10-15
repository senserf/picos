/* --------------------------- */
/* Spiral topology (segmented) */
/* --------------------------- */

	// By defining XMITPORT you can chose a variant with a separate
	// port for transmission. Then you can set the length of the link
	// connecting this port to the "Left" (outgoing) port.

	// We expect NLOOPS to be defined

station SPPORTS virtual {

    Port *LPort [NLOOPS], // Left (outgoing) ports
	 *RPort [NLOOPS]; // Right (incoming) ports
#ifdef	XMITPORT
    Port *XPort [NLOOPS]; // Transmission ports
#endif

    void mkPorts (RATE r) {
	int i;
	for (i = 0; i < NLOOPS; i++) {
	  RPort [i] = create Port, form ("R%1d", i), (r);
	  LPort [i] = create Port, form ("L%1d", i), (r);
#ifdef	XMITPORT
	  XPort [i] = create Port, form ("X%1d", i), (r);
#endif
	}
    };
};

#ifndef LINKTYPE
#define LINKTYPE PLink
#endif

LINKTYPE   **Segments [NLOOPS], // Inter-station loop segments
	   *Connect [NLOOPS];   // CLosing segments

void    initSPTopology (DISTANCE, DISTANCE,
#ifdef	XMITPORT
                                       DISTANCE,
#endif
						RATE, TIME);
inline void initSPTopology (DISTANCE b, DISTANCE c, RATE r) {
	initSPTopology (b, c, 
#ifdef	XMITPORT
			DISTANCE_0,
#endif
				r, TIME_0);
};

#ifdef	XMITPORT
inline void initSPTopology (DISTANCE b, DISTANCE c, DISTANCE d, RATE r) {
	initSPTopology (b, c, d, r, TIME_0);
};
#endif

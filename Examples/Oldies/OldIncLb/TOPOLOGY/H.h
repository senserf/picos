/* ---------------------------------------------------------- */
/* This  file  contains  declarations  for a network topology */
/* with two unidirectional links, e.g., FASNET, DQDB          */
/* ---------------------------------------------------------- */

station HPORTS virtual {

    Port *LRPort,       // Port for the left-to-right bus
	 *RLPort;       // Port for the right-to-left bus

    void mkPorts (RATE r) {
	 LRPort = create Port (r);
	 RLPort = create Port (r);
    };
};

#ifndef	LINKTYPE
#define	LINKTYPE PLink
#endif

LINKTYPE   *LRLink, *RLLink;

void   		initHTopology (DISTANCE, RATE, TIME);
inline	void    initHTopology (DISTANCE d, RATE r) {
	initHTopology (d, r, TIME_0);
};

#define	LEFT	0
#define	RIGHT	1	// Direction indication

/* ---------------------------------------------------------- */
/* This  file  contains  declarations  for a network topology */
/* with an U-shaped single unidirectional link.               */
/* ---------------------------------------------------------- */

station UPORTS virtual {

    Port *OPort,        // Output port: towards the turn
	 *IPort;        // Input port: from the turn

    void mkPorts (RATE r) {
	 OPort = create Port (r);
	 IPort = create Port (r);
    };
};

#ifndef	LINKTYPE
#define	LINKTYPE PLink
#endif

LINKTYPE   *Bus;

void    initUTopology (DISTANCE, DISTANCE, RATE, TIME at);
inline   void    initUTopology (DISTANCE d, DISTANCE e, RATE r) {
	initUTopology (d, e, r, TIME_0);
};

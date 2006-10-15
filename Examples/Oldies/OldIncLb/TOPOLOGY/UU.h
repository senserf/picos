/* ---------------------------------------------------------- */
/* This  file  contains  declarations  for a network topology */
/* with two U-shaped unidirectional links.                    */
/* ---------------------------------------------------------- */

station UUPORTS virtual {

    Port *OPort [2],    // Output ports: towards the turn on links 0 and 1
	 *IPort [2];    // Input ports: from the turn on links 0 and 1

    void mkPorts (RATE r) {
	 OPort [0] = create Port (r);
	 IPort [0] = create Port (r);
	 OPort [1] = create Port (r);
	 IPort [1] = create Port (r);
    };
};

#ifndef	LINKTYPE
#define	LINKTYPE PLink
#endif

LINKTYPE   *Bus [2];	// Links number 0 and 1

void    initUUTopology (DISTANCE, DISTANCE, RATE, TIME at = TIME_0);

#define	BUS0	0
#define	BUS1	1

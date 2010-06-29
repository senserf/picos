/* ---------------------------------------------------------- */
/* This  file  contains  declarations  for a network topology */
/* with two unidirectional busses which are segmented in such */
/* a way that two neigbouring stations are connected by a se- */
/* parate link.                                               */
/* ---------------------------------------------------------- */

station HSPORTS virtual {

    Port *ILRPort,      // Incoming port for the left-to-right bus
         *SLRPort,	// Switching port for the left-to-right bus
	 *OLRPort,      // Outgoing port for the left-to-right bus
         *IRLPort,      // Incoming port for the right-to-left bus
         *SRLPort,	// Switching port for the right-to-left bus
	 *ORLPort;      // Outgoing port for the right-to-left bus

    void mkPorts (RATE r) {
	 ILRPort = create Port, "ILR", (r);
	 SLRPort = create Port, "SLR", (r);
	 OLRPort = create Port, "OLR", (r);
	 IRLPort = create Port, "IRL", (r);
	 SRLPort = create Port, "SRL", (r);
	 ORLPort = create Port, "ORL", (r);
    };
};

#ifndef	LINKTYPE
#define	LINKTYPE PLink
#endif

LINKTYPE   **LRLink, **RLLink;	// Link arrays

void    initHSTopology (DISTANCE, RATE, TIME at = TIME_0);

#define	LEFT	0
#define	RIGHT	1	// Direction indication

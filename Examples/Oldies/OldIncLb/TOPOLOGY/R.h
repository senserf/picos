/* ------------- */
/* Ring topology */
/* ------------- */

station RPORTS virtual {

    Port *IPort,        // Incoming port
	 *OPort;        // Outgoing port

    void mkPorts (RATE r) {
	 IPort = create Port (r);
	 OPort = create Port (r);
    };
};

#ifndef LINKTYPE
#define LINKTYPE PLink
#endif

LINKTYPE   **RLinks;

void    initRTopology (DISTANCE, RATE, TIME at);
inline  void initRTopology (DISTANCE d, RATE r) {
	initRTopology (d, r, TIME_0);
};

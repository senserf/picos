/* --------------------------- */
/* Uniform RPC traffic pattern */
/* --------------------------- */

station	Node;

traffic RQTraffic {
	// Request generator: message receive events to be caught
	virtual void pfmMRC (Packet*);
};

traffic RPTraffic {
	// Reply generator: message receive events to be caught
	virtual void pfmMRC (Packet*);
};

RQTraffic	*RQTPattern;
RPTraffic	*RPTPattern;

station	RPCSTAT virtual {
	// The part of the station describing the RPC status
	TIME	  RPCSTime;
	Station	  *RPCSender;
	RVariable *RPCSRDel;
};

process RPCClient (Node) {
	// Non-standard client process (run at every station)
	states {Wait, NewRequest};
};

process RPCServer (Node) {
	// Request processor (run at every station)
	states {Wait, Done};
};

#define	RQUEUE			-7	// Request queue id
#define	ResponseReceived	-8	// A signal

RVariable	*RPCSRDel;		// Global service time statistics

typedef	int(*QUALTYPE)(Message*);

void    initRPCTraffic (double, double, double, double);
void	printRPCPFM ();
int     getRPCPacket (Packet*, long min=0, long max=0, long frame=0);
int     getRPCPacket (Packet*, QUALTYPE, long min=0, long max=0, long frame=0);

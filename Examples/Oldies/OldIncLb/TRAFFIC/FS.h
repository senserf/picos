/* ----------- */
/* File server */
/* ----------- */

station Node;

traffic FRQTraffic {
	// Requests (message receiving events intercepted)
	virtual void pfmMRC (Packet*);
};

traffic FRPTraffic {
	// Replies (message receiving events intercepted)
	virtual void pfmMRC (Packet*);
};

	// Note: the length distribution of replies describes the distribution
	//	 of file length; thus, for a write requests, the request
	//	 length is obtained from FRPTPattern. Similarly, the response
	//	 length for a write request is obtained from FRQTPattern.

FRQTraffic	*FRQTPattern;
FRPTraffic	*FRPTPattern;

station	FSSTAT virtual {
	// The part of the station describing the FS status
	TIME	  FSTime;
	Node	  *FSSender;
	int	  FSRQType;
	RVariable *FSRDel;
};

process FSClient (Node) {
	// Non-standard client process (run at every station except the server)
	states {Wait, NewRequest};
};

process FServer (Node) {
	// Request processor (run at the server station)
	states {Wait, Done};
};

double	WRRatio;			// Write to read ratio
int	ServSttn;			// Server station Id

#define	FRQUEUE			-17	// Request queue id
#define	FResponseReceived	-18	// A signal

#define	RQST_RD	0			// Read request type
#define	RQST_WR 1			// Write request type

RVariable	*FSRDel;		// Global service time statistics

typedef	int(*QUALTYPE)(Message*);

void    initFSTraffic (double, double, double, double, double, int);
void	printFSPFM ();
int     getFSPacket (Packet*, long min=0, long max=0, long frame=0);
int     getFSPacket (Packet*, QUALTYPE, long min=0, long max=0, long frame=0);

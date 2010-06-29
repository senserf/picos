/* ---------------------------------------------------------- */
/* Simple  uniform  traffic  pattern,  standard  messages and */
/* packets,   message   length   and    inter-arrival    time */
/* exponentially distributed                                  */
/* ---------------------------------------------------------- */

	// MESSAGE_TYPE and PACKET_TYPE can be defined by the user to indicate
	// specific message and packet types other than Message and Packet
#ifndef	MESSAGE_TYPE
#define	MESSAGE_TYPE	Message
#endif
#ifndef	PACKET_TYPE
#define	PACKET_TYPE	Packet
#endif

#ifndef	STATION_TYPE
#define	STATION_TYPE	Node
#endif

traffic UTraffic (MESSAGE_TYPE, PACKET_TYPE) {

#ifdef	LOCAL_MEASURES
	// Virtual functions extending performance measures
#ifdef	LOCAL_MESSAGE_ACCESS_TIME
	virtual void pfmMTR (PACKET_TYPE*);
#else
	virtual void pfmMRC (PACKET_TYPE*);
#endif
	virtual void pfmPTR (PACKET_TYPE*);
	exposure;
#endif

#ifdef	SNAPSHOT_THROUGHPUT
	virtual void pfmPRC (PACKET_TYPE*);
#endif

};

#ifdef	LOCAL_MEASURES

station USTAT virtual {
	RVariable *UAPAcc, *UAMDel;
};

#else

station USTAT virtual { };

#endif

UTraffic *UTPattern;

void	printUPFM ();

#ifdef	SNAPSHOT_THROUGHPUT

void	initUTraffic (double, double, TIME, int *sl = NULL);

#define	STBUFSIZE 40

BITCOUNT STNRcvdBits = 0;
float	 STBuffer [STBUFSIZE];
int	 STBIndex = 0;

process	SThpMeter {

	TIME	Interval;

	void setup (TIME intvl) { Interval = intvl; };

	states {Wait, AddSample};

        perform;
};

#else

void	initUTraffic (double, double, int *sl = NULL);

#endif

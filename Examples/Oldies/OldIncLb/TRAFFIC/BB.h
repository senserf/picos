/* ---------------------------------------------------------- */
/* Bursty traffic against uniform background. In this traffic */
/* pattern,  there  is  a  uniform background traffic and one */
/* selected station generates occasionally a fixed-size burst */
/* of messages arriving all at the same time. This pattern is */
/* used to determine the network fairness.                    */
/* ---------------------------------------------------------- */

#ifdef  EXTENDED_MTYPE
traffic BUUTraffic (UMessage, UPacket) {
		// These types must be supplied in the including file
#else
traffic BUUTraffic {
#endif

#ifdef  LOCAL_MEASURES
	// Virtual functions extending performance measures
	virtual void pfmMRC (Packet*);
	virtual void pfmPTR (Packet*);
#endif

};

#ifdef  EXTENDED_MTYPE
traffic BUBTraffic (UMessage, UPacket) {
		// These types must be supplied in the including file
#else
traffic BUBTraffic {
#endif
	RVariable *BUBWPAcc;    // Weighted packet access time
	BUBTraffic ();
	virtual void pfmPTR (Packet*);
	virtual void pfmMTR (Packet*);
};

#ifdef  LOCAL_MEASURES

station USTAT virtual {
	RVariable *BUUAPAcc,    // Absolute packet access time
		  *BUUWPAcc,    // Weighted packet access time
		  *BUUAMDel;    // Absolute message delay
};

#endif

void    initBUTraffic (double, double, int, double, double, int, double);
void    printBUPFM ();

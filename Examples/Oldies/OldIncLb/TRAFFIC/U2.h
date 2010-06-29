/* ---------------------------------------------------------- */
/* Two uniform traffic patterns with exponential distribution */
/* of arrival and length                                      */
/* ---------------------------------------------------------- */

#ifdef  EXTENDED_MTYPE
traffic U0Traffic (UMessage, UPacket) {
		// These types must be supplied in the including file
#else
traffic U0Traffic {
#endif

#ifdef  LOCAL_MEASURES
	// Virtual functions extending performance measures
	virtual void pfmMRC (Packet*);
	virtual void pfmPTR (Packet*);
#endif

};

#ifdef  EXTENDED_MTYPE
traffic U1Traffic (UMessage, UPacket) {
		// These types must be supplied in the including file
#else
traffic U1Traffic {
#endif

#ifdef  LOCAL_MEASURES
	// Virtual functions extending performance measures
	virtual void pfmMRC (Packet*);
	virtual void pfmPTR (Packet*);
#endif

};

#ifdef  LOCAL_MEASURES

station USTAT virtual {
	RVariable *U0APAcc, *U0AMDel, *U1APAcc, *U1AMDel, *UAPAcc, *UAMDel;
};

#endif

U0Traffic *U0TPattern;
U1Traffic *U1TPattern;

void    initU2Traffic (double, double, double, double);
void    printU2PFM ();

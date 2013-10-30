#ifndef	__wchannt_h__
#define	__wchannt_h__

// Neutrino channel

#include "wchan.h"

rfchannel RFNeutrino : RadioChannel {
//
	double		Range;

	// Assessment methods
	double  RFC_att (const SLEntry*, double, Transceiver*);
	Boolean RFC_act (double, const SLEntry*);
	Boolean RFC_bot (RATE, const SLEntry*, const SLEntry*, const IHist*);
	Boolean RFC_eot (RATE, const SLEntry*, const SLEntry*, const IHist*);
	double  RFC_cut (double, double);
	Long    RFC_erb (RATE, const SLEntry*, const SLEntry*, double, Long);
	Long    RFC_erd (RATE, const SLEntry*, const SLEntry*, double, Long);

	void setup (
		Long,			// The number of transceivers
		double,			// Range
		int,			// Bits per byte
		int,			// Packet frame (extra physical bits)
		IVMapper **ivcc, 	// Value converters
		MXChannels *mxc = NULL	// Channels
	);
};

#endif

/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __types_h__
#define	__types_h__

/*
 * For debugging, you may define TRACE_MAC_EVENTS as well as set
 * DEBUGGING to 1. The latter will enable tracing in rfmod_raw.cc.
 */
#ifdef	TRACE_MAC_EVENTS
#define	mac_trc(a, ...)	trace (a, ## __VA_ARGS__)
#else
#define	mac_trc(a, ...) do { } while (0)
#endif

#include "rfmod_raw.h"

// A Monitor needed by the Dispatcher process
#define	EVENT_PACKET_TRANSMITTED	MONITOR_LOCAL_EVENT(0)

class Node;

class MY_RFModule : public RFModule {

// This class completes the specification of RFModule (from rfmod_raw.h), which
// is an abstract library class. Some virtual methods that can be declared here
// provide useful hooks for various occasions.

	Node	*N;			// Pointer to the Node structure
	double	MinBackoff, MaxBackoff;	// For LBT

	public:

	void UPPER_rcv (Packet*, double);
		// This method MUST be defined (see node.cc for the definition).
		// It is invoked whenever a packet has been received by the MAC
		// layer. The second argument is the RSSI (signal strength).


	void UPPER_snd (Packet *p) {
		// Called when the RF module is about to start transmitting
		// a packet
		mac_trc ("SND DATA: %1d -> %1d", TheStation->getId (),
			p->Receiver);
	};

	void UPPER_snt (Packet *p) {
		// Called immediately after the RF module stops sending a
		// packet; the packet is pointed to by the argument
		mac_trc ("SNT DATA: %1d -> %1d", TheStation->getId (),
			p->Receiver);
		// We signal the Dispatcher process that the previous packet
		// has been expedited
		Monitor->signal (EVENT_PACKET_TRANSMITTED);
	};

	void UPPER_lbt (int) {
		// Called when the channel has been sensed busy before a
		// transmission attempt. LBT stands for Listen Before Transmit.
		backoff (dRndUniform (MinBackoff, MaxBackoff));
	};

	MY_RFModule (	Transceiver *t,		// Our Transceiver
			double lbth,		// LBT threshold (signal level)
			double lbtd,		// LBT interval (seconds)
			double minb,		// Minimum backoff
			double maxb		// Maximum backoff
		    ) :
		    // RFModule constructor
		    RFModule (	t,
				1,		// Internal queue size is 1,
						// i.e., one outstanding packet	
				lbth,
				lbtd
		    ) {

		// Back to our subtype constructor

		N = (Node*) TheStation;
		MinBackoff = minb;
		MaxBackoff = maxb;
	};
};

station Node {

	Packet Buffer;		// For Client packets to be transmitted
	Transceiver *RFI;	// The RF interface
	MY_RFModule *RFM;	// The RF Module interface

	void receive (Packet*, double);	// Called by MAC-level reception
	void setup (RATE, Long, double, double, double, double, double);
};

#define	TheNode	((Node*)TheStation)

process Dispatcher (Node) {

	// See node.cc for the code method. Sends out Client packets.

	states { WMess, PDone } ;

	perform;
};

process PositionReporter {

	// See node.cc for the code method. This is an optional process that
	// reports the positions of all nodes at regular intervals.

	states { Report } ;

	perform;
};

void initMobility ();
void initNodes (Long, Long);
void initChannel (Long&, Long&);
void printStatistics ();

extern Long BitRate;

#endif

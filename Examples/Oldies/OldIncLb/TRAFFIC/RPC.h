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

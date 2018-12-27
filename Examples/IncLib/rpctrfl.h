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

#ifndef __rpctrafficl_h__
#define __rpctrafficl_h__

// This is an RPC-type traffic in which every station behaves as a server and
// a client (customer) at the same time. Every now and then a customer
// generates a request message addressed to a given server (selected at
// random) which is always different from the customer station. The request
// generator at the sender is suspended until a replay to the last request
// arrives. Reply messages have priority over requests, i.e., if a station
// has a reply message queued, the packets of this message will be selected
// (acquired for transmission) before any possible request packets available
// at the same time. We collect service time statistics individually for
// each station.

#define MAXSTATIONS 256   // Increase if SMURPH starts complaining

traffic RQTraffic {
  // Request traffic: message receive events to be caught
  void pfmMRC (Packet*);
  exposure;
};

traffic RPTraffic {
  // Reply traffic: message receive events to be caught
  void pfmMRC (Packet*);
};

mailbox RQMailbox (Long);  // Request mailbox type (storing station Id's)

station ClientInterface virtual {
  Packet Buffer;
  TIME StartTime;     // For calculating service time
  RVariable *RWT;     // Waiting time statistics
  RQMailbox *RQ;      // Request queue
  Mailbox *RPR;       // Reply received
  Boolean ready (Long, Long, Long);
  void configure ();
};

process RQPilot {
  ClientInterface *S; // Overrides standard attribute S
  Mailbox *RPR;       // Reply received
  void setup (ClientInterface *s) {
    S = s;
    RPR = S->RPR;
  };
  states {Wait, NewRequest};
  perform;
};

process RPPilot {
  ClientInterface *S; // Overrides standard attribute S
  RQMailbox *RQ;      // Request queue
  Long RQSender;      // The Id of the last request sender
  void setup (ClientInterface *s) {
    S = s;
    RQ = S->RQ;
  };
  states {Wait, Done};
  perform;
};

void initTraffic ();
void printLocalMeasures ();

#endif

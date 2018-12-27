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

#ifndef __fstrafficl_h__
#define __fstrafficl_h__

// This is a file-server type traffic in which one selected station behaves as
// a server and the remaining stations are clients issuing page transfer
// requests to the server. This traffic can be viewed as a variant of RPC with
// only one server separated from the clients. The requests are divided into
// reads and writes (the percentage of writes is read from the input data
// file). The size of a read request packet is fixed and equal to the size of
// the confirmation packet that arrives in response to a write request.
// Similarly, the size of a page is also fixed. One read request results in
// exactly one page being transmitted from the server to the client. One
// write request involves one page being transmitted to the server.

#define MAXSTATIONS 256     // Increase if SMURPH starts complaining

#define WRITE_FLAG PF_usr0  // Packet flag identifying write requests

traffic RQTraffic {
  // Request traffic: message receive events to be caught
  virtual void pfmMRC (Packet*);
  exposure;
};

traffic RPTraffic {
  // Reply traffic: message receive events to be caught
  virtual void pfmMRC (Packet*);
};

class Request {
  // This simple data structure represents requests
  public:
  Long SId;       // The station Id
  Boolean Write;  // Write request flag
  Request (Long, Boolean);
};

mailbox RQMailbox (Request*);  // Request mailbox type

station ClientInterface virtual {
  Packet Buffer;
  TIME StartTime;     // For calculating service time
  Boolean Write;      // Request type
  RVariable *RWT;     // Waiting time statistics
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
  Request *LastRq;    // The last request
  states {Wait, Done};
  perform;
};

void initTraffic ();
void printLocalMeasures ();

#endif

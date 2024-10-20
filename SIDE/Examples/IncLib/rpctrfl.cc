/*
	Copyright 1995-2020 Pawel Gburzynski

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

#ifndef __rpctrafficl_c__
#define __rpctrafficl_c__

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

#include "rpctrfl.h"

static RQTraffic *RQTP;
static RPTraffic *RPTP;

static RVariable *RWT;     // Global request waiting time statistics

static ClientInterface *CInt [MAXSTATIONS];

void initTraffic () {
  double rqit, rqle, srvt, rple;
  int i;
  readIn (rqit);    // Mean request interarrival time (ETU per station)
  readIn (rqle);    // Mean request length in bits
  readIn (srvt);    // Mean request service time (ETU)
  readIn (rple);    // Mean reply length in bits
  RQTP = create RQTraffic (MIT_exp+MLE_exp+SCL_off, rqit, rqle);
  RPTP = create RPTraffic (MIT_exp+MLE_exp+SCL_off, srvt, rple);
  RWT = create RVariable;
  for (i = 0; i < MAXSTATIONS; i++) CInt [i] = NULL;
};

void ClientInterface::configure () {
  RQTP->addSender (TheStation);
  RQTP->addReceiver (TheStation);
  // Note: for RPTP senders and recipients are implicit
  RWT = create RVariable, form ("RWT Sttn %3d", TheStation->getId ());
  RQ = create RQMailbox (MAX_Long);
  RPR = create Mailbox (0);
  Assert (TheStation->getId () < MAXSTATIONS,
    "Too many stations, increase MAXSTATIONS in rpctrfl.h");
  CInt [TheStation->getId ()] = this;
  create RQPilot (this);
  create RPPilot (this);
};

Boolean ClientInterface::ready (Long mn, Long mx, Long fm) {
  if (Buffer.isFull () || RPTP->getPacket (&Buffer, mn, mx, fm))
    return YES;
  else if (RQTP->getPacket (&Buffer, mn, mx, fm)) {
    StartTime = Buffer.QTime;  // Start counting waiting time
    return YES;
  } else
    return NO;
};

void RQTraffic::pfmMRC (Packet *p) {
  // Request received
  CInt [p->Receiver] -> RQ -> put (p->Sender);
};

void RPTraffic::pfmMRC (Packet *p) {
  double d;
  ClientInterface *CI;
  // Reply received
  (CI = CInt [p->Receiver]) -> RPR -> put ();
  d = (Time - CI->StartTime) * Itu;
  CI->RWT->update (d);
  RWT->update (d);
};

RQPilot::perform {
  state Wait:
    Timer->wait (RQTP->genMIT (), NewRequest);
  state NewRequest:
    if (RQTP->isSuspended ()) {
      RQTP->wait (RESUME, Wait);
      sleep;
    }
    RQTP->genCGR (TheStation);  // To exclude the current station
    RQTP->genMSG (TheStation->getId (), RQTP->genRCV (), RQTP->genMLE ());
    RPR->wait (NEWITEM, Wait);
};

RPPilot::perform {
  state Wait:
    if (RQ->empty ())
      RQ->wait (NONEMPTY, Wait);
    else {
      RQSender = RQ->get ();
      Timer->wait (RPTP->genMIT (), Done);
    }
  state Done:
    RPTP->genMSG (TheStation->getId (), RQSender, RPTP->genMLE ());
    proceed Wait;
};

#include "lrwtexp.cc"

#endif

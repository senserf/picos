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

#ifndef __fstrafficl_c__
#define __fstrafficl_c__

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

#include "fstrfl.h"

static RQTraffic *RQTP;
static RPTraffic *RPTP;

static RVariable *RWT;     // Global request waiting time statistics

static ClientInterface *CInt [MAXSTATIONS];

static Long PageLength,    // The fixed page length
            StatLength,    // Status packet length (read request/confirmation)
            TheServer;     // The Id of the server station
			
static double WriteProbability;

static RQMailbox *RQ;      // The request queue is made global

void initTraffic () {
  double rqit, srvt;
  int i;
  readIn (PageLength);
  readIn (StatLength);
  readIn (rqit);    // Mean request interarrival time (ETU per station)
  readIn (srvt);    // Mean request service time (ETU)
  readIn (WriteProbability);
  readIn (TheServer);
  RQTP = create RQTraffic (MIT_exp+SCL_off, rqit);
  RPTP = create RPTraffic (MIT_exp+SCL_off, srvt);
  // For simplicity, we assume that the service time is the same for read
  // and write requests
  RWT = create RVariable;
  for (i = 0; i < MAXSTATIONS; i++) CInt [i] = NULL;
};

void ClientInterface::configure () {
  Long Id;
  if ((Id = TheStation->getId ()) != TheServer) {
    // A "client" station
    RQTP->addSender (TheStation);
    RWT = create RVariable, form ("RWT Sttn %3d", Id);
    RPR = create Mailbox (0);
    create RQPilot (this);
    Assert (TheStation->getId () < MAXSTATIONS,
      "Too many stations, increase MAXSTATIONS in fstrfl.h");
    CInt [Id] = this;
  } else {
    RQTP->addReceiver (TheServer);  // Only one receiver
    create RPPilot;
    RQ = create RQMailbox (MAX_Long);
    // Note that the server doesn't go to CInt
  }
};

Request::Request (Long sid, Boolean wf) {
  SId = sid;
  Write = wf;
};

Boolean ClientInterface::ready (Long mn, Long mx, Long fm) {
  // This is exactly the same as for RPC traffic: replies have priority
  // over requests
  if (Buffer.isFull () || RPTP->getPacket (&Buffer, mn, mx, fm))
    return YES;
  else if (RQTP->getPacket (&Buffer, mn, mx, fm)) {
    if (Write) setFlag (Buffer.Flags, WRITE_FLAG);
    StartTime = Buffer.QTime;  // Start counting waiting time
    return YES;
  } else
    return NO;
};

void RQTraffic::pfmMRC (Packet *p) {
  // Request received
  RQ -> put (new Request (p->Sender, flagSet (p->Flags, WRITE_FLAG)));
};

void RPTraffic::pfmMRC (Packet *p) {
  double d;
  ClientInterface *CI;
  // Reply received: give a go to the pilot process
  (CI = CInt [p->Receiver]) -> RPR -> put ();
  d = (Time - CI->StartTime) * Itu;
  CI->RWT->update (d);
  RWT->update (d);
};

RQPilot::perform {
  Long ml;
  state Wait:
    Timer->wait (RQTP->genMIT (), NewRequest);
  state NewRequest:
    if (RQTP->isSuspended ()) {
      RQTP->wait (RESUME, Wait);
      sleep;
    }
    if (rnd (SEED_traffic) < WriteProbability) {
      S->Write = YES;
      ml = PageLength;
    } else {
      S->Write = NO;
      ml = StatLength;
    }
    RQTP->genMSG (TheStation->getId (), TheServer, ml);
    RPR->wait (NEWITEM, Wait);
};

RPPilot::perform {
  Long ml;
  state Wait:
    if (RQ->empty ())
      RQ->wait (NONEMPTY, Wait);
    else {
      LastRq = RQ->get ();
      Timer->wait (RPTP->genMIT (), Done);
    }
  state Done:
    ml = LastRq->Write ? StatLength : PageLength;
    RPTP->genMSG (TheServer, LastRq->SId, ml);
    delete LastRq;
    proceed Wait;
};

#include "lrwtexp.cc"

#endif

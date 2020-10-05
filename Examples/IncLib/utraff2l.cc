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

#ifndef __utraffic2l_c__
#define __utraffic2l_c__

// Interface from a dual-bus station to a uniform traffic pattern (all
// stations) with Poisson arrival and exponentially distributed message
// length. Each station has two buffers, each buffers for storing
// packets going in a given direction. This is the same as utraffic.[ch],
// except that here we calculate message access time individually for
// each station -- to see how fair/unfair the protocol is.

#include "utraff2l.h"

static Traffic *UTP;

static ClientInterface *CInt [MAXSTATIONS];

void initTraffic () {
  int i;
  double mit, mle;
  readIn (mit);
  readIn (mle);
  UTP = create UTraffic (MIT_exp+MLE_exp, mit, mle);
  for (i = 0; i < MAXSTATIONS; i++) CInt [i] = NULL;
};

void ClientInterface::configure () {
  UTP->addSender (TheStation);
  UTP->addReceiver (TheStation);
  MAT = create RVariable, form ("MAT Sttn %3d", TheStation->getId ());
  Assert (TheStation->getId () < MAXSTATIONS,
    "Too many stations, increase MAXSTATIONS in utraff2l.h");
  CInt [TheStation->getId ()] = this;
};

#ifdef PrivateQualifier

  // Note: by defining PrivateQualifier you can program your own 'qual'
  // function that will be used for packet acquisition. This is done
  // in Metaring.

  extern int Direction;
  int qual (Message*);

#else

  static int Direction;      // For packet acquisition

  static int qual (Message *m) {
    // The qualifier function for getPacket
    return (Direction == RLBus && TheStation->getId () > m->Receiver) ||
        (Direction == LRBus && TheStation->getId () < m->Receiver);
  };
  
#endif

Boolean ClientInterface::ready (int dir, Long mn, Long mx, Long fm) {
  Direction = dir;
  return Buffer [dir] . isFull () ||
    Client->getPacket (&(Buffer [dir]), qual, mn, mx, fm);
};

void UTraffic::pfmMTR (Packet *p) {
  double d;
  d = (double) (Time - p->QTime) * Itu;
  CInt [TheStation->getId ()] -> MAT -> update (d);
};

#include "lmatexp.cc"

#endif

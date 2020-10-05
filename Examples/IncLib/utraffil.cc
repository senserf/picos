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

#ifndef __utrafficl_c__
#define __utrafficl_c__

// Interface to a uniform traffic pattern (all stations) with Poisson arrival
// and exponentially distributed message length. This is the same as
// utraffic.[ch], except that here we calculate message access time
// individually for each station -- to see how fair/unfair the protocol is.

#include "utraffil.h"

static UTraffic *UTP;

static ClientInterface *CInt [MAXSTATIONS];

void initTraffic () {
  double mit, mle;
  int i;
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
    "Too many stations, increase MAXSTATIONS in utraffil.h");
  CInt [TheStation->getId ()] = this;
};

Boolean ClientInterface::ready (Long mn, Long mx, Long fm) {
  return Buffer.isFull () || Client->getPacket (&Buffer, mn, mx, fm);
};

void UTraffic::pfmMTR (Packet *p) {
  double d;
  d = (double) (Time - p->QTime) * Itu;
  CInt [TheStation->getId ()] -> MAT -> update (d);
};

#include "lmatexp.cc"

#endif

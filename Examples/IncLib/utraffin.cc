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

#ifndef __utrafficn_c__
#define __utrafficn_c__

// Interface to a uniform traffic pattern (all stations) with Poisson arrival
// and exponentially distributed message length. Each station has a number of
// buffers which are filled independently. This traffic pattern is used in
// mesh networks (e.g., MNA) in which a single station runs a number of
// independent transmitters.

#include "utraffin.h"

traffic UTraffic (MESSAGE_TYPE, PACKET_TYPE);

static UTraffic *UTP;

void initTraffic (int FixedMessageLength) {
  double mit, mle;
  int TrafP;
  readIn (mit);
  readIn (mle);
  TrafP = MIT_exp;
  if (FixedMessageLength) TrafP += MLE_fix; else TrafP += MLE_exp;
  UTP = create UTraffic (TrafP, mit, mle);
};

void ClientInterface::configure (int nb) {
  int i;
  Buffer = new PACKET_TYPE* [nb];
  for (i = 0; i < nb; i++) Buffer [i] = create PACKET_TYPE;
  UTP->addSender (TheStation);
  UTP->addReceiver (TheStation);
};

Boolean ClientInterface::ready (int nb, Long mn, Long mx, Long fm) {
  return Buffer [nb] -> isFull () ||
    Client->getPacket (Buffer [nb], mn, mx, fm);
};

#endif

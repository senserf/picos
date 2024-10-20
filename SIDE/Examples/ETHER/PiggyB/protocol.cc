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

#include "types.h"
#include "pxmitter.cc"

identify "Piggyback Ethernet (B)";

Mtr::perform {
  state Waiting:
    Bus->wait (ACTIVITY, Active);
  state Active:
    S->Blocked = YES;
    Bus->wait (EOT, EndPacket);
    Bus->wait (SILENCE, EndActivity);
  state EndActivity:
    // This is after a collision
    Timer->wait (TPSpace, GoSignal);
  state GoSignal:
    if (Bus->busy ()) proceed Active;
    S->Blocked = NO;
    Ready->put ();
    proceed Waiting;
  state EndPacket:
    if (eligible ())
      Timer->wait (DelayP, MyTurn);
    else
      Timer->wait (DelayX, GoSignal);
    Bus->wait (ACTIVITY, Active);
  state MyTurn:
    // Keep it blocked to avoid spontaneous transmissions
    Ready->put ();
    Timer->wait (DelayX-DelayP, GoSignal);
    Bus->wait (ACTIVITY, Active);
};

void Mtr::setup () {
  Bus = S->Bus;
  Ready = S->Ready;
};

Boolean Mtr::eligible () {
  Long Sender, SId;
  Sender = ThePacket->Sender;
  SId = S->getId ();
  if ((S->PiggyDirection = piggyDirection (ThePacket)) == Left) {
    if (SId < Sender)
      DelayP = (Sender - SId) * DelayQuantum;
    else
      DelayP = TIME_0;
    DelayX = 2 * L + (Sender + 1) * DelayQuantum;
  } else {
    if (SId > Sender)
      DelayP = (SId - Sender) * DelayQuantum;
    else
      DelayP = TIME_0;
    DelayX = 2 * L + (NStations - Sender) * DelayQuantum;
  }
  return DelayP != TIME_0;
};

void Transmitter::setPiggyDirection () {
  if (S->getId () == 0)
    setPiggyRight (Buffer);
  else if (S->getId () == NStations - 1)
    setPiggyLeft (Buffer);
  else
    if (flip ()) setPiggyLeft (Buffer); else setPiggyRight (Buffer);
};

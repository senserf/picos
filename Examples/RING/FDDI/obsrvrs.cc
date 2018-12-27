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

#include "types.h"
#include "obsrvrs.h"

TokenMonitor::perform {
  state Resume:
    inspect (ANY, Relay, MyTkn, Verify);
    timeout (TokenPassingTimeout, Lost);
  state Verify:
    inspect (TheStation, Relay, PsTkn, Resume);
    inspect (TheStation, Relay, IgTkn, Resume);
    inspect (ANY, Relay, MyTkn, Duplicate);
    timeout (TokenPassingTimeout, Lost);
  state Duplicate:
    excptn ("Duplicate token");
  state Lost:
    excptn ("Lost token");
};

FairnessMonitor::perform {
  Packet *Buf;
  TIME Delay;
  state Resume:
    inspect (ANY, Relay, IgTkn, CheckDelay);
  state CheckDelay:
    Buf = &(((FStation*)TheStation)->Buffer);
    if (Buf->isFull ()) {
      Delay = Time - Buf->TTime;
      Assert (Delay <= MaxDelay, "Starvation");
    }
    proceed Resume;
};


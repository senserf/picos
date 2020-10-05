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

identify Expressnet;

Long MinPL,    // Minimum packet length (payload)
     MaxPL,    // Maximum packet length
     FrameL,   // Header and trailer (preamble excluded)
     PrmbL;    // Preamble length

TIME EOTDelay; // Amount of silence needed to detect the end of train 

EOTMonitor::perform {
  state Wait:
    Bus->wait (EOT, Count);
  state Count:
    Timer->wait (EOTDelay, Signal);
    Bus->wait (ACTIVITY, Retry);
  state Signal:
    if (signal () != ACCEPTED) excptn ("End of train signal not accepted");
  transient Retry:
    skipto Wait;
};

Transmitter::perform {
  state Wait:
    Bus->wait (EOT, CheckBuf);
    if (EOTrain != NULL)
      EOTrain->wait (SIGNAL, Transmit);    // On end of train
  state CheckBuf:
    if (S->ready (MinPL, MaxPL, FrameL))
      proceed Transmit;
    else
      skipto Wait;
  state Transmit:
    Bus->transmit (Preamble, PDone);
    Bus->wait (COLLISION, Yield);
  state Yield:
    Bus->abort ();
    skipto Wait;
  state PDone:
    if (S->ready (MinPL, MaxPL, FrameL)) {
      Bus->abort ();  // The preamble won't trigger EOC
      Bus->transmit (Buffer, XDone);
      Bus->wait (COLLISION, Error);
    } else {
      Bus->stop ();   // Forced preamble - must trigger EOC
      skipto Wait;
    }
  state XDone:
    Bus->stop ();
    Buffer->release ();
    skipto Wait;
  state Error:
    excptn ("Illegal collision");
};
      
Receiver::perform {
  state WPacket:
    Bus->wait (EMP, Rcvd);
  state Rcvd:
    Client->receive (ThePacket, ThePort);
    skipto (WPacket);
};

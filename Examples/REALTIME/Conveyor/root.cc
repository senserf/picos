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

#include "operator.h"

#include "sensor.cc"

Process *TheRoot;   // To make Root visible to the operator

process Root {
  states {Init, Start, Stop};
  perform {
    Long nml;
    state Init:
      TheRoot = this;
      initSystem ();
      initBoard ();
      initIo ();
      initOperator ();
      readIn (nml);
      if (nml) {
        // Run on your own
        setLimit (nml);
        signal ((void*)1);
      }
      signal (); // To force immediate start
      wait (SIGNAL, Start);
    state Start:
      startIo ();
      Kernel->wait (DEATH, Stop);
    state Stop:
      outputIo ();
      terminate;
  };
};

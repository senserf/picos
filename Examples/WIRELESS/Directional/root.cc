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
#include "channel.h"
#include "traffic.h"
#include "rfmodule.h"

identify (Very Simple Wireless Network);

static void initAll () {

// Here things start. This functions reads the input data and initializes 
// things. Note the order (which determines the order in which the input
// data are read:
//
//	initChannel, initNodes, initTraffic, InitMobility
//

	initNodes (initChannel ());
	initTraffic ();
	initMobility ();
}

process Root {

	double TimeLimit;

	states { Start, RunClient, Stop };

	perform {

		double Delay;

		state Start:

			initAll ();

			// Client delay - to postpone generating traffic until
			// the network has acquired some wisdom
			readIn (Delay);

			// Simulation time limit
			readIn (TimeLimit);

			Client->suspend ();

			Timer->delay (Delay, RunClient);

		state RunClient:

			Client->resume ();
			setLimit (0, etuToItu (TimeLimit));
			Kernel->wait (DEATH, Stop);

		state Stop:

			Client->printPfm ();
			terminate;
	};
};

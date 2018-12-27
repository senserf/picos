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
#include "traffic.h"

identify (Very Simple Wireless Network);

#include "rfmod_raw.cc"

static void initAll () {

// Here things start. This function reads the input data and initializes 
// things. Note the order (which determines the order in which the input
// data are read:
//
//	initChannel, initNodes, initTraffic, InitMobility
//
	Long NS, PRE;

	settraceFlags (
			TRACE_OPTION_TIME +
			TRACE_OPTION_ETIME +
			TRACE_OPTION_STATID +
			TRACE_OPTION_STATE
		 );

	initChannel (NS, PRE);
	initNodes (NS, PRE);
	initTraffic ();
	initMobility ();
}

process Root {


	states { Start, Stop };

	perform {

		state Start:

			double TimeLimit;

			initAll ();
			// Reset after initAll (it may have changed TheStation)
			TheStation = System;

			readIn (TimeLimit);
			setLimit (0, etuToItu (TimeLimit));
			Kernel->wait (DEATH, Stop);

		state Stop:

			Client->printPfm ();
			printStatistics ();
			terminate;
	};
};

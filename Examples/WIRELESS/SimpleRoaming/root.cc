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

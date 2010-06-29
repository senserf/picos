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

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
identify (Simple ALOHA);

#include "types.h"

Long NTerminals;
ALOHATraffic *HTTrf, *THTrf;
ALOHARF *HTChannel, *THChannel;

#ifdef	LOCAL_MEASURES
Long *HXmt, *TRcv;
#endif

double	LBTTime;

// ============================================================================

void Hub::setup () {
	ALOHADevice::setup ();
	AB = new unsigned int [NTerminals];
	memset (AB, 0, NTerminals * sizeof (unsigned int));
	AQ.setLimit (NTerminals);
	ABuffer.fill (getId (), 0, ACK_LENGTH + FRAME_LENGTH, ACK_LENGTH);
	create HTransmitter;
	create HReceiver;
}

void Terminal::setup () {
	ALOHADevice::setup ();
	AB = 0;
	When = TIME_0;
	create THalfDuplex;
#ifdef	LOCAL_MEASURES
	RC = create RVariable;
#endif
}

// ============================================================================

process Root {

	void buildNetwork ();
	void initTraffic ();
	void setLimits ();
	void printResults ();

	states { Start, Stop };

	perform;
};

void Root::buildNetwork () {

	double rate, xpower, bnoise, athrs;
	int minpreamble, nb, i;
	SIRtoBER *sib;

	// 1ITU = propagation time across 1m, du = 1m
	setEtu (299792458.0);
	setDu (1.0);

	readIn (xpower);		// Transmit power (dBm)
	readIn (bnoise);
	readIn (athrs);
	readIn (minpreamble);
	readIn (LBTTime);

	sib = new SIRtoBER ();

	readIn (NTerminals);

	HTChannel = create ALOHARF (NTerminals + 1,
					DATA_RATE,
					xpower,
					bnoise,
					athrs,
					minpreamble,
					sib);

	THChannel = create ALOHARF (NTerminals + 1,
					DATA_RATE,
					xpower,
					bnoise,
					athrs,
					minpreamble,
					sib);

	for (i = 0; i < NTerminals; i++)
		create Terminal;

	create Hub;

#ifdef	LOCAL_MEASURES
	HXmt = new Long [NTerminals];
	TRcv = new Long [NTerminals];
	bzero (HXmt, sizeof (Long) * NTerminals);
	bzero (TRcv, sizeof (Long) * NTerminals);
#endif
}

void Root::initTraffic () {

	double hmit, tmit;
	Long sn;

	readIn (hmit);
	readIn (tmit);

	HTTrf = create ALOHATraffic (MIT_exp + MLE_fix, hmit,
		(double) PACKET_LENGTH);

	THTrf = create ALOHATraffic (MIT_exp + MLE_fix, tmit,
		(double) PACKET_LENGTH);

	for (sn = 0; sn < NTerminals; sn++) {
		HTTrf->addReceiver (sn);
		THTrf->addSender (sn);
	}

	HTTrf->addSender (NTerminals);
	THTrf->addReceiver (NTerminals);
}

void Root::setLimits () {

	Long maxm;
	double tlim;

	readIn (maxm);
	readIn (tlim);
#if 0
	Ouf << "Limits: " << maxm << ", " << tlim << "\n";
#endif
	setLimit (maxm, tlim);
}

void Root::printResults () {

	HTTrf->printPfm ();
	THTrf->printPfm ();
	Client->printPfm ();
	HTChannel->printPfm ();
	THChannel->printPfm ();

#ifdef LOCAL_MEASURES

	{
		Long i;

		for (i = 0; i < NTerminals; i++)
			TheTerminal (i)->printPfm ();
	}
#endif
}

// ============================================================================

Root::perform {

	state Start:

		settraceFlags (
			TRACE_OPTION_TIME +
			TRACE_OPTION_ETIME +
			TRACE_OPTION_STATID +
			TRACE_OPTION_STATE
		);

		buildNetwork ();
		initTraffic ();
		setLimits ();

		Kernel->wait (DEATH, Stop);

	state Stop:

		printResults ();
		terminate;
}

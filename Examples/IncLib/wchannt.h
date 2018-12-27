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

#ifndef	__wchannt_h__
#define	__wchannt_h__

// Neutrino channel

#include "wchan.h"

rfchannel RFNeutrino : RadioChannel {
//
	double		Range;

	// Assessment methods
	double	RFC_add (int, int, const SLEntry**, const SLEntry*);
	double  RFC_att (const SLEntry*, double, Transceiver*);
	Boolean RFC_act (double, const SLEntry*);
	Boolean RFC_bot (RATE, const SLEntry*, const SLEntry*, const IHist*);
	Boolean RFC_eot (RATE, const SLEntry*, const SLEntry*, const IHist*);
	double  RFC_cut (double, double);
	Long    RFC_erb (RATE, const SLEntry*, const SLEntry*, double, Long);
	Long    RFC_erd (RATE, const SLEntry*, const SLEntry*, double, Long);

	void setup (
		Long,			// The number of transceivers
		double,			// Range
		int,			// Bits per byte
		int,			// Packet frame (extra physical bits)
		IVMapper **ivcc, 	// Value converters
		MXChannels *mxc = NULL	// Channels
	);
};

#endif

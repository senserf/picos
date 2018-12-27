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
identify "A simple plant";

#define	SERVER_HOST	"localhost"

#define	TTR_ROOT		0.25
#define	HEATING_RATE		0.05
#define	COOLING_RATE		0.05
#define	DELTA_INTERVAL		1.0

#define	bounded(v,mi,ma)	((v) > (ma) ? (ma) : (v) < (mi) ? (mi) : (v))

int	ServerPort, VMin, VMax, Target, SMin, SMax;
int32_t	Output, Setting;
Mailbox *CInt;

// ============================================================================

void set_target () {
//
// We make this symmetrical w.r.t. the middle
//
	double s;

	// Transforms the setting into a number between -1.0 and 1.0
	s = (((double) Setting - (double) SMin) * 2.0 / (SMax - SMin)) - 1.0;

	// Deform
	if (s >= 0.0)
		s = pow (s, TTR_ROOT);
	else
		s = -pow (-s, TTR_ROOT);

	// Transform into the target
	s = round ((double) VMin + ((VMax - VMin) * (s + 1.0) / 2.0));

	Target = (int32_t) bounded (s, VMin, VMax);
}

void update () {
//
// This is the essence of the model: oven state update function
//
	double DOutput, DTarget;

	DOutput = Output;
	DTarget = Target;

	if (DTarget > DOutput)
		// Heating
		DOutput += (DTarget - DOutput) * HEATING_RATE;
	else
		// Cooling
		DOutput += (DTarget - DOutput) * COOLING_RATE;

	Output = (int32_t) round (DOutput);

	trace ("O = %1d [-> %1d]", Output, Target);
}

// ============================================================================

process Actuator {

	states { Loop };

	perform {

		state Loop:

			int32_t val;
			int nc;

			if ((nc = CInt->read ((char*)(&val), 4)) == ERROR)
				excptn ("Connection lost");

			if (nc != ACCEPTED) {
				CInt->wait (4, Loop);
				sleep;
			}

			trace ("U = %d", val);

			val = bounded (val, SMin, SMax);

			if (val != Setting) {
				Setting = val;
				set_target ();
			}

			sameas Loop;
	};
};

process Root {

	states { Start, Sensor, Update };

	void setup () {

		readIn (ServerPort);
		readIn (VMin);	// Skip
		readIn (VMin);
		readIn (VMax);
		readIn (Target);
		readIn (SMin);
		readIn (SMax);

		Output = (int32_t) VMin;
		Setting = (int32_t) SMin;
	};

	perform {

		state Start:

			CInt = create Mailbox;

			if (CInt->connect (INTERNET + CLIENT, SERVER_HOST,
			    ServerPort, 4) != OK)
				excptn ("Cannot connect to controller");

			create Actuator;

		transient Sensor:

			// We behave as sensor

			if (CInt->write ((char*)(&Output), 4) == REJECTED) {
				CInt->wait (OUTPUT, Sensor);
				sleep;
			}

			Timer->delay (DELTA_INTERVAL, Update);

		state Update:

			update ();
			sameas Sensor;
	};
};

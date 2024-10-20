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

#ifndef	__plant_c__
#define	__plant_c__

// This is a generic plant model for constructing plants to be interfaced to
// virtual sensors and actuators in VUEE models of controllers.

#include "plant.h"

void Controller::setSensor (int v) {
//
// Set the value of the sensor and trigger the sensor event. This is formally
// done by the plant to return to the controller its current output.
//
	if (Sensor != v) {
		Sensor = v;
		SensorUpdated = YES;
		trigger_sensor ();
	}
}

void Controller::setActuator (int v) {
//
// Set the value of the actuator providing controller input to the plant. This
// is formally done by the controller.
//
	if (Actuator != v) {
		Actuator = v;
		ActuatorUpdated = YES;
		trigger_actuator ();
	}
}

void Controller::setup (double delta, const char *hostname, int port,
				int node, int sen, int act, int flags) {

	// Parameters for connecting to the controller
	Hostname = hostname;
	Port = port;
	SID = sen;
	AID = act;

	Connected = NO;
	Delta = etuToItu (delta);

	// Build the handshake sequence
	Handshake [0] = 0xBA;
	Handshake [1] = 0xB4;
	Handshake [2] = 0;
	Handshake [3] = 7;	// Connecting to the SENSORS module
	*((uint32_t*)(Handshake+4)) = htonl ((uint32_t)node);
	// This indicates whether we use the SMURPH nod enumber (0) or
	// Host Id (1)
	*((uint32_t*)(Handshake+5)) = htonl ((uint32_t)(flags & 1));

	create SADriver;
}

void Controller::logmess (const char *fmt, ...) {

	VA_TYPE ap;

	va_start (ap, fmt);

	Ouf << ::form ("Time: %9.2f ", ituToEtu (Time)) << ::vform (fmt, ap) <<
		'\n';
	Ouf.flush ();
}

void SADriver::setup () {

	CInt = create Mailbox;
}

void Controller::turnOn () {

	// Called upon connection
	if (Connected)
		// Do nothing
		return;

	Driver = create PDriver;
	Connected = YES;
	SensorUpdated = ActuatorUpdated = YES;
}

void Controller::turnOff () {

	// When disconnected
	if (!Connected)
		// Do nothing
		return;
	Driver->terminate ();
	Connected = NO;
}

// ============================================================================

Boolean SADriver::read (int dsc, int ret, int tim) {
//
// Read RBLength bytes; three return states: disconnection, retry,
// timeout; dsc is mandatory
//
	int nc;

	if ((nc = CInt->read (RBuffer, RBLength)) == ERROR) {
		// Failure, disconnection
#if PLANT_TRACING
		S->logmess ("read failure");
#endif
		proceed (dsc);
	}
	if (nc != ACCEPTED) {
		if (ret != NONE) {
			// Wait for the bytes to show up
			CInt->wait (4, ret);
			if (tim != NONE)
				// Detect timeout while waiting
				Timer->delay (PLANT_CNTRL_TIMEOUT, tim);
			sleep;
		}
		return NO;
	}
	return YES;
}

Boolean SADriver::readln (int dsc, int ret, int tim) {
//
// Read a line of text (until the EOL sentinel)
//
	int nc = CInt->readToSentinel (RBuffer, PLANT_CNTRL_BUFSIZE);

	if (nc == ERROR) {
		// Failure, disconnection
#if PLANT_TRACING
		S->logmess ("readln failure");
#endif
		proceed (dsc);
	}
	if (nc) {
		// Done, we should be checking if everything has been read,
		// but we (somewhat recklessly) assume that the buffer is
		// always long enough
		RBLength = nc;
		return YES;
	}
	if (ret != NONE) {
		// Wait for the data
		CInt->wait (SENTINEL, ret);
		if (tim != NONE)
			// Detect timeout while waiting
			Timer->delay (PLANT_CNTRL_TIMEOUT, tim);
		sleep;
	}
	return NO;
}

Boolean SADriver::write (int dsc, int ret, int tim) {
//
// Write WBLength bytes
//
	if (CInt->write (WBuffer, WBLength) != ACCEPTED) {
		// Failed
		if (CInt->isConnected ()) {
			// We are connected
			if (ret != NONE) {
				// Wait for a retry
				CInt->wait (OUTPUT, ret);
				if (tim != NONE)
					// Detect timeouts while waiting
					Timer->delay (PLANT_CNTRL_TIMEOUT, dsc);
				sleep;
			}
			return NO;
		}
#if PLANT_TRACING
		S->logmess ("write failure");
#endif
		proceed (dsc);
	}
	return YES;
}

// ============================================================================
	
SADriver::perform {

	state Connect:

		// Try to connect until succeed
		if (CInt->connect (INTERNET+CLIENT, S->Hostname, S->Port,
		    PLANT_CNTRL_BUFSIZE) != OK)
			// Failed, delay and try again
			proceed Disconnected;

		// For text-oriented input from the mailbox
		CInt->setSentinel ('\n');

		// Copy the handshake sequence to the buffer
		memcpy (WBuffer, S->Handshake, WBLength = 12);
#if PLANT_TRACING
		S->logmess ("connected");
#endif
	transient Handshake:

		// Send the request
		write (Disconnected, Handshake, Disconnected);
		// Expected length of response
		RBLength = 4;

	transient Wack:

		// Wait for the response or timeout
		read (Disconnected, Wack, Disconnected);

		// Check the code, must be 129
		if ((ntohl (*((uint32_t*)RBuffer)) & 0xff) < 129) {
#if PLANT_TRACING
			S->logmess ("bad ack code");
#endif
			proceed Disconnected;
		}

	transient Rsign:

		// Expect a node signature
		readln (Disconnected, Rsign, Disconnected);
		if (!processSignature ())
			// Invalid
			sameas Disconnected;

		// reset the plant
		S->reset ();
		// and start it
		S->turnOn ();

	transient Dloop:

		// This is the loop executed while connected to the
		// controller

		if (readln (Disconnected)) {
			// Try to read an actuator update, but don't wait for it
			decodeActuatorUpdate ();
			// Keep going
			sameas Dloop;
		}

		if (encodeSensorUpdate ())
			// Write the new sensor value, but only if it has
			// changed since last time
			sameas Sendupdate;

		// Wait for the actuator update
		CInt->wait (SENTINEL, Dloop);
		// And for a sensor change
		Monitor->wait (&(S->Sensor), Dloop);

	state Sendupdate:

		// Send a sensor update
		write (Disconnected, Sendupdate, Disconnected);
		// And keep going
		sameas Dloop;

	state Disconnected:

		// On disconnection or connection failure: turn off the plant;
		// we assume it shouldn't be running without being controlled
		S->turnOff ();
		// Make sure we are in fact disconnected
		CInt->disconnect (CLEAR);
		// And try to connect again after a delay
		Timer->delay (PLANT_CNTRL_RECDELAY, Connect);
			sleep;
}

// ============================================================================

Boolean SADriver::processSignature () {

	char junk [PLANT_CNTRL_BUFSIZE];
	int ni;
	unsigned int nn, hn, to, na, ao, ns, so;
	char ps;

	// Replace the newline with null
	RBuffer [RBLength - 1] = '\0';
	ni = sscanf (RBuffer, "P %u %c %u %u %s %u %u %u %u",
		&nn, &ps, &hn, &to, junk, &na, &ao, &ns, &so);
	if (ni != 9) {
#if PLANT_TRACING
		S->logmess ("bad signature, %1d items", ni);
#endif
		// Something wrong
		return NO;
	}

	// We should check whether the node number is OK, but we trust the
	// simulator, so just get the sensor and actuator
	SID = S->SID + so;
	AID = S->AID + ao;

	if (SID >= ns || AID >= na) {
		// Illegal
#if PLANT_TRACING
		S->logmess ("bad signature, SID %1d, AID %1d", SID, AID);
#endif
		return NO;
	}

	return YES;
}

void SADriver::decodeActuatorUpdate () {

	unsigned int sn, val;
	char junk [64];

	// Replace the newline with null
	RBuffer [RBLength - 1] = '\0';

	if (sscanf (RBuffer, "A %s %u %x", junk, &sn, &val) != 3) {
		// Only accept if valid
		return;
	}

	if (sn != AID)
		return;

	S->setActuator ((int)val);
}

Boolean SADriver::encodeSensorUpdate () {

	int val;

	if (!S->readSensor (val))
		// The value hasn't changed
		return NO;

	sprintf (WBuffer, "S %u 0x%x\nE %u\n", SID, val, SID);
	WBLength = strlen (WBuffer);

	return YES;
}

// ============================================================================

PDriver::perform {

	state Start:

		// Initialize
		NextRunTime = Time;

	transient Loop:

		// Execute one response cycle; this method is provided by
		// the "implementation"
		S->response ();

		// When we are going to run again
		NextRunTime += S->Delta;

		if (NextRunTime > Time)
			// It is actually in the future, so wait for the
			// residual delay
			Timer->wait (NextRunTime - Time, Loop);
		else
			// In case we are running late
			proceed Loop;
}

#endif

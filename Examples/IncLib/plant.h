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

#ifndef	__plant_h__
#define	__plant_h__

#define	PLANT_OPTION_HID	0x01

#define	PLANT_CNTRL_BUFSIZE	256

#define	PLANT_CNTRL_TIMEOUT	5.0	// Seconds
#define	PLANT_CNTRL_RECDELAY	5.0	// Seconds

#ifndef	PLANT_TRACING
#define	PLANT_TRACING		0
#endif

station Controller {

	// These are the configuration parameters
	const char	*Hostname;	// How to connect to the controller
	int		Port,		// TCP port
			SID, AID; 	// Sensor/actuator IDs at the controller
	TIME		Delta;		// The time quantum

	// The request sent to the VUEE model to connect to the Sensor module
	// at the controller node
	byte		Handshake [12];

	// The values of the sensor and the actuator
	int		Sensor, Actuator;

	// Flags
	Boolean		SensorUpdated, ActuatorUpdated, Connected;

	Process		*Driver;	// The driver process

	// Called to calculate the plan't response
	virtual	void response () { };

	// Called to reset the plant to the initial state
	virtual void reset () { };

	inline Boolean isConnected () { return Connected; };
	
	inline Boolean readActuator (int &val) {
		// ActuatorUpdated is set when there has been a change in the
		// value, but the process hasn't seen it yet, so we don't miss
		// updates happening while we haven't been waiting for the
		// respective event
		val = Actuator;
		if (ActuatorUpdated) {
			ActuatorUpdated = NO;
			return YES;
		}
		return NO;
	};

	inline Boolean readSensor (int &val) {
		val = Sensor;
		if (SensorUpdated) {
			SensorUpdated = NO;
			return YES;
		}
		return NO;
	};

	inline void wait_for_actuator (int st) {
		Monitor->wait ((void*)&Actuator, st);
	};

	inline void trigger_actuator () {
		Monitor->signal ((void*)&Actuator);
	};

	inline void wait_for_sensor (int st) {
		Monitor->wait ((void*)&Sensor, st);
	};

	inline void trigger_sensor () {
		Monitor->signal ((void*)&Sensor);
	};

	void setSensor (int);
	void setActuator (int);

	void setup (double, const char*, int, int, int, int, int);

	void turnOn (),			// Called upon connection
	     turnOff ();		// Called upon disconnection

	void logmess (const char*, ...);

};

process SADriver (Controller) {
//
// This process connects the plant model to the controller, receives updates
// to the Actuator, and sends out updates to the Sensor
//
	Mailbox	*CInt;
	char	RBuffer [PLANT_CNTRL_BUFSIZE],	// input buffer
		WBuffer [PLANT_CNTRL_BUFSIZE];	// output buffer

	int	SID, AID,	// Raw (offset) Id's of the sensor/actuator
		RBLength,	// Length of data in the buffer
		WBLength;

	Boolean read (int, int rt = NONE, int tm = NONE);
	Boolean readln (int, int rt = NONE, int tm = NONE);
	Boolean write (int, int rt = NONE, int tm = NONE);

	Boolean processSignature ();
	Boolean encodeSensorUpdate ();
	void decodeActuatorUpdate ();

	void setup ();

	states { Connect, Handshake, Wack, Rsign, Dloop, Sendupdate,
								Disconnected };
	perform;

};

process PDriver (Controller) {
//
// This process implements the plant, i.e., it exacutes at Delta intervals the
// response function
//
	TIME	NextRunTime;

	states { Start, Loop };

	perform;
};

#define	TheController	((Controller*)TheStation)

#endif

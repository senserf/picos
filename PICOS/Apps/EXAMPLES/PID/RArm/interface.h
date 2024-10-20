/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __interface_h__
#define	__interface_h__

// Sensor/actuator identifiers in the controller
#define	THE_SENSOR	0
#define	THE_ACTUATOR	0

// Sensor range, i.e., the arm position indication 0-full left, 2047-full right
#define	SENSOR_MIN	0
#define	SENSOR_MAX	2047
#define	SENSOR_ZERO	0	// This is irrelevant in the model

// Actuator range, i.e., the valid motor actuator settings. The minimum is 1
// (full backward), and the maximum is 1023 (full forward), with 512 meaning
// off
#define	ACTUATOR_MIN	1
#define	ACTUATOR_MAX	1023
#define	ACTUATOR_ZERO	512

#endif

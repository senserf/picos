/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __params_h__
#define	__params_h__

// ============================================================================
// Parameters:
//
// DELTA_T
// 	Interval in seconds between two consecutive iterations
// MAX_FORCE_F
//	The maximum force in the direction of motion applicable by the motor
//	in N
// MAX_FORCE_B
//	The braking force, i.e., the force applied backwards to the direction
//	of motion to brake the arm or to change the direction
// MIN_FORCE
//	The minimum force required to move a stationary arm per 1kg of mass,
//	i.e., the static friction force
// ARM_RANGE
// 	This is the distance assigned to the full range of the arm's movement
//	in m
// ARM_MASS
//	The tare mass of the arm, anything carried by the arm adds to this
// MAX_SPEED
//	The maximum speed at which the arm can move
// ============================================================================
	

#define	DELTA_T		0.01
#define	MAX_FORCE_F	0.5	// Maximum force in the direction of motion
#define	MAX_FORCE_B	4.0	// The braking force
#define	MIN_FORCE	0.01
#define	ARM_RANGE	2.0	// Meters
#define	ARM_MASS	0.3	// kg
#define	MAX_SPEED	1.0	// m/s

// ============================================================================
#endif

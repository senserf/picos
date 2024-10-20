/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "sensors.h"
#include "actuators.h"
#include "plant.h"

#define	enforce_bounds(v,min,max) \
	do { \
	     if ((v) < (min)) (v) = (min); else if ((v) > (max)) (v) = (max); \
	} while (0)

	// Default control interval: 16 PicOS milliseconds, i.e., 16/1024 = 
	// 0.015625 sec
word	control_interval = 16,
	// Every this many millisecs, i.e., every second, we report the state
	// to the UART
	monitor_interval = 1024;

	// In our arithmetic, the calculations are carried out assuming the
	// coefficients are scaled up by the factor of 100, so we basically
	// view these as percentages. We also use long signed arithmetics in
	// the calculations to make sure we do not accidentally go out of
	// range.
lint	Kp = 0, Ki = 0, Kd = 0;

	// Tells whether the controller is running
Boolean	Active = NO;

lint	setpoint,		// The target (aka the setpoint)
	output,			// Plant output
	setting,		// Plant input
	error,			// The error
	previous_error,		// For calculating the "derivative"
	integral,		// The accumulated integral
	derivative;		// The current derivative

// ============================================================================

fsm controller;

void get_error (word st) {
//
// Returns the error between the setpoint and current plant output
//
	word val;

	// This may involve waiting
	read_sensor (st, THE_SENSOR, &val);

	// Make sure the input is sane, i.e., within bounds
	enforce_bounds (val, SENSOR_MIN, SENSOR_MAX);

	output = (lint) val;
	// The range of error covers the range of sensor indications, so at
	// this stage the error is absolute
	error = setpoint - output;
}

void set_plant (word st) {
//
// Set the plant input, i.e., set the actuator to the new setting
//
	word val = (word) setting;

	write_actuator (st, THE_ACTUATOR, &val);
}

void update_integral () {
//
// We assume, as one possible (IMHO the most natural) remedy for the windup
// problem, that the integrator is reset whenever the error gets to (or crosses)
// zero
//
	// Specifically, we go like this:
	if (Ki == 0 || error == 0) {
		// Switched off or error at zero
		integral = 0;
		return;
	}
	if (previous_error > 0 && error < 0 ||
	    previous_error < 0 && error > 0) {
		// Crossed zero, start at where we are now
		integral = error;
		return;
	}

	// Otherwise, keep going as prescribed
	integral += error;
}

void update_derivative () {
//
// This is just the difference
//
	derivative = error - previous_error;
}

void calculate_new_input () {
//
// This is the actual control function calculating the new input to the plant
//
	lint delta;

	// New plant input calculated as positive or negative "delta" with
	// respect to the zero setting
	delta = ((
		// First the proportional component. Note that we assume that
		// Kp are percentages.
		Kp * error +
		// Now the integral component. As the dt for the integration is
		// equal to control_interval, which is a (settable) fraction of
		// the second, we pre-scale the component multiplying it by
		// control_interval/1024, which basically averages the error
		// over one second.
		(Ki * integral * control_interval)/1024 +
		// Now the derivative. As it is taken over control_interval,
		// we multiply it by 1024/control_interval, i.e., the reciprocal
		// of the factor applied to the integral component, again
		// pre-scaling it to 1 sec.
		(Kd * derivative * 1024)/control_interval
		// Now we normalize whatever comes out of this to the range of
		// the values returned by the sensor (so it is formally between
		// -1 and +1) and expand that to the actuator's range.
		) * (ACTUATOR_MAX - ACTUATOR_ZERO)) /
		// Then, as the K coeffs are percentages, we divide everything
		// by 100, which is factored into the denominator. This way we
		// carry out all the computations using integer arithmetics.
		((lint)(SENSOR_MAX - SENSOR_MIN) * 100);

	// The calculated delta is applied relative to the "zero" value of the
	// actuator
	setting = ACTUATOR_ZERO + delta;

	// Now we make sure that the bounds are obeyed
	enforce_bounds (setting, ACTUATOR_MIN, ACTUATOR_MAX);
}

Boolean start () {

	if (Active)
		return NO;

	previous_error = integral = output = 0;
	setting = ACTUATOR_ZERO;
	runfsm controller;
	trigger (&monitor_interval);

	Active = YES;
	return YES;
}

Boolean stop () {

	if (Active) {
		killall (controller);
		Active = NO;
		return YES;
	}

	return NO;
}

void control_cycle (word st) {
//
// Run through one control cycle
//
	get_error (st);

	update_integral ();
	update_derivative ();
	calculate_new_input ();

	// For the next cycle
	previous_error = error;
}

// ============================================================================

fsm controller {

	state LOOP:

		set_plant (LOOP);
		delay (control_interval, GET_RESPONSE);
		release;

	state GET_RESPONSE:

		control_cycle (GET_RESPONSE);
		sameas LOOP;
}

fsm monitor {

	state LOOP:

		ser_outf (LOOP, "%lu : %ld %ld <- %ld "
					"[E: %ld, I: %ld, D: %ld]\r\n",
						seconds (),
						setting,
						setpoint,
						output,
						error,
						integral,
						derivative);

		if (Active)
			// Keep running periodically for as long as active
			delay (monitor_interval, LOOP);

		// Always run on this event
		when (&monitor_interval, LOOP);
}

// ============================================================================

fsm root {

	char cmd [64];

	state INIT:

		runfsm monitor;

	state BANNER:

		ser_out (BANNER,
			"Commands:\r\n"
			"  c v [set control interval]\r\n"
			"  m v [set logging frequency]\r\n"
			"  t v [set the setpoint]\r\n"
			"  a v [set the actuator]\r\n"
			"  p v [set Kp]\r\n"
			"  i v [set Ki]\r\n"
			"  d v [set Kd]\r\n"
			"  r [run] \r\n"
			"  s [stop]\r\n"
			"  v [view, show]\r\n"
			"  . [cycle step]\r\n"
		);

	state UART_INPUT:

		lint a, b, c;

		ser_in (UART_INPUT, cmd, 64);

		switch (cmd [0]) {

		    case 'c' :

			// Set the control interval
			a = 0;
			scan (cmd + 1, "%ld", &a);
			if (a < 16 || a > 32768)
				sameas ILLEGAL_PARAMETER;
			control_interval = (word) a;
			sameas UART_INPUT;

		    case 'm' :

			// Set the monitor interval
			a = 0;
			scan (cmd + 1, "%ld", &a);
			if (a > 32768)
				sameas ILLEGAL_PARAMETER;
			// zero disables
			monitor_interval = (word) a;
			trigger (&monitor_interval);
			sameas UART_INPUT;

		    case 't' :
	
			// Set the target (setpoint)
			a = 0;
			scan (cmd + 1, "%ld", &a);
			if (a < SENSOR_MIN || a > SENSOR_MAX)
				sameas ILLEGAL_PARAMETER;
			// zero disables
			setpoint = (word) a;
			sameas UART_INPUT;

		    case 'a' :

			// Set the actuator
			a = 0;
			scan (cmd + 1, "%ld", &a);
			if (a < ACTUATOR_MIN || a > ACTUATOR_MAX)
				sameas ILLEGAL_PARAMETER;

			setting = a;
			sameas SET_PLANT;

		    case 'p' :

			a = 0;
			scan (cmd + 1, "%ld", &a);
			Kp = a;
			sameas UART_INPUT;

		    case 'i' :

			a = 0;
			scan (cmd + 1, "%ld", &a);
			Ki = a;
			sameas UART_INPUT;

		    case 'd' :
	
			a = 0;
			scan (cmd + 1, "%ld", &a);
			Kd = a;
			sameas UART_INPUT;

		    case 'r' :

			if (!start ())
				sameas RUNNING_ALREADY;
			sameas UART_INPUT;

		    case 's' :

			if (!stop ())
				sameas STOPPED_ALREADY;
			sameas UART_INPUT;

		    case 'v' :

			proceed SHOW_PARAMS;

		    case '.' :

			// Empty line, run a cycle
			if (Active)
				sameas RUNNING_ALREADY;
			proceed RUN_CYCLE;
		}

	state BAD_COMMAND:

		ser_out (BAD_COMMAND, "bad command!\r\n");
		sameas BANNER;

	state ILLEGAL_PARAMETER:

		ser_out (ILLEGAL_PARAMETER, "llegal parameter!\r\n");
		sameas BANNER;

	state RUNNING_ALREADY:

		ser_out (RUNNING_ALREADY, "running already!\r\n");
		sameas UART_INPUT;

	state STOPPED_ALREADY:

		ser_out (STOPPED_ALREADY, "stopped already!\r\n");
		sameas UART_INPUT;

	state SET_PLANT:

		set_plant (SET_PLANT);
		trigger (&monitor_interval);
		proceed UART_INPUT;

	state SHOW_PARAMS:

		ser_outf (SHOW_PARAMS,
	        "c=%ld, m=%ld, t=%ld, a=%ld, p=%ld, i=%ld, d=%ld\r\n",
			control_interval,
			monitor_interval,
			setpoint,
			setting,
			Kp, Ki, Kd);

		proceed UART_INPUT;

	state RUN_CYCLE:

		control_cycle (RUN_CYCLE);
		sameas SET_PLANT;

}

// ============================================================================

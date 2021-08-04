/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// IRQ service for the MPU9250 accelerometer
//
if (mpu9250_int) {

	// Indicate that the event has occurred
	_BIS (mpu9250_status, MPU9250_STATUS_EVENT);
	// Prevent further interrupts, but don't clear
	mpu9250_disable;

	if (mpu9250_status & MPU9250_STATUS_WAIT) {
		// A process is waiting
		i_trigger ((aword)(&mpu9250_status));
		_BIC (mpu9250_status, MPU9250_STATUS_WAIT);
		RISE_N_SHINE;
	}
}

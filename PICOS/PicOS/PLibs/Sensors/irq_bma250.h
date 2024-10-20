/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// IRQ service for the BMA250 accelerometer
//
if (bma250_int) {

	bma250_clear;

	_BIS (bma250_status, BMA250_STATUS_EVENT);
	bma250_disable;

	if (bma250_status & BMA250_STATUS_WAIT) {
		i_trigger ((aword)(&bma250_status));
		_BIC (bma250_status, BMA250_STATUS_WAIT);
		RISE_N_SHINE;
	}
}

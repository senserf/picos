/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
if (pin_sensor_int) {
	i_trigger ((aword)(&__input_pins));
	__pinsen_disable_and_clear;
	RISE_N_SHINE;
}

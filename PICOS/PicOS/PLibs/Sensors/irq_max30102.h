/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// IRQ service for the MAX30102 oximeter
//
if (max30102_int) {

	max30102_clear;
	max30102_disable;
	i_trigger (max30102_event);
	RISE_N_SHINE;
}

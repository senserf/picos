/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// IRQ service for the TMP007 thermopile
//
if (tmp007_int) {

	tmp007_clear;

	_BIS (tmp007_status, TMP007_STATUS_EVENT);
	tmp007_disable;

	if (tmp007_status & TMP007_STATUS_WAIT) {
		i_trigger ((aword)(&tmp007_status));
		_BIC (tmp007_status, TMP007_STATUS_WAIT);
		RISE_N_SHINE;
	}
}

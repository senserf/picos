/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
if (buttons_int) {
	buttons_disable ();
	i_trigger ((aword)&__button_list);
	RISE_N_SHINE;
}

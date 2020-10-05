/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "ser.h"

fsm root {
	state START:
		ser_out (START, "Hello world!!\r\n");
		finish;
}

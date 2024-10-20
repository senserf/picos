/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "kernel.h"
#include "as3932.h"

interrupt (AS3932_TCI_VECTOR) as3932_timer_int () {

#include "irq_as3932_t.h"

	RTNI;
}

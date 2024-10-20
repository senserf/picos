/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "kernel.h"
#include "dm2200.h"

interrupt (TIMERA0_VECTOR) dm2200_st_int () {

// Signal strobe for transmission
#include "irq_dm2200_xmt.h"

	RTNI;

}

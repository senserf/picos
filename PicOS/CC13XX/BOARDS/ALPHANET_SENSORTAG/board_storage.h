/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "pins.h"
#include "storage_mx25r8035.h"

#define	ee_bring_up	CNOP
#define	ee_bring_down	CNOP

#define	ee_start	GPIO_setOutputEnableDio (IOID_14, GPIO_OUTPUT_ENABLE)

#define	ee_stop		GPIO_setOutputEnableDio (IOID_14, GPIO_OUTPUT_DISABLE)

#define	ee_clkh		GPIO_setDio (IOID_17)
#define	ee_clkl		GPIO_clearDio (IOID_17)

#define	ee_outh		GPIO_setDio (IOID_19)
#define	ee_outl		GPIO_clearDio (IOID_19)

#define	ee_inp		GPIO_readDio (IOID_18)

// This should properly power down the flash chip
// disabled for debugging
#define	EXTRA_INITIALIZERS	do { ee_open (); ee_close (); } while (0)

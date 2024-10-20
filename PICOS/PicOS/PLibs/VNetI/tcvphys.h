/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __tcvphys_h
#define __tcvphys_h

#include "sysio.h"

/*
 * This is the type of the control function for phys that is provided by the
 * phys when it registers with TCV.
 */
typedef	int (*ctrlfun_t) (int option, address);
typedef	int (*pktqual_t) (address);

#define	tcvphy_isurgent(p)	tcv_isurgent (p)

#ifndef	__SMURPH__

int tcvphy_reg (int, ctrlfun_t, int);
int tcvphy_rcv (int, address, int);
address tcvphy_get (int, int*);
address tcvphy_top (int);
int tcvphy_erase (int, pktqual_t);
void tcvphy_end (address);

#endif

#endif

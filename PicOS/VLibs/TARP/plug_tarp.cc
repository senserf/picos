/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "tcvplug.h"
#include "tarp.h"

extern void tarp_init(void);
extern int tarp_rx (address buffer, int length, int *ses);
extern int tarp_tx (address buffer);

#if TARP_RTR
extern int tarp_xmt (address buffer);
#endif

/**************************
 *  SiTARP plugin
 *  (Simplistic TARP)
 *************************/

static int tcv_ope_tarp (int, int, va_list);
static int tcv_clo_tarp (int, int);
static int tcv_rcv_tarp (int, address, int, int*, tcvadp_t*);
static int tcv_frm_tarp (address, tcvadp_t*);
static int tcv_out_tarp (address);
static int tcv_xmt_tarp (address);

#if TARP_RTR
static int tcv_tmt_tarp (address);
#else
#define tcv_tmt_tarp NULL
#endif

trueconst tcvplug_t plug_tarp =
		{ tcv_ope_tarp, tcv_clo_tarp, tcv_rcv_tarp, tcv_frm_tarp,
			tcv_out_tarp, tcv_xmt_tarp, tcv_tmt_tarp,
				INFO_PLUG_TARP };

static int* desc = NULL;

#ifdef	myName
#undef myName
#endif

#define myName "pl_tarp"

static int tcv_ope_tarp (int phy, int fd, va_list plid) {

/*
 * This is very simple - we are allowed to have one descriptor per phy.
 */
// why more sessions? (looking for scenarios)

	int i;

	if (desc == NULL) {
		desc = (int*) umalloc (sizeof (int) * TCV_MAX_PHYS);
		if (desc == NULL)
			syserror (EMALLOC, myName);
		for (i = 0; i < TCV_MAX_PHYS; i++)
			desc [i] = NONE;
	}

	/* phy has been verified by TCV */
	if (desc [phy] != NONE) {
		dbg_2 (0xA000); // tcv_ope_tarp phy?
		diag ("%s: phy?", myName);
		return ERROR;
	}

	desc [phy] = fd;

	tarp_init ();
	return 0;
}

static int tcv_clo_tarp (int phy, int fd) {

	/* phy/fd has been verified */

	if (desc == NULL || desc [phy] != fd) {
		dbg_2 (0xB000); // tcv_clo_tarp desc
		diag ("%s: desc?", myName);
		return ERROR;
	}

	desc [phy] = NONE;
	return 0;
}

static int tcv_rcv_tarp (int phy, address p, int len, int *ses, tcvadp_t *bounds) {
	int	rc;
	if (desc == NULL || (*ses = desc [phy]) == NONE) {
		return TCV_DSP_PASS;
	}

	// ethernet header handling could be here, but we'd need
	// bounds in tcv_out_tarp as well (?)
	bounds->head = bounds->tail = 0;

	rc = tarp_rx (p, len, ses);
	return rc;
	
}

static int tcv_frm_tarp (address p, tcvadp_t *bounds) {

	// can't use this one if framing (e.g. ethernet) is done in the application
	return bounds->head = bounds->tail = 0;

}

static int tcv_out_tarp (address p) {
	int rc = tarp_tx (p);
	return rc;

}

static int tcv_xmt_tarp (address p) {
#if TARP_RTR
	return tarp_xmt (p);
#else
	return TCV_DSP_DROP;
#endif
}

#if TARP_RTR
static int tcv_tmt_tarp (address p) {

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)

	if (p[(tcv_tlength(p) >> 1) -1] != tarp_pxopts) {

#ifdef __SMURPH__
#ifndef	SOFTWARE_CRC
#define	SOFTWARE_CRC	0
#endif
#if !SOFTWARE_CRC
		trace ("tcv_tmt_tarp PXOPTS: %u %u", tarp_pxopts, p[(tcv_tlength(p) >> 1) -1]);
#endif
#endif

		p[(tcv_tlength(p) >> 1) -1] = tarp_pxopts;
	}
#endif

	// tarp rtr monitoring is in tarp_xmt and tarp_rx, as they deal
	// with tarp structs. Here, just re-xmt as urgent.
	return TCV_DSP_XMTU;
}
#endif

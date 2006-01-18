/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "tcvplug.h"
#include "tarp.h"
/**************************
 *  SiTARP plugin
 *  (Simplistic TARP)
 *************************/

extern void tarp_init();
extern int tarp_rx (address buffer, int length, int *ses);
extern int tarp_tx (address buffer);

static int tcv_ope (int, int, va_list);
static int tcv_clo (int, int);
static int tcv_rcv (int, address, int, int*, tcvadp_t*);
static int tcv_frm (address, int, tcvadp_t*);
static int tcv_out (address);
static int tcv_xmt (address);

const tcvplug_t plug_tarp =
		{ tcv_ope, tcv_clo, tcv_rcv, tcv_frm, tcv_out, tcv_xmt, NULL,
			INFO_PLUG_TARP };

// ********************** in plug_null, desc should be static as well (?)
static int	*desc = NULL;
static const char myName[] = "pl_tarp";

static int tcv_ope (int phy, int fd, va_list plid) {

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
		dbg_2 (0xA000); // tcv_ope phy?
		diag ("%s: phy?", myName);
		return ERROR;
	}

	desc [phy] = fd;

	tarp_init();
	return 0;
}

static int tcv_clo (int phy, int fd) {

	/* phy/fd has been verified */

	if (desc == NULL || desc [phy] != fd) {
		dbg_2 (0xB000); // tcv_clo desc
		diag ("%s: desc?", myName);
		return ERROR;
	}

	desc [phy] = NONE;
	return 0;
}

static int tcv_rcv (int phy, address p, int len, int *ses, tcvadp_t *bounds) {
	int	rc;
	if (desc == NULL || (*ses = desc [phy]) == NONE) {
		return TCV_DSP_PASS;
	}

	// ethernet header handling could be here, but we'd need
	// bounds in tcv_out as well (?)
	bounds->head = bounds->tail = 0;

	rc = tarp_rx (p, len, ses);
	return rc;
	
}

static int tcv_frm (address p, int phy, tcvadp_t *bounds) {

	// can't use this one if framing (e.g. ethernet) is done in the application
	return bounds->head = bounds->tail = 0;

}

static int tcv_out (address p) {
	int rc = tarp_tx (p);
	return rc;

}

static int tcv_xmt (address p) {
	return TCV_DSP_DROP;
}

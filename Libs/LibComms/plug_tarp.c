/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "tcvplug.h"
#include "tarp.h"
#include "diag.h"
/*
 *  SiTARP plugin
 *  (Simplistic TARP)
 */

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

static int tcv_ope (int phy, int fd, va_list plid) {
	static char * myName = "plug_tarp::tcv_ope";

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
		net_diag (D_FATAL, "%s: phy?", myName);
		return ERROR;
	}

	desc [phy] = fd;
	net_diag (D_DEBUG, "%s: 0", myName);

	tarp_init();
	return 0;
}

static int tcv_clo (int phy, int fd) {
	static char * myName = "plug_tarp::tcv_clo";

	/* phy/fd has been verified */

	if (desc == NULL || desc [phy] != fd) {
		net_diag (D_SERIOUS, "%s: desc?", myName);
		return ERROR;
	}

	net_diag (D_DEBUG, "%s: 0", myName);
	desc [phy] = NONE;
	return 0;
}

static int tcv_rcv (int phy, address p, int len, int *ses, tcvadp_t *bounds) {
	static char * myName = "plug_tarp::tcv_rcv";
	int	rc;
	//address dup;
	if (desc == NULL || (*ses = desc [phy]) == NONE) {
		net_diag (D_DEBUG, "%s: TCV_DSP_PASS", myName);
		return TCV_DSP_PASS;
	}

	// ethernet header handling could be here, but we'd need
	// bounds in tcv_out as well (?)
	bounds->head = bounds->tail = 0;

	rc = tarp_rx (p, len, ses);
	net_diag (D_DEBUG, "%s: %d", myName, rc);
	return rc;
	
/* moved to tarp_rx
	if ((rc = tarp_rx (p, len)) >= 0) {
		net_diag (D_DEBUG, "%s: %d", myName, rc);
		return rc;
	}

	// clone() from tcv_rcv -- how to??
	// we want XMT and RCV...
	if ((dup = tcvp_new (len, TCV_DSP_XMT, *ses)) == NULL) {
		net_diag (D_WARNING, "%s: Dup failed", myName);
		return TCV_DSP_DROP;
	}
	memcpy ((char *)dup, (char *)p, len); // we'll see...
	// intent: at return, dup goes XMT, p becomes a packet and goes up to the app
	return TCV_DSP_RCV;
*/
}

static int tcv_frm (address p, int phy, tcvadp_t *bounds) {

	// can't use this one if framing (e.g. ethernet) is done in the application
	return bounds->head = bounds->tail = 0;

}

static int tcv_out (address p) {
	static char * myName = "plug_tarp::tcv_out";
	int rc = tarp_tx (p);

	net_diag (D_DEBUG, "%s: %d", myName, rc);
	return rc;

}

static int tcv_xmt (address p) {
	return TCV_DSP_DROP;
}

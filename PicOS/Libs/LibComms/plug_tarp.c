/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	__SMURPH__

#include "node.h"

#else

#include "tcvplug.h"
extern void tarp_init();
extern int tarp_rx (address buffer, int length, int *ses);
extern int tarp_tx (address buffer);

#endif

#include "tarp.h"
/**************************
 *  SiTARP plugin
 *  (Simplistic TARP)
 *************************/

static int tcv_ope (int, int, va_list);
static int tcv_clo (int, int);
static int tcv_rcv (int, address, int, int*, tcvadp_t*);
static int tcv_frm (address, int, tcvadp_t*);
static int tcv_out (address);
static int tcv_xmt (address);

const tcvplug_t plug_tarp =
		{ tcv_ope, tcv_clo, tcv_rcv, tcv_frm, tcv_out, tcv_xmt, NULL,
			INFO_PLUG_TARP };

#include "plug_tarp_node_data.h"

static const char myName[] = "pl_tarp";

#define	desc_na		__NA (desc)
#define	tarp_init_na	__NA (tarp_init)
#define	tarp_rx_na	__NA (tarp_rx)
#define	tarp_tx_na	__NA (tarp_tx)

static int tcv_ope (int phy, int fd, va_list plid) {

/*
 * This is very simple - we are allowed to have one descriptor per phy.
 */
// why more sessions? (looking for scenarios)

	int i;

	if (desc_na == NULL) {
		desc_na = (int*) umalloc (sizeof (int) * TCV_MAX_PHYS);
		if (desc_na == NULL)
			syserror (EMALLOC, myName);
		for (i = 0; i < TCV_MAX_PHYS; i++)
			desc_na [i] = NONE;
	}

	/* phy has been verified by TCV */
	if (desc_na [phy] != NONE) {
		dbg_2 (0xA000); // tcv_ope phy?
		diag ("%s: phy?", myName);
		return ERROR;
	}

	desc_na [phy] = fd;

	tarp_init_na ();
	return 0;
}

static int tcv_clo (int phy, int fd) {

	/* phy/fd has been verified */

	if (desc_na == NULL || desc_na [phy] != fd) {
		dbg_2 (0xB000); // tcv_clo desc
		diag ("%s: desc?", myName);
		return ERROR;
	}

	desc_na [phy] = NONE;
	return 0;
}

static int tcv_rcv (int phy, address p, int len, int *ses, tcvadp_t *bounds) {
	int	rc;
	if (desc_na == NULL || (*ses = desc_na [phy]) == NONE) {
		return TCV_DSP_PASS;
	}

	// ethernet header handling could be here, but we'd need
	// bounds in tcv_out as well (?)
	bounds->head = bounds->tail = 0;

	rc = tarp_rx_na (p, len, ses);
	return rc;
	
}

static int tcv_frm (address p, int phy, tcvadp_t *bounds) {

	// can't use this one if framing (e.g. ethernet) is done in the application
	return bounds->head = bounds->tail = 0;

}

static int tcv_out (address p) {
	int rc = tarp_tx_na (p);
	return rc;

}

static int tcv_xmt (address p) {
	return TCV_DSP_DROP;
}

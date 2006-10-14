/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	__SMURPH__

#include "board.h"

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

static int tcv_ope_tarp (int, int, va_list);
static int tcv_clo_tarp (int, int);
static int tcv_rcv_tarp (int, address, int, int*, tcvadp_t*);
static int tcv_frm_tarp (address, int, tcvadp_t*);
static int tcv_out_tarp (address);
static int tcv_xmt_tarp (address);

const tcvplug_t plug_tarp =
		{ tcv_ope_tarp, tcv_clo_tarp, tcv_rcv_tarp, tcv_frm_tarp,
			tcv_out_tarp, tcv_xmt_tarp, NULL,
				INFO_PLUG_TARP };

#ifndef	__SMURPH__
#include "plug_tarp_node_data.h"
#endif

#ifdef	myName
#undef myName
#endif

#define myName "pl_tarp"

#define	desc_na		__NA (TNode, desc)
#define	tarp_init_na	__NA (TNode, tarp_init)
#define	tarp_rx_na	__NA (TNode, tarp_rx)
#define	tarp_tx_na	__NA (TNode, tarp_tx)

static int tcv_ope_tarp (int phy, int fd, va_list plid) {

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
		dbg_2 (0xA000); // tcv_ope_tarp phy?
		diag ("%s: phy?", myName);
		return ERROR;
	}

	desc_na [phy] = fd;

	tarp_init_na ();
	return 0;
}

static int tcv_clo_tarp (int phy, int fd) {

	/* phy/fd has been verified */

	if (desc_na == NULL || desc_na [phy] != fd) {
		dbg_2 (0xB000); // tcv_clo_tarp desc
		diag ("%s: desc?", myName);
		return ERROR;
	}

	desc_na [phy] = NONE;
	return 0;
}

static int tcv_rcv_tarp (int phy, address p, int len, int *ses, tcvadp_t *bounds) {
	int	rc;
	if (desc_na == NULL || (*ses = desc_na [phy]) == NONE) {
		return TCV_DSP_PASS;
	}

	// ethernet header handling could be here, but we'd need
	// bounds in tcv_out_tarp as well (?)
	bounds->head = bounds->tail = 0;

	rc = tarp_rx_na (p, len, ses);
	return rc;
	
}

static int tcv_frm_tarp (address p, int phy, tcvadp_t *bounds) {

	// can't use this one if framing (e.g. ethernet) is done in the application
	return bounds->head = bounds->tail = 0;

}

static int tcv_out_tarp (address p) {
	int rc = tarp_tx_na (p);
	return rc;

}

static int tcv_xmt_tarp (address p) {
	return TCV_DSP_DROP;
}

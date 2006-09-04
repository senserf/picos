/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#ifdef	__SMURPH__
// The simulator
#include "node.h"
#else
// The real world
#include "tcvplug.h"
#endif

/*
 * This implements a trivial sample plugin whereby we are allowed to have
 * one session per phy interface.
 */

// Note: under the simulator, plugin functions are static. Add the moment,
// this implies that all stations use the same plugin(s), although, of course,
// they may use different parameters, which are node attributes.

static int tcv_ope (int, int, va_list);
static int tcv_clo (int, int);
static int tcv_rcv (int, address, int, int*, tcvadp_t*);
static int tcv_frm (address, int, tcvadp_t*);
static int tcv_out (address);
static int tcv_xmt (address);

const tcvplug_t plug_null =
		{ tcv_ope, tcv_clo, tcv_rcv, tcv_frm, tcv_out, tcv_xmt, NULL,
			0x0001 /* Plugin Id */ };

#ifndef	__SMURPH__
#include "plug_null_node_data.h"
#endif

// This is how we should access node attributes, i.e., the plugin's dynamic
// data. __NA expands into TheNode-> in the simulator.
#define	desc_na	__NA (desc)

static int tcv_ope (int phy, int fd, va_list plid) {
/*
 * This is very simple - we are allowed to have one descriptor per phy.
 */
	int i;

	if (desc_na == NULL) {
		desc_na = (int*) umalloc (sizeof (int) * TCV_MAX_PHYS);
		if (desc_na == NULL)
			syserror (EMALLOC, "plug_null tcv_ope");
		for (i = 0; i < TCV_MAX_PHYS; i++)
			desc_na [i] = NONE;
	}

	/* phy has been verified by TCV */
	if (desc_na [phy] != NONE)
		return ERROR;

	desc_na [phy] = fd;
	return 0;
}

static int tcv_clo (int phy, int fd) {

	/* phy/fd has been verified */

	if (desc_na == NULL || desc_na [phy] != fd)
		return ERROR;

	desc_na [phy] = NONE;
	return 0;
}

static int tcv_rcv (int phy, address p, int len, int *ses, tcvadp_t *bounds) {

	if (desc_na == NULL || (*ses = desc_na [phy]) == NONE)
		return TCV_DSP_PASS;

	bounds->head = bounds->tail = 0;

	return TCV_DSP_RCV;
}

static int tcv_frm (address p, int phy, tcvadp_t *bounds) {

	return bounds->head = bounds->tail = 0;
}

static int tcv_out (address p) {

	return TCV_DSP_XMT;

}

static int tcv_xmt (address p) {

	return TCV_DSP_DROP;
}

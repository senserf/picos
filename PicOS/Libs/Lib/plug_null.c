/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#ifdef	__SMURPH__
// The simulator
#include "board.h"
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

static int tcv_ope_null (int, int, va_list);
static int tcv_clo_null (int, int);
static int tcv_rcv_null (int, address, int, int*, tcvadp_t*);
static int tcv_frm_null (address, int, tcvadp_t*);
static int tcv_out_null (address);
static int tcv_xmt_null (address);

const tcvplug_t plug_null =
		{ tcv_ope_null, tcv_clo_null, tcv_rcv_null, tcv_frm_null,
			tcv_out_null, tcv_xmt_null, NULL,
				0x0001 /* Plugin Id */ };

#ifndef	__SMURPH__
#include "plug_null_node_data.h"
#endif

// This is how we should access node attributes, i.e., the plugin's dynamic
// data. __NA expands into TheNode-> in the simulator.
#define	ndesc_na	__NA (NNode, desc)

static int tcv_ope_null (int phy, int fd, va_list plid) {
/*
 * This is very simple - we are allowed to have one descriptor per phy.
 */
	int i;

	if (ndesc_na == NULL) {
		ndesc_na = (int*) umalloc (sizeof (int) * TCV_MAX_PHYS);
		if (ndesc_na == NULL)
			syserror (EMALLOC, "plug_null tcv_ope_null");
		for (i = 0; i < TCV_MAX_PHYS; i++)
			ndesc_na [i] = NONE;
	}

	/* phy has been verified by TCV */
	if (ndesc_na [phy] != NONE)
		return ERROR;

	ndesc_na [phy] = fd;
	return 0;
}

static int tcv_clo_null (int phy, int fd) {

	/* phy/fd has been verified */

	if (ndesc_na == NULL || ndesc_na [phy] != fd)
		return ERROR;

	ndesc_na [phy] = NONE;
	return 0;
}

static int tcv_rcv_null (int phy, address p, int len, int *ses,
							     tcvadp_t *bounds) {

	if (ndesc_na == NULL || (*ses = ndesc_na [phy]) == NONE)
		return TCV_DSP_PASS;

	bounds->head = bounds->tail = 0;

	return TCV_DSP_RCV;
}

static int tcv_frm_null (address p, int phy, tcvadp_t *bounds) {

	return bounds->head = bounds->tail = 0;
}

static int tcv_out_null (address p) {

	return TCV_DSP_XMT;

}

static int tcv_xmt_null (address p) {

	return TCV_DSP_DROP;
}

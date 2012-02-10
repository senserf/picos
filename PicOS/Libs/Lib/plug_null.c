/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#ifndef	__SMURPH__
// The real world
#include "tcvplug.h"
#endif

/*
 * This implements a trivial sample plugin whereby we are allowed to have
 * one session per phy interface.
 */

// Note: under the simulator, plugin functions are static. This implies that
// all stations use the same plugin(s), although, of course, they may use
// different parameters, which are node attributes.

static int tcv_ope_null (int, int, va_list);
static int tcv_clo_null (int, int);
static int tcv_rcv_null (int, address, int, int*, tcvadp_t*);
static int tcv_frm_null (address, int, tcvadp_t*);
static int tcv_out_null (address, int);
static int tcv_xmt_null (address, int);

const tcvplug_t plug_null =
		{ tcv_ope_null, tcv_clo_null, tcv_rcv_null, tcv_frm_null,
			tcv_out_null, tcv_xmt_null, NULL,
				0x0001 /* Plugin Id */ };

#ifndef	__SMURPH__
#include "plug_null_node_data.h"
#endif

#define	ndsc	_dac (PicOSNode, ndsc)

static int tcv_ope_null (int phy, int fd, va_list plid) {
/*
 * This is very simple - we are allowed to have one descriptor per phy.
 */
	int i;

	if (ndsc == NULL) {
		ndsc = (int*) umalloc (sizeof (int) * TCV_MAX_PHYS);
		if (ndsc == NULL)
			syserror (EMALLOC, "plug_null tcv_ope_null");
		for (i = 0; i < TCV_MAX_PHYS; i++)
			ndsc [i] = NONE;
	}

	/* phy has been verified by TCV */
	if (ndsc [phy] != NONE)
		return ERROR;

	ndsc [phy] = fd;
	return 0;
}

static int tcv_clo_null (int phy, int fd) {

	/* phy/fd has been verified */

	if (ndsc == NULL || ndsc [phy] != fd)
		return ERROR;

	ndsc [phy] = NONE;
	return 0;
}

static int tcv_rcv_null (int phy, address p, int len, int *ses,
							     tcvadp_t *bounds) {

	if (ndsc == NULL || (*ses = ndsc [phy]) == NONE)
		return TCV_DSP_PASS;

	bounds->head = bounds->tail = 0;

	return TCV_DSP_RCV;
}

static int tcv_frm_null (address p, int phy, tcvadp_t *bounds) {

	return bounds->head = bounds->tail = 0;
}

static int tcv_out_null (address p, int s) {

	return TCV_DSP_XMT;

}

static int tcv_xmt_null (address p, int s) {

	return TCV_DSP_DROP;
}

#undef ndsc

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2015                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"

#define	MAX_PACKET_LENGTH	60
#define	IBUF_LENGTH		82

sint 	sfd = -1;		// Session descriptor

word	lmin = 4, lmax = 58, pmin = 0, pmax = 7, cmin = 0, cmax = 0, nolbt = 0;

word	npackets, pln, pwr, cav, pmt;


// ============================================================================
// This is a simple illustration of radio communication via NULL the plugin
// ============================================================================

void show (word st, address pkt) {

	ser_outf (st, "RCV: len = %u rss = %d qua = %d\r\n",
		tcv_left (pkt),
		((byte*)pkt) [tcv_left (pkt) - 1],
		((byte*)pkt) [tcv_left (pkt) - 2]
	);
}

fsm receiver {

  address rpkt;

  state RC_TRY:

	rpkt = tcv_rnp (RC_TRY, sfd);

  state RC_SHOW:

	show (RC_SHOW, rpkt);
	tcv_endp (rpkt);
	proceed RC_TRY;
}

// ============================================================================

fsm root {

  char ibuf [IBUF_LENGTH];

  state RS_INIT:

	phys_cc1100 (0, MAX_PACKET_LENGTH);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (WNONE, 0, 0);
	tcv_control (sfd, PHYSOPT_ON, NULL);

	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	runfsm receiver;

  state RS_RCMD_M:

	ser_out (RS_RCMD_M,
		"\r\nRF S-R example\r\n"
		"Command:\r\n"
		"p lmin lmax pmin pmax cmin cmax nolbt\r\n"
		"s n\r\n"
	);

  state RS_RCMD:

	ser_in (RS_RCMD, ibuf, IBUF_LENGTH);

	if (ibuf [0] == 'p') {

		scan (ibuf + 1, "%u %u %u %u %u %u %u",
			&lmin, &lmax, &pmin, &pmax, &cmin, &cmax, &nolbt);

		if (lmin < 4)
			lmin = 2;
		if (lmax > 58)
			lmax = 58;
		if (lmin > lmax)
			lmin = lmax;

		if (pmax > 7)
			pmax = 7;
		if (pmin > pmax)
			pmin = pmax;

		if (cmax > 4095)
			cmax = 4095;
		if (cmin > cmax)
			cmin = cmax;

		if (nolbt > 1)
			nolbt = 1;

		proceed RS_SHOWPARS;
	}

	if (ibuf [0] == 's') {

		npackets = 1;
		scan (ibuf + 1, "%u", &npackets);

		if (npackets == 0)
			npackets = 1;
		else if (npackets > 128)
			npackets = 128;

		proceed RS_SENDPACKETS;
	}

  state RS_RCMD_E:

	ser_out (RS_RCMD_E, "Illegal command\r\n");
	proceed RS_RCMD_M;

  state RS_SHOWPARS:

	ser_outf (RS_SHOWPARS, "lmin=%u, lmax=%u, pmin=%u, pmax=%u, "
		"cmin=%u, cmax=%u, nolbt=%u\r\n",
		lmin, lmax, pmin, pmax, cmin, cmax, nolbt);

	proceed RS_RCMD;

  state RS_SENDPACKETS:

	if (npackets == 0)
		proceed RS_DONE;

	pln = (rnd () % (lmax - lmin + 1) + lmin) & 0xFFE;
	pwr = rnd () % (pmax - pmin + 1) + pmin;
	cav = rnd () % (cmax - cmin + 1) + cmin;

  state RS_XMT:

	address pkt;
	sint i;

	pkt = tcv_wnp (RS_XMT, sfd, pln + 2);

	for (i = 2; i < pln; i++)
		((byte*)pkt) [i] = (byte) i;

	pkt [pln >> 1] = pmt = (nolbt ? 0x8000 : 0) | (pwr << 12) | cav;

	tcv_endp (pkt);

  state RS_TELL:

	ser_outf (RS_TELL, "SND: pln=%u, pwr=%u, cav=%u, nolbt=%u, left=%u, "
		"<%x>\r\n", pln, pwr, cav, nolbt, npackets, pmt);

	npackets--;

	proceed RS_SENDPACKETS;

  state RS_DONE:

	ser_out (RS_DONE, "Done\r\n");
	proceed RS_RCMD;
}

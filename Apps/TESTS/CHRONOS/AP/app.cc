/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
//
// A simple "access point" for the CHRONOS watch
//

#include "sysio.h"
#include "tcvphys.h"

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#include "phys_cc1100.h"
#include "plug_null.h"
#include "ab.h"

// This is the maximum for CC1100
#define	MAXPLEN		60
// 4 bytes are needed for XRS header
#define	UBUFLEN		(MAXPLEN-4)

// ============================================================================

fsm receiver {

  char *ibuf;

  state RC_READ:

	ibuf = ab_in (RC_READ);

  state RC_ECHO:

	ser_out (RC_ECHO, ibuf);
	ufree (ibuf);

  state RC_EL:

	ser_out (RC_EL, "\r\n");
	proceed RC_READ;
}

// ============================================================================

fsm root {

  char ubuf [UBUFLEN];
  int sfd = -1;

  state RS_INIT:

	word w;

	phys_cc1100 (0, MAXPLEN);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}
	w = 0xffff;
	tcv_control (sfd, PHYSOPT_SETSID, &w);
	tcv_control (sfd, PHYSOPT_TXON, NULL);
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	ab_init (sfd);
	// We are in charge of the link
	ab_mode (AB_MODE_ACTIVE);
	runfsm receiver;

  state RS_WINP:

	ser_in (RS_WINP, ubuf, UBUFLEN);
	if (ubuf [0] == '\0')
		proceed RS_WINP;

  state RS_ECHO:

	ser_outf (RS_ECHO, "Sending: %s\r\n", ubuf);

  state RS_SEND:

	ab_outf (RS_SEND, "%s", ubuf);
	proceed RS_WINP;
}

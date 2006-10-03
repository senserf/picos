/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"
#include "plug_null.h"

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#include "phys_dm2200.h"

#define	MAX_PACKET_LENGTH	64
#define	IBUFLEN			64

static	long	CntRcvd = 0,	// Received packets
		CntLost = 0;	// Lost packets

static	word	LastRcvd;	// Last received

static int sfd;
static byte RcvStart = YES;

/* ======================================================================= */

#define	RC_WAIT		0
#define	RC_DISP		1

process (receiver, void)

	static address packet;
	static word len;
	word cnt;

	nodata;

  entry (RC_WAIT)

	packet = tcv_rnp (RC_WAIT, sfd);

	len = tcv_left (packet);
	cnt = packet [1];

	if (RcvStart) {
		// Reset counters
		CntRcvd = 0;
		CntLost = 0;
		LastRcvd = cnt - 1;
		RcvStart = NO;
	}

	CntRcvd ++;
	LastRcvd ++;

	CntLost += (cnt - LastRcvd);
	LastRcvd = cnt;

  entry (RC_DISP)

	ser_outf (RC_DISP, "RCV: [%d] %x %x [%ld %ld]\r\n",
		len,
		packet [ 1],
		packet [ 2],
		CntRcvd + CntLost, CntLost);

	tcv_endp (packet);
	proceed (RC_WAIT);

endprocess (1)

void do_start () {

	RcvStart = YES;
	if (!running (receiver))
		fork (receiver, NULL);
	tcv_control (sfd, PHYSOPT_RXON, NULL);
}

void do_quit () {

	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	killall (receiver);
}

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_STA		20
#define	RS_SPA		30
#define	RS_DON		40

process (root, int)

	static char *ibuf;
	int n;

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	phys_dm2200 (0, MAX_PACKET_LENGTH + 4);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

  entry (RS_RCMD-2)

  	ser_outf (RS_RCMD-2,
	"\r\nRECEIVER:\r\n"
	"Commands:\r\n"
	"a d      -> action: 0-stop, 1-start\r\n"
	"r        -> reset packet counters at receiver\r\n"
	);

  entry (RS_RCMD)
  
	ibuf [0] = ' ';
	ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 'a')
		proceed (RS_STA);
	if (ibuf [0] == 'r')
		proceed (RS_SPA);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-2);

  entry (RS_STA)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n < 0)
		proceed (RS_RCMD+1);
	if (n)
		do_start ();
	else
		do_quit ();

	proceed (RS_DON);

  entry (RS_SPA)

	RcvStart = YES;

  entry (RS_DON)

	diag ("DONE");
	proceed (RS_RCMD);

endprocess (1)

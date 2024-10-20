/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "phys_uart.h"
#include "plug_null.h"
#include "form.h"

#define	MAXPAYLEN	82		// Maximum packet payload length

#if UART_TCV_MODE == UART_TCV_MODE_N

// Straight packet mode
#define	MAXPAKLEN	(MAXPAYLEN+2)	// Maximum packet length
#define	rq_pklen(l)	(((l)&1) ? (l) + 3 : (l) + 2)

#else

#define	MAXPAKLEN	MAXPAYLEN
#define	rq_pklen(l)	(l)

#endif

word sfailures;
sint SFD;

word drand, nrand, frand, lrand;

Boolean send (char *buf, sint len) {
//
// Send a packet out
//
	address op;

	if ((op = tcv_wnp (WNONE, SFD, rq_pklen (len))) == NULL) {
		sfailures++;
		return NO;
	}

	memcpy (op, buf, len);
	tcv_endp (op);
	return YES;
}

Boolean sout (char *fmt, ...) {

	address op;
	sint len;
	va_list ap;

	va_start (ap, fmt);

	// Required packet length
	len = vfsize (fmt, ap);

	if ((op = tcv_wnp (WNONE, SFD, rq_pklen (len))) == NULL) {
		sfailures++;
		return NO;
	}

	vform ((char*)op, fmt, ap);

	tcv_endp (op);
	return YES;
}

fsm turnon (word del) {

	state START:

		delay (del, TURNON);
		release;

	state TURNON:

		tcv_control (SFD, PHYSOPT_ON, NULL);
		sout ((char*)"UART goes on");
		finish;
}

fsm genrandom {

	state START:

		sint i, len;
		address op;

		if (frand < lrand)
			// Generate a random length between frand and lrand
			len = (sint) ((rnd () % (lrand - frand + 1)) + frand);
		else
			// Fixed length
			len = (sint) frand;

		if ((op = tcv_wnp (WNONE, SFD, rq_pklen (len))) == NULL) {
			sfailures++;
			return;
		}

		// Fill the packet with random contents
		((char*)op) [0] = 'P';
		((char*)op) [len-2] = 'E';
		((char*)op) [len-1] = '\0';

		for (i = 1; i < len - 2; i++)
			((char*)op) [i] = (char)('a' + 
				(rnd () % ('z' - 'a' + 1)));

		tcv_endp (op);

		if (--nrand == 0)
			finish;

		delay (drand, START);
}

// ============================================================================

fsm root {

	address in_packet;

	state START:

		word w;

		phys_uart (0, MAXPAKLEN, UART_A);
		tcv_plug (0, &plug_null);
		if ((SFD = tcv_open (WNONE, 0, 0)) < 0)
			syserror (ENODEVICE, "uart");

#if UART_TCV_MODE == UART_TCV_MODE_N
		w = 0xffff;
		tcv_control (SFD, PHYSOPT_SETSID, &w);
#endif
	state NEXTPK:

		in_packet = tcv_rnp (NEXTPK, SFD);

		switch (((char*)in_packet) [0]) {

			case 'o' : proceed DO_OFFNON;
			case 'r' : proceed DO_RANDOM;
		}

	state DO_ECHO:

		send ((char*)in_packet, tcv_left (in_packet));
		tcv_endp (in_packet);
		sameas LOOP;

	state DO_OFFNON:

		word del;

		if (running (turnon))
			killall (turnon);

		del = 0;
		scan (((char*)in_packet) + 1, "%u", &del);
		tcv_endp (in_packet);

		sout ((char*)"UART goes off for %u msecs", del);

		tcv_control (SFD, PHYSOPT_OFF, NULL);
		runfsm turnon (del);
		sameas LOOP;

	state DO_RANDOM:

		if (running (genrandom))
			killall (genrandom);

		drand = 0;
		nrand = 1;
		frand = 8;
		lrand = MAXPAYLEN - 1;

		scan (((char*)in_packet) + 1, "%u %u %u %u", &drand, &nrand,
			&frand, &lrand);
		tcv_endp (in_packet);

		if (nrand < 1)
			nrand = 1;

		if (frand < 4)
			frand = 4;

		if (lrand < 4)
			lrand = 4;
		else if (lrand > MAXPAYLEN - 1)
			lrand = MAXPAYLEN - 1;

		if (frand > lrand)
			frand = lrand;

		runfsm genrandom;
		sameas LOOP;

	state LOOP:

		// Check for failures
		if (sfailures && sout ((char*)"Send failures: %u", sfailures))
			// Zero out reported failures
			sfailures = 0;

		sameas NEXTPK;
}

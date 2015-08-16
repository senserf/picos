#include "sysio.h"
#include "tcvphys.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "cc1100.h"
#include "phys_cc1100.h"
#include "plug_null.h"

#include "dw1000.h"
#include "sensors.h"
#include "actuators.h"

// ============================================================================

#define	IBUFLEN	72

// ============================================================================

sint	sfd, thefsm;

dw1000_locdata_t location;

// ============================================================================

fsm peg_thread {

    word cnt;

    state EVENT:

	read_sensor (SENSOR_RANGE, EVENT, (address)&location);
	if (location.tag == 0) {
		wait_sensor (SENSOR_RANGE, EVENT);
		release;
	}

    state SHOW:

	ser_outf (SHOW, "r %u %u\r\n", location.tag, location.seq);
	cnt = 0;

    state SHOWTS:

	lword lw;

	if (cnt >= 25)
		sameas EVENT;

	memcpy (&lw, location.tst + cnt, 4);

	ser_outf (SHOWTS, "%x%lx\r\n", location.tst [cnt + 4], lw);

	cnt += 5;
	sameas SHOWTS;
}

// ============================================================================

fsm root {

    char *ibuf;

    state INIT:

	ibuf = (char*) umalloc (IBUFLEN);

	// We also test the coexistence of CC1100 with DecaWave
	phys_cc1100 (0, CC1100_MAXPLEN);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (WNONE, 0, 0);

	if (sfd < 0) {
		diag ("fail cc1100");
		halt ();
	}

    state BANNER:

	ser_outf (BANNER,
		"\r\nDECA TEST, NId = %u\r\n"
		"d 0 - off\r\n"
		"d 1 m r - on\r\n"
		"r\r\n"
			, (word)host_id
	);

    state COMMAND:

	ibuf [0] = 0;
	ser_in (COMMAND, ibuf, IBUFLEN);

	switch (ibuf [0]) {

		case 'd': proceed DECAONOFF;
		case 'r': proceed POLL;

	}

    state ILLEGAL:

	ser_out (ILLEGAL, "Illegal!\r\n");
	proceed BANNER;

    state DECAONOFF:

	sint on, mode, role;

	on = mode = role = -1;
	scan (ibuf + 1, "%u %u %u", &on, &mode, &role);

	if (on < 0)
		proceed ILLEGAL;

	if (on == 0) {
		dw1000_stop ();
		if (thefsm) {
			kill (thefsm);
			thefsm = 0;
		}
		proceed WOK;
	}

	if (mode < 0 || mode > 7)
		proceed ILLEGAL;

	if (role != 0 && role != 1)
		proceed ILLEGAL;

	if (thefsm)
		proceed ILLEGAL;

	if (role)
		thefsm = runfsm peg_thread;

	dw1000_start ((byte)mode, (byte)role, 0xC0C0);

    state WOK:

	ser_out (WOK, "OK\r\n");
	proceed COMMAND;

    state POLL:

	if (thefsm)
		proceed ILLEGAL;

	location.seq = (byte) seconds ();
	write_actuator (ACTUATOR_RANGE, POLL, (address)&(location.seq));
	proceed COMMAND;
}

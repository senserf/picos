#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "max30102.h"

fsm collect {

	lword res, irs;


	state READ_SAMPLE:

		max30102_read_sample (READ_SAMPLE, &irs, &res);

	state PRINT_SAMPLE:

		ser_outf (PRINT_SAMPLE, "%lx %lx %lx\r\n", seconds (),
			irs, res);

		sameas READ_SAMPLE;
}

fsm root {

	word par, val;

	state MENU:

		ser_out (MENU,
			"Commands:\r\n"
			"  o [mode], no arg == off\r\n"
			"  r nx\r\n"
			"  w nx vx\r\n"
			"  c [0|1]\r\n"
		);

	state INPUT:

		char cmd [32];

		ser_in (INPUT, cmd, 32);

		if (cmd [0] == 'o' || cmd [0] == 'c') {

			par = WNONE;
			scan (cmd + 1, "%u", &par);

			if (cmd [0] == 'o') {
				if (par == WNONE)
					max30102_stop ();
				else
					max30102_start (par);
			} else {
				if (par == WNONE || par == 0)
					killall (collect);
				else if (!running (collect))
					runfsm collect;
			}

			proceed SAYOK;
		
		} else if (cmd [0] == 'r') {
			// Register read
			par = WNONE;
			scan (cmd + 1, "%x", &par);
			if (par <= 0xff) {
				val = max30102_rreg ((byte)par);
				proceed OUTREG;
			}
		} else if (cmd [0] == 'w') {
			// Register write
			par = WNONE;
			val = 0;
			scan (cmd + 1, "%x %x", &par, &val);
			if (par <= 0xfd) {
				max30102_wreg ((byte)par, (byte)val);
				proceed OUTREG;
			}
		}

	state BAD_INPUT:

		ser_out (BAD_INPUT, "Illegal input\r\n");
		proceed MENU;

	state SAYOK:

		ser_out (SAYOK, "OK\r\n");
		proceed INPUT;

	state OUTREG:

		ser_outf (OUTREG, "Reg [%x] = %x\r\n", par, val);
		proceed INPUT;
}

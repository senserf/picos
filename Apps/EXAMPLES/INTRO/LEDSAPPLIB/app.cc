#include "sysio.h"
#include "ser.h"
#include "serf.h"

// Illustrates AppLib
#include "blink.h"

fsm root {

	state START:

		ser_out (START,
			"Commands:\r\n"
			"  b led cnt on off space\r\n"
		);
			
	state INPUT:

		char cmd [32];
		word led, count, ont, oft, spa;

		ser_in (INPUT, cmd, 4);

		if (cmd [0] != 'b')
			proceed BAD_INPUT;

		led = 0;
		count = 1;
		ont = 512;
		oft = 512;
		spa = 32;

		scan (cmd + 1, "%u %u %u %u %u",
			&led, &count, &ont, &oft, &spa);

		blink ((byte)led, (byte)count, ont, oft, spa);

	state OKMSG:

		ser_out (OKMSG, "OK\r\n");
		proceed INPUT;

	state BAD_INPUT:

		ser_out (BAD_INPUT, "Illegal command\r\n");
		proceed INPUT;
}

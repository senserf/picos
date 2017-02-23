#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "sensors.h"

typedef struct {
	byte led, state;
} led_status_t;

fsm blinker (led_status_t *lstat) {

	state CHECK_STATUS:

		if (lstat->state < 2) {
			leds (lstat->led, 0);
			if (lstat->state) {
				lstat->state = 2;
				delay (512, CHECK_STATUS);
			}
		} else {
			leds (lstat->led, 1);
			lstat->state = 1;
			delay (768, CHECK_STATUS);
		}

		when (lstat, CHECK_STATUS);
}

void blink (led_status_t *lstat, Boolean on) {
	lstat->state = on ? 2 : 0;
	trigger (lstat);
}

fsm testsensor {

	word value, voltage, temperature;

	state STARTIT:

		// dump_sensor_list ();

	state MEASURE_A:

		read_sensor (MEASURE_A, 0, &value);

	state MEASURE_V:

		read_sensor (MEASURE_V, -1, &voltage);

		// Hard delay
		read_sensor (WNONE, -2, &temperature);

	state PRINTIT:

		ser_outf (PRINTIT, "Values: %u [%x], %u [%x], %u [%x]\r\n",
			value, value, voltage, voltage,
				temperature, temperature);

		delay (2048, MEASURE_A);

#if 0
	state INIT:

diag ("ADC CLOCK");
		AUXWUCClockEnable (AUX_WUC_ADI_CLOCK | AUX_WUC_ANAIF_CLOCK);
		delay (100, NEXT);
		release;

	state NEXT:

		AUXADCSelectInput (ADC_COMPB_IN_AUXIO7);
		AUXADCEnableSync (AUXADC_REF_FIXED, AUXADC_SAMPLE_TIME_2P7_US,
			AUXADC_TRIGGER_MANUAL);

	state SENSE_LOOP:

		AUXADCFlushFifo ();
		AUXADCGenManualTrigger ();

	state WAIT_SAMPLE:

		if (AUXADCGetFifoStatus () & AUXADC_FIFO_EMPTY_M) {
			delay (1, WAIT_SAMPLE);
			release;
		}

diag ("VALUE: %d", (word) AUXADCReadFifo ());

		delay (2048, SENSE_LOOP);
#endif
}

fsm testdelay {

	word d;

	state INIT:

		d = 512 + (lrnd () & 0x1fff);

		delay (d, WAKE);
		release;

	state WAKE:

		// diag ("W: %u, %lu", d, seconds ());
		ser_outf (WAKE, "W: %u, %lu\r\n", d, seconds ());
		proceed INIT;
}

static word Buttons;

static void butpress (word but) {

	Buttons |= (1 << but);
	trigger (&Buttons);
}

fsm button_thread {

	state BT_LOOP:

		if (Buttons == 0) {
			when (&Buttons, BT_LOOP);
			release;
		}

		ser_outf (BT_LOOP, "Press: %x\r\n", Buttons);
		Buttons = 0;
		sameas BT_LOOP;
}

fsm root {

	led_status_t *my_led;

	state START:

		ser_out (START,
			"Commands:\r\n"
			"  on\r\n"
			"  off\r\n"
		);
			
		my_led = (led_status_t*)umalloc (sizeof (led_status_t));
		my_led -> led = 1;
		blink (my_led, YES);
		leds (0, 2);
		runfsm blinker (my_led);
		runfsm testdelay;

		buttons_action (butpress);
		runfsm button_thread;

		runfsm testsensor;

	state INPUT:

		char cmd [4];

		ser_in (INPUT, cmd, 4);
		if (cmd [1] == 'n')
			blink (my_led, YES);
		else if (cmd [1] == 'f')
			blink (my_led, NO);
		else
			proceed BAD_INPUT;

	state OKMSG:

		ser_out (OKMSG, "OK\r\n");
		proceed INPUT;

	state BAD_INPUT:

		ser_out (BAD_INPUT, "Illegal command\r\n");
		proceed INPUT;
}

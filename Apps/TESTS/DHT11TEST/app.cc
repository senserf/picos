#include "sysio.h"
#include "phys_cc1100.h"
#include "phys_uart.h"
#include "plug_null.h"

#include "ossi.h"

#ifndef __SMURPH__
#include "cc1100.h"
#endif

#define	MAX_PACKET_LENGTH 	CC1100_MAXPLEN
#define	MAX_PAYLOAD_LENGTH	(MAX_PACKET_LENGTH - 2 - 2 - 4)

// ============================================================================

command_radio_t rf_control = { 0, 7, 0, { 1024, 1024 } , { 32, 32} , { 0 } };
byte __payload_storage__ [MAX_PAYLOAD_LENGTH];

sint sd_rf, sd_uart;
Boolean power_up = 1,
	switch_on = 0;

word	nsecs = 1;

oss_hdr_t	*CMD;
address		PMT;
word		PML;

#define	MAXFILL		((OSS_PACKET_LENGTH - 2 - 2) / 2)

word dh_time [MAXFILL];
word dh_fill;

// ============================================================================

#define	msghdr	((oss_hdr_t*)msg)

void handle_command ();

// ============================================================================

static void set_switch (Boolean on) {

	_BIS (P4DIR, 0x08);
	if ((switch_on = on))
		_BIS (P4OUT, 0x08);
	else
		_BIC (P4OUT, 0x08);
}

static void dht11_time () {

	lword tusec;
	word curr;
	address buff;
	byte lev;

	tusec = (lword) nsecs * 999000;
	dh_fill = 0;
	buff = dh_time;

	_BIC (P2OUT, 0x01);
	_BIS (P2DIR, 0x01);

	mdelay (50);

	_BIC (P2DIR, 0x01);
	udelay (4);

	lev = 1;

	while (dh_fill < MAXFILL && tusec) {

		curr = 0;

		if (lev) {
			// Wait until it goes down
			while (1) {
				if ((P2IN & 0x01) == 0)
					break;
				if (++curr == 0xFFFF)
					break;
				if (--tusec == 0)
					break;
				udelay (1);
			}
			lev = 0;
		} else {
			// Wait until it goes up
			while (1) {
				if ((P2IN & 0x01))
					break;
				if (++curr == 0xFFFF)
					break;
				if (--tusec == 0)
					break;
				udelay (1);
			}
			lev = 1;
		}

		*buff++ = curr;
		dh_fill++;
	}
}

// ============================================================================

void oss_ack (word status) {

	address msg;

	if ((msg = tcv_wnp (WNONE, sd_uart, sizeof (oss_hdr_t) + 2)) != NULL) {
		((oss_hdr_t*)msg)->code = 0;
		((oss_hdr_t*)msg)->ref = CMD->ref;
		msg [1] = status;
		tcv_endp (msg);
	} else
		diag ("MEM1");
}

// ============================================================================

#define msgblk	((message_packet_t*)(msg + 1))

static void pktmsg (lword cnt, byte rss, address pay, word plen) {
//
// Issue a "packet" message
//
	address msg;

	if ((msg = tcv_wnp (WNONE, sd_uart, sizeof (oss_hdr_t) +
	    sizeof (message_packet_t) + plen + 2)) == NULL) {
		diag ("MEM2");
		return;
	}

	msghdr->code = 0x02;
	msghdr->ref = 0;
	msgblk->counter = cnt;
	msgblk->rssi = rss;
	msgblk->payload.size = plen;
	memcpy (msgblk->payload.content, pay, plen);

	tcv_endp (msg);

}

#undef	msgblk
#define msgblk	((message_status_t*)(msg + 1))

static void statmsg () {
//
// Issue a status message
//
	address msg;
	word p;

	if ((msg = tcv_wnp (WNONE, sd_uart, sizeof (oss_hdr_t) +
	    sizeof (message_status_t) + 2)) == NULL) {
		diag ("MEM3");
		return;
	}

	msghdr->code = 0x01;
	msghdr->ref = CMD->ref;
	msgblk->rstatus = rf_control.status;
	msgblk->rpower = rf_control.power;
	msgblk->rchannel = (byte) rf_control.channel;
	*((lword*)(msgblk->rinterval)) = *((lword*)(rf_control.interval));
	*((lword*)(msgblk->rlength)) = *((lword*)(rf_control.length));
	msgblk->smemstat [0] = memfree (0, msgblk->smemstat + 1);
	msgblk->smemstat [2] = stackfree ();
	msgblk->spower = power_up;
	msgblk->swtch = switch_on;

	tcv_endp (msg);
}

#undef msgblk

static void command_dht11 () {

	address msg;

	if (PML < sizeof (command_dht11_t)) {
		oss_ack (1);
		return;
	}

#define	pmt	((command_dht11_t*)PMT)

	if (pmt->duration < 32 && pmt->duration > 0)
		nsecs = pmt->duration;

	// Run the experiment
	dht11_time ();

	// Send the message
	if ((msg == tcv_wnp (WNONE, sd_uart, sizeof (oss_hdr_t) +
	    dh_fill * 2 + 2)) == NULL) {
		oss_ack (2);
		return;
	}

	msghdr->code = 0x03;
	msghdr->ref = CMD->ref;
	
#define msgblk	((message_dht11t_t*)(msg + 1))

	msgblk->times.size = dh_fill * 2;
	memcpy (msgblk->times.content, dh_time,  msgblk->times.size);

	tcv_endp (msg);

#undef	pmt
}

#undef msgblk
#undef msghdr

fsm radio_receiver {

	address pkt;


	state RCV_WAIT:

		word len;

		pkt = tcv_rnp (RCV_WAIT, sd_rf);
		len = tcv_left (pkt);

		if (len >= 8)
			pktmsg (*((lword*)(pkt + 1)),
				((byte*)pkt) [len - 1],
				pkt + 3, len - 8);

		tcv_endp (pkt);
		proceed RCV_WAIT;
}

fsm radio_sender {

	lword counter;
	word len;

	state SND_GO:

		// Generate packet length
		len = ((rnd () % (rf_control.length [1] -
			rf_control.length [0] + 1)) +
				rf_control.length [0]) & ~1;

	state SND_ACQ:

		address pkt, msg;

		pkt = tcv_wnp (SND_ACQ, sd_rf, len);
		// NetID
		pkt [0] = 0;
		*((lword*)(pkt + 1)) = counter;
		len -= 8;
		memset (pkt + 3, 0, len);
		if (rf_control.data.size)
			memcpy (pkt + 3, rf_control.data.content, 
				rf_control.data.size);
		// Message
		pktmsg (counter, 0, pkt + 3, len);
		counter++;

		tcv_endp (pkt);

		// Delay
		len = (rnd () % (rf_control.interval [1] -
			rf_control.interval [0] + 1)) +
				rf_control.interval [0];

		delay (len, SND_GO);
}

// ============================================================================

static void init_dht11 () {

	_BIC (P2DIR, 0x01);
	_BIC (P2OUT, 0x01);
	set_switch (0);
}

// ============================================================================

fsm root {

	state RS_INIT:

		init_dht11 ();

		word si = 0xFFFF;

		phys_cc1100 (0, MAX_PACKET_LENGTH);
		phys_uart (1, OSS_PACKET_LENGTH, 0);
		tcv_plug (0, &plug_null);

		sd_rf = tcv_open (WNONE, 0, 0);		// CC1100
		sd_uart = tcv_open (WNONE, 1, 0);	// UART

		// Note: remove statid from UART_N (or make WNONE the default)
		tcv_control (sd_uart, PHYSOPT_SETSID, &si);

		if (sd_rf < 0 || sd_uart < 0)
			syserror (ERESOURCE, "ini");

		runfsm radio_receiver;

	state RS_CMD:

		// Process commands
		CMD = (oss_hdr_t*)tcv_rnp (RS_CMD, sd_uart);
		PML = tcv_left ((address)CMD);
		if (PML >= sizeof (oss_hdr_t)) {
			PML -= sizeof (oss_hdr_t);
			PMT = (address)(CMD + 1);
			handle_command ();
		}
		tcv_endp ((address)CMD);
		proceed RS_CMD;
}

// ============================================================================

static void reset_radio_thread () {

	word p;

	killall (radio_sender);

	tcv_control (sd_rf, PHYSOPT_OFF, NULL);
	tcv_control (sd_rf, PHYSOPT_SETCHANNEL, &(rf_control.channel));
	p = rf_control.power;
	tcv_control (sd_rf, PHYSOPT_SETPOWER, &p);
	if (p = (rf_control.status & 0x0f)) {
		if (p < 2)
			p = 0;
		tcv_control (sd_rf, PHYSOPT_RXON, &p);
	}
	if (rf_control.status > 0x0F)
		runfsm radio_sender;
}

static Boolean set_interval (word *src, word *trg, word min, word max) {

	if (src [0] != WNONE) {

		if (src [0] < min)
			src [0] = min;
		else if (src [0] > max)
			src [0] = max;

		if (src [1] == WNONE || src [1] < src [0])
			src [1] = src [0];
		else if (src [1] > max)
			src [1] = max;

		if (*((lword*)src) != *((lword*)trg)) {
		    	*((lword*)trg) = *((lword*)src);
			return YES;
		}
	}

	return NO;
}

static void command_radio () {
//
// Process the radio command
//
	byte len;
	Boolean reset = NO;

	if (PML < sizeof (command_radio_t)) {
		oss_ack (1);
		return;
	}

#define	pmt	((command_radio_t*)PMT)

	if (pmt->power <= 7 && pmt->power != rf_control.power) {
		rf_control.power = pmt->power;
		reset = YES;
	}

	if (pmt->channel <= 255 && pmt->channel != rf_control.channel) {
		rf_control.channel = pmt->channel;
		reset = YES;
	}

	if (pmt->status <= 0x12 && pmt->status != rf_control.status) {
		rf_control.status = pmt->status;
		reset = YES;
	}

	if (set_interval (pmt->interval, rf_control.interval, 256, 65534))
		reset = YES;

	set_interval (pmt->length, rf_control.length, 8, MAX_PACKET_LENGTH);

	if ((len = pmt->data.size)) {
		if (len > MAX_PAYLOAD_LENGTH)
			len = MAX_PAYLOAD_LENGTH;
		rf_control.data.size = (byte) len;
		memcpy (&(rf_control.data.content), pmt->data.content, len);
	}

	if (reset)
		reset_radio_thread ();

	// ACK
	oss_ack (0);

#undef	pmt

}

static void command_system () {
//
// Process the system command
//
	word i, k;

	if (PML < sizeof (command_system_t)) {
		oss_ack (1);
		return;
	}

#define	pmt	((command_system_t*)PMT)

	if (pmt->request == 1)
		// This one is easy
		reset ();

	for (i = 0; i < 4; i++) {
		k = (pmt->leds >> (i + i)) & 0x3;
		if (k == 0)
			leds (i, 0);
		else if (k == 1)
			leds (i, 1);
		else if (k == 2)
			leds (i, 2);
	}

	if (pmt->blinkrate != BNONE)
		fastblink (pmt->blinkrate);

	if (pmt->power != BNONE) {
		if (pmt->power) {
			powerup ();
			power_up = YES;
		} else {
			powerdown ();
			power_up = NO;
		}
	}

	if (pmt->swtch != BNONE)
		set_switch (pmt->swtch);
		
	if (pmt->diag.size > 1)
		diag ((const char*)(pmt->diag.content));

	if (pmt->request)
		statmsg ();
	else
		oss_ack (0);
#undef	pmt

}

void handle_command () {

	address msg;

	switch (CMD->code) {

		case 0:
			// Heartbeat, autoconnect
			if (*((lword*)PMT) == OSS_PRAXIS_ID && (msg = 
			    tcv_wnp (WNONE, sd_uart, sizeof (oss_hdr_t) + 4))
				!= NULL) {

				msg [0] = 0;
				*(msg + 1) = (word)(OSS_PRAXIS_ID ^ 
					(OSS_PRAXIS_ID >> 16));
				tcv_endp (msg);
			} else
				diag ("MEM5");
			return;

		case 1:

			command_radio ();
			return;

		case 2:

			command_system ();
			return;

		case 3:

			command_dht11 ();
			return;
	}
}

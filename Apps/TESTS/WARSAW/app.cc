/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// ============================================================================

#include "boards.h"

// ============================================================================
#include "sysio.h"
#include "tcvphys.h"
#include "iflash_sys.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	42

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"
#include "storage.h"

#include "phys_cc1100.h"
#include "plug_null.h"

#include "pinopts.h"
#include "hold.h"

#ifdef	BUTTON_LIST
#include "buttons.h"
#endif

#ifndef AUTO_RADIO_START
#define	AUTO_RADIO_START	0
#endif

#define	IBUFLEN		132
#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

#define	EEPROM_INCR	255

extern void* _etext;

static sint 	sfd = -1, off, packet_length, rcvl, b;
static word	rssi, err, w, len, bs, nt, sl, ss, dcnt;
static lword	adr, max, val, last_snt, last_rcv, s, u, pat;
static byte	str [129], *blk, silent;
static char	*ibuf;
static address	packet;
static word	send_interval = 512;

#ifdef RTC_TEST
rtc_time_t dtime;
#endif

// ============================================================================

static void radio_start (word);
static void radio_stop ();

fsm test_auto {

  state AU_INIT:

#ifdef EPR_TEST
	ser_out (AU_INIT, "EEPROM ...\r\n");
#endif
	// Delay for a short while to clean the UART
	delay (750, AU_EE);
	release;

  state AU_EE:

// ============================================================================
#ifdef EPR_TEST

	if (ee_open ()) {
		strcpy (ibuf, "Cannot open EEPROM!\r\n");
		proceed AU_FAIL;
	}

  state AU_EEP1:

	if (ee_erase (AU_EEP1, 0, 0)) {
		strcpy (ibuf, "Failed to erase EEPROM!\r\n");
		proceed AU_FAIL;
	}

	rssi = pin_read_adc (WNONE, 0, 3, 4);
	adr = 0;

  state AU_EEP2:

	if (adr >= ee_size (NULL, NULL))
		proceed AU_EEP3;
	val = adr + rssi;
	if (ee_write (AU_EEP2, adr, (byte*)&val, 4)) {
		form (ibuf, "EEPROM write failed at %lx!\r\n", adr);
		proceed AU_FAIL;
	}
	adr += 1024;
	proceed AU_EEP2;

  state AU_EEP3:

	adr = 0;

  state AU_EEP4:

	if (adr >= ee_size (NULL, NULL))
		proceed AU_EEP5;
	if (ee_read (adr, (byte*)&val, 4)) {
		form (ibuf, "EEPROM read failed at %lx!\r\n", adr);
		proceed AU_FAIL;
	}
	if (val != adr + rssi) {
		form (ibuf, "EEPROM misread at %lx (%lx != %lx)!\r\n", adr,
			val, adr + rssi);
		proceed AU_FAIL;
	}
	adr += 1024;
	proceed AU_EEP4;

  state AU_EEP5:

	ee_close ();
	ser_out (AU_EEP5, "EEPROM OK\r\n");
#endif

// ============================================================================
#ifdef RTC_TEST

  state AU_RC:

	ser_out (AU_RC, "RTC ...\r\n");
	delay (750, AU_RCP1);
	release;

  state AU_RCP1:
	
	dtime.year = 9;
	dtime.month = 8;
	dtime.day = 11;
	dtime.dow = 2;
	dtime.hour = 9;
	dtime.minute = 9;
	dtime.second = 9;

	rtc_set (&dtime);
	delay (2048, AU_RCP2);
	release;

  state AU_RCP2:

	bzero (&dtime, sizeof (dtime));
	rtc_get (&dtime);
	if (dtime.second < 10) {
		strcpy (ibuf, "RTC doesn't tick!\r\n");
		proceed AU_FAIL;
	}

  state AU_RCP3:

	ser_out (AU_RCP3, "RTC OK\r\n");

#endif

// ============================================================================
#ifdef LCD_TEST

  state AU_LC:

	ser_out (AU_LC, "LCD (see the display) ...\r\n");
	delay (1024, AU_LCP1);
	release;

  state AU_LCP1:

	lcd_on (0);
	lcd_write (0, "This will stay  for 5 seconds!");
	delay (5*1024, AU_LCP2);
	release;

  state AU_LCP2:

	lcd_off ();
	ser_out (AU_LCP2, "LCD done\r\n");

#endif

  state AU_PN:

	ser_out (AU_PN,
		"Pins will light up in the order of defs in board_pins.h\r\n");
	delay (750, AU_RF);
	release;

  state AU_RF:

	ser_out (AU_RF, "Starting radio ... (this will go forever)\r\n");
	radio_start (3);

	for (w = 0; w < PIN_MAX; w++)
		pin_write (w, 0);

  state AU_PNP1:

	if (w == PIN_MAX)
		w = 0;
	else
		w++;

	pin_write (w, 1);
	delay (500, AU_PNP2);
	release;

  state AU_PNP2:

	pin_write (w, 0);
	proceed AU_PNP1;

  state AU_FAIL:

	radio_stop ();
	leds (0, 2);
	leds (1, 2);
	leds (2, 2);

  state AU_FAILP1:

	ser_outf (AU_FAIL, ibuf);
	delay (1024, AU_FAILP1);

}

// ============================================================================

static word gen_packet_length (void) {

#if MIN_PACKET_LENGTH >= MAX_PACKET_LENGTH
	return MIN_PACKET_LENGTH;
#else
	return ((rnd () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
#endif

}

fsm sender {

  state SN_SEND:

	packet_length = gen_packet_length ();

	if (packet_length < 10)
		packet_length = 10;
	else if (packet_length > MAX_PACKET_LENGTH)
		packet_length = MAX_PACKET_LENGTH;

  state SN_NEXT:

	sint pl, pp;

	packet = tcv_wnp (SN_NEXT, sfd, packet_length + 2);

	packet [0] = 0;
	packet [1] = 0xBABA;

	// In words
	pl = packet_length / 2;
	((lword*)packet)[1] = wtonl (last_snt);

	for (pp = 4; pp < pl; pp++)
		packet [pp] = (word) entropy;

	tcv_endp (packet);

  state SN_MESS:

	if (!silent) {
		ser_outf (SN_MESS, "Sent: %lu [%d]\r\n", last_snt,
			packet_length);
	}
	last_snt++;
	delay (send_interval, SN_SEND);
}

fsm receiver {

  state RC_TRY:

  	address packet;

	packet = tcv_rnp (RC_TRY, sfd);
	last_rcv = ntowl (((lword*)packet) [1]);
	rcvl = tcv_left (packet) - 2;
	rssi = packet [rcvl >> 1];
	tcv_endp (packet);

  state RC_MESS:

	if (!silent) {
		ser_outf (RC_MESS, "Rcv: %lu [%d], RSSI = %d, QUA = %d\r\n",
			last_rcv, rcvl, (rssi >> 8) & 0x00ff, rssi & 0x00ff);
	}
	proceed RC_TRY;

}

static void radio_start (word d) {

	if (sfd < 0)
		return;

	if (d & 1) {
		tcv_control (sfd, PHYSOPT_RXON, NULL);
		if (!running (receiver))
			runfsm receiver;
	}
	if (d & 2) {
		tcv_control (sfd, PHYSOPT_TXON, NULL);
		if (!running (sender))
			runfsm sender;
	}
}

static void radio_stop () {

	if (sfd < 0)
		return;

	killall (sender);
	killall (receiver);

	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	tcv_control (sfd, PHYSOPT_TXOFF, NULL);
}

// ============================================================================

fsm test_pin {

  state PI_INIT:

	ser_out (PI_INIT,
		"\r\nRF Pin Test\r\n"
		"r p r -> read ADC pin 'p' with reference r:\r\n"
		" 0-1.5V, 1-2.5V, 2-Vcc, 3-Eref\r\n"
		"s p v -> set pin 'p' to digital v (0/1)\r\n"
		"v p -> show the value of pin 'p'\r\n"
		"S p v -> set raw pin'p' [0,1-out, 2-in, 3-sp]\r\n"
		"V p -> show\r\n"
		"q -> return to main test\r\n"
	);

  state PI_RCMD:

	ser_in (PI_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'r': proceed PI_ADC;
		case 's': proceed PI_SET;
		case 'v': proceed PI_VIE;
		case 'S': proceed PI_SRAW;
		case 'V': proceed PI_VRAW;
		case 'q': { finish; };
	}
	
  state PI_RCMDP1:

	ser_out (PI_RCMDP1, "Illegal\r\n");
	proceed PI_INIT;

  state PI_ADC:

	nt = sl = 0;
	scan (ibuf + 1, "%u %u", &nt, &sl);

  state PI_ADCP1:

	nt = pin_read_adc (PI_ADCP1, nt, sl, 4);

  state PI_ADCP2:

	ser_outf (PI_ADCP2, "Value: %u\r\n", nt);
	proceed PI_RCMD;

  state PI_SET:

	nt = sl = 0;
	scan (ibuf + 1, "%u %u", &nt, &sl);
	pin_write (nt, sl);
	proceed PI_RCMD;

  state PI_VIE:

	nt = 0;
	scan (ibuf + 1, "%u", &nt);
	nt = pin_read (nt);

  state PI_VIEP1:

	ser_outf (PI_VIEP1, "Value: %u\r\n", nt);
	proceed PI_RCMD;

  state PI_SRAW:

	w = WNONE;
	ss = 0;

	scan (ibuf + 1, "%u %u", &w, &ss);

	if (ss < 2) {
		_PFS (w, 0);
		_PDS (w, 1);
		_PVS (w, ss);
	} else if (ss == 2) {
		// Set input
		_PFS (w, 0);
		_PDS (w, 0);
	} else {
		// Special function
		_PFS (w, 1);
	}
	proceed PI_RCMD;

  state PI_VRAW:

	w = WNONE;
	scan (ibuf + 1, "%u", &w);

	nt = _PV (w);
	sl = _PD (w);
	ss = _PF (w);

  state PI_VRAWP1:

	ser_outf (PI_VRAWP1, "Pin %u = val %u, dir %u, fun %u\r\n", nt, sl, ss);
	proceed PI_RCMD;

}

// ============================================================================

fsm test_ifl {

  state IF_START:

	ser_out (IF_START,
		"\r\nFLASH Test\r\n"
		"l led w -> led status [w = 0, 1, 2]\r\n"
		"b w -> blinkrate 0-low, 1-high\r\n"
		"d m -> write a diag message\r\n"
		"w adr w -> write word to info flash\r\n"
		"r adr -> read word from info flash\r\n"
		"e adr -> erase info flash\r\n"
		"W adr w -> write word to code flash\r\n"
		"R adr -> read word from code flash\r\n"
		"E adr -> erase code flash\r\n"
		"T adr -> flash overwrite test\r\n"
		"q -> return to main test\r\n"
	);

  state IF_RCMD:

	err = 0;
	ser_in (IF_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'l': proceed IF_LED;
		case 'b': proceed IF_BLI;
		case 'd': proceed IF_DIA;
		case 'w': proceed IF_FLW;
		case 'r': proceed IF_FLR;
		case 'e': proceed IF_FLE;
		case 'W': proceed IF_CLW;
		case 'R': proceed IF_CLR;
		case 'E': proceed IF_CLE;
		case 'T': proceed IF_COT;
		case 'q': { finish; };
	}
	
  state IF_RCMDP1:

	ser_out (IF_RCMDP1, "Illegal\r\n");
	proceed IF_START;

  state IF_LED:

	scan (ibuf + 1, "%u %u", &bs, &nt);
	leds (bs, nt);
	proceed IF_RCMD;

  state IF_BLI:

	scan (ibuf + 1, "%u", &bs);
	fastblink (bs);
	proceed IF_RCMD;

  state IF_DIA:

	diag ("MSG %d (%x) %u: %s", dcnt, dcnt, dcnt, ibuf+1);
	dcnt++;
	proceed IF_RCMD;

  state IF_FLR:

	scan (ibuf + 1, "%u", &w);
	if (w >= IFLASH_SIZE)
		proceed IF_RCMDP1;
	diag ("IF [%u] = %x", w, IFLASH [w]);
	proceed IF_RCMD;

  state IF_FLW:

	scan (ibuf + 1, "%u %u", &w, &bs);
	if (w >= IFLASH_SIZE)
		proceed IF_RCMDP1;
	if (if_write (w, bs))
		diag ("FAILED");
	else
		diag ("OK");
	goto Done;

  state IF_FLE:

	b = -1;
	scan (ibuf + 1, "%d", &b);
	if_erase (b);
	goto Done;

  state IF_CLR:

	scan (ibuf + 1, "%u", &w);
	diag ("CF [%u] = %x, et = %x", w, *((address)w), (word)(&_etext));
	proceed IF_RCMD;

  state IF_CLW:

	scan (ibuf + 1, "%u %u", &w, &bs);
	cf_write ((address)w, bs);
Done:
	diag ("OK");
	proceed IF_RCMD;

  state IF_CLE:

	b = 0;
	scan (ibuf + 1, "%d", &b);
	cf_erase ((address)b);
	goto Done;

  state IF_COT:

	w = 0;
	scan (ibuf + 1, "%u", &w);

	if (*((address)w) != 0xffff) {
		diag ("Word not erased: %x", *((address)w));
		proceed IF_RCMD;
	}

	for (b = 1; b <= 16; b++) {
		nt = 0xffff << b;
		cf_write ((address)w, nt);
		sl = *((address)w);
		diag ("Written %x, read %x", nt, sl);
	}
	goto Done;

}
// ============================================================================

#ifdef EPR_TEST

fsm test_epr {

  state EP_START:

	if (ee_open () == 0)
		proceed EP_INIT;

  state EP_STARTP1:

	ser_out (EP_STARTP1, "Failed to open\r\n");
	finish;

  state EP_INIT:

	ser_out (EP_INIT,
		"\r\nEEPROM Test\r\n"
		"a adr int -> store word\r\n"
		"b adr lint -> store lword\r\n"
		"c adr str -> store string\r\n"
		"d adr -> read word\r\n"
		"e adr -> read lword\r\n"
		"f adr n -> read string\r\n"
		"g adr n p -> write n longwords with p at adr\r\n"
		"h adr n b t -> read n blks of b at adr t times\r\n"
		"x frm upt -> erase eeprom from upto\r\n"
		"s -> sync eeprom\r\n"
		"w fr ln pat -> erase-write-read test\r\n"
		"q -> return to main test\r\n"
	);

  state EP_RCMD:

	err = 0;
	ser_in (EP_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'a': proceed EP_SWO;
		case 'b': proceed EP_SLW;
		case 'c': proceed EP_SST;
		case 'd': proceed EP_RWO;
		case 'e': proceed EP_RLW;
		case 'f': proceed EP_RST;
		case 'g': proceed EP_WRI;
		case 'h': proceed EP_REA;
		case 'x': proceed EP_ERA;
		case 's': proceed EP_SYN;
		case 'w': proceed EP_ETS;
		case 'q': { ee_close (); finish; };
	}
	
  state EP_RCMDP1:

	ser_out (EP_RCMDP1, "Illegal\r\n");
	proceed EP_INIT;

  state EP_SWO:

	scan (ibuf + 1, "%lu %u", &adr, &w);
	err = ee_write (WNONE, adr, (byte*)(&w), 2);

  state EP_SWOP1:

	ser_outf (EP_SWOP1, "[%d] Stored %u at %lu\r\n", err, w, adr);
	proceed EP_RCMD;

  state EP_SLW:

	scan (ibuf + 1, "%lu %lu", &adr, &val);
	err = ee_write (WNONE, adr, (byte*)(&val), 4);

  state EP_SLWP1:

	ser_outf (EP_SLWP1, "[%d] Stored %lu at %lu\r\n", err, val, adr);
	proceed EP_RCMD;

  state EP_SST:

	scan (ibuf + 1, "%lu %s", &adr, str);
	len = strlen (str);
	if (len == 0)
		proceed EP_RCMDP1;

	err = ee_write (WNONE, adr, str, len);

  state EP_SSTP1:

	ser_outf (EP_SSTP1, "[%d] Stored %s (%u) at %lu\r\n", err, str, len,
		adr);
	proceed EP_RCMD;

  state EP_RWO:

	scan (ibuf + 1, "%lu", &adr);
	err = ee_read (adr, (byte*)(&w), 2);

  state EP_RWOP1:

	ser_outf (EP_RWOP1, "[%d] Read %u (%x) from %lu\r\n", err, w, w, adr);
	proceed EP_RCMD;

  state EP_RLW:

	scan (ibuf + 1, "%lu", &adr);
	err = ee_read (adr, (byte*)(&val), 4);

  state EP_RLWP1:

	ser_outf (EP_RLWP1, "[%d] Read %lu (%lx) from %lu\r\n",
		err, val, val, adr);
	proceed EP_RCMD;

  state EP_RST:

	scan (ibuf + 1, "%lu %u", &adr, &len);
	if (len == 0)
		proceed EP_RCMDP1;

	str [0] = '\0';
	err = ee_read (adr, str, len);
	str [len] = '\0';

  state EP_RSTP1:

	ser_outf (EP_RSTP1, "[%d] Read %s (%u) from %lu\r\n",
		err, str, len, adr);
	proceed EP_RCMD;

  state EP_WRI:

	len = 0;
	scan (ibuf + 1, "%lu %u %lu", &adr, &len, &val);
	if (len == 0)
		proceed EP_RCMDP1;
	while (len--) {
		err += ee_write (WNONE, adr, (byte*)(&val), 4);
		adr += 4;
	}

  state EP_WRIP1:

Done:
	ser_outf (EP_WRIP1, "Done %d\r\n", err);
	proceed EP_RCMD;

  state EP_REA:

	len = 0;
	bs = 0;
	nt = 0;
	scan (ibuf + 1, "%lu %u %u %u", &adr, &len, &bs, &nt);
	if (len == 0)
		proceed EP_RCMDP1;
	if (bs == 0)
		bs = 4;

	if (nt == 0)
		nt = 1;

	blk = (byte*) umalloc (bs);

	while (nt--) {

		sl = len;
		ss = adr;
		while (sl--) {
			err += ee_read (ss, blk, bs);
			ss += bs;
		}

	}

	ufree (blk);

	goto Done;

  state EP_ERA:

	adr = 0;
	u = 0;
	scan (ibuf + 1, "%lu %lu", &adr, &u);
	err = ee_erase (WNONE, adr, u);
	goto Done;

  state EP_SYN:

	err = ee_sync (WNONE);
	goto Done;

  state EP_ETS:

	// ERASE-WRITE-READ

	s = 1;
	u = 0;
	pat = LWNONE;
	scan (ibuf + 1, "%lu %lu %lx", &s, &u, &pat);

	// Truncate to longword boundaries
	s = s & 0xfffffffc;
	u = u & 0xfffffffc;
	if (u != 0)
		u += s;
	else
		// Everything
		s = 0;

  state EP_ETS_O:

	if (u == 0) {
		ser_out (EP_ETS_O, "ERASING ALL FLASH\r\n");
	} else {
		ser_outf (EP_ETS_O, "ERASING from %lu (%lx) to %lu (%lx)\r\n",
			s, s, u - 4, u - 4);
	}

  state EP_ETS_E:

	//w = ee_erase (WNONE, s, u);
	w = ee_erase (EP_ETS_E, s, u);

  state EP_ETS_F:

	ser_outf (EP_ETS_F, "ERASE COMPLETE, %u ERRORS\r\n", w);

	adr = s;
	w = 0;

  state EP_ETS_G:

	if (pat == LWNONE)
		val = adr;
	else
		val = pat;

	//w += ee_write (WNONE, adr, (byte*)(&adr), 4);
	w += ee_write (EP_ETS_G, adr, (byte*)(&val), 4);

	if ((adr & 0xFFF) == 0)
		proceed EP_ETS_K;

  state EP_ETS_M:

	adr += 4;
	if (adr < u)
		proceed EP_ETS_G;

  state EP_ETS_H:

	//w += ee_sync (WNONE);
	w += ee_sync (EP_ETS_H);
	ser_outf (EP_ETS_H, "WRITE COMPLETE, %u ERRORS\r\n", w);

	// Start reading
	adr = s;
	w = 0;
	err = 0;

  state EP_ETS_I:

	w += ee_read (adr, (byte*)(&val), 4);

	if (pat != LWNONE) {
		if (val != pat) {
			diag ("MISREAD (PATTERN): %x %x => %x %x",
				(word)(adr >> 16), (word) adr,
				(word)(val >> 16), (word) val);
			err++;
		}
	} else {
		if (val != adr) {
			diag ("MISREAD (ADDRESS): %x %x != %x %x",
				(word)(val >> 16), (word) val,
				(word)(adr  >> 16), (word) adr );
			err++;
		}
	}
	if ((adr & 0xFFF) == 0)
		proceed EP_ETS_L;

  state EP_ETS_N:

	adr += 4;
	if (adr < u)
		proceed EP_ETS_I;

  state EP_ETS_J:

	ser_outf (EP_ETS_J, "READ COMPLETE, %u ERRORS, %u MISREADS\r\n",
		w, err);

	proceed EP_RCMD;

  state EP_ETS_K:

	ser_outf (EP_ETS_K, "WRITTEN %lu (%lx)\r\n", adr, adr);
	proceed EP_ETS_M;

  state EP_ETS_L:

	ser_outf (EP_ETS_L, "READ %lu (%lx)\r\n", adr, adr);
	proceed EP_ETS_N;

}

#endif

// ============================================================================

#ifdef SDC_TEST

fsm test_sdcard {

  state SD_INIT:

	if ((err = sd_open ()) == 0) 
		proceed SD_OK;

  state SD_IFAIL:

	ser_outf (SD_IFAIL, "Failed to open: %u\r\n", err);
	finish;

  state SD_OK:

	ser_outf (SD_OK, "Card size: %lu\r\n", sd_size ());

  state SD_START:

	ser_out (SD_START,
		"\r\nSD Test\r\n"
		"a adr int -> store a word\r\n"
		"b adr lint -> store a lword\r\n"
		"c adr str -> store a string\r\n"
		"d adr -> read word\r\n"
		"e adr -> read lword\r\n"
		"f adr n -> read string\r\n"
		"g adr n p -> write n longwords with p at adr\r\n"
		"h adr n b t -> read n blks of b at adr t times\r\n"
		"x frm upt -> erase from upto\r\n"
		"s -> sync card\r\n"
		"w fr ln pat -> write-read test\r\n"
		"i -> set idle state\r\n"
		"q -> return to main test\r\n"
	);

  state SD_RCMD:

	err = 0;
	ser_in (SD_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'a': proceed SD_SWO;
		case 'b': proceed SD_SLW;
		case 'c': proceed SD_SST;
		case 'd': proceed SD_RWO;
		case 'e': proceed SD_RLW;
		case 'f': proceed SD_RST;
		case 'g': proceed SD_WRI;
		case 'h': proceed SD_REA;
		case 'x': proceed SD_ERA;
		case 's': proceed SD_SYN;
		case 'w': proceed SD_ETS;
		case 'i': proceed SD_IDL;
		case 'q': { sd_close (); finish; };
	}
	
  state SD_RCMDP1:

	ser_out (SD_RCMDP1, "Illegal\r\n");
	proceed SD_START;

  state SD_SWO:

	scan (ibuf + 1, "%lu %u", &adr, &w);
	err = sd_write (adr, (byte*)(&w), 2);

  state SD_SWOP1:

	ser_outf (SD_SWOP1, "[%d] Stored %u at %lu\r\n", err, w, adr);
	proceed SD_RCMD;

  state SD_SLW:

	scan (ibuf + 1, "%lu %lu", &adr, &val);
	err = sd_write (adr, (byte*)(&val), 4);

  state SD_SLWP1:

	ser_outf (SD_SLWP1, "[%d] Stored %lu at %lu\r\n", err, val, adr);
	proceed SD_RCMD;

  state SD_SST:

	scan (ibuf + 1, "%lu %s", &adr, str);
	len = strlen (str);
	if (len == 0)
		proceed SD_RCMDP1;

	err = sd_write (adr, str, len);

  state SD_SSTP1:

	ser_outf (SD_SSTP1, "[%d] Stored %s (%u) at %lu\r\n", err, str, len,
		adr);
	proceed SD_RCMD;

  state SD_RWO:

	scan (ibuf + 1, "%lu", &adr);
	err = sd_read (adr, (byte*)(&w), 2);

  state SD_RWOP1:

	ser_outf (SD_RWOP1, "[%d] Read %u (%x) from %lu\r\n", err, w, w, adr);
	proceed SD_RCMD;

  state SD_RLW:

	scan (ibuf + 1, "%lu", &adr);
	err = sd_read (adr, (byte*)(&val), 4);

  state SD_RLWP1:

	ser_outf (SD_RLWP1, "[%d] Read %lu (%lx) from %lu\r\n",
		err, val, val, adr);
	proceed SD_RCMD;

  state SD_RST:

	scan (ibuf + 1, "%lu %u", &adr, &len);
	if (len == 0)
		proceed SD_RCMDP1;

	str [0] = '\0';
	err = sd_read (adr, str, len);
	str [len] = '\0';

  state SD_RSTP1:

	ser_outf (SD_RSTP1, "[%d] Read %s (%u) from %lu\r\n",
		err, str, len, adr);
	proceed SD_RCMD;

  state SD_WRI:

	len = 0;
	scan (ibuf + 1, "%lu %u %lu", &adr, &len, &val);
	if (len == 0)
		proceed SD_RCMDP1;
	while (len--) {
		err += (sd_write (adr, (byte*)(&val), 4) != 0);
		adr += 4;
	}

  state SD_WRIP1:

Done:
	ser_outf (SD_WRIP1, "Done %d\r\n", err);
	proceed SD_RCMD;

  state SD_REA:

	len = 0;
	bs = 0;
	nt = 0;
	scan (ibuf + 1, "%lu %u %u %u", &adr, &len, &bs, &nt);
	if (len == 0)
		proceed SD_RCMDP1;
	if (bs == 0)
		bs = 4;

	if (nt == 0)
		nt = 1;

	blk = (byte*) umalloc (bs);

	while (nt--) {

		sl = len;
		ss = adr;
		while (sl--) {
			err += (sd_read (ss, blk, bs) != 0);
			ss += bs;
		}

	}

	ufree (blk);

	goto Done;

  state SD_ERA:

	s = u = 0;
	scan (ibuf + 1, "%lu %lu", &s, &u);
	err = sd_erase (s, u);
	goto Done;

  state SD_SYN:

	err = sd_sync ();
	goto Done;

  state SD_ETS:

	// WRITE-READ

	s = 1;
	u = 0;
	pat = LWNONE;
	scan (ibuf + 1, "%lu %lu %lx", &s, &u, &pat);

	// Truncate to longword boundaries
	s = s & 0xfffffffc;
	u = u & 0xfffffffc;
	if (u != 0)
		u += s;
	else
		// Everything
		s = 0;

	adr = s;
	w = 0;

  state SD_ETS_G:

	if (pat == LWNONE)
		val = adr;
	else
		val = pat;

	w += (sd_write (adr, (byte*)(&val), 4) != 0);

	if ((adr & 0xFFF) == 0)
		proceed SD_ETS_K;

  state SD_ETS_M:

	adr += 4;
	if (adr < u)
		proceed SD_ETS_G;

  state SD_ETS_H:

	w += (sd_sync () != 0);
	ser_outf (SD_ETS_H, "WRITE COMPLETE, %u ERRORS\r\n", w);

	// Start reading
	adr = s;
	w = 0;
	err = 0;

  state SD_ETS_I:

	w += (sd_read (adr, (byte*)(&val), 4) != 0);

	if (pat != LWNONE) {
		if (val != pat) {
			diag ("MISREAD (PATTERN): %x %x => %x %x",
				(word)(adr >> 16), (word) adr,
				(word)(val >> 16), (word) val);
			err++;
		}
	} else {
		if (val != adr) {
			diag ("MISREAD (ADDRESS): %x %x != %x %x",
				(word)(val >> 16), (word) val,
				(word)(adr  >> 16), (word) adr );
			err++;
		}
	}
	if ((adr & 0xFFF) == 0)
		proceed SD_ETS_L;

  state SD_ETS_N:

	adr += 4;
	if (adr < u)
		proceed SD_ETS_I;

  state SD_ETS_J:

	ser_outf (SD_ETS_J, "READ COMPLETE, %u ERRORS, %u MISREADS\r\n",
		w, err);

	proceed SD_RCMD;

  state SD_ETS_K:

	ser_outf (SD_ETS_K, "WRITTEN %lu (%lx)\r\n", adr, adr);
	proceed SD_ETS_M;

  state SD_ETS_L:

	ser_outf (SD_ETS_L, "READ %lu (%lx)\r\n", adr, adr);
	proceed SD_ETS_N;

  state SD_IDL:

	sd_idle ();
	goto Done;

}

#endif

// ============================================================================

fsm test_delay {

  state DE_INIT:

	ser_out (DE_INIT,
		"\r\nRF Power Test\r\n"
#if GLACIER
		"f s -> freeze\r\n"
#endif
		"l s -> hold\r\n"
		"d -> PD mode (unsafe)\r\n"
		"u -> PU mode\r\n"
		"s n -> spin test for n sec\r\n"
		"q -> return to main test\r\n"
	);

  state DE_RCMD:

	ser_in (DE_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
#if GLACIER
		case 'f': proceed DE_FRE;
#endif
		case 'l': proceed DE_LHO;
		case 'd': proceed DE_PDM;
		case 'u': proceed DE_PUM;
		case 's': proceed DE_SPN;
		case 'q': { finish; }
	}
	
  state DE_RCMDP1:

	ser_out (DE_RCMDP1, "Illegal\r\n");
	proceed DE_INIT;

#if GLACIER
  state DE_FRE:

	nt = 0;
	scan (ibuf + 1, "%u", &nt);
	diag ("Start");
	freeze (nt);
	diag ("Stop");
	proceed DE_RCMD;
#endif

  state DE_LHO:

	nt = 0;
	scan (ibuf + 1, "%u", &nt);
	val = (lword) nt + seconds ();
	diag ("Start %u", (word) seconds ());

  state DE_LHOP1:

	hold (DE_LHOP1, val);
	diag ("Stop %u", (word) seconds ());
	proceed DE_RCMD;

  state DE_PDM:

	diag ("Entering PD mode");
	powerdown ();
	proceed DE_RCMD;

  state DE_PUM:

	diag ("Entering PU mode");
	powerup ();
	proceed DE_RCMD;

  state DE_SPN:

	nt = 0;
	scan (ibuf + 1, "%u", &nt);

	// Wait for the nearest round second
	s = seconds ();

  state DE_SPNP1:

	if ((u = seconds ()) == s)
		proceed DE_SPNP1;

	u += nt;
	s = 0;

  state DE_SPNP2:

	if (seconds () != u) {
		s++;
		proceed DE_SPNP2;
	}

  state DE_SPNP3:

	ser_outf (DE_SPNP2, "Done: %lu cycles\r\n", s);
	proceed DE_RCMD;

}

// ============================================================================

fsm test_uart {

    state UA_LOOP:

	ser_in (UA_LOOP, ibuf, IBUFLEN-1);

    state UA_BACK:

	ser_outf (UA_BACK, "%s\r\n", ibuf);

	if (ibuf [0] == 'q' && ibuf [1] == '\0')
		// Done
		finish;
	proceed UA_LOOP;

}

// ============================================================================

#ifdef gps_bring_up

fsm minput {

  char c;

  state MI_READ:

	io (MI_READ, UART_B, READ, &c, 1);

  state MI_WRITE:

	io (MI_WRITE, UART_A, WRITE, &c, 1);
	proceed MI_READ;

}

fsm test_gps {

  state GP_INI:

  state GP_MEN:

	ser_out (GP_MEN,
		"\r\nGPS Test\r\n"
		"w string -> write line to module\r\n"
		"e -> enable\r\n"
		"d -> disable\r\n"
		"q -> quit\r\n"
	);

  state GP_RCM:

	ser_in (GP_RCM, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

	    case 'w': proceed GP_WRI;
	    case 'e': proceed GP_ENA;
	    case 'd': proceed GP_DIS;
	    case 'q': {
			gps_bring_down;
			killall (minput);
			finish;
	    }

	}

  state GP_ERR:

	ser_out (GP_ERR, "Illegal\r\n");
	proceed GP_MEN;

  state GP_WRI:

	for (b = 1; ibuf [b] == ' '; b++);
	off = strlen (ibuf + b);

	ibuf [b + off    ] = '\r';
	ibuf [b + off + 1] = '\n';
	off += 2;
	ibuf [b + off    ] = '\0';

  state GP_WRL:

	while (off) {
		rcvl = io (GP_WRL, UART_B, WRITE, ibuf + b, off);
		off -= rcvl;
		b += rcvl;
	}

	proceed GP_RCM;

  state GP_ENA:

	gps_bring_up;
	killall (minput);
	runfsm minput;
	proceed GP_RCM;

  state GP_DIS:

	gps_bring_down;
	killall (minput);
	proceed GP_RCM;

}

#endif /* gps_bring_up */

// ============================================================================

#ifdef RTC_TEST

fsm test_rtc {

  state RT_MEN:

	ser_out (RT_MEN,
		"\r\nRTC Test\r\n"
		"s y m d dw h m s -> set the clock\r\n"
		"r -> read the clock\r\n"
#ifdef RTC_REG
		"w b -> write reg\r\n"
		"g -> read reg\r\n"
#endif
		"q -> quit\r\n"
	);

  state RT_RCM:

	ser_in (RT_RCM, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

	    case 's': proceed RT_SET;
	    case 'r': proceed RT_GET;
#ifdef RTC_REG
	    case 'w': proceed RT_SETR;
	    case 'g': proceed RT_GETR;
#endif
	    case 'q': {
			finish;
	    }

	}

  state RT_ERR:

	ser_out (RT_ERR, "Illegal\r\n");
	proceed RT_MEN;

  state RT_SET:

	sl = WNONE;
	scan (ibuf + 1, "%u %u %u %u %u %u %u",
		&rssi, &err, &w, &len, &bs, &nt, &sl);

	if (sl == WNONE)
		proceed RT_ERR;

	dtime.year = rssi;
	dtime.month = err;
	dtime.day = w;
	dtime.dow = len;
	dtime.hour = bs;
	dtime.minute = nt;
	dtime.second = sl;

	rtc_set (&dtime);

  state RT_SETP1:

	ser_out (RT_SETP1, "Done\r\n");
	proceed RT_RCM;

  state RT_GET:

	bzero (&dtime, sizeof (dtime));
	rtc_get (&dtime);

  state RT_GETP1:

	ser_outf (RT_GETP1, "Date = %u %u %u %u %u %u %u\r\n",
				dtime.year,
				dtime.month,
				dtime.day,
				dtime.dow,
				dtime.hour,
				dtime.minute,
				dtime.second);
	proceed RT_RCM;

#ifdef RTC_REG

  state RT_SETR:

	sl = 0;
	scan (ibuf + 1, "%u", &sl);
	err = rtc_setr ((byte) sl);
	proceed RT_SETP1;

  state RT_GETR:

	str [0] = 0xff;
	err = rtc_getr (str);

  state RT_GETRP1:

	ser_outf (RT_GETRP1, "Status = %u // %u\r\n", err, (word) (str [0]));
	proceed RT_RCM;

#endif

}

#endif /* RTC_TEST */

// ============================================================================

#ifdef LCD_TEST

fsm test_lcd {

  state LT_MEN:

	ser_out (LT_MEN,
		"\r\nLCD Test\r\n"
		"o n -> on (1 = cursor shown)\r\n"
		"f -> off\r\n"
		"d t -> display text\r\n"
		"c -> clear\r\n"
		"q -> quit\r\n"
	);

  state LT_RCM:

	ser_in (LT_RCM, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

	    case 'o': proceed LT_ON;
	    case 'f': proceed LT_OFF;
	    case 'd': proceed LT_DIS;
	    case 'c': proceed LT_CLE;
	    case 'q': {
			lcd_off ();
			finish;
	    }

	}

  state LT_ERR:

	ser_out (LT_ERR, "Illegal\r\n");
	proceed LT_MEN;

  state LT_ON:

	sl = 0;
	scan (ibuf + 1, "%x", &sl);

	if (sl)
		sl = 1;

	lcd_on (sl);
	proceed LT_RCM;

  state LT_OFF:

	lcd_off ();
	proceed LT_RCM;

  state LT_DIS:

	char *t;

	for (t = ibuf + 1; *t != '\0'; t++)
		if (*t != ' ' && *t != '\t')
			break;

	if (*t == '\0')
		proceed LT_ERR;

	lcd_write (0, t);
	proceed LT_RCM;

  state LT_CLE:

	lcd_clear (0, 0);
	proceed LT_RCM;

}

#endif /* LCD_TEST */

// ============================================================================

#ifdef	SENSOR_LIST

fsm test_sensors {

  state SE_INIT:

	ser_out (SE_INIT,
		"\r\nSensor Test\r\n"
#ifdef	cma3000_csel
		"s 0|1 -> cma3000 off, on\r\n"
#endif
		"r s -> read sensor s\r\n"
		"c s d n -> read continually at d ms, n times\r\n"
		"q -> quit\r\n"
		);

  state SE_RCMD:

	ser_in (SE_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
#ifdef	cma3000_csel
		case 's' : proceed SE_CMA3;
#endif
		case 'r' : proceed SE_GSEN;
		case 'c' : proceed SE_CSEN;
	    	case 'q': { finish; };
	}

  state SE_RCMDP1:

	ser_out (SE_RCMDP1, "Illegal\r\n");
	proceed SE_INIT;

  state SE_OK:

	ser_out (SE_OK, "OK\r\n");
	proceed SE_RCMD;

#ifdef	cma3000_csel

  state SE_CMA3:

	w = 0;
	scan (ibuf + 1, "%u", &w);
	if (w == 0)
		cma3000_off ();
	else 
		cma3000_on (0, 1, 3);

	proceed SE_OK;
#endif

  state SE_GSEN:

	b = 0;
	scan (ibuf + 1, "%d", &b);

  state SE_GSENP1:

	// To detect absent sensors, value ABSENT01
	val = 0xAB2E4700;
	read_sensor (SE_GSENP1, b, (address)(&val));

  state SE_GSENP2:

	ser_outf (SE_GSENP2, "Val: %lx [%u] <%d, %d, %d, %d>\r\n",
		val,
		*((word*)(&val)),
		((char*)(&val)) [0],
		((char*)(&val)) [1],
		((char*)(&val)) [2],
		((char*)(&val)) [3]);
	proceed SE_RCMD;

  state SE_CSEN:

	b = 0;
	bs = 0;
	nt = 0;

	scan (ibuf + 1, "%d %u %u", &b, &bs, &nt);
	if (nt == 0)
		nt = 1;

  state SE_CSENP1:

	val = 0xAB2E4700;
	read_sensor (SE_CSENP1, b, (address)(&val));
	nt--;

  state SE_CSENP2:

	ser_outf (SE_CSENP2, "Val: %lx [%u] <%d, %d, %d, %d> (%u left)\r\n",
		val,
		*((word*)(&val)),
		((char*)(&val)) [0],
		((char*)(&val)) [1],
		((char*)(&val)) [2],
		((char*)(&val)) [3], nt);

	if (nt == 0)
		proceed SE_RCMD;
	
	delay (bs, SE_CSENP1);
	release;

}

#endif

// ============================================================================

#ifdef bma250_csel

#ifndef	SENSOR_MOTION
#error "SENSOR_MOTION must be defined along with bma250_csel!!"
#endif

#define	_w	((word*)str)

static word bma250_rdelay;

fsm bma250_thread {

  bma250_data_t c;
  char tp;

  state AT_WAIT:

	tp = 'V';

	if (bma250_rdelay)
		delay (bma250_rdelay, AT_TIMEOUT);

	wait_sensor (SENSOR_MOTION, AT_EVENT);

	release;

  state AT_TIMEOUT:

	read_sensor (AT_TIMEOUT, SENSOR_MOTION, (address)(&c));

  state AT_REPORT:

	ser_outf (AT_REPORT, "%c: [%x] t=%d <%d,%d,%d>\r\n",
		tp, c.stat, c.temp, c.x, c.y, c.z);

	sameas AT_WAIT;

  state AT_EVENT:

	tp = 'E';
	sameas AT_TIMEOUT;
}

fsm test_bma250 {

  state BT_INIT:

	ser_out (BT_INIT,
		"\r\nBMA250 Test\r\n"
		"s r b s -> on\r\n"
		"e m -> off\r\n"
		"q -> quit\r\n"
		"m ns th -> motion\r\n"
		"t md th de ns -> tap\r\n"
		"o bl md th hy -> orient\r\n"
		"f th ho -> flat\r\n"
		"l md th du hy -> fall\r\n"
		"h th du hy -> hit\r\n"
		"r de -> run thread\r\n"
		"u -> stop\r\n"
	);

  state BT_RCMD:

	ser_in (BT_RCMD, ibuf, IBUFLEN-1);

	_w[0] = 0;
	_w[1] = 0;
	_w[2] = 0;
	_w[3] = 0;

	switch (ibuf [0]) {

		case 's' : proceed BT_START;
		case 'e' : proceed BT_OFF;
		case 'q' : {
			killall (bma250_thread);
			finish;
		}
		case 'm' : proceed BT_MOVE;
		case 't' : proceed BT_TAP;
		case 'o' : proceed BT_ORIENT;
		case 'f' : proceed BT_FLAT;
		case 'l' : proceed BT_FALL;
		case 'h' : proceed BT_HIT;
		case 'r' : proceed BT_RUN;
		case 'u' : proceed BT_STOP;
	}

  state BT_IERR:

	ser_out (BT_IERR, "Illegal\r\n");
	proceed BT_INIT;

  state BT_OK:

	ser_out (BT_OK, "OK\r\n");
	proceed BT_RCMD;

  state BT_START:

	scan (ibuf + 1, "%u %u %x", _w+0, _w+1, _w+2);

	switch (_w[0]) {
		case 0:  _w[0] = BMA250_RANGE_2G; break;
		case 1:  _w[0] = BMA250_RANGE_4G; break;
		case 2:  _w[0] = BMA250_RANGE_8G; break;
		default: _w[0] = BMA250_RANGE_16G;
	}

	if (_w[1] > 7)
		_w[1] = 7;

	bma250_on ((byte)(_w[0]), (byte)(_w[1]), (byte)(_w[2]));

	proceed BT_OK;

  state BT_OFF:

	scan (ibuf + 1, "%u", _w+0);

	if (_w[0] > 12)
		_w[0] = 12;

	bma250_off ((byte)(_w[0]));

	proceed BT_OK;

  state BT_MOVE:

	scan (ibuf + 1, "%u %u", _w+0, _w+1);
	bma250_move ((byte)(_w[0]), (byte)(_w[1]));
	proceed BT_OK;

  state BT_TAP:

	scan (ibuf + 1, "%u %u %u %u", _w+0, _w+1, _w+2, _w+3);
	if (_w[0] > 3)
		_w[0] = 3;
	bma250_tap ((byte)(_w[0] << 6), (byte)(_w[1]), (byte)(_w[2]),
		(byte)(_w[3]));
	proceed BT_OK;

  state BT_ORIENT:

	scan (ibuf + 1, "%u %u %u %u", _w+0, _w+1, _w+2, _w+3);
	if (_w[0] > 3)
		_w[0] = 3;
	if (_w[1] > 3)
		_w[1] = 3;
	if (_w[2] > 63)
		_w[2] = 63;
	if (_w[3] > 7)
		_w[3] = 7;
	bma250_orient ((byte)(_w[0] << 2), (byte)(_w[1]), (byte)(_w[2]),
		(byte)(_w[3]));
	proceed BT_OK;

  state BT_FLAT:

	scan (ibuf + 1, "%u %u", _w+0, _w+1);
	bma250_flat ((byte)(_w[0]), (byte)(_w[1]));
	proceed BT_OK;

  state BT_FALL:

	scan (ibuf + 1, "%u %u %u %u", _w+0, _w+1, _w+2, _w+3);
	if (_w[0])
		_w[0] = BMA250_LOWG_MODSUM;
	bma250_lowg ((byte)(_w[0]), (byte)(_w[1]), (byte)(_w[2]),
		(byte)(_w[3]));
	proceed BT_OK;

  state BT_HIT:

	scan (ibuf + 1, "%u %u %u", _w+0, _w+1, _w+2);
	bma250_highg ((byte)(_w[0]), (byte)(_w[1]), (byte)(_w[2]));
	proceed BT_OK;

  state BT_RUN:

	scan (ibuf + 1, "%u", &bma250_rdelay);
	if (!running (bma250_thread))
		runfsm bma250_thread;
	proceed BT_OK;

  state BT_STOP:

	killall (bma250_thread);
	proceed BT_OK;
}

#endif

// ============================================================================

fsm test_adc {

  state AD_INIT:

	ser_out (AD_INIT,
		"\r\nADC Test\r\n"
		"c p rf st -> configure\r\n"
		"s -> start\r\n"
		"h -> stop & read\r\n"
		"f -> off\r\n"
		"d -> disable\r\n"
		"q -> quit\r\n"
		);

  state AD_RCMD:

	ser_in (AD_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'c' : proceed AD_CONF;
		case 's' : { adc_start;    proceed AD_OK; }
		case 'f' : { adc_off;      proceed AD_OK; }
		case 'd' : { adc_disable;  proceed AD_OK; }
		case 'h' : proceed AD_STOP;
	    	case 'q' : { adc_disable;  finish; };
	}

  state AD_RCMDP1:

	ser_out (AD_RCMDP1, "Illegal\r\n");
	proceed AD_INIT;

  state AD_CONF:

	nt = 0;
	sl = 0;
	ss = 0;

	scan (ibuf + 1, "%u %u %u", &nt, &sl, &ss);

	if (nt > 7 || sl > 3 || ss > 15)
		proceed AD_RCMDP1;

	adc_config_read (nt, sl, ss);

  state AD_OK:

	ser_out (AD_OK, "OK\r\n");
	proceed AD_RCMD;

  state AD_STOP:

	adc_stop;
	if (adc_busy)
		proceed AD_STOPP1;
Value:
	nt = adc_value;

  state AD_STOPP3:

	ser_outf (AD_STOPP3, "Value = %u [%x]\r\n", nt, nt);
	proceed AD_RCMD;

  state AD_STOPP1:

	ser_out (AD_STOPP1, "Waiting for idle ...\r\n");
	while (adc_busy);

  state AD_STOPP2:

	ser_out (AD_STOPP2, "Idle\r\n");
	goto Value;
}

// ============================================================================

#ifdef	BUTTON_LIST

static word Buttons;

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

static void butpress (word but) {

	Buttons |= (1 << but);
	trigger (&Buttons);
}

fsm test_buttons {

	state TB_INIT:

		ser_out (TB_INIT, "Press button, q to quit\r\n");

		buttons_action (butpress);

		if (!running (button_thread))
			runfsm button_thread;

	state TB_WAIT:

		ser_in (TB_WAIT, ibuf, IBUFLEN-1);

		if (ibuf [0] == 'q') {
			buttons_action (NULL);
			killall (button_thread);
			finish;
		}

		sameas TB_WAIT;
}

#endif

// ============================================================================

#if (RADIO_OPTIONS & 0x40)

#define	MAXRREGS 8

static byte rregs [2*MAXRREGS+1] = { 255 };

static char *dec_hex (char *p, byte *b) {

	while (!isxdigit (*p)) {
		if (*p == '\0')
			return NULL;
		p++;
	}

	*b = hexcode (*p);
	p++;
	if (!isxdigit (*p))
		return NULL;
	*b = ((*b) << 4) | hexcode (*p);
	return p + 1;
}

static void set_rregs (char *p) {

	sint nc;
	byte r, v;

	for (nc = 0; nc < MAXRREGS*2; ) {
		if ((p = dec_hex (p, &r)) == NULL)
			break;
		if ((p = dec_hex (p, &v)) == NULL)
			break;
		rregs [nc++] = r;
		rregs [nc++] = v;
	}

	rregs [nc] = 255;
}

#endif

fsm root {

  state RS_INIT:

#if STACK_GUARD
	sl = stackfree ();
#endif
#if MALLOC_STATS
	ss = memfree (0, NULL);
#endif
#if STACK_GUARD || MALLOC_STATS
	diag ("MEM: %d %d", sl, ss);
#endif
	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0;

#if 0
#ifdef RTC_TEST

	// Check if the clock is running; if not, set it to anything as
	// otherwise it drains current;
	// Note: the problem disappears if the pull-up resisitors are driven
	// from a pin (which is only on when required for communication)

	if (rtc_get (&dtime) == 0) {
		if (dtime.year == 0) {
			dtime.year = 9;
			rtc_set (&dtime);
		}
	}
#endif
#endif

	diag ("");
	diag ("");

#if CC1100
	diag ("Radio ...");
	phys_cc1100 (0, MAXPLEN);

	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	off = 255;
	tcv_control (sfd, PHYSOPT_SETPOWER, (address)&off);
	off = 0;
	tcv_control (sfd, PHYSOPT_SETCHANNEL, (address)&off);
	off = 0;
	tcv_control (sfd, PHYSOPT_SETSID, (address)&off);
#endif

#if AUTO_RADIO_START
	w = 3;
	goto StartRadio;
#endif

  state RS_RCMDM2:

	ser_out (RS_RCMDM2,
		"\r\nRoot:\r\n"
		"a -> auto\r\n"
#if (RADIO_OPTIONS & 0x40)
		"r s regs -> start radio\r\n"
#else
		"r s -> start radio\r\n"
#endif
		"i d -> xmit interval\r\n"
		"p v  -> xmit pwr\r\n"
		"c v  -> channel\r\n"
		"q -> stop radio\r\n"
#if STACK_GUARD || MALLOC_STATS
		"m -> memstat\r\n"
#endif
		"n -> reset\r\n"
		"u 0|1 v  -> set uart 0|1 rate [def = 96]\r\n"
		"d -> pwr: 0-d, 1-u\r\n"
#ifdef cswitch_on
		"o c  -> cswitch on\r\n"
		"f c  -> cswitch off\r\n"
#endif
		"F -> flash test\r\n"

#ifdef EPR_TEST
		"E -> EEPROM test\r\n"
#endif

#ifdef SDC_TEST
		"S -> SD test\r\n"
#endif
		"P -> pin test\r\n"
		"D -> power\r\n"

#ifdef	gps_bring_up
		"G -> GPS\r\n"
#endif

#ifdef SENSOR_LIST
		"V -> sensors\r\n"
#endif

#ifdef bma250_csel
		"B -> bma250\r\n"
#endif

		"A -> ADC\r\n"

#ifdef RTC_TEST
		"T -> RTC\r\n"
#endif

#ifdef LCD_TEST
		"L -> LCD\r\n"
#endif
#ifdef BUTTON_LIST
		"b -> buttons\r\n"
#endif
		"U -> UART echo\r\n"
	);

  state RS_RCMDM1:

RS_Err:

	if ((unsigned char) ibuf [0] == 0xff)
		ser_out (RS_RCMDM1,
			"No cmd in 30 sec -> start radio\r\n"
			);
  state RS_RCMD:

	if ((unsigned char) ibuf [0] == 0xff)
		delay (1024*30, RS_AUTOSTART);
  
	ser_in (RS_RCMD, ibuf, IBUFLEN-1);
	unwait ();

	switch (ibuf [0]) {

		case 'a' : proceed RS_AUTO;

		case 'r' : {

			char *p;

			w = 0;
			p = ibuf + 1;
			while (!isdigit (*p) && *p != '\0')
				p++;
			if (*p != '\0') {
				w = (*p) - '0';
				p++;
			}
			if ((w & 3) == 0)
				w |= 3;
#if (RADIO_OPTIONS & 0x40)
			set_rregs (p);
			tcv_control (sfd, PHYSOPT_RESET, (address)rregs);
			diag ("RREGS: %x %x %x %x %x %x",
				rregs [0], rregs [1], rregs [2], rregs [3],
					rregs [4], rregs [5]);
#endif
			diag ("RSTAR: %d", w);
StartRadio:
			radio_start (w);

RS_Loop:		proceed RS_RCMD;
		}

		case 'i' : {

			send_interval = 512;
			b = -1;
			scan (ibuf + 1, "%u %d", &send_interval, &b);
			if (b >= 0)
				silent = b;
			goto RS_Loop;
		}
	
		case 'p' : {
			// Setpower, default = max
			off = 255;
			scan (ibuf + 1, "%d", &off);
			tcv_control (sfd, PHYSOPT_SETPOWER, (address)&off);
			goto RS_Loop;
		}

		case 'c' : {
			off = 0;
			scan (ibuf + 1, "%d", &off);
			tcv_control (sfd, PHYSOPT_SETCHANNEL, (address)&off);
			goto RS_Loop;
		}

		case 'q' : {
			radio_stop ();
			goto RS_Loop;
		}

		case 'u' : {
			off = 0;
			w = 0;
			scan (ibuf + 1, "%u %d", &w, &off);
			if (off == 0)
				off = 96;
			if (w > 1)
				w = 1;
			ion (UART_A + w, CONTROL, (char*) &off,
				UART_CNTRL_SETRATE);
			goto RS_Loop;
		}

		case 'd' : {
			off = 0;
			scan (ibuf + 1, "%d", &off);
			if (off)
				powerup ();
			else
				powerdown ();
			goto RS_Loop;
		}

#ifdef cswitch_on
		case 'o' : {
			w = 0;
			scan (ibuf + 1, "%u", &w);
			if (w == 0)
				goto RS_Err;
			cswitch_on (w);
			goto RS_Loop;
		}

		case 'f' : {
			w = 0;
			scan (ibuf + 1, "%u", &w);
			if (w == 0)
				goto RS_Err;
			cswitch_off (w);
			goto RS_Loop;
		}
#endif
		case 'n' : reset ();

		case 'F' : {
				runfsm test_ifl;
				joinall (test_ifl, RS_RCMDM2);
				release;
		}

#ifdef EPR_TEST
		case 'E' : {
				runfsm test_epr;
				joinall (test_epr, RS_RCMDM2);
				release;
		}
#endif

#ifdef SDC_TEST
		case 'S' : {
				runfsm test_sdcard;
				joinall (test_sdcard, RS_RCMDM2);
				release;
		}
#endif

		case 'P' : {
				runfsm test_pin;
				joinall (test_pin, RS_RCMDM2);
				release;
		}
		case 'D' : {
				runfsm test_delay;
				joinall (test_delay, RS_RCMDM2);
				release;
		}
		case 'U' : {
				runfsm test_uart;
				joinall (test_uart, RS_RCMDM2);
				release;
		}
#ifdef gps_bring_up
		case 'G' : {
				runfsm test_gps;
				joinall (test_gps, RS_RCMDM2);
				release;
		}
#endif

#ifdef SENSOR_LIST
		case 'V' : {
				runfsm test_sensors;
				joinall (test_sensors, RS_RCMDM2);
				release;
		}
#endif

#ifdef	bma250_csel
		case 'B' : {
				runfsm test_bma250;
				joinall (test_bma250, RS_RCMDM2);
				release;
		}
#endif
		case 'A' : {
				runfsm test_adc;
				joinall (test_adc, RS_RCMDM2);
				release;
		}

#ifdef RTC_TEST
		case 'T' : {
				runfsm test_rtc;
				joinall (test_rtc, RS_RCMDM2);
				release;
		}
#endif
#ifdef LCD_TEST
		case 'L' : {
				runfsm test_lcd;
				joinall (test_lcd, RS_RCMDM2);
				release;
		}
#endif
#if STACK_GUARD || MALLOC_STATS
		case 'm' : {
#if STACK_GUARD
			sl = stackfree ();
#else
			sl = 0;
#endif
#if MALLOC_STATS
			ss = memfree (0, &bs);
			w = maxfree (0, &len);
#else
			ss = w = bs = len = 0;
#endif
			diag ("S = %u, MF = %u, FA = %u, MX = %u, NC = %u",
				sl, ss, bs, w, len);
			proceed RS_RCMD;
		}
#endif

#ifdef	BUTTON_LIST
		case 'b' : {
			runfsm test_buttons;
			joinall (test_buttons, RS_RCMDM2);
			release;
		}
#endif
	}

  state RS_RCMDP1:

	ser_out (RS_RCMDP1, "Illegal\r\n");
	proceed RS_RCMDM2;

  state RS_AUTO:

	ser_out (RS_AUTO, "Auto, q to stop ...\r\n");
	runfsm test_auto;

  state RS_AUTOP1:

	ser_in (RS_AUTOP1, ibuf, IBUFLEN-1);
	reset ();

  state RS_AUTOSTART:

	ibuf [0] = 0;
	radio_start (3);
	proceed RS_RCMD;
}

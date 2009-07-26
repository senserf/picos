/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"
#include "iflash_sys.h"
#include "storage.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	42

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"
#include "storage.h"

#if CC1100
#include "phys_cc1100.h"
#include "plug_null.h"
#endif

#include "pinopts.h"
#include "lhold.h"

#ifdef	RTC_PRESENT
#include "rtc.h"
#endif

#define	IBUFLEN		132
#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

#define	EEPROM_INCR	255
#define	SEND_INTERVAL	1024

extern void* _etext;

static int 	sfd = -1, off, packet_length, rcvl, b;
static word	rssi, err, w, len, bs, nt, sl, ss, dcnt;
static lword	adr, max, val, last_snt, last_rcv, s, u, pat;
static byte	str [129], *blk;
static char	*ibuf;
static address	packet;

#ifdef RTC_PRESENT
rtc_time_t dtime;
#endif

static word gen_packet_length (void) {

#if MIN_PACKET_LENGTH >= MAX_PACKET_LENGTH
	return MIN_PACKET_LENGTH;
#else
	return ((rnd () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
#endif

}

#define	SN_SEND		0
#define	SN_NEXT		1
#define	SN_MESS		2

thread (sender)

  int pl, pp;

  entry (SN_SEND)

	packet_length = gen_packet_length ();

	if (packet_length < 10)
		packet_length = 10;
	else if (packet_length > MAX_PACKET_LENGTH)
		packet_length = MAX_PACKET_LENGTH;

  entry (SN_NEXT)

	packet = tcv_wnp (SN_NEXT, sfd, packet_length + 2);

	packet [0] = 0;
	packet [1] = 0xBABA;

	// In words
	pl = packet_length / 2;
	((lword*)packet)[1] = wtonl (last_snt);

	for (pp = 4; pp < pl; pp++)
		packet [pp] = (word) entropy;

	tcv_endp (packet);

  entry (SN_MESS)

	ser_outf (SN_MESS, "Sent: %lu [%d]\r\n", last_snt, packet_length);
	last_snt++;
	delay (SEND_INTERVAL, SN_SEND);

endthread;

#define	RC_TRY		0
#define	RC_MESS		1

thread (receiver)

  address packet;

  entry (RC_TRY)

	packet = tcv_rnp (RC_TRY, sfd);
	last_rcv = ntowl (((lword*)packet) [1]);
	rcvl = tcv_left (packet) - 2;
	rssi = packet [rcvl >> 1];
	tcv_endp (packet);

  entry (RC_MESS)

	ser_outf (RC_MESS, "Rcv: %lu [%d], RSSI = %d, QUA = %d\r\n",
		last_rcv, rcvl, (rssi >> 8) & 0x00ff, rssi & 0x00ff);
	proceed (RC_TRY);

endthread;

void radio_start () {

	if (sfd < 0)
		return;

	tcv_control (sfd, PHYSOPT_RXON, NULL);
	tcv_control (sfd, PHYSOPT_TXON, NULL);

	if (!running (sender))
		runthread (sender);

	if (!running (receiver))
		runthread (receiver);
}

void radio_stop () {

	if (sfd < 0)
		return;

	killall (sender);
	killall (receiver);

	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	tcv_control (sfd, PHYSOPT_TXOFF, NULL);
}

// ============================================================================

#define	PI_INIT		0
#define	PI_RCMD		10
#define	PI_ADC		20
#define	PI_SET		30
#define	PI_VIE		40
#define	PI_SRAW		50
#define	PI_VRAW		60

thread (test_pin)

  entry (PI_INIT)

	ser_out (PI_INIT,
		"\r\nRF Pin Test\r\n"
		"Commands:\r\n"
		"r p r    -> read ADC pin 'p' with reference r:\r\n"
		"                 0-1.5V, 1-2.5V, 2-Vcc, 3-Eref\r\n"
		"s p v    -> set pin 'p' to digital v (0/1)\r\n"
		"v p      -> show the value of pin 'p'\r\n"
		"S p v    -> set raw pin'p' [0,1-out, 2-in, 3-sp]\r\n"
		"V p      -> show\r\n"
		"q        -> return to main test\r\n"
	);

  entry (PI_RCMD)

	ser_in (PI_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'r': proceed (PI_ADC);
		case 's': proceed (PI_SET);
		case 'v': proceed (PI_VIE);
		case 'S': proceed (PI_SRAW);
		case 'V': proceed (PI_VRAW);
		case 'q': { finish; };
	}
	
  entry (PI_RCMD+1)

	ser_out (PI_RCMD+1, "Illegal command or parameter\r\n");
	proceed (PI_INIT);

  entry (PI_ADC)

	nt = sl = 0;
	scan (ibuf + 1, "%u %u", &nt, &sl);

  entry (PI_ADC+1)

	nt = pin_read_adc (PI_ADC+1, nt, sl, 4);

  entry (PI_ADC+2)

	ser_outf (PI_ADC+2, "Value: %u\r\n", nt);
	proceed (PI_RCMD);

  entry (PI_SET)

	nt = sl = 0;
	scan (ibuf + 1, "%u %u", &nt, &sl);
	pin_write (nt, sl);
	proceed (PI_RCMD);

  entry (PI_VIE)

	nt = 0;
	scan (ibuf + 1, "%u", &nt);
	nt = pin_read (nt);

  entry (PI_VIE+1)

	ser_outf (PI_VIE+1, "Value: %u\r\n", nt);
	proceed (PI_RCMD);

  entry (PI_SRAW)

	w = WNONE;
	ss = 0;

	scan (ibuf + 1, "%u %u", &w, &ss);
	if (w >= PORTNAMES_NPINS)
		proceed (PI_RCMD+1);

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
	proceed (PI_RCMD);

  entry (PI_VRAW)

	w = WNONE;
	scan (ibuf + 1, "%u", &w);
	if (w >= PORTNAMES_NPINS)
		proceed (PI_RCMD+1);

	nt = _PV (w);
	sl = _PD (w);
	ss = _PF (w);

  entry (PI_VRAW+1)

	ser_outf (PI_VRAW+1, "Pin %u = val %u, dir %u, fun %u\r\n", nt, sl, ss);
	proceed (PI_RCMD);

endthread

// ============================================================================

#define	EP_START	0
#define	EP_INIT		5
#define	EP_RCMD		10
#define	EP_SWO		20
#define	EP_SLW		30
#define	EP_SST		40
#define	EP_RWO		50
#define	EP_RLW		60
#define	EP_RST		70
#define	EP_WRI		80
#define	EP_REA		90
#define	EP_ERA		100
#define	EP_SYN		110
#define	EP_LED		120
#define	EP_BLI		130
#define	EP_DIA		140
#define	EP_FLR		150
#define	EP_FLW		160
#define	EP_FLE		170
#define	EP_CLR		175
#define	EP_CLW		180
#define	EP_CLE		190
#define	EP_COT		195
#define	EP_ETS		200
#define	EP_ETS_O	210
#define	EP_ETS_E	220
#define	EP_ETS_F	230
#define	EP_ETS_G	240
#define	EP_ETS_M	250
#define	EP_ETS_H	260
#define	EP_ETS_I	270
#define	EP_ETS_N	280
#define	EP_ETS_J	290
#define	EP_ETS_K	300
#define	EP_ETS_L	310

thread (test_epr)

  entry (EP_START)

	if (ee_open () == 0)
		proceed (EP_INIT);

  entry (EP_START+1)

	ser_out (EP_START+1, "Failed to open EEPROM\r\n");
	finish;

  entry (EP_INIT)

	ser_out (EP_INIT,
		"\r\nEEPROM Test\r\n"
		"Commands:\r\n"
		"a adr int    -> store a word\r\n"
		"b adr lint   -> store a lword\r\n"
		"c adr str    -> store a string\r\n"
		"d adr        -> read word\r\n"
		"e adr        -> read lword\r\n"
		"f adr n      -> read string\r\n"
		"g adr n p    -> write n longwords with p starting at adr\r\n"
		"h adr n b t  -> read n blks of b starting at adr t times\r\n"
		"x frm upt    -> erase eeprom from upto\r\n"
		"s            -> sync eeprom\r\n"
		"w fr ln pat  -> erase-write-read test\r\n"
		"i led w      -> led status [w = 0, 1, 2]\r\n"
		"j w          -> blinkrate 0-low, 1-high\r\n"
		"k m          -> write a diag message\r\n"
		"m adr w      -> write word to info flash\r\n"
		"n adr        -> read word from info flash\r\n"
		"o adr        -> erase info flash\r\n"
		"M adr w      -> write word to code flash\r\n"
		"N adr        -> read word from code flash\r\n"
		"O adr        -> erase code flash\r\n"
		"T adr        -> flash overwrite test\r\n"
		"q            -> return to main test\r\n"
	);

  entry (EP_RCMD)

	err = 0;
	ser_in (EP_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'a': proceed (EP_SWO);
		case 'b': proceed (EP_SLW);
		case 'c': proceed (EP_SST);
		case 'd': proceed (EP_RWO);
		case 'e': proceed (EP_RLW);
		case 'f': proceed (EP_RST);
		case 'g': proceed (EP_WRI);
		case 'h': proceed (EP_REA);
		case 'x': proceed (EP_ERA);
		case 's': proceed (EP_SYN);
		case 'w': proceed (EP_ETS);
		case 'i': proceed (EP_LED);
		case 'j': proceed (EP_BLI);
		case 'k': proceed (EP_DIA);
		case 'm': proceed (EP_FLW);
		case 'n': proceed (EP_FLR);
		case 'o': proceed (EP_FLE);
		case 'M': proceed (EP_CLW);
		case 'N': proceed (EP_CLR);
		case 'O': proceed (EP_CLE);
		case 'T': proceed (EP_COT);
		case 'q': { ee_close (); finish; };
	}
	
  entry (EP_RCMD+1)

	ser_out (EP_RCMD+1, "Illegal command or parameter\r\n");
	proceed (EP_INIT);

  entry (EP_SWO)

	scan (ibuf + 1, "%lu %u", &adr, &w);
	err = ee_write (WNONE, adr, (byte*)(&w), 2);

  entry (EP_SWO+1)

	ser_outf (EP_SWO+1, "[%d] Stored %u at %lu\r\n", err, w, adr);
	proceed (EP_RCMD);

  entry (EP_SLW)

	scan (ibuf + 1, "%lu %lu", &adr, &val);
	err = ee_write (WNONE, adr, (byte*)(&val), 4);

  entry (EP_SLW+1)

	ser_outf (EP_SLW+1, "[%d] Stored %lu at %lu\r\n", err, val, adr);
	proceed (EP_RCMD);

  entry (EP_SST)

	scan (ibuf + 1, "%lu %s", &adr, str);
	len = strlen (str);
	if (len == 0)
		proceed (EP_RCMD+1);

	err = ee_write (WNONE, adr, str, len);

  entry (EP_SST+1)

	ser_outf (EP_SST+1, "[%d] Stored %s (%u) at %lu\r\n", err, str, len,
		adr);
	proceed (EP_RCMD);

  entry (EP_RWO)

	scan (ibuf + 1, "%lu", &adr);
	err = ee_read (adr, (byte*)(&w), 2);

  entry (EP_RWO+1)

	ser_outf (EP_RWO+1, "[%d] Read %u (%x) from %lu\r\n", err, w, w, adr);
	proceed (EP_RCMD);

  entry (EP_RLW)

	scan (ibuf + 1, "%lu", &adr);
	err = ee_read (adr, (byte*)(&val), 4);

  entry (EP_RLW+1)

	ser_outf (EP_RLW+1, "[%d] Read %lu (%lx) from %lu\r\n",
		err, val, val, adr);
	proceed (EP_RCMD);

  entry (EP_RST)

	scan (ibuf + 1, "%lu %u", &adr, &len);
	if (len == 0)
		proceed (EP_RCMD+1);

	str [0] = '\0';
	err = ee_read (adr, str, len);
	str [len] = '\0';

  entry (EP_RST+1)

	ser_outf (EP_RST+1, "[%d] Read %s (%u) from %lu\r\n",
		err, str, len, adr);
	proceed (EP_RCMD);

  entry (EP_WRI)

	len = 0;
	scan (ibuf + 1, "%lu %u %lu", &adr, &len, &val);
	if (len == 0)
		proceed (EP_RCMD+1);
	while (len--) {
		err += ee_write (WNONE, adr, (byte*)(&val), 4);
		adr += 4;
	}

  entry(EP_WRI+1)

Done:
	ser_outf (EP_WRI+1, "Done %d\r\n", err);
	proceed (EP_RCMD);

  entry (EP_REA)

	len = 0;
	bs = 0;
	nt = 0;
	scan (ibuf + 1, "%lu %u %u %u", &adr, &len, &bs, &nt);
	if (len == 0)
		proceed (EP_RCMD+1);
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

  entry (EP_ERA)

	adr = 0;
	u = 0;
	scan (ibuf + 1, "%lu %lu", &adr, &u);
	err = ee_erase (WNONE, adr, u);
	goto Done;

  entry (EP_SYN)

	err = ee_sync (WNONE);
	goto Done;

  entry (EP_LED)

	scan (ibuf + 1, "%u %u", &bs, &nt);
	leds (bs, nt);
	proceed (EP_RCMD);

  entry (EP_BLI)

	scan (ibuf + 1, "%u", &bs);
	fastblink (bs);
	proceed (EP_RCMD);

  entry (EP_DIA)

	diag ("MSG %d (%x) %u: %s", dcnt, dcnt, dcnt, ibuf+1);
	dcnt++;
	proceed (EP_RCMD);

  entry (EP_FLR)

	scan (ibuf + 1, "%u", &w);
	if (w >= IFLASH_SIZE)
		proceed (EP_RCMD+1);
	diag ("IF [%u] = %x", w, IFLASH [w]);
	proceed (EP_RCMD);

  entry (EP_FLW)

	scan (ibuf + 1, "%u %u", &w, &bs);
	if (w >= IFLASH_SIZE)
		proceed (EP_RCMD+1);
	if (if_write (w, bs))
		diag ("FAILED");
	else
		diag ("OK");
	goto Done;

  entry (EP_FLE)

	b = -1;
	scan (ibuf + 1, "%d", &b);
	if_erase (b);
	goto Done;

  entry (EP_CLR)

	scan (ibuf + 1, "%u", &w);
	diag ("CF [%u] = %x, et = %x", w, *((address)w), (word)(&_etext));
	proceed (EP_RCMD);

  entry (EP_CLW)

	scan (ibuf + 1, "%u %u", &w, &bs);
	cf_write ((address)w, bs);
	diag ("OK");
	goto Done;

  entry (EP_CLE)

	b = 0;
	scan (ibuf + 1, "%d", &b);
	cf_erase ((address)b);
	goto Done;

  entry (EP_COT)

	w = 0;
	scan (ibuf + 1, "%u", &w);

	if (*((address)w) != 0xffff) {
		diag ("Word not erased: %x", *((address)w));
		proceed (EP_RCMD);
	}

	for (b = 1; b <= 16; b++) {
		nt = 0xffff << b;
		cf_write ((address)w, nt);
		sl = *((address)w);
		diag ("Written %x, read %x", nt, sl);
	}
	goto Done;

  entry (EP_ETS)

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

  entry (EP_ETS_O)

	if (u == 0) {
		ser_out (EP_ETS_O, "ERASING ALL FLASH\r\n");
	} else {
		ser_outf (EP_ETS_O, "ERASING from %lu (%lx) to %lu (%lx)\r\n",
			s, s, u - 4, u - 4);
	}

  entry (EP_ETS_E)

	//w = ee_erase (WNONE, s, u);
	w = ee_erase (EP_ETS_E, s, u);

  entry (EP_ETS_F)

	ser_outf (EP_ETS_F, "ERASE COMPLETE, %u ERRORS\r\n", w);

	adr = s;
	w = 0;

  entry (EP_ETS_G)

	if (pat == LWNONE)
		val = adr;
	else
		val = pat;

	//w += ee_write (WNONE, adr, (byte*)(&adr), 4);
	w += ee_write (EP_ETS_G, adr, (byte*)(&val), 4);

	if ((adr & 0xFFF) == 0)
		proceed (EP_ETS_K);

  entry (EP_ETS_M)

	adr += 4;
	if (adr < u)
		proceed (EP_ETS_G);

  entry (EP_ETS_H)

	//w += ee_sync (WNONE);
	w += ee_sync (EP_ETS_H);
	ser_outf (EP_ETS_H, "WRITE COMPLETE, %u ERRORS\r\n", w);

	// Start reading
	adr = s;
	w = 0;
	err = 0;

  entry (EP_ETS_I)

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
		proceed (EP_ETS_L);

  entry (EP_ETS_N)

	adr += 4;
	if (adr < u)
		proceed (EP_ETS_I);

  entry (EP_ETS_J)

	ser_outf (EP_ETS_J, "READ COMPLETE, %u ERRORS, %u MISREADS\r\n",
		w, err);

	proceed (EP_RCMD);

  entry (EP_ETS_K)

	ser_outf (EP_ETS_K, "WRITTEN %lu (%lx)\r\n", adr, adr);
	proceed (EP_ETS_M);

  entry (EP_ETS_L)

	ser_outf (EP_ETS_L, "READ %lu (%lx)\r\n", adr, adr);
	proceed (EP_ETS_N);

endthread

// ============================================================================

#ifdef	SDCARD_PRESENT

#define	SD_INIT		0
#define	SD_OK		1
#define	SD_IFAIL	2
#define	SD_START	5
#define	SD_RCMD		10
#define	SD_SWO		20
#define	SD_SLW		30
#define	SD_SST		40
#define	SD_RWO		50
#define	SD_RLW		60
#define	SD_RST		70
#define	SD_WRI		80
#define	SD_REA		90
#define	SD_ERA		100
#define	SD_SYN		110
#define	SD_ETS		200
#define	SD_ETS_G	240
#define	SD_ETS_M	250
#define	SD_ETS_H	260
#define	SD_ETS_I	270
#define	SD_ETS_N	280
#define	SD_ETS_J	290
#define	SD_ETS_K	300
#define	SD_ETS_L	310
#define	SD_IDL		320

thread (test_sdcard)

  entry (SD_INIT)

	if ((err = sd_open ()) == 0) 
		proceed (SD_OK);

  entry (SD_IFAIL)

	ser_outf (SD_IFAIL, "Failed to open SD card: %u\r\n", err);
	finish;

  entry (SD_OK)

	ser_outf (SD_OK, "SD card size: %lu\r\n", sd_size ());

  entry (SD_START)

	ser_out (SD_START,
		"\r\nSD Test\r\n"
		"Commands:\r\n"
		"a adr int    -> store a word\r\n"
		"b adr lint   -> store a lword\r\n"
		"c adr str    -> store a string\r\n"
		"d adr        -> read word\r\n"
		"e adr        -> read lword\r\n"
		"f adr n      -> read string\r\n"
		"g adr n p    -> write n longwords with p starting at adr\r\n"
		"h adr n b t  -> read n blks of b starting at adr t times\r\n"
		"x frm upt    -> erase from upto\r\n"
		"s            -> sync card\r\n"
		"w fr ln pat  -> write-read test\r\n"
		"i            -> set idle state\r\n"
		"q            -> return to main test\r\n"
	);

  entry (SD_RCMD)

	err = 0;
	ser_in (SD_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'a': proceed (SD_SWO);
		case 'b': proceed (SD_SLW);
		case 'c': proceed (SD_SST);
		case 'd': proceed (SD_RWO);
		case 'e': proceed (SD_RLW);
		case 'f': proceed (SD_RST);
		case 'g': proceed (SD_WRI);
		case 'h': proceed (SD_REA);
		case 'x': proceed (SD_ERA);
		case 's': proceed (SD_SYN);
		case 'w': proceed (SD_ETS);
		case 'i': proceed (SD_IDL);
		case 'q': { sd_close (); finish; };
	}
	
  entry (SD_RCMD+1)

	ser_out (SD_RCMD+1, "Illegal command or parameter\r\n");
	proceed (SD_START);

  entry (SD_SWO)

	scan (ibuf + 1, "%lu %u", &adr, &w);
	err = sd_write (adr, (byte*)(&w), 2);

  entry (SD_SWO+1)

	ser_outf (SD_SWO+1, "[%d] Stored %u at %lu\r\n", err, w, adr);
	proceed (SD_RCMD);

  entry (SD_SLW)

	scan (ibuf + 1, "%lu %lu", &adr, &val);
	err = sd_write (adr, (byte*)(&val), 4);

  entry (SD_SLW+1)

	ser_outf (SD_SLW+1, "[%d] Stored %lu at %lu\r\n", err, val, adr);
	proceed (SD_RCMD);

  entry (SD_SST)

	scan (ibuf + 1, "%lu %s", &adr, str);
	len = strlen (str);
	if (len == 0)
		proceed (SD_RCMD+1);

	err = sd_write (adr, str, len);

  entry (SD_SST+1)

	ser_outf (SD_SST+1, "[%d] Stored %s (%u) at %lu\r\n", err, str, len,
		adr);
	proceed (SD_RCMD);

  entry (SD_RWO)

	scan (ibuf + 1, "%lu", &adr);
	err = sd_read (adr, (byte*)(&w), 2);

  entry (SD_RWO+1)

	ser_outf (SD_RWO+1, "[%d] Read %u (%x) from %lu\r\n", err, w, w, adr);
	proceed (SD_RCMD);

  entry (SD_RLW)

	scan (ibuf + 1, "%lu", &adr);
	err = sd_read (adr, (byte*)(&val), 4);

  entry (SD_RLW+1)

	ser_outf (SD_RLW+1, "[%d] Read %lu (%lx) from %lu\r\n",
		err, val, val, adr);
	proceed (SD_RCMD);

  entry (SD_RST)

	scan (ibuf + 1, "%lu %u", &adr, &len);
	if (len == 0)
		proceed (SD_RCMD+1);

	str [0] = '\0';
	err = sd_read (adr, str, len);
	str [len] = '\0';

  entry (SD_RST+1)

	ser_outf (SD_RST+1, "[%d] Read %s (%u) from %lu\r\n",
		err, str, len, adr);
	proceed (SD_RCMD);

  entry (SD_WRI)

	len = 0;
	scan (ibuf + 1, "%lu %u %lu", &adr, &len, &val);
	if (len == 0)
		proceed (SD_RCMD+1);
	while (len--) {
		err += (sd_write (adr, (byte*)(&val), 4) != 0);
		adr += 4;
	}

  entry(SD_WRI+1)

Done:
	ser_outf (SD_WRI+1, "Done %d\r\n", err);
	proceed (SD_RCMD);

  entry (SD_REA)

	len = 0;
	bs = 0;
	nt = 0;
	scan (ibuf + 1, "%lu %u %u %u", &adr, &len, &bs, &nt);
	if (len == 0)
		proceed (SD_RCMD+1);
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

  entry (SD_ERA)

	s = u = 0;
	scan (ibuf + 1, "%lu %lu", &s, &u);
	err = sd_erase (s, u);
	goto Done;

  entry (SD_SYN)

	err = sd_sync ();
	goto Done;

  entry (SD_ETS)

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

  entry (SD_ETS_G)

	if (pat == LWNONE)
		val = adr;
	else
		val = pat;

	w += (sd_write (adr, (byte*)(&val), 4) != 0);

	if ((adr & 0xFFF) == 0)
		proceed (SD_ETS_K);

  entry (SD_ETS_M)

	adr += 4;
	if (adr < u)
		proceed (SD_ETS_G);

  entry (SD_ETS_H)

	w += (sd_sync () != 0);
	ser_outf (SD_ETS_H, "WRITE COMPLETE, %u ERRORS\r\n", w);

	// Start reading
	adr = s;
	w = 0;
	err = 0;

  entry (SD_ETS_I)

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
		proceed (SD_ETS_L);

  entry (SD_ETS_N)

	adr += 4;
	if (adr < u)
		proceed (SD_ETS_I);

  entry (SD_ETS_J)

	ser_outf (SD_ETS_J, "READ COMPLETE, %u ERRORS, %u MISREADS\r\n",
		w, err);

	proceed (SD_RCMD);

  entry (SD_ETS_K)

	ser_outf (SD_ETS_K, "WRITTEN %lu (%lx)\r\n", adr, adr);
	proceed (SD_ETS_M);

  entry (SD_ETS_L)

	ser_outf (SD_ETS_L, "READ %lu (%lx)\r\n", adr, adr);
	proceed (SD_ETS_N);

  entry (SD_IDL)

	sd_idle ();
	goto Done;

endthread

#endif

// ============================================================================

#define	DE_INIT		0
#define	DE_RCMD		10
#define	DE_FRE		20
#define	DE_LHO		30
#define DE_PDM		40
#define	DE_PUM		50
#define	DE_SPN		60

thread (test_delay)

  entry (DE_INIT)

	ser_out (DE_INIT,
		"\r\nRF Pin Test\r\n"
		"Commands:\r\n"
		"f s      -> freeze\r\n"
		"l s      -> lhold\r\n"
		"d        -> PD mode (unsafe)\r\n"
		"u        -> PU mode\r\n"
		"s n      -> spin test for n sec\r\n"
		"q        -> return to main test\r\n"
	);

  entry (DE_RCMD)

	ser_in (DE_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'f': proceed (DE_FRE);
		case 'l': proceed (DE_LHO);
		case 'd': proceed (DE_PDM);
		case 'u': proceed (DE_PUM);
		case 's': proceed (DE_SPN);
		case 'q': { finish; }
	}
	
  entry (DE_RCMD+1)

	ser_out (DE_RCMD+1, "Illegal command or parameter\r\n");
	proceed (DE_INIT);

  entry (DE_FRE)

	nt = 0;
	scan (ibuf + 1, "%u", &nt);
	diag ("Start");
	freeze (nt);
	diag ("Stop");
	proceed (DE_RCMD);

  entry (DE_LHO)

	nt = 0;
	scan (ibuf + 1, "%u", &nt);
	val = (lword) nt;
	diag ("Start %u", (word) seconds ());

  entry (DE_LHO+1)

	lhold (DE_LHO+1, &val);
	diag ("Stop %u", (word) seconds ());
	proceed (DE_RCMD);

  entry (DE_PDM)

	diag ("Entering PD mode");
	powerdown ();
	proceed (DE_RCMD);

  entry (DE_PUM)

	diag ("Entering PU mode");
	powerup ();
	proceed (DE_RCMD);

  entry (DE_SPN)

	nt = 0;
	scan (ibuf + 1, "%u", &nt);

	// Wait for the nearest round second
	s = seconds ();

  entry (DE_SPN+1)

	if ((u = seconds ()) == s)
		proceed (DE_SPN+1);

	u += nt;
	s = 0;

  entry (DE_SPN+2)

	if (seconds () != u) {
		s++;
		proceed (DE_SPN+2);
	}

  entry (DE_SPN+3)

	ser_outf (DE_SPN+2, "Done: %lu cycles\r\n", s);
	proceed (DE_RCMD);

endthread

// ============================================================================

#define	UA_LOOP		00
#define	UA_BACK		10

thread (test_uart)

    entry (UA_LOOP)

	ser_in (UA_LOOP, ibuf, IBUFLEN-1);

    entry (UA_BACK)

	ser_outf (UA_BACK, "%s\r\n", ibuf);

	if (ibuf [0] == 'q' && ibuf [1] == '\0')
		// Done
		finish;
	proceed (UA_LOOP);

endthread

// ============================================================================

#ifdef gps_bring_up

#define	MI_READ		0
#define	MI_WRITE	1

thread (minput)

  static char c;

  entry (MI_READ)

	io (MI_READ, UART_B, READ, &c, 1);

  entry (MI_WRITE)

	io (MI_WRITE, UART_A, WRITE, &c, 1);
	proceed (MI_READ);

endthread

#define	GP_INI	0
#define	GP_MEN	1
#define	GP_RCM	2
#define	GP_WRI	3
#define	GP_ENA	5
#define	GP_DIS	6
#define	GP_WRL	9
#define	GP_ERR	10

thread (test_gps) 

  entry (GP_INI)

  entry (GP_MEN)

	ser_out (GP_MEN,
		"\r\nGPS Test\r\n"
		"Commands:\r\n"
		"w string    -> write line to module\r\n"
		"e           -> enable\r\n"
		"d           -> disable\r\n"
		"q           -> quit\r\n"
	);

  entry (GP_RCM)

	ser_in (GP_RCM, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

	    case 'w': proceed (GP_WRI);
	    case 'e': proceed (GP_ENA);
	    case 'd': proceed (GP_DIS);
	    case 'q': {
			gps_bring_down;
			killall (minput);
			finish;
	    }

	}

  entry (GP_ERR)

	ser_out (GP_ERR, "Illegal command or parameter\r\n");
	proceed (GP_MEN);

  entry (GP_WRI)

	for (b = 1; ibuf [b] == ' '; b++);
	off = strlen (ibuf + b);

	ibuf [b + off    ] = '\r';
	ibuf [b + off + 1] = '\n';
	off += 2;
	ibuf [b + off    ] = '\0';

  entry (GP_WRL)

	while (off) {
		rcvl = io (GP_WRL, UART_B, WRITE, ibuf + b, off);
		off -= rcvl;
		b += rcvl;
	}

	proceed (GP_RCM);

  entry (GP_ENA)

	gps_bring_up;
	killall (minput);
	runthread (minput);
	proceed (GP_RCM);

  entry (GP_DIS)

	gps_bring_down;
	killall (minput);
	proceed (GP_RCM);

endthread

#endif /* gps_bring_up */

// ============================================================================

#ifdef RTC_PRESENT

#define	RT_MEN	0
#define	RT_RCM	10
#define	RT_ERR	20
#define	RT_SET	30
#define	RT_GET	40

thread (test_rtc) 

  entry (RT_MEN)

	ser_out (RT_MEN,
		"\r\nRTC Test\r\n"
		"Commands:\r\n"
		"s y m d dw h m s -> set the clock\r\n"
		"r                -> read the clock\r\n"
		"q                -> quit\r\n"
	);

  entry (RT_RCM)

	ser_in (RT_RCM, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

	    case 's': proceed (RT_SET);
	    case 'r': proceed (RT_GET);
	    case 'q': {
			finish;
	    }

	}

  entry (RT_ERR)

	ser_out (RT_ERR, "Illegal command or parameter\r\n");
	proceed (RT_MEN);

  entry (RT_SET)

	sl = WNONE;
	scan (ibuf + 1, "%u %u %u %u %u %u %u",
		&rssi, &err, &w, &len, &bs, &nt, &sl);

	if (sl == WNONE)
		proceed (RT_ERR);

	dtime.year = rssi;
	dtime.month = err;
	dtime.day = w;
	dtime.dow = len;
	dtime.hour = bs;
	dtime.minute = nt;
	dtime.second = sl;

	err = rtc_set (&dtime);

  entry (RT_SET+1)

	ser_outf (RT_SET+1, "Status = %u\r\n", err);
	proceed (RT_RCM);

  entry (RT_GET)

	bzero (&dtime, sizeof (dtime));
	err = rtc_get (&dtime);

  entry (RT_GET+1)

	ser_outf (RT_GET+1, "Status = %u // %u %u %u %u %u %u %u\r\n", err,
				dtime.year,
				dtime.month,
				dtime.day,
				dtime.dow,
				dtime.hour,
				dtime.minute,
				dtime.second);
	proceed (RT_RCM);

endthread

#endif /* RTC_PRESENT */

// ============================================================================

#if LCD_ST7036

#define	LT_MEN	0
#define	LT_RCM	10
#define	LT_ERR	20
#define	LT_ON	30
#define	LT_OFF	40
#define	LT_DIS	50
#define	LT_CLE	60

thread (test_lcd) 

  char *t;

  entry (LT_MEN)

	ser_out (LT_MEN,
		"\r\nLCD Test\r\n"
		"Commands:\r\n"
		"o n   -> on (1 = cursor shown)\r\n"
		"f     -> off\r\n"
		"d n t -> display text at pos n\r\n"
		"c     -> clear\r\n"
		"q     -> quit\r\n"
	);

  entry (LT_RCM)

	ser_in (LT_RCM, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

	    case 'o': proceed (LT_ON);
	    case 'f': proceed (LT_OFF);
	    case 'd': proceed (LT_DIS);
	    case 'c': proceed (LT_CLE);
	    case 'q': {
			lcd_off ();
			finish;
	    }

	}

  entry (LT_ERR)

	ser_out (LT_ERR, "Illegal command or parameter\r\n");
	proceed (LT_MEN);

  entry (LT_ON)

	sl = 0;
	scan (ibuf + 1, "%u", &sl);

	if (sl)
		sl = 1;

	lcd_on (sl);
	proceed (LT_RCM);

  entry (LT_OFF)

	lcd_off ();
	proceed (LT_RCM);

  entry (LT_DIS)

	sl = 0;
	scan (ibuf + 1, "%u", &sl);

	if (sl > 31)
		proceed (LT_ERR);

	for (t = ibuf + 1; *t != '\0'; t++)
		if (*t != ' ' && *t != '\t' && !isdigit (*t))
			break;

	if (*t == '\0')
		proceed (LT_ERR);

	lcd_write (sl, t);
	proceed (LT_RCM);

  entry (LT_CLE)

	lcd_clear (0, 0);
	proceed (LT_RCM);

endthread

#endif /* RTC_PRESENT */

// ============================================================================

#ifdef	SENSOR_LIST

#define	SE_INIT		00
#define	SE_RCMD		10
#define	SE_GSEN		20
#define	SE_CSEN		30

thread (test_sensors)

  entry (SE_INIT)

	ser_out (SE_INIT,
		"\r\nSensor Test\r\n"
		"Commands:\r\n"
		"r s      -> read sensor s\r\n"
		"c s d n  -> read sensor s continually at d ms, n times\r\n"
		"q        -> quit\r\n"
		);

  entry (SE_RCMD)

	ser_in (SE_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'r' : proceed (SE_GSEN);
		case 'c' : proceed (SE_CSEN);
	    	case 'q': { finish; };
	}

  entry (SE_RCMD+1)

	ser_out (SE_RCMD+1, "Illegal command or parameter\r\n");
	proceed (SE_INIT);

  entry (SE_GSEN)

	w = 0;
	scan (ibuf + 1, "%u", &w);

  entry (SE_GSEN+1)

	read_sensor (SE_GSEN+1, w, &ss);

  entry (SE_GSEN+2)

	ser_outf (SE_GSEN+2, "Value: %u\r\n", ss);
	proceed (SE_RCMD);

  entry (SE_CSEN)

	w = 0;
	bs = 0;
	nt = 0;

	scan (ibuf + 1, "%u %u %u", &w, &bs, &nt);
	if (nt == 0)
		nt = 1;

  entry (SE_CSEN+1)

	read_sensor (SE_CSEN+1, w, &ss);
	nt--;

  entry (SE_CSEN+2)

	ser_outf (SE_GSEN+2, "Value: %u (%u left)\r\n", ss, nt);

	if (nt == 0)
		proceed (SE_RCMD);
	
	delay (bs, SE_CSEN+1);
	release;

endthread

#endif

// ============================================================================

#define	AD_INIT		00
#define	AD_RCMD		01
#define	AD_CONF		03
#define	AD_OK		04
#define AD_STOP		05

thread (test_adc)

  entry (AD_INIT)

	ser_out (AD_INIT,
		"\r\nADC Test\r\n"
		"Commands:\r\n"
		"c pin ref sht -> configure\r\n"
		"s             -> start\r\n"
		"h             -> stop & read\r\n"
		"f             -> off\r\n"
		"d             -> disable\r\n"
		"q             -> quit\r\n"
		);

  entry (AD_RCMD)

	ser_in (AD_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'c' : proceed (AD_CONF);
		case 's' : { adc_start;    proceed (AD_OK); }
		case 'f' : { adc_off;      proceed (AD_OK); }
		case 'd' : { adc_disable;  proceed (AD_OK); }
		case 'h' : proceed (AD_STOP);
	    	case 'q' : { adc_disable;  finish; };
	}

  entry (AD_RCMD+1)

	ser_out (AD_RCMD+1, "Illegal command or parameter\r\n");
	proceed (AD_INIT);

  entry (AD_CONF)

	nt = 0;
	sl = 0;
	ss = 0;

	scan (ibuf + 1, "%u %u %u", &nt, &sl, &ss);

	if (nt > 7 || sl > 3 || ss > 15)
		proceed (AD_RCMD+1);

	adc_config_read (nt, sl, ss);

  entry (AD_OK)

	ser_out (AD_OK, "OK\r\n");
	proceed (AD_RCMD);

  entry (AD_STOP)

	adc_stop;
	if (adc_busy)
		proceed (AD_STOP+1);
Value:
	nt = adc_value;

  entry (AD_STOP+3)

	ser_outf (AD_STOP+3, "Value = %u [%x]\r\n", nt, nt);
	proceed (AD_RCMD);

  entry (AD_STOP+1)

	ser_out (AD_STOP+1, "Waiting for ADC to become idle ...\r\n");
	while (adc_busy);

  entry (AD_STOP+2)

	ser_out (AD_STOP+2, "Idle\r\n");
	goto Value;

endthread

// ============================================================================

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_EPR		20
#define RS_SPO		30
#define	RS_RAD		40
#define	RS_QRA		50
#define	RS_SID		60
#define	RS_SCH		70
#define	RS_UAR		80
#define	RS_CON		90
#define RS_COF		100
#define	RS_AUTOSTART	200

thread (root)

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0;

#ifdef RTC_PRESENT

	// Check if the clock is running; if not, set it to anything as
	// otherwise it drains current

	if (rtc_get (&dtime) == 0) {
		if (dtime.year == 0) {
			dtime.year = 9;
			rtc_set (&dtime);
		}
	}
#endif

#if CC1100
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

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nWarsaw board test (GENERAL)\r\n"
		"Commands:\r\n"
		"e    -> EEPROM test\r\n"
		"r    -> start radio test (xmit/receive)\r\n"
		"p v  -> set xmit power [def = max]\r\n"
		"c v  -> set channel [def = 0]\r\n"
		"q    -> stop radio test\r\n"
		"n    -> reset\r\n"
		"i v  -> set SID [def = 0]\r\n"
		"u v  -> set uart rate [def = 96]\r\n"
#ifdef cswitch_on
		"o c  -> cswitch on\r\n"
		"f c  -> cswitch off\r\n"
#endif
		"E    -> detailed EEPROM test\r\n"
#ifdef SDCARD_PRESENT
		"S    -> detailed SD test\r\n"
#endif
		"P    -> detailed pin test (including ADC)\r\n"
		"D    -> delay/freeze/spin test\r\n"
#ifdef	gps_bring_up
		"G    -> GPS test\r\n"
#endif
#ifdef SENSOR_LIST
		"V    -> sensors\r\n"
#endif
		"A    -> ADC\r\n"
#ifdef RTC_PRESENT
		"T    -> RTC test\r\n"
#endif
#if LCD_ST7036
		"L    -> LCD test\r\n"
#endif
		"U    -> UART echo test\r\n"
	);

  entry (RS_RCMD-1)

	if ((unsigned char) ibuf [0] == 0xff)
		ser_out (RS_RCMD-1,
			"No command in 30 seconds -> start radio test\r\n"
			);
  entry (RS_RCMD)

	if ((unsigned char) ibuf [0] == 0xff)
		delay (1024*30, RS_AUTOSTART);
  
	ser_in (RS_RCMD, ibuf, IBUFLEN-1);
	unwait (WNONE);

	switch (ibuf [0]) {
		case 'e' : proceed (RS_EPR);
		case 'r' : proceed (RS_RAD);
		case 'p' : proceed (RS_SPO);
		case 'c' : proceed (RS_SCH);
		case 'q' : proceed (RS_QRA);
		case 'u' : proceed (RS_UAR);
#ifdef cswitch_on
		case 'o' : proceed (RS_CON);
		case 'f' : proceed (RS_COF);
#endif
		case 'n' : reset ();
		case 'E' : {
				runthread (test_epr);
				joinall (test_epr, RS_RCMD-2);
				release;
		}
#ifdef SDCARD_PRESENT
		case 'S' : {
				runthread (test_sdcard);
				joinall (test_sdcard, RS_RCMD-2);
				release;
		}
#endif
		case 'P' : {
				runthread (test_pin);
				joinall (test_pin, RS_RCMD-2);
				release;
		}
		case 'D' : {
				runthread (test_delay);
				joinall (test_delay, RS_RCMD-2);
				release;
		}
		case 'U' : {
				runthread (test_uart);
				joinall (test_uart, RS_RCMD-2);
				release;
		}
#ifdef gps_bring_up
		case 'G' : {
				runthread (test_gps);
				joinall (test_gps, RS_RCMD-2);
				release;
		}
#endif

#ifdef SENSOR_LIST
		case 'V' : {
				runthread (test_sensors);
				joinall (test_sensors, RS_RCMD-2);
				release;
		}
#endif
		case 'A' : {
				runthread (test_adc);
				joinall (test_adc, RS_RCMD-2);
				release;
		}

#ifdef RTC_PRESENT
		case 'T' : {
				runthread (test_rtc);
				joinall (test_rtc, RS_RCMD-2);
				release;
		}
#endif
#if LCD_ST7036
		case 'L' : {
				runthread (test_lcd);
				joinall (test_lcd, RS_RCMD-2);
				release;
		}
#endif
	}

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-2);

  entry (RS_EPR)

	// EEPROM test
	err = ee_open ();
	ser_outf (RS_EPR, "Open status: %s, erasing ...\r\n", err ? "ERROR" :
		"OK");

  entry (RS_EPR+1)

	err = ee_erase (RS_EPR+1, 0, 0);

  entry (RS_EPR+2)

	ser_outf (RS_EPR+2, "Done (%s)\r\n", err ? "ERROR" : "OK");

  entry (RS_EPR+3)

	ser_outf (RS_EPR+3, "Writing ...\r\n");
	adr = 0;
	max = ee_size (NO, NULL);
	err = 0;
	off = rnd ();

  entry (RS_EPR+4)

E_more:
	val = adr + off;
	err += ee_write (RS_EPR+4, adr, (byte*)(&val), 4);

	if ((adr += EEPROM_INCR) < max)
		goto E_more;

  entry (RS_EPR+5)

	ser_outf (RS_EPR+5, "Done (%s), now reading ...\r\n",
		err ? "ERROR" : "OK");
	adr = 0;
	err = 0;

	while (adr < max) {
		val = (lword)(-1);
		ee_read (adr, (byte*)(&val), 4);
		if (val != (adr + off))
			err++;
		adr += EEPROM_INCR;
	}

  entry (RS_EPR+6)

	ser_outf (RS_EPR+6, "Done (%s)\r\n", err ? "ERROR" : "OK");
	ee_close ();
	proceed (RS_RCMD);

  entry (RS_SPO)

	// Setpower, default = max
	off = 255;
	scan (ibuf + 1, "%d", &off);
	tcv_control (sfd, PHYSOPT_SETPOWER, (address)&off);
	proceed (RS_RCMD);

  entry (RS_SCH)

	// Setpower, default = max
	off = 0;
	scan (ibuf + 1, "%d", &off);
	tcv_control (sfd, PHYSOPT_SETCHANNEL, (address)&off);
	proceed (RS_RCMD);

  entry (RS_RAD)

	radio_start ();
	proceed (RS_RCMD);

  entry (RS_QRA)

	radio_stop ();
	proceed (RS_RCMD);

  entry (RS_SID)

	off = 0;
	scan (ibuf + 1, "%d", &off);
	tcv_control (sfd, PHYSOPT_SETSID, (address)&off);
	proceed (RS_RCMD);

  entry (RS_UAR)

	off = 0;
	scan (ibuf + 1, "%d", &off);
	ion (UART, CONTROL, (char*) &off, UART_CNTRL_SETRATE);
	proceed (RS_RCMD);

#ifdef cswitch_on

  entry (RS_CON)

	w = 0;
	scan (ibuf + 1, "%u", &w);
	if (w == 0)
		proceed (RS_RCMD+1);
	cswitch_on (w);
	proceed (RS_RCMD);

  entry (RS_COF)

	w = 0;
	scan (ibuf + 1, "%u", &w);
	if (w == 0)
		proceed (RS_RCMD+1);
	cswitch_off (w);
	proceed (RS_RCMD);
#endif

  entry (RS_AUTOSTART)

	ibuf [0] = 0;
	radio_start ();
	proceed (RS_RCMD);

endthread

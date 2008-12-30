/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	42

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "pinopts.h"
#include "lhold.h"

#define	IBUFLEN		132
#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

#define	EEPROM_INCR	255
#define	SEND_INTERVAL	1024

static int 	sfd, off, packet_length, rcvl, b;
static word	rssi, err, w, len, bs, nt, sl, ss, dcnt;
static lword	adr, max, val, last_snt, last_rcv, s, u, pat;
static byte	str [129], *blk;
static char	*ibuf;
static address	packet;

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

	ser_outf (SN_MESS, "Rcv: %lu [%d], RSSI = %d, QUA = %d\r\n",
		last_rcv, rcvl, (rssi >> 8) & 0x00ff, rssi & 0x00ff);
	proceed (RC_TRY);

endthread;

void radio_start () {

	tcv_control (sfd, PHYSOPT_RXON, NULL);
	tcv_control (sfd, PHYSOPT_TXON, NULL);

	if (!running (sender))
		runthread (sender);

	if (!running (receiver))
		runthread (receiver);
}

void radio_stop () {

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

thread (test_pin)

  entry (PI_INIT)

	ser_out (PI_INIT,
		"\r\nRF Pin Test\r\n"
		"Commands:\r\n"
		"r p r    -> read ADC pin 'p' with reference r:\r\n"
		"                 0-1.5V, 1-2.5V, 2-Vcc, 3-Eref\r\n"
		"s p v    -> set pin 'p' to digital v (0/1)\r\n"
		"v p      -> show the value of pin 'p'\r\n"
		"q        -> return to main test\r\n"
	);

  entry (PI_RCMD)

	ser_in (PI_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'r': proceed (PI_ADC);
		case 's': proceed (PI_SET);
		case 'v': proceed (PI_VIE);
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

endthread

// ============================================================================

#define	EP_INIT		0
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
		case 'q': { finish; };
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

	scan (ibuf + 1, "%u", &adr);
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

	scan (ibuf + 1, "%u", &adr);
	if (adr >= IFLASH_SIZE)
		proceed (EP_RCMD+1);
	diag ("IF [%u] = %x", adr, if_read (adr));
	proceed (EP_RCMD);

  entry (EP_FLW)

	scan (ibuf + 1, "%u %u", &adr, &bs);
	if (adr >= IFLASH_SIZE)
		proceed (EP_RCMD+1);
	if (if_write (adr, bs))
		diag ("FAILED");
	else
		diag ("OK");
	goto Done;

  entry (EP_FLE)

	b = -1;
	scan (ibuf + 1, "%d", &b);
	if_erase (b);
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
			diag ("MISREAD (PATTERN): %x %x",
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

thread (test_sdram)

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
		"s            -> sync sdram\r\n"
		"w fr ln pat  -> write-read test\r\n"
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
		case 's': proceed (SD_SYN);
		case 'w': proceed (SD_ETS);
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

	scan (ibuf + 1, "%u", &adr);
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
			diag ("MISREAD (PATTERN): %x %x",
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

endthread

// ============================================================================

#define	DE_INIT		0
#define	DE_RCMD		10
#define	DE_FRE		20
#define	DE_LHO		30

thread (test_delay)

  entry (DE_INIT)

	ser_out (DE_INIT,
		"\r\nRF Pin Test\r\n"
		"Commands:\r\n"
		"f s      -> freeze\r\n"
		"l s      -> lhold\r\n"
		"q        -> return to main test\r\n"
	);

  entry (DE_RCMD)

	ser_in (DE_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'f': proceed (DE_FRE);
		case 'l': proceed (DE_LHO);
		case 'q': { finish; };
	}
	
  entry (DE_RCMD+1)

	ser_out (DE_RCMD+1, "Illegal command or parameter\r\n");
	proceed (DE_INIT);

  entry (DE_FRE)

	nt = 0;
	scan (ibuf + 1, "%u", &nt);
	diag ("Start %u", (word) seconds ());
	freeze (nt);
	diag ("Stop %u", (word) seconds ());
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

endthread

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_EPR		20
#define RS_SPO		30
#define	RS_RAD		40
#define	RS_QRA		50
#define	RS_SID		60
#define	RS_SCH		70
#define	RS_UAR		80
#define	RS_FRE		90
#define	RS_AUTOSTART	200

thread (root)

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0;

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

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nWarsaw board test (GENERAL)\r\n"
		"Commands:\r\n"
		"e        -> EEPROM test\r\n"
		"r        -> start radio test (xmit/receive)\r\n"
		"p v      -> set xmit power [def = max]\r\n"
		"c v      -> set channel [def = 0]\r\n"
		"q        -> stop radio test\r\n"
		"n        -> reset\r\n"
		"i v      -> set SID [def = 0]\r\n"
		"u v      -> set uart rate [def = 96]\r\n"
		"s v      -> sleep (low power) for v seconds\r\n"
		"E        -> detailed EEPROM test\r\n"
		"S        -> detailed SD test\r\n"
		"P        -> detailed pin test (including ADC)\r\n"
		"D        -> delay test\r\n"
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
		case 's' : proceed (RS_FRE);
		case 'n' : reset ();
		case 'E' : {
				runthread (test_epr);
				joinall (test_epr, RS_RCMD-2);
				release;
		}
		case 'S' : {
				runthread (test_sdram);
				joinall (test_sdram, RS_RCMD-2);
				release;
		}
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
	}

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-2);

  entry (RS_EPR)

	// EEPROM test
	ser_out (RS_EPR, "Erasing ...\r\n");

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

  entry (RS_FRE)

	off = 1;
	scan (ibuf + 1, "%d", &off);
	freeze (off);

  entry (RS_FRE+1)

	ser_out (RS_FRE+1, "Wake up\r\n");
	proceed (RS_RCMD);

  entry (RS_AUTOSTART)

	ibuf [0] = 0;
	radio_start ();
	proceed (RS_RCMD);

endthread

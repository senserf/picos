/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// Stress test for SD cards
//

#include "sysio.h"

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"
#include "storage.h"

#define	IBUFLEN	82

static lword	NR, CN, GWErr, GRErr, WErr, RErr, Ptr, Cst,
			NRounds, SDSize, From, Upto;
static word	IX, CP, Flag, Err, SErr, Pat, BSize, CHSize, Intv;
static byte	b, Blk [128];
static char	*ibuf, *Msg;
		

#define	SD_INIT		0
#define	SD_ROUND	10
#define	SD_WRITE	20
#define	SD_EWRITE	30
#define	SD_READ		40
#define	SD_EREAD	50
#define	SD_EROUND	60
#define	SD_EXIT		70
#define	SD_WERR		80
#define	SD_RERR		90

static byte sbn () {

	return (byte) (Ptr + Pat);
}

thread (sd_test)

  entry (SD_INIT)

	ser_outf (SD_INIT,
		"starting from %lu * %u = %lu to %lu * %u = %lu, nr = %lu, "
		"cs = %u, in = %u, pa = %x\r\n",
		From, BSize, From * BSize, Upto, BSize, Upto * BSize, NRounds,
			CHSize, Intv, Pat);

	NR = 0;		// Round number

  entry (SD_ROUND)

	Ptr = Cst = From;
	CN = 0;
	CP = 0;
	GWErr = GRErr = WErr = RErr = 0;

  entry (SD_WRITE)

	if (Flag == 2)
		proceed (SD_EXIT);

	b = sbn (); 
	for (IX = 0; IX < BSize; IX++)
		Blk [IX] = b++;

//diag ("WRITING: %x %x", (word)((Ptr * BSize) >> 16), (word)(Ptr * BSize));
	SErr = sd_write (Ptr * BSize, Blk, BSize);
	if (SErr)
		goto WError;
KWrite:
	CP++;
	Ptr++;

	if (Ptr > Upto || CP == CHSize)
		delay (Intv, SD_EWRITE);
	else
		delay (Intv, SD_WRITE);
	release;

  entry (SD_EWRITE)

	ser_outf (SD_EWRITE, "end of write, round %lu, chunk %lu\r\n", NR, CN);
	CP = 0;
	Ptr = Cst;

  entry (SD_READ)

	if (Flag == 2)
		proceed (SD_EXIT);

	bzero (Blk, BSize);
//diag ("READING: %x %x", (word)((Ptr * BSize) >> 16), (word)(Ptr * BSize));
	SErr = sd_read (Ptr * BSize, Blk, BSize);
	if (SErr)
		goto RError;
	// Verify
	b = sbn ();
	for (IX = 0; IX < BSize; IX++) {
		if (Blk [IX] != b) {
			SErr = 0;
			goto RError;
		}
		b++;
	}
KRead:
	CP++;
	Ptr++;

	if (Ptr > Upto || CP == CHSize) 
		delay (Intv, SD_EREAD);
	else
		delay (Intv, SD_READ);
	release;

  entry (SD_EREAD)

	if (WErr == 0 && RErr == 0)
		ser_outf (SD_EREAD, "end of chunk %lu at %lu, OK\r\n",
			CN, Ptr * BSize);
	else
		ser_outf (SD_EREAD, "end of chunk %lu at %lu, "
			"!errors! (w/r) = %lu/%lu\r\n",
				CN, Ptr * BSize, WErr, RErr);
	GWErr += WErr;
	GRErr += RErr;

	WErr = RErr = 0;

	if (Ptr > Upto) {
		delay (Intv, SD_EROUND);
		release;
	}

	CN++;
	CP = 0;
	Cst = Ptr;

	delay (Intv, SD_WRITE);
	release;

  entry (SD_EROUND)

	if (GWErr == 0 && GRErr == 0)
		ser_outf (SD_EROUND, "end of round %lu, OK", NR);
	else
		ser_outf (SD_EROUND, "end of round %lu, "
			"!errors! (w/r) = %lu/%lu\r\n", NR, GWErr, GRErr);
	NR++;
	if (NR < NRounds)
		proceed (SD_ROUND);

  entry (SD_EXIT)

	ser_outf (SD_EXIT, "stopped\r\n");
	sd_close ();
	Flag = 0;
	finish;

  entry (SD_WERR)

WError:
	ser_outf (SD_WERR,
		"write error %u at %lu => %lu\r\n", SErr, Ptr, Ptr * BSize);

	WErr++;
	goto KWrite;

  entry (SD_RERR)

RError:
	if (SErr) {
		ser_outf (SD_RERR, "read error %u at %lu => %lu\r\n",
			SErr, Ptr, Ptr * BSize);
		goto KRead;
	}

	ser_outf (SD_RERR, "misread at %lu => %lu [%u] = %x != %x\r\n",
		Ptr, Ptr * BSize, IX, Blk [IX], b);
	goto KRead;
	
endthread

// ============================================================================

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_START	20
#define RS_STOP		30
#define	RS_ERR		40

thread (root)

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0;

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nCommands:\r\n"
		"s from to nr bsize chs intv spat -> start test\r\n"
		"q                                -> stop test\r\n"
	);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

		case 's' : proceed (RS_START);
		case 'q' : proceed (RS_STOP);
	}

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command\r\n");
	proceed (RS_RCMD-2);

  // ==========================================================================

  entry (RS_START)

	if (Flag) {
		Err = 0;
		Msg = "already running, stop first";
		proceed (RS_ERR);
	}

	if ((Err = sd_open ()) != 0) {
		Msg = "failed to open card";
		proceed (RS_ERR);
	}

	SDSize = sd_size ();

  entry (RS_START+1)

	ser_outf (RS_START+1, "Card size: %lu\r\n", SDSize);

	From = Upto = NRounds = 0;
	BSize = 0;
	CHSize = 0;
	Pat = 0;
	Intv = 0;
	scan (ibuf + 1, "%lu %lu %lu %u %u %u %u",
		&From, &Upto, &NRounds, &BSize, &CHSize, &Intv, &Pat);

	if (Upto == 0 || NRounds == 0)
		// Quietly exit
		proceed (RS_RCMD);

	if (Upto > SDSize - 1 || From > Upto || BSize > 128) {
		Err = 0;
		Msg = "illegal parameters";
		proceed (RS_ERR);
	}

	if (BSize == 0)
		BSize = 16;

	if (CHSize == 0)
		CHSize = 128;

	// Adjust the boundaries
	From = From / BSize;
	Upto = Upto / BSize;

	while ((From + 1) * BSize > SDSize)
		From--;
	while ((Upto + 1) * BSize > SDSize)
		Upto--;

	Flag = 1;
	runthread (sd_test);

  entry (RS_START+2)

	ser_out (RS_START+2, "Started\r\n");
	proceed (RS_RCMD);

  // ==========================================================================

  entry (RS_STOP)

	if (Flag == 0) {
		Err = 0;
		Msg = "already stopped";
		proceed (RS_ERR);
	}

	Flag = 2;
	proceed (RS_RCMD);

  // ==========================================================================

  entry (RS_ERR)

	ser_outf (RS_ERR, "Error %u: %s\r\n", Err, Msg);
	proceed (RS_RCMD);

endthread

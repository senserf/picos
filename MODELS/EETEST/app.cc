/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "node.h"

process NRoot (Node) {

	states { RS_INIT, RS_RCMD, RS_SWO, RS_SLW, RS_SST, RS_RWO, RS_RLW,
		 RS_RST,  RS_WRI,  RS_REA, 
		 RS_FLR,  RS_FLW,  RS_FLE, RS_SYS, RS_MAL,

		 RS_RCMDM2, RS_RCMDP1, RS_SWOP1, RS_SLWP1, RS_SSTP1, 
		 RS_RWOP1,  RS_RLWP1,  RS_RSTP1, RS_WRIP1, RS_FLRP1
	};

	perform;
};

#define	a	(((Node*)TheStation)->_na_a)
#define	w	(((Node*)TheStation)->_na_w)
#define	len	(((Node*)TheStation)->_na_len)
#define	bs	(((Node*)TheStation)->_na_bs)
#define	nt	(((Node*)TheStation)->_na_nt)
#define	sl	(((Node*)TheStation)->_na_sl)
#define	ss	(((Node*)TheStation)->_na_ss)
#define	dcnt	(((Node*)TheStation)->_na_dcnt)
#define	b	(((Node*)TheStation)->_na_b)
#define	lw	(((Node*)TheStation)->_na_lw)
#define	str	(((Node*)TheStation)->_na_str)
#define	blk	(((Node*)TheStation)->_na_blk)
#define	ibuf	(((Node*)TheStation)->_na_ibuf)

NRoot::perform {

  state RS_INIT:

  entry (RS_RCMDM2)

	S->ser_out (RS_RCMDM2,
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
		"m adr w      -> write word to info flash\r\n"
		"n adr        -> read word from info flash\r\n"
		"o adr        -> erase info flash\r\n"
	);

  entry (RS_RCMD)

	S->ser_in (RS_RCMD, ibuf, 132-1);

	switch (ibuf [0]) {

	    case 'a' : proceed (RS_SWO);
	    case 'b' : proceed (RS_SLW);
	    case 'c' : proceed (RS_SST);
	    case 'd' : proceed (RS_RWO);
	    case 'e' : proceed (RS_RLW);
	    case 'f' : proceed (RS_RST);
	    case 'g' : proceed (RS_WRI);
	    case 'h' : proceed (RS_REA);
	    case 'm' : proceed (RS_FLW);
	    case 'n' : proceed (RS_FLR);
	    case 'o' : proceed (RS_FLE);

	}

  entry (RS_RCMDP1)

	S->ser_out (RS_RCMDP1, "?????????\r\n");
	proceed (RS_RCMDM2);

  entry (RS_SWO)

	S->scan (ibuf + 1, "%u %u", &a, &w);
	S->ee_write (a, (byte*)(&w), 2);

  entry (RS_SWOP1)

	S->ser_outf (RS_SWOP1, "Stored %u at %u\r\n", w, a);
	proceed (RS_RCMD);

  entry (RS_SLW)

	S->scan (ibuf + 1, "%u %lu", &a, &lw);
	S->ee_write (a, (byte*)(&lw), 4);

  entry (RS_SLWP1)

	S->ser_outf (RS_SLWP1, "Stored %lu at %u\r\n", lw, a);
	proceed (RS_RCMD);

  entry (RS_SST)

	S->scan (ibuf + 1, "%u %s", &a, str);
	len = strlen ((const char*)str);
	if (len == 0)
		proceed (RS_RCMDP1);

	S->ee_write (a, str, len);

  entry (RS_SSTP1)

	S->ser_outf (RS_SSTP1, "Stored %s (%u) at %u\r\n", str, len, a);
	proceed (RS_RCMD);

  entry (RS_RWO)

	S->scan (ibuf + 1, "%u", &a);
	S->ee_read (a, (byte*)(&w), 2);

  entry (RS_RWOP1)

	S->ser_outf (RS_SSTP1, "Read %u (%x) from %u\r\n", w, w, a);
	proceed (RS_RCMD);

  entry (RS_RLW)

	S->scan (ibuf + 1, "%u", &a);
	S->ee_read (a, (byte*)(&lw), 4);

  entry (RS_RLWP1)

	S->ser_outf (RS_SSTP1, "Read %lu (%lx) from %u\r\n", lw, lw, a);
	proceed (RS_RCMD);

  entry (RS_RST)

	S->scan (ibuf + 1, "%u %u", &a, &len);
	if (len == 0)
		proceed (RS_RCMDP1);

	str [0] = '\0';
	S->ee_read (a, str, len);
	str [len] = '\0';

  entry (RS_RSTP1)

	S->ser_outf (RS_SSTP1, "Read %s (%u) from %u\r\n", str, len, a);
	proceed (RS_RCMD);

  entry (RS_WRI)

	len = 0;
	S->scan (ibuf + 1, "%u %u %lu", &a, &len, &lw);
	if (len == 0)
		proceed (RS_RCMDP1);
	while (len--) {
		S->ee_write (a, (byte*)(&lw), 4);
		a += 4;
	}

  entry(RS_WRIP1)

Done:
	S->ser_out (RS_WRIP1, "Done\r\n");
	proceed (RS_RCMD);

  entry (RS_REA)

	len = 0;
	bs = 0;
	nt = 0;
	S->scan (ibuf + 1, "%u %u %u %u", &a, &len, &bs, &nt);
	if (len == 0)
		proceed (RS_RCMDP1);
	if (bs == 0)
		bs = 4;

	if (nt == 0)
		nt = 1;

	blk = (byte*) (umalloc (bs));

	while (nt--) {

		sl = len;
		ss = a;
		while (sl--) {
			S->ee_read (ss, blk, bs);
			ss += bs;
		}

	}

	goto Done;

  entry (RS_FLR)

	S->scan (ibuf + 1, "%u", &a);
	if (a >= 128)
		proceed (RS_RCMDP1);

  entry (RS_FLRP1)

	S->ser_outf (RS_FLRP1, "IF [%u] = %x\r\n", a, S->if_read (a));
	proceed (RS_RCMD);

  entry (RS_FLW)

	S->scan (ibuf + 1, "%u %u", &a, &bs);
	if (a >= 128)
		proceed (RS_RCMDP1);
	S->if_write (a, bs);
	goto Done;

  entry (RS_FLE)

	b = -1;
	S->scan (ibuf + 1, "%d", &b);
	S->if_erase (b);
	goto Done;
}

void Node::appStart () {

	create NRoot;
}

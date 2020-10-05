/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"

heapmem {40, 60};

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	MAX_N_CHUNKS	8

address chunks [MAX_N_CHUNKS];
word	chsize [MAX_N_CHUNKS];

#if	MALLOC_SINGLEPOOL == 0
byte	pools [MAX_N_CHUNKS];
#endif

char	ibuf [48];

fsm root (int*) {

  state RS_INIT:

  state RS_RCMDM2:

	diag (
		"\r\nMALLOC Test\r\n"
		"Commands:\r\n"
#if	MALLOC_SINGLEPOOL
		"a n k    -> allocate n bytes, chunk id is k\r\n"
#else
		"a n p k  -> allocate n bytes from pool p, chunk id is k\r\n"
#endif
		"d k      -> deallocate the chunk k\r\n"
		"l        -> list\r\n"
		);

  state RS_RCMD:

	ser_in (RS_RCMD, ibuf, 48-1);

	if (ibuf [0] == 'a')
		proceed RS_ALL;

	if (ibuf [0] == 'd')
		proceed RS_DEA;

	if (ibuf [0] = 'l')
		proceed RS_LIS;

  state RS_RCMDP1:

	diag ("??");
	proceed RS_RCMDM2;

  state RS_ALL:

	word	n, p, k;
  	address	chunk;

#if	MALLOC_SINGLEPOOL
	scan (ibuf + 1, "%u %u", &n, &k);
	if (n == 0)
		proceed RS_RCMDP1;
	if (k >= MAX_N_CHUNKS || chunks [k] != NULL)
		proceed RS_RCMDP1;

	chunk = malloc (0, n);
	if (chunk == NULL) {
		diag ("FAILED");
		proceed RS_RCMD;
	}
#else
	scan (ibuf + 1, "%u %u %u", &n, &p, &k);
	if (n == 0)
		proceed RS_RCMDP1;
	if (k >= MAX_N_CHUNKS || chunks [k] != 0)
		proceed RS_RCMDP1;

	chunk = malloc (p, n);
	if (chunk == NULL) {
		diag ("FAILED");
		proceed RS_RCMD;
	}
#endif
	diag ("ALLOCATED at %x", chunk);
	chunks [k] = chunk;
	chsize [k] = n;
	for (p = 0; p < n; p++)
		((byte*)chunk) [p] = (byte) k;

#if	MALLOC_SINGLEPOOL == 0
	pools [k] = p;
#endif
	proceed RS_RCMD;

  state RS_DEA:

	word k;

	scan (ibuf + 1, "%u", &k);
	if (k >= MAX_N_CHUNKS || chunks [k] == NULL)
		proceed RS_RCMDP1;

#if	MALLOC_SINGLEPOOL
	free (0, chunks [k]);
#else
	free (pools [k], chunks [k]);
#endif
	chunks [k] = NULL;
	diag ("DONE");
	proceed RS_RCMD;

  state RS_LIS:

	word	m, n, p, k;
	address	chunk;

	diag ("Allocated chunks:");
	for (k = 0; k < MAX_N_CHUNKS; k++) {
		if ((chunk = chunks [k]) == NULL)
			continue;
		// Validate
		for (p = 0; p < chsize [k]; p++)
			if (((byte*)chunk) [p] != (byte) k)
				break;
		if (p < chsize [k])
			diag ("Chunk %d [%d] is corrupted at %d", k, chsize [k],
				p);
		else
			diag ("Chunk %d [%d] OK", k, chsize [k]);
	}

	n = memfree (0, &p);
	m = maxfree (0, &k);
	diag ("Memfree: %d %d %d %d", n, p, m, k);

#if MALLOC_SINGLEPOOL == 0
	n = memfree (1, &p);
	m = maxfree (1, &k);
	diag ("Memfree: %d %d %d %d", n, p, m, k);
#endif
	proceed RS_RCMD;
}

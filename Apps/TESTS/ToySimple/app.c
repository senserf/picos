/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "form.h"

#define	IBUF_SIZE	80

#define	OU_INIT		0
#define	OU_WRITE	1
#define	OU_RETRY	2

process (output, char)

    static int nc;
    int k;

    entry (OU_INIT)

	nc = strlen (data);

    entry (OU_WRITE)

	if (nc == 0)
		finish;

    entry (OU_RETRY)

        k = io (OU_RETRY, UART, WRITE, data, nc);
	nc -= k;
	data += k;
	savedata (data);
	proceed (OU_WRITE);

endprocess (1)

#define	IN_INIT		0
#define	IN_READ		1

process (input, char)

    static int nc;
    int k;

    entry (IN_INIT)

	nc = IBUF_SIZE;

    entry (IN_READ)

    	k = io (IN_READ, UART, READ, data, nc);

	if (*(data+k-1) == '\n') {
		// End of line
		*(data+k-2) = '\0';
		finish;
	}

	if (k > nc - 2)
		k = nc - 2;

	nc -= k;
	data += k;
	savedata (data);

	proceed (IN_READ);

endprocess (1)

#define	RS_INIT		0
#define	RS_SHOW		1
#define	RS_READ		2
#define	RS_CMND		3
#define	RS_FORMAT	4
#define	RS_SHOWMEM	5

process (root, void)
/* =========================================== */
/* This is the main program of the application */
/* =========================================== */

  static char *ibuf;

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUF_SIZE);

  entry (RS_SHOW)

	call (output, "\r\n"
		"Welcome to PicOS!\r\n"
		"Commands:\r\n"
		"          'f %d %u %x %c %s %ld %lu %lx' (format test)\r\n"
		"          'v' (show free memory)\r\n"
		"          'h' (HALT)\r\n"
		"          'r' (RESET)\r\n",
			RS_READ);

  entry (RS_READ)

	call (input, ibuf, RS_CMND);

  entry (RS_CMND)

	switch (ibuf [0]) {
		case 'f' : proceed (RS_FORMAT);
		case 'v' : proceed (RS_SHOWMEM);
		case 'r' : reset ();
	 	case 'h' : halt ();
	}

	call (output, "Illegal command or parameter\r\n", RS_SHOW);

  entry (RS_FORMAT)

	{
	    int vi, q;
	    word vu, vx;
	    char vc, vs [24];
	    long vli;
	    lword vlu, vlx;

	    vc = 'x';
	    strcpy (vs, "--empty--");
	    vli = 0;
	    vlu = vlx = 0;

	    q = scan (ibuf + 1, "%d %u %x %c %s %ld %lu %lx",
				&vi, &vu, &vx, &vc, vs, &vli, &vlu, &vlx);

	    form (ibuf, "%d ** %d, %u, %x, %c, %s, %ld, %lu, %lx **\r\n", q,
				vi, vu, vx, vc, vs, vli, vlu, vlx);

	    call (output, ibuf, RS_READ);
	}

  entry (RS_SHOWMEM)

        form (ibuf, "Stack %u wds, static data %u wds, heap %u wds\r\n",
		stackfree (), staticsize (), memfree (0, 0));

	call (output, ibuf, RS_READ);

endprocess (1)
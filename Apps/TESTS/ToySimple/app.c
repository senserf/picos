/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "form.h"

#define	IBUF_SIZE	80

#define	OU_INIT		0
#define	OU_WRITE	1
#define	OU_RETRY	2

strand (output, char)

    static int nc;
    static char *ptr;
    int k;

    entry (OU_INIT)

	nc = strlen (ptr = data);

    entry (OU_WRITE)

	if (nc == 0)
		finish;

    entry (OU_RETRY)

        k = io (OU_RETRY, UART, WRITE, ptr, nc);
	nc -= k;
	ptr += k;
	proceed (OU_WRITE);

endstrand

#define	IN_INIT		0
#define	IN_READ		1

strand (input, char)

    static int nc;
    static char *ptr;
    int k;

    entry (IN_INIT)

	nc = IBUF_SIZE;
	ptr = data;

    entry (IN_READ)

    	k = io (IN_READ, UART, READ, ptr, nc);

	if (*(ptr+k-1) == '\n') {
		// End of line
		*(ptr+k-2) = '\0';
		finish;
	}

	if (k > nc - 2)
		k = nc - 2;

	nc -= k;
	ptr += k;

	proceed (IN_READ);

endstrand

#define	RS_INIT		0
#define	RS_SHOW		1
#define	RS_READ		2
#define	RS_CMND		3
#define	RS_FORMAT	4
#define	RS_SHOWMEM	5
#define	RS_DELAY	6
#define	RS_DUP		7
#define	RS_PSAVE	8
#define	RS_FREEZE	9
#define	RS_PUP		10

thread (root)
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
		" 'f %d %u %x %c %s %ld %lu %lx' (format test)\r\n"
		" 'v' (mem)\r\n"
		" 'd n' (sleep n secs)\r\n"
		" 'p n' (PD mode n secs)\r\n"
		" 'g n' (freeze n secs)\r\n"
		" 'h' (HALT)\r\n"
		" 'r' (RESET)\r\n",
			RS_READ);

  entry (RS_READ)

	call (input, ibuf, RS_CMND);

  entry (RS_CMND)

	switch (ibuf [0]) {
		case 'f' : proceed (RS_FORMAT);
		case 'v' : proceed (RS_SHOWMEM);
		case 'd' : proceed (RS_DELAY);
		case 'p' : proceed (RS_PSAVE);
		case 'g' : proceed (RS_FREEZE);
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

  entry (RS_DELAY)

	{
	    word d;

	    d = 1;
	    scan (ibuf + 1, "%u", &d);
	    if (d > 60)
		d = 60;

diag ("DEL %u", d * 1024);
	    delay (d * 1024, RS_DUP);
        }

	release;

  entry (RS_PSAVE)

	{
	    word d;

	    d = 1;
	    scan (ibuf + 1, "%u", &d);
	    if (d > 60)
		d = 60;

	    clockdown ();
	    powerdown ();

	    delay (d * 1024, RS_PUP);
        }
	release;

  entry (RS_PUP)

	powerup ();
	clockup ();

  entry (RS_DUP)

	call (output, "done\r\n", RS_READ);

  entry (RS_FREEZE)

	{
	    word d;

	    d = 1;
	    scan (ibuf + 1, "%u", &d);
	    freeze (d);

	}
	proceed (RS_DUP);

endthread

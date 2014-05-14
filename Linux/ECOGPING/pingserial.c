#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
/* ============================================================================ */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002, 2003                 */
/*                                                                              */
/* Permission is hereby granted, free of charge, to any person obtaining a copy */
/* of this software and associated documentation files (the "Software"), to     */
/* deal in the Software without restriction, including without limitation the   */
/* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  */
/* sell copies of the Software, and to permit persons to whom the Software is   */
/* furnished to do so, subject to the following conditions:                     */
/*                                                                              */
/* The above copyright notice and this permission notice shall be included in   */
/* all copies or substantial portions of the Software.                          */
/*                                                                              */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS */
/* IN THE SOFTWARE.                                                             */
/*                                                                              */
/* ============================================================================ */

/*
 * This is a serial version of the ping server. It forks into two copies.
 * One of them at randomized intervals between 1 and 8 seconds sends a
 * packet over the serial port, while the other retrieves packets arriving
 * over the port and dumps them to the standard output. This is how those
 * packets look:
 *
 *              SYN ... SYN DLE STX ... data ... DLE ETX
 *
 * where the starting sequence of SYN's can be of arbitrary length. In the
 * packets sent by the server, there are 4 SYN characters in front. A DLE
 * symbol occurring within the payload must be escaped by another DLE, i.e.,
 * two consecutive occurences of DLE are merged into one.
 */

#define	BUFLEN		128

#define	CHR_SYN		0x16
#define	CHR_DLE		0x10
#define	CHR_STX		0x02
#define	CHR_ETX		0x03

#define	DEFTTY		"ttyS0"	/* Default serial device */
#define	SPEED		B19200

void			sender (void), receiver (void), dump (int);

int			openserial (const char*);

char 			buf [BUFLEN];

int			sfd;

void badusage () {

	fprintf (stderr, "Usage: pingserial [dev]\n");
	exit (99);
}

main (int argc, char *argv []) {

	char devname [32];

	strcpy (devname, "/dev/");
	if (argc > 1) {
		argv++;
		if (strlen (*argv) > 24)
			badusage ();
		strcat (devname, *argv);
	} else {
		strcat (devname, DEFTTY);
	}

	sfd = openserial (devname);

	if (fork ())
		receiver ();
	else
		sender ();
}

int openserial (const char *dev) {

	int fd;
	struct termios tm;

	if ((fd = open (dev, O_NOCTTY | O_RDWR, 0660)) < 0) {
		perror ("Cannot open serial device");
		exit (99);
	}

	if (tcgetattr (fd, &tm) < 0) {
		perror ("Cannot get serial attributes");
		exit (99);
	}

	cfmakeraw (&tm);
	cfsetispeed (&tm, SPEED);
	cfsetospeed (&tm, SPEED);

	tm.c_iflag &= ~(IXON | IXOFF);
	tm.c_oflag &= ~(ONLCR | OCRNL | ONOCR | ONLRET);
	tm.c_cflag &= ~(CSTOPB | CRTSCTS | PARENB | PARODD | HUPCL);
	tm.c_cflag |=  (CREAD | CLOCAL | CS8);
	tm.c_lflag &= ~(ICANON | ISIG | ECHO | ECHOE | ECHOK | ECHONL);

	if (tcsetattr (fd, TCSANOW, &tm) < 0) {
		perror ("Cannot set serial attributes");
		exit (99);
	}

	return fd;
}

void sender (void) {

	int start, counter, length;
	struct timeval RT;
	char *wt, *ct;

	start = 0;
	counter = 0;

	/* Prepare the header */
	buf [start++] = CHR_SYN;
	buf [start++] = CHR_SYN;
	buf [start++] = CHR_SYN;
	buf [start++] = CHR_SYN;
	buf [start++] = CHR_DLE;
	buf [start++] = CHR_STX;

	while (1) {
		gettimeofday (&RT, NULL);
		for (wt = ct = ctime ((time_t*)(&(RT.tv_sec))); *ct != '\0';
			ct++) {

			if (*ct == '\n') {
				*ct = '\0';
				break;
			}
		}

		/* Make it shorter */
		*(wt + 11 + 8) = '\0';
		sprintf (buf + start, "Count: %1d, Time: %s", counter, wt + 11);
		/* Include the NULL byte in the packet to simplify printing */
		length = start + strlen (buf + start) + 1;
		buf [length++] = CHR_DLE;
		buf [length++] = CHR_ETX;

		write (sfd, buf, length);

		counter++;

		sleep (1 + ((rand () & 0x70) >> 4));
	}
}

#define	store(c)	do { \
				if (nc < BUFLEN) \
					buf [nc++] = (c); \
			} while (0)

char getbyte (void) {

	char c;

	if (read (sfd, &c, 1) < 0) {
		perror ("Error reading from serial line");
		exit (99);
	}

	return c;
}

void receiver (void) {

	int	nc;
	char	bt;

	while (1) {

		while (getbyte () != CHR_SYN);
		while ((bt = getbyte ()) == CHR_SYN);
		if (bt != CHR_DLE)
			continue;
		if (getbyte () != CHR_STX)
			continue;

		nc = 0;

		while (1) {
			bt = getbyte ();
			if (bt == CHR_DLE) {
				/* Possible escape */
				bt = getbyte ();
				if (bt == CHR_DLE) {
					store (bt);
					continue;
				} else {
					/* Assume blindly this is ETX */
					getbyte ();
					break;
				}
			} else {
				store (bt);
			}
		}

		dump (nc);
	}
}

void dump (int len) {

	int i;
	static int count = 0;

	printf ("Packet %1d, Length = %1d:", count, len);

	count++;

	for (i = 0; i < len; i++) {
		if (i % 16 == 0)
			printf ("\n");
		printf ("%02x ", ((int)(buf [i])) & 0xff);
	}
	printf ("\n");
}

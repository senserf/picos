#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
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

/* #define	DEBUG		1 */

#define	ETH_PROTO	0x6006		// Protocol identifier
#define	ETH_PROMISC	1
#define	MAXPLEN		16
#define	MIN_PAYLOAD	48
#define	GROUP		0
#define	PTYPE		0x0000
#define	PTYPE_INC	0x8000
#define	POWER		9

#define	TIMEOUT		5		// In seconds

typedef struct {
	unsigned short command;
	unsigned short length;
	char stuff [MAXPLEN];
} tpacket_t;

typedef struct {
	unsigned short command;
	unsigned short cardid;
	unsigned short length;
	char stuff [MAXPLEN];
} rpacket_t;

static tpacket_t 		tbuf;
static rpacket_t 		rbuf;
static int 			csok, ifd;
static struct sockaddr_ll 	saddr, daddr;

static void badusage (const char *pname) {

	fprintf (stderr, "Usage: %s [filename [ifname]]\n", pname);
	exit (99);
}

#ifdef	DEBUG

void dump (const char *p, const char *h, int len) {

	int i;

	printf ("%s, length = %1d:", h, len);

	for (i = 0; i < len; i++) {
		if (i % 16 == 0)
			printf ("\n");
		printf ("%02x ", ((int)(p [i])) & 0xff);
	}
	printf ("\n");
}

#else
#define	dump(a,b,c)
#endif

static int send_packet () {

	int len, rlen, fromlen, i;
	char expect;

	len = ntohs (tbuf.length) + (2 * sizeof (short));
	if (len < MIN_PAYLOAD)
		len = MIN_PAYLOAD;

	expect = tbuf.stuff [0] & 0x1;

	while (1) {
		/* For retransmissions */
		dump ((const char*) &tbuf, "Sending", len);
		while (sendto (csok, (char*) &tbuf, len, 0,
			(const struct sockaddr*) &saddr, sizeof (saddr)) < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK)
				return 1;
			usleep (1000);
		}
		/* Wait for the ACK */
		fromlen = sizeof (daddr);
		for (i = 0; i < TIMEOUT * 500; i++) {
			if ((rlen = recvfrom (csok, &rbuf,
				MAXPLEN + (4 * sizeof (short)), 0,
					(struct sockaddr*) &daddr, &fromlen)) <
						0) {
				if (errno != EAGAIN && errno != EWOULDBLOCK)
					return 1;
				// printf ("WOULDBLOCK\n");
				usleep (2000);
			} else {
				/* Done */
				dump ((const char*) &rbuf, "Received", rlen);
				if (rbuf.stuff [0] & 2)
					/* Abort */
					return 1;
				if ((rbuf.stuff [0] & 0x1) == expect &&
					ntohs (rbuf.command) == PTYPE_INC &&
					ntohs (rbuf.length) == 1)
						return 0;
			}
		}
	}
}

main (int argc, char *argv []) {

	struct	ifreq		ifioctl;
	int			nb, ifx;

	if (argc > 3)
		badusage (argv [0]);

	/* Default input */
	ifd = 0;
	/* Default interface */
	strcpy (ifioctl . ifr_name, "eth0");

	if (argc > 1) {
		if (strcmp (argv [1], "-") != 0) {
			/* A file to open */
			ifd = open (argv [1], O_RDONLY);
			if (ifd < 0) {
				perror ("Cannot open input file");
				exit (99);
			}
		}
		if (argc > 2) {
			if (strlen (argv [2]) > IFNAMSIZ - 1) {
				fprintf (stderr, "Interface name '%s' too long",
					argv [2]);
				exit (99);
			}
			strcpy (ifioctl . ifr_name, argv [2]);
		}
	}

	csok = socket (PF_PACKET, SOCK_DGRAM, htons (ETH_PROTO));

	if (csok < 0) {
		perror ("Cannot create socket");
		exit (99);
	}

	/* Determine the interface index */
	ifioctl . ifr_ifindex = 0x01010101;

	if (ioctl (csok, SIOCGIFINDEX, &ifioctl) < 0 ||
				ifioctl . ifr_ifindex == 0x01010101) {
		perror ("Cannot get interface index");
		exit (99);
	}
	ifx = ifioctl . ifr_ifindex;

#ifdef	ETH_PROMISC
	/* =============================================================== */
	/* Turn the interface into promiscuous mode. Some cards don't need */
	/* that (e.g.,  the one on my office machine),  as they are set by */
	/* default  to  accept all multicast (not only broadcast) packets. */
	/* The card on my laptop, however, seems to need that.             */
	/* =============================================================== */
	if (ioctl (csok, SIOCGIFFLAGS, &ifioctl) < 0) {
		perror ("Cannot get flags");
		exit (1);
	}
	ifioctl . ifr_flags |= IFF_PROMISC;
	if (ioctl (csok, SIOCSIFFLAGS, &ifioctl) < 0) {
		perror ("Cannot set flags");
		exit (1);
	}
#endif

#if 1
	/* Bind the socket (this is optional, I believe) */
	saddr . sll_family = AF_PACKET;
	saddr . sll_protocol = htons (ETH_PROTO);
	saddr . sll_ifindex = ifx;
	saddr . sll_halen = 6;
	saddr . sll_addr  [ 0] = 0xE1;
	saddr . sll_addr  [ 1] = 0xAA;
	saddr . sll_addr  [ 2] = 0xBB;
	saddr . sll_addr  [ 3] = 0xCC;
	saddr . sll_addr  [ 4] = 0xDD;
	saddr . sll_addr  [ 5] = 0xEF;

	memcpy (&daddr, &saddr, sizeof (saddr));

	if (bind (csok, (struct sockaddr*) (&saddr), sizeof (saddr)) < 0) {
		perror ("Bind failed");
		exit (99);
	}
#endif

	if (fcntl (csok, F_SETFL, fcntl (csok, F_GETFL) | O_NONBLOCK) < 0) {
		perror ("Fnctl failed");
		exit (99);
	}

	tbuf.command = htons (PTYPE);
	tbuf.stuff [0] = 2;	/* Start */
	tbuf.length = htons ((short)1);

	if (send_packet ()) {
		fprintf (stderr, "Aborted by the board\n");
		exit (0);
	}

#ifdef	DEBUG
	printf ("Header sent and accepted\n");
#endif

	tbuf.stuff [0] = 1;	/* Initialize alternating bit */

	while (1) {

		nb = read (ifd, tbuf.stuff + 1, MAXPLEN - 1);
		if (nb <= 0) {
			if (nb < 0)
				perror ("Input error");
			tbuf.stuff [0] = 4 | tbuf.stuff [0];
			tbuf.length = htons ((short)1);
			if (send_packet ())
				fprintf (stderr, "Aborted by the board\n");
#ifdef	DEBUG
			printf ("Trailer sent and accepted\n");
#endif
			exit (0);
		}
		tbuf.length = htons ((short)(nb + 1));
		if (send_packet ()) {
			fprintf (stderr, "Aborted by the board\n");
			exit (0);
		}
#ifdef	DEBUG
		printf ("Packet sent and accepted\n");
#endif
		/* Flip the alternating bit */
		nb = (tbuf.stuff [0] & 0x1) ? 0 : 1;
		tbuf.stuff [0] = nb | (tbuf.stuff [0] & 0xfe);
	}
}

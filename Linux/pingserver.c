#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define	ETH_PROTO	0x6006
#define	ETH_INAME	"eth0"
#define	BUF_LNGTH	4096
#define	ETH_PROMISC	1
#define	MAXCIDS		16
#define	ETH_MINLEN	(60 - 6*2 - 2)
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
 * This program forks into two copies. One of them periodically (every five
 * seconds) multicasts a packet addressed to the eCOG farm, while the other
 * attempts to receive packets from the farm and display their contents.
 * If the first argument looks like 'ethX', where X is a digit, it is viewed
 * as the name of the Ethernet interface to be used. Any integer arguments
 * are interpreted as the Id's of the cards to which the outgoing packets
 * are to be addressed. If no card Id's are given, all outgoing packets are
 * addressed to all cards (their Id lists are empty).
 */

void			sender (void), receiver (void),
				dump (const char*, int, int);
unsigned char 		buf [BUF_LNGTH];
struct	sockaddr_ll 	saddr;
int			sok, loffs;

#define	cardids		((short*)(buf + 2))
#define	ncids		(buf [1])
#define	payload		(buf + loffs + 2)
#define	length		(*((short*)(buf + loffs)))

void badusage () {

	fprintf (stderr,
		"Usage: pingserver [ethX] [id ... id]    (<= 16 id's)\n");
	exit (99);
}

main (int argc, char *argv []) {

	struct	ifreq		ifioctl;
	char			*eptr;
	int			ifindex;

	/* Default interface name */
	strcpy (ifioctl . ifr_name, ETH_INAME);
	/* Id list is empty */
	ncids = 0;
	loffs = 2;
	buf [0] = 0;

	while (--argc > 0) {
		argv++;
		if ((*argv) [0] == 'e' &&
		    (*argv) [1] == 't' &&
		    (*argv) [2] == 'h' &&
		    (*argv) [4] == '\0' ) {
			strcpy (ifioctl . ifr_name, *argv);
			continue;
		}
		ifindex = strtol (*argv, &eptr, 10);
		if (**argv == '\0' || *eptr != '\0')
			badusage ();
		if (ncids == MAXCIDS)
			badusage ();
		cardids [ncids++] = htons ((short) ifindex);
		loffs += 2;
	}

	sok = socket (PF_PACKET, SOCK_DGRAM, htons (ETH_PROTO));
	if (sok < 0) {
		perror ("Cannot create packet socket");
		exit (1);
	}

	/* Determine the interface index */
	ifioctl . ifr_ifindex = 0x01010101;
	if (ioctl (sok, SIOCGIFINDEX, &ifioctl) < 0
	    || ifioctl . ifr_ifindex == 0x01010101) {
		perror ("Cannot get interface index");
		exit (1);
	}

	ifindex = ifioctl . ifr_ifindex;
	printf ("Interface %s index = %1d\n", ETH_INAME, ifindex);

#ifdef	ETH_PROMISC
	/* =============================================================== */
	/* Turn the interface into promiscuous mode. Some cards don't need */
	/* that (e.g.,  the one on my office machine),  as they are set by */
	/* default  to  accept all multicast (not only broadcast) packets. */
	/* The card on my laptop, however, seems to need that.             */
	/* =============================================================== */
	if (ioctl (sok, SIOCGIFFLAGS, &ifioctl) < 0) {
		perror ("Cannot get flags");
		exit (1);
	}
	ifioctl . ifr_flags |= IFF_PROMISC;
	if (ioctl (sok, SIOCSIFFLAGS, &ifioctl) < 0) {
		perror ("Cannot set flags");
		exit (1);
	}
#endif

	/* ============================================================= */
	/* Bind the socket to the interface - this is optional and makes */
	/* sure we will not try to interpret packets arriving on another */
	/* interface.                                                    */
	/* ============================================================= */
	saddr . sll_family = AF_PACKET;
	saddr . sll_protocol = htons (ETH_PROTO);
	saddr . sll_ifindex = ifindex;
	/* =============================================================== */
	/* Set the destination address. This is a multicast address, which */
	/* is pretty much irrelevant because the eCOG card will accept all */
	/* multicast  packets and use the protocol number  (ETH_PROTO)  to */
	/* tell the interesting ones apart.                                */
	/* =============================================================== */
	saddr . sll_halen = 6;
	saddr . sll_addr  [ 0] = 0xE1;
	saddr . sll_addr  [ 1] = 0xAA;
	saddr . sll_addr  [ 2] = 0xBB;
	saddr . sll_addr  [ 3] = 0xCC;
	saddr . sll_addr  [ 4] = 0xDD;
	saddr . sll_addr  [ 5] = 0xEF;

	if (bind (sok, (struct sockaddr*) (&saddr), sizeof (saddr)) < 0) {
		perror ("Bind failed");
		exit (1);
	}

	if (fork ())
		receiver ();
	else
		sender ();
}

void sender (void) {

	int count, n, len;
	struct timeval RT;
	char *wt, *ct;

	count = 0;		/* Packet counter */

	while (1) {

		gettimeofday (&RT, NULL);
		for (wt = ct = ctime ((time_t*)(&(RT.tv_sec))); *ct != '\0';
			ct++) {

			if (*ct == '\n') {
				*ct = '\0';
				break;
			}
		}

		/* Make it no longer than 16 bytes */
		*(wt + 11 + 8) = '\0';
		sprintf (payload, "Time: %s", wt + 11);
		count++;

		/* Set the length */
		len = strlen (payload);
		length = htons ((short)len);

		if ((len += loffs + 2) < ETH_MINLEN)
			len = ETH_MINLEN;

		n = sendto (sok, buf, len, 0, (const struct sockaddr*)
			(&saddr), sizeof (saddr));

		if (n < 0) {
			perror ("Send failed");
			exit (1);
		}

		sleep (5);
	}
}

void receiver (void) {

	int count, n, len;

	count = 0;

	while (1) {
		len = sizeof (saddr);
		n = recvfrom (sok, buf, BUF_LNGTH-1, MSG_TRUNC,
			(struct sockaddr*) (&saddr), &len);
		if (n < 0) {
			perror ("Receive failed");
			exit (1);
		}
		/* Check if this is an uplink packet sent by a card */
		if ((buf [0] & 0x80) == 0)
			/* Ignore */
			continue;
		dump (buf, count, n);
		count++;
	}
}

void dump (const char *p, int num, int len) {

	int i, alen;

	printf ("Packet %1d, CId = %1d, Length = %1d/%1d:", num,
		(int)(((unsigned int) (p [2]) << 8) | (unsigned int) (p [3])),
		len, alen =
	        (int)(((unsigned int) (p [4]) << 8) | (unsigned int) (p [5])));
	p += 6;

	for (i = 0; i < alen; i++) {
		if (i % 16 == 0)
			printf ("\n");
		printf ("%02x ", ((int)(p [i])) & 0xff);
	}
	printf ("\n");
}

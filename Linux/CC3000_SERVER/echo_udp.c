#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

// Opens a simple service on port 2234. Following a connection, it reads
// lines from the socket and echoes each line with the case of letters
// reversed. Used to test CC3000.

#define MY_PORT		2234	/* Master socket port number */
#define	BUFSIZE		512

char buffer [BUFSIZE];
int bp;

void doit () {

	int pp;
	char c;

	for (pp = 0; pp < bp; pp++) {
		c = buffer [pp];
		fprintf (stderr, " B: %02x\n", (unsigned char)c);
		if (c >= 'a' && c <= 'z')
			buffer [pp] = c - 'a' + 'A';
		else if (c >= 'A' && c <= 'Z')
			buffer [pp] = c - 'A' + 'a';
	}
}

main() 
{
	int	sock, fromlength;
	struct	sockaddr_in master, from;
	unsigned char *ap;

	sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock < 0) {
		perror ("Server: cannot open master socket");
		exit (1);
	}

	master.sin_family = AF_INET;
	master.sin_addr.s_addr = INADDR_ANY;
	master.sin_port = htons (MY_PORT);

	/* Bind the port to the master socket */
	if (bind (sock, (struct sockaddr*) &master, sizeof (master))) {
		perror ("Server: cannot bind master socket");
		exit (1);
	}

	listen (sock, 5);
	fromlength = sizeof (from);

	fprintf (stderr, "Ready:\n");

	while (1) {

		/* The main loop */
		if ((bp = recvfrom (sock, buffer, BUFSIZE, 0,
		    (struct sockaddr*) &from, &fromlength)) < 0) {
			perror ("Server: recvfrom error");
			exit (1);
		}

		ap = ((unsigned char*)(&from)) + 2;
		fprintf (stderr, "RCV %1d FROM: [%1u] %1u.%1u.%1u.%1u\n", bp,
			((unsigned short)(ap [0]) << 8) |
			 (unsigned short)(ap [1]),
			 (unsigned int)(ap [2]),
			 (unsigned int)(ap [3]),
			 (unsigned int)(ap [4]),
			 (unsigned int)(ap [5]));

		doit ();

		if (sendto (sock, buffer, bp, 0, (struct sockaddr*) &from,
		    fromlength) < 0) {
			perror ("Server: sendto error");
			exit (1);
		}
	}
}

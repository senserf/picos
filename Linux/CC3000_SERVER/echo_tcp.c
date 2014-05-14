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

int client;

main() 
{
	int	sock, fromlength;
	struct	sockaddr_in	master, from;

	/* Create master socket to await connections */
	sock = socket (AF_INET, SOCK_STREAM, 0);
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

	while (1) {
		/* The main loop */
		client = accept (sock, (struct sockaddr*) & from, & fromlength);
		if (client < 0) {
			perror ("Server: accept failed");
			continue;
		}
                if (fork ())
			close (client);
		else {
			close (sock);
			server ();
			exit (0);
		}
	}
}

// ============================================================================

char buffer [BUFSIZE];
int bp;

void getl () {

	int st;
	char c;

	bp = 0;

	while (1) {

		if ((st = read (0, &c, 1)) == 0) {
			// EOF
			fprintf (stderr, "Connection closed\n");
			close (0);
			exit (0);
		}
		if (st < 0) {
			perror ("Read error");
			exit (0);
		}
fprintf (stderr, "Byte read: %02x\n", c);

		if (c == '\n' || c == '\0') {
			buffer [bp++] = '\n';
			return;
		}

		if (bp < BUFSIZE-1)
			buffer [bp++] = c;
	}
}

void putl () {

	char *sb;
	int st, lf;

	sb = buffer;
	lf = bp;

	while (lf) {

		st = write (1, sb, lf);

		if (st < 0) {
			perror ("Write error");
			exit (0);
		}

		if (st == 0) {
			fprintf (stderr, "Zero length write\n");
			exit (0);
		}

		sb += st;
		lf -= st;
	}
}

void doit () {

	int pp;
	char c;

	for (pp = 0; pp < bp; pp++) {
		c = buffer [pp];
		if (c >= 'a' && c <= 'z')
			buffer [pp] = c - 'a' + 'A';
		else if (c >= 'A' && c <= 'Z')
			buffer [pp] = c - 'A' + 'a';
	}
}

server () {

	close (0); close (1);
	dup2 (client, 0); dup2 (client, 1);

	fprintf (stderr, "CONNECTION!\n");

	while (1) {
		getl ();
		fprintf (stderr, "LINE: %s", buffer);
		doit ();
		putl ();
	}
}

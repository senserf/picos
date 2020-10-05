/*
	Copyright 1995-2020 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

/* ---------------------------------------- */
/* Virtual socket layer -- for Windows port */
/* ---------------------------------------- */

#include "../version.h"

#include "socklib.h"

#include        <sys/types.h>
#include        <sys/socket.h>
#include        <sys/time.h>
#include        <sys/file.h>
#include        <sys/un.h>
#include        <sys/socket.h>
#include        <fcntl.h>
#include	<unistd.h>
#include	<string.h>
#include	<errno.h>
#include        <netinet/in.h>
#include        <netdb.h>

void setSocketOptions (int sfd, int lgon, int lgtime, int kal) {

  struct linger linp;

  bzero (&linp, sizeof (linp));

  linp.l_onoff = lgon;
  linp.l_linger = lgtime;

  setsockopt (sfd, SOL_SOCKET, SO_LINGER, &linp, sizeof (linp));
  setsockopt (sfd, SOL_SOCKET, SO_KEEPALIVE, &kal, sizeof (kal));
}

int openClientSocket (const char *hostname, int port) {
  // Connect to the specified port on the specified host
  int sk, er, i, j;
  const char *h; char addr [5];
  struct hostent *host;
  struct sockaddr_in saddr;
  if ((sk = socket (AF_INET, SOCK_STREAM, 0)) < 0) return sk;
  bzero ((BCPCASTD)(&saddr), sizeof (saddr));
  // Check if the hostname is a numeric IP address
  j = 0;
  addr [0] = 0;
  for (h = hostname; *h != '\0'; h++) {
    if (*h == '.') {
      addr [++j] = 0;
      if (j == 4) break;
    } else {
      if (*h < '0' || *h > '9') break;
      addr [j] = addr [j] * 10 + (*h - '0');
    }
  }
  if (*h == '\0' && j == 3) {
    // We have a numeric address
    addr [4] = 0;
    saddr.sin_family = AF_INET;
    bcopy ((BCPCASTS)(addr), (BCPCASTD)(&(saddr.sin_addr)), 4);
  } else {
    host = gethostbyname (hostname);
    bcopy ((BCPCASTS)(host->h_addr), (BCPCASTD)(&(saddr.sin_addr)),
                                                                host->h_length);
    saddr.sin_family = host->h_addrtype;
  }
  saddr.sin_port = htons (port);
  if (connect (sk, (struct sockaddr*)(&saddr), sizeof (saddr))) {
  // Note: linux plays tricks here; connect returns error
  // although it works
    if (errno != EINTR) {
      er = errno;
      close (sk);
      errno = er;
      return -1;
    }
  }
  setSocketOptions (sk, 0, 0, 1);
  return sk;
};

int openClientSocket (const char *fname) {
  // Connect to the specified UNIX socket on this host
  int sk, er;
  struct sockaddr_un saddr;
  if (strlen (fname) >= sizeof (saddr.sun_path)) {
    errno = ENAMETOOLONG;
    return -1;
  }
  if ((sk = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) return sk;
  strcpy (saddr.sun_path, fname);
  saddr.sun_family = AF_UNIX;
  if (connect (sk, (struct sockaddr*)(&saddr), sizeof (saddr))) {
    if (errno != EINTR) {
      er = errno;
      close (sk);
      errno = er;
      return -1;
    }
  }
  setSocketOptions (sk, 0, 0, 1);
  return sk;
};

int openServerSocket (int port) {
  // Open a server socket and await connections
  int sk, er;
  struct sockaddr_in saddr;
  if ((sk = socket (AF_INET, SOCK_STREAM, 0)) < 0) return sk;
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons (port);
  if (bind (sk, (struct sockaddr*) (&saddr), sizeof (saddr))) {
    er = errno;
    close (sk);
    errno = er;
    return -1;
  }
  setSocketOptions (sk, 0, 0, 1);
  listen (sk, 5);
  return sk;
};

int openServerSocket (const char *fname) {
  // Open a server socket and await connections
  int sk, er;
  struct sockaddr_un saddr;

  if (strlen (fname) >= sizeof (saddr.sun_path)) {
    errno = ENAMETOOLONG;
    return -1;
  }

  if ((sk = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) return sk;

  strcpy (saddr.sun_path, fname);
  saddr.sun_family = AF_UNIX;

  if (bind (sk, (struct sockaddr*) (&saddr), sizeof (saddr))) {
    er = errno;
    close (sk);
    errno = er;
    return -1;
  }
  setSocketOptions (sk, 0, 0, 1);
  listen (sk, 5);
  return sk;
};

int getSocketConnection (int sock) {
  // Accept an incoming connection
  int sk;
  if ((sk = accept (sock, NULL, NULL)) < 0)
    return (errno == EAGAIN) ? (-2) : (-1);
  return sk;
};

int setSocketUnblocking (int sock) {
 return
  fcntl (sock, F_SETFL, (fcntl(sock, F_GETFL) | (O_NONBLOCK | O_NDELAY)));
};

int setSocketBlocking (int sock) {
 return
  fcntl (sock, F_SETFL, (fcntl(sock, F_GETFL) & ~(O_NONBLOCK | O_NDELAY)));
};

// We leave it empty, because it is only needed under Windows (if at all)
void closeAllSockets () { };

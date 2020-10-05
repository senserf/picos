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

/* ----------------------------------------------------------------- */
/* Virtual socket layer -- for a transition from UNIX to DOS/Windows */
/* ----------------------------------------------------------------- */

// Client
int openClientSocket (const char*, int);        // TCP/IP
int openClientSocket (const char*);             // UNIX

// Server
int openServerSocket (int);                     // TCP/IP
int openServerSocket (const char*);             // UNIX
int getSocketConnection (int);                  // Accept

// Both
int setSocketUnblocking (int);
int setSocketBlocking (int);

// Our options: linger + keepalive; DSD doesn't like nolinger
void setSocketOptions (int, int, int, int);

void closeAllSockets ();

// Standard BSD stuff
#define readSocket(s,b,c)  ::read (s, b, c)
#define writeSocket(s,b,c)  ::write (s, b, c)
#define closeSocket(s)  ::close(s)
#define myHostName(s,n)  ::gethostname (s, n)

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

#ifndef __server_c__
#define __server_c__

// Note: ACCEPTOR must be defined as the type of the process accepting
// a new connection. The setup method of that process must take one
// argument -- Socket mailbox pointer -- for cloning the connection
// mailbox.

void Server::setup (int pn) {
  Master = create Socket;
  if (Master->connect (INTERNET + SERVER + MASTER, pn) != OK)
    excptn ("Server: cannot setup MASTER mailbox");
};

Server::perform {

  state WaitConnection:

    if (Master->isPending ()) {
      // Accept a new connection
      create ACCEPTOR (Master);
      proceed WaitConnection;
    }
    Master->wait (UPDATE, WaitConnection);
};
 
#endif

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

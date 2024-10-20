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

#ifndef __server_h__
#define __server_h__

// Definitions for implementing socket-based servers

mailbox Socket (int);

process Server {
  Socket *Master;
  void setup (int);
  states {WaitConnection};
  perform;
};

#define readSocket(sk,buf,len,wai,fai) \
          do { \
            if (sk->read (buf, len)) { \
              if (sk->isActive ()) { \
                sk->wait (len, wai); \
                sleep; \
              } else { \
                proceed (fai); \
              } \
            }\
          } while (0)

#define readToSentinel(sk,buf,len,wai,fai) \
          do { \
            if (sk->readToSentinel (buf, len) <= 0) { \
              if (sk->isActive ()) { \
                sk->wait (SENTINEL, wai); \
                sleep; \
              } else { \
                proceed (fai); \
              } \
            }\
          } while (0)

#define writeSocket(sk,buf,len,wai,fai) \
          do { \
            if (sk->write (buf, len)) { \
              if (!(sk->isActive ())) proceed (fai); \
              sk->wait (OUTPUT, wai); \
              sleep; \
            } \
          } while (0)

#define disconnectClient(sk,again,timeout) \
          do { \
            if (sk->isConnected ()) { \
              sk->erase (); \
              if (sk->disconnect (CLIENT) == ERROR) { \
                Timer->wait (timeout, again); \
                sleep; \
              } \
            }\
            delete (sk); \
          } while (0)

#endif

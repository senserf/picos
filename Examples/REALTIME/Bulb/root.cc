#include "bulb.h"
#include "board.h"
#include "sensor.cc"

#include "server.h"

#define	ACCEPTOR Idler

TIME IdleTime;

process Idler;

static Idler *Idl = NULL;

process Idler {
  void setup (Socket *msk = NULL) {
    Socket *Sk;
    if (msk != NULL) {
      Sk = create Socket;
      Sk->connect (msk, 32);
      Sk->disconnect (CLEAR);
      delete Sk;
    }
    setLimit (0, Time+IdleTime);
    if (Idl == NULL)
      // To make the time limit effective
      Idl = this;
    else
      terminate ();
  };
  states {Void};
  perform {
    state Void:
      Timer->wait (5000000, Void);
  };
};

#include "server.cc"

static void initTimer () {
  int PortNumber;
  double Idle;
  readIn (PortNumber);
  readIn (Idle);
  IdleTime = (TIME) (Second * Idle);
  create Server (PortNumber);
  setLimit (0, IdleTime);
};

process Root {
  TIME TimeLimit;
  states {Start, Stop};
  perform {
    state Start:
      initSystem ();
      initBoard ();
      initTimer ();
      create Idler;
      Kernel->wait (DEATH, Stop);
    state Stop:
      terminate;
  };
};

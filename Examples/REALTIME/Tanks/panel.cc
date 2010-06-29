#include "panel.h"

#define	ACCEPTOR Receiver

#include "server.cc"

// #define DEBUG

TIME UpdateDelay, ForcedUpdateDelay, IdleTime;

static int NConnections = 0;       // The number of active connections

#ifdef	DEBUG
static void dumpBuf (const char *hdr, const char *buf, int len) {
  int i;
  Ouf << hdr << '\n';
  for (i = 0; i < len; i++) {
    Ouf << form ("%2x (%c) ", ((int) buf [i]) & 0xff,
        buf [i] >= 0x20 ? buf [i] : ' ');
    if (i > 0 && (i % 10 == 0) && i < len-1) Ouf << '\n';
  }
  Ouf << '\n' << '\n';
  Ouf.flush ();
}
#else
#define dumpBuf(a,b,c)
#endif


Updater::perform {

  state Init:

    MyReceiver->wait (Receiver::SendMap, Update);
    MyReceiver->wait (Receiver::Disconnecting, Update);

  state Update:

    int i, hf;
    Boolean Force;

    if (Sk->isConnected () == NO) {
      // Clean up and terminate
      // trace ("Updater terminating");
      delete UpdateBuffer;
      delete TFill;
      delete PStat;
      MyReceiver->MyUpdater = NULL;
      terminate;
    }

    if (UpdateFill == 0) {
      // The buffer may be full from previous update
      Force = (Time - LastUpdate >= ForcedUpdateDelay);
      UpdateFill = 3;
      for (i = 0; i < NTanks; i++) {
        hf = (int) (MAXFILL * (Tanks [i] -> Fill / TankCapacity));
        if (hf > MAXFILL) hf = MAXFILL;
        if (hf != TFill [i] || Force) {
          TFill [i] = hf;
          UpdateBuffer [UpdateFill++] = byte3 (i);
          UpdateBuffer [UpdateFill++] = byte2 (hf);
          UpdateBuffer [UpdateFill++] = byte3 (hf);
        }
      }
      for (i = 0; i < NPumps; i++) {
        hf = PumpModels [i] -> Motor -> getValue ();
        if (hf != PStat [i] || Force) {
          PStat [i] = hf;
          UpdateBuffer [UpdateFill++] = byte3 (i+128);
          UpdateBuffer [UpdateFill++] = byte3 (hf);
        }
      }
      if (UpdateFill > 3) {
        i = 0;
        UpdateFill -= 3;
        UpdateBuffer [i++] = CMD_UPDATE;
        UpdateBuffer [i++] = byte2 (UpdateFill);
        UpdateBuffer [i  ] = byte3 (UpdateFill);
        UpdateFill += 3;
      } else
        UpdateFill = 0;
    }
    if (UpdateFill) {
      LastUpdate = Time;
      if ((hf = Sk->write (UpdateBuffer, UpdateFill)) == OK) {
        dumpBuf ("Sent update:", UpdateBuffer, UpdateFill);
        UpdateFill = 0;
      } else if (hf == REJECTED) {
        Sk->wait (OUTPUT, Update);
      } else {
        // Disconnected
        proceed Update;
      }
    }
    Timer->wait (UpdateDelay, Update);
};

void Receiver::resize (int ns) {
  char *nb;
  int i, os;
  if (ns > CommandBufferLength) {
    os = CommandBufferLength;
    while (ns > CommandBufferLength) CommandBufferLength += CommandBufferLength;
    nb = new char [CommandBufferLength];
    for (i = 0; i < os; i++) nb [i] = CommandBuffer [i];
    delete CommandBuffer;
    CommandBuffer = nb;
  }
};

Receiver::perform {

  int nt, nl;

  state WaitCommand:

    if (Sk->read (CommandBuffer, CMDHDRL)) {
      if (Sk->isActive ()) {
        Sk->wait (CMDHDRL, WaitCommand);
        sleep;
      }
      // The other party is gone
      proceed Disconnecting;
    }

  transient WaitTail:

    if ((CommandLength = CMDLENGTH) > 0) {
      resize (CommandLength+CMDHDRL);
      if (Sk->read (CommandBuffer+CMDHDRL, CommandLength)) {
        if (!(Sk->isActive ())) proceed Disconnecting;
        Sk->wait (CommandLength, WaitTail);
        sleep;
      }
    }
    dumpBuf ("Received: ", CommandBuffer, CommandLength + CMDHDRL);

    if (COMMAND == CMD_UPDATE) {
      nt = (int)(CommandBuffer [CMDLENGTH]);
      nl = extshort (CMDLENGTH + 1);
      if (nt < NTanks)
        Tanks [nt] -> Fill = (TankCapacity * nl) / MAXFILL;
      proceed WaitCommand;
    }

    if (COMMAND == CMD_GETMAP) {
      CommandBuffer [1] = 0;
      CommandBuffer [2] = 1;
      CommandBuffer [3] = (char) NTanks;
      proceed SendMap;
    }

    if (COMMAND == CMD_DISCONNECT) proceed Disconnect;

    proceed WaitCommand;

  state SendMap:

    if (Sk->write (CommandBuffer, 4)) {
      if (!(Sk->isActive ())) proceed Disconnecting;
      Sk->wait (OUTPUT, SendMap);
    } else {
      dumpBuf ("Sent map: ", CommandBuffer, 4);
      proceed WaitCommand;
    }

  state Disconnect:

    if (Sk->write (CommandBuffer, 3)) {
      if (!(Sk->isActive ())) proceed Disconnecting;
      Sk->wait (OUTPUT, Disconnect);
      sleep;
    }
    Timer->wait ((TIME)(DISCONNECTDELAY * Second), Disconnecting);

  state Disconnecting:

    // trace ("Receiver disconnecting");
    if (Sk->isConnected ()) {
      Sk->erase ();
      if (Sk->disconnect (CLIENT) == ERROR) {
        Timer->wait (500, Disconnecting);
        sleep;
      }
    }
    if (MyUpdater != NULL) {
      // trace ("Receiver waiting for Updater to die");
      MyUpdater->wait (DEATH, Disconnecting);
      sleep;
    }
    // Cleanup
    // trace ("Receiver cleaning up");
    delete CommandBuffer;
    delete Sk;
    if (--NConnections <= 0)
      // The last one turns out the lights
      setLimit (0, Time+IdleTime);
    terminate;
};

void Updater::setup (Socket *sk, Receiver *r) {
  Sk = sk;
  UpdateBuffer = new char [NTanks * TANK_UPDATE_SIZE +
                                                NPumps * PUMP_UPDATE_SIZE + 32];
  UpdateFill = 0;
  LastUpdate = TIME_0;
  TFill = new int [NTanks];
  PStat = new int [NPumps];
  MyReceiver = r;
  reset ();
  // trace ("Updater starting");
};

void Updater::reset () {
  int i;
  // To force update
  for (i = 0; i < NTanks; i++) TFill [i] = -1;
  for (i = 0; i < NPumps; i++) PStat [i] = 99;
};

void Receiver::setup (Socket *msk) {
  Sk = create Socket;
  if (Sk->connect (msk, OPBUFSIZE) != OK) {
    // trace ("Receiver starting (failing)");
    delete Sk;
    terminate ();
    return;
  }
  NConnections++;
  setLimit (0, TIME_0);
  // trace ("Receiver starting (successful)");
  CommandBuffer = new char [CommandBufferLength = MINCMDBUFSIZE];
  MyUpdater = create Updater (Sk, this);
};

void initPanel () {
  int PortNumber;
  double Idle;
  int i;
  readIn (PortNumber);
  readIn (Idle);
  UpdateDelay = (TIME)(Second * DEFAULT_UPDATE_DELAY);
  IdleTime = (TIME)(Second * Idle);
  ForcedUpdateDelay = (TIME)(Second * FORCED_UPDATE_DELAY);
  TheStation = System;
  create Server (PortNumber);
  setLimit (0, IdleTime);
};

#include "types.h"
#include "obsrvrs.h"

TokenMonitor::perform {
  state Resume:
    inspect (ANY, Relay, MyTkn, Verify);
    timeout (TokenPassingTimeout, Lost);
  state Verify:
    inspect (TheStation, Relay, PsTkn, Resume);
    inspect (TheStation, Relay, IgTkn, Resume);
    inspect (ANY, Relay, MyTkn, Duplicate);
    timeout (TokenPassingTimeout, Lost);
  state Duplicate:
    excptn ("Duplicate token");
  state Lost:
    excptn ("Lost token");
};

FairnessMonitor::perform {
  Packet *Buf;
  TIME Delay;
  state Resume:
    inspect (ANY, Relay, IgTkn, CheckDelay);
  state CheckDelay:
    Buf = &(((FStation*)TheStation)->Buffer);
    if (Buf->isFull ()) {
      Delay = Time - Buf->TTime;
      Assert (Delay <= MaxDelay, "Starvation");
    }
    proceed Resume;
};


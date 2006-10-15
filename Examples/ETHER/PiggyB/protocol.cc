#include "types.h"
#include "pxmitter.cc"

identify "Piggyback Ethernet (B)";

Monitor::perform {
  state Waiting:
    Bus->wait (ACTIVITY, Active);
  state Active:
    S->Blocked = YES;
    Bus->wait (EOT, EndPacket);
    Bus->wait (SILENCE, EndActivity);
  state EndActivity:
    // This is after a collision
    Timer->wait (TPSpace, GoSignal);
  state GoSignal:
    if (Bus->busy ()) proceed Active;
    S->Blocked = NO;
    Ready->put ();
    proceed Waiting;
  state EndPacket:
    if (eligible ())
      Timer->wait (DelayP, MyTurn);
    else
      Timer->wait (DelayX, GoSignal);
    Bus->wait (ACTIVITY, Active);
  state MyTurn:
    // Keep it blocked to avoid spontaneous transmissions
    Ready->put ();
    Timer->wait (DelayX-DelayP, GoSignal);
    Bus->wait (ACTIVITY, Active);
};

void Monitor::setup () {
  Bus = S->Bus;
  Ready = S->Ready;
};

Boolean Monitor::eligible () {
  Long Sender, SId;
  Sender = ThePacket->Sender;
  SId = S->getId ();
  if ((S->PiggyDirection = piggyDirection (ThePacket)) == Left) {
    if (SId < Sender)
      DelayP = (Sender - SId) * DelayQuantum;
    else
      DelayP = TIME_0;
    DelayX = 2 * L + (Sender + 1) * DelayQuantum;
  } else {
    if (SId > Sender)
      DelayP = (SId - Sender) * DelayQuantum;
    else
      DelayP = TIME_0;
    DelayX = 2 * L + (NStations - Sender) * DelayQuantum;
  }
  return DelayP != TIME_0;
};

void Transmitter::setPiggyDirection () {
  if (S->getId () == 0)
    setPiggyRight (Buffer);
  else if (S->getId () == NStations - 1)
    setPiggyLeft (Buffer);
  else
    if (flip ()) setPiggyLeft (Buffer); else setPiggyRight (Buffer);
};

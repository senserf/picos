#include "types.h"
#include "pxmitter.cc"

identify "Piggyback Ethernet (A)";

Mtr::perform {
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
    setDelays ();
    Timer->wait (Delay1, FTurn);
    Bus->wait (ACTIVITY, Active);
  state FTurn:
    // Keep it blocked to avoid spontaneous transmissions
    S->Turn = 1;
    Ready->put ();
    Timer->wait (Delay2, STurn);
    Bus->wait (ACTIVITY, Active);
  state STurn:
    S->Turn = 2;
    Ready->put ();
    Timer->wait (DelayX-Delay2-Delay1, GoSignal);
    Bus->wait (ACTIVITY, Active);
};

void Mtr::setup () {
  Bus = S->Bus;
  LDist = S->LDist;
  RDist = S->RDist;
  Ready = S->Ready;
  DelayX = 2 * L + 2 * NStations * DelayQuantum;
};

void Mtr::setDelays () {
  Long Sender, SId;
  Sender = ThePacket->Sender;
  SId = S->getId ();
  if ((S->PiggyDirection = piggyDirection (ThePacket)) == Left) {
    if (SId < Sender) {
      Delay1 = (Sender - SId) * DelayQuantum;
      // Note that the station next to the sender waits for one space unit
      // This allows the sender to piggyback its next packet immediately, if
      // needed
      Delay2 = 2 * (LDist + SId * DelayQuantum) + DelayQuantum;
      // Note: DelayX is precomputed permanently
    } else {
      Delay1 = 2 * ((PiggyStation*) idToStation (Sender))->LDist +
        (SId + Sender + 1) * DelayQuantum;
      Delay2 = 2 * (RDist + (NStations - SId) * DelayQuantum) -
        DelayQuantum;
    }
  } else {
    // Piggyback direction is right
    if (SId > Sender) {
      Delay1 = (SId - Sender) * DelayQuantum;
      Delay2 = 2 * (RDist + (NStations - SId) * DelayQuantum) - DelayQuantum;
    } else {
      Delay1 = 2 * (((PiggyStation*) idToStation (Sender))->RDist) +
        (NStations + NStations - SId - Sender - 1) * DelayQuantum;
      Delay2 = 2 * (LDist + SId * DelayQuantum) + DelayQuantum;
    }
  }
};

void Transmitter::setPiggyDirection () {
  if (S->Blocked) {
    if (S->PiggyDirection == Left && S->Turn == 1 ||
      S->PiggyDirection == Right && S->Turn == 2)
        setPiggyLeft (Buffer);
    else
        setPiggyRight (Buffer);
  } else
    if (flip ()) setPiggyLeft (Buffer); else setPiggyRight (Buffer);
};

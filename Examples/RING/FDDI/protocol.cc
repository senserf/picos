#include "types.h"

identify "FDDI";

TIME TTRT, PrTime;

Transmitter::perform {
  state Xmit:
    TStarted = Time;
    // Start with a preamble
    ORing->sendJam (PrTime, PDone);
  state PDone:
    ORing->stop ();
    ORing->transmit (S->Buffer, EXmit);
  state EXmit:
    ORing->stop ();
    S->Buffer.release ();
    if (S->ready (0, MaxPL, FrameL) &&
     (S->THT += Time - TStarted) < TTRT) 
      // The station has more packets and it is allowed to transmit
      proceed Xmit;
    else
      terminate;
};

Relay::perform {
  Transmitter *Xmitter;
  state Mtr:                         // Wait for a preamble
    IRing->wait (BOJ, SPrm);
  state SPrm:
    ORing->startJam ();              // Copy the preamble
    IRing->wait (EOJ, EPrm);
  state EPrm:
    ORing->stop ();                  // Terminate the preamble
    IRing->wait (BOT, Frm);          // Should be followed by a packet
  state Frm:
    *Relayed = *ThePacket;
    if (Relayed->Sender == S->getId ())
      Relayed->Receiver = NONE;      // Destroy own packet
    ORing->startTransfer (Relayed);  // Start relaying the packet
    skipto WFrm;
  state WFrm:
    IRing->wait (ANYEVENT, EFrm);
  state EFrm:
    if (IRing->events (EOT)) {       // The packet ends normally
      if (Relayed->TP == TOKEN)
        proceed MyTkn;
      else
        ORing->stop ();              // Terminate the packet normally
    } else                           // Aborted packet (token)
      ORing->abort ();
    proceed Mtr;
  state MyTkn:
    S->THT = Time - S->TRT;          // Token rotation time
    S->TRT = Time;                   // The token arrives now
    if (S->ready (0, MaxPL, FrameL) && S->THT < TTRT) {
      ORing->abort ();               // Destroy the token
      Xmitter = create Transmitter;
      Xmitter->wait (DEATH, PsTkn);
    } else
      proceed IgTkn;
  state IgTkn:
    ORing->stop ();                  // Terminate the token normally
    proceed Mtr;
  state PsTkn:                       // Pass the token
    ORing->sendJam (PrTime, PDone);  // Preamble first
  state PDone:
    ORing->stop ();
    ORing->transmit (Relayed, TDone);
  state TDone:
    ORing->stop ();
    proceed Mtr;
};

Receiver::perform {
  state WPacket:
    IRing->wait (EMP, Rcvd);
  state Rcvd:
    Client->receive (ThePacket, ThePort);
    skipto (WPacket);
};

Starter::perform {
  state Start:       // Build and insert the token into the ring
    Token = create Packet;
    Token->fill (NONE, NONE, TokL);
    ORing->sendJam (PrTime, PDone);
  state PDone:
    ORing->stop ();
    ORing->transmit (Token, Stop);
    delete Token;
  state Stop:
    ORing->stop ();
    terminate;
};

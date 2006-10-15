#include "fddi.h"
#include "sring.h"
#include "utraffic.h"

#define TOKEN NONE   // Token packet type

extern TIME TTRT,    // Target token rotation time
            PrTime;  // Preamble insertion time

station FStation : SRingInterface, ClientInterface {
  TIME TRT, THT;
  void setup () {
    SRingInterface::configure ();
    ClientInterface::configure ();
    TRT = THT = TIME_0;
  };
};

process Transmitter (FStation) {
  Port *ORing;
  TIME TStarted;
  void setup () { ORing = S->ORing; };
  states {Xmit, PDone, EXmit};
  perform;
};

process Relay (FStation) {
  Port *IRing, *ORing;
  Packet *Relayed;
  void setup () {
    IRing = S->IRing;
    ORing = S->ORing;
    Relayed = create Packet;
  };
  states {Mtr, SPrm, EPrm, Frm, WFrm, EFrm, MyTkn, IgTkn, PsTkn, PDone, TDone};
  perform;
};

process Receiver (FStation) {
  Port *IRing;
  void setup () { IRing = S->IRing; };
  states {WPacket, Rcvd};
  perform;
};

process Starter (FStation) {
  Port *ORing;
  Packet *Token;
  void setup () { ORing = S->ORing; };
  states {Start, PDone, Stop};
  perform;
};

#include "hbus.h"
#include "dqdb.h"
#include "utraff2l.h"
#include "sprotocl.h"

extern TIME SegmWindow; // Segment window length in ITUs

station HStation : HBusInterface, ClientInterface {
  Mailbox *Strobe [2];  // Go signals from the strobers
  int CD [2],           // The count-down counter (one version per bus)
      RQ [2];           // Request counter
  void setup () {
    int i;
    HBusInterface::configure ();
    ClientInterface::configure ();
    for (i = 0; i < 2; i++) {
      Strobe [i] = create Mailbox (0);
      CD [i] = 0;
      RQ [i] = 0;
    }
  };
};

station HeadEnd : HStation {
  Packet SMarker;
  void setup () {
    HStation::setup ();
    SMarker.fill (NONE, NONE, SlotML);
  };
};

process Transmitter : STransmitter (HStation) {
  int BusId,     // Direction
      *CD,       // Private pointers to station's counters
      *RQ;
  void setup (int dir) {
    Bus = S->Bus [BusId = dir];
    Buffer = &(S->Buffer [dir]);
    Strobe = S->Strobe [dir];
    CD = &(S->CD [dir]);
    RQ = &(S->RQ [dir]);
  };
  Boolean gotPacket ();
};

process Receiver : SReceiver (HStation) {
  void setup (int dir) {
    Bus = S->Bus [dir];
  };
};

process Strober (HStation) {
  Port *Bus;
  Mailbox *Strobe;
  Packet *Buffer;
  Transmitter *OtherXmitter;
  int *MyRQ, *OtherRQ, *MyCD;
  void setup (int dir, Transmitter *pr) {
    Bus = S->Bus [dir];
    OtherXmitter = pr;
    Strobe = S->Strobe [dir];
    Buffer = &(S->Buffer [dir]);
    MyRQ = &(S->RQ [dir]);
    MyCD = &(S->CD [dir]);
    OtherRQ = &(S->RQ [1-dir]);
  };
  states {WaitSlot, WaitLoop};
  perform;
};

process SlotGen (HeadEnd) {
  Port *Bus;
  Packet *SMarker;
  void setup (int dir) {
    Bus = S->Bus [dir];
    SMarker = &(S->SMarker);
  };
  states {Generate, XDone};
  perform;
};
  

#include "sbus.h"
#include "utraffic.h"

extern Long MinPL,    // Minimum packet length (payload)
            MaxPL,    // Maximum packet length
            FrameL,   // Header and trailer (preamble excluded)
            PrmbL;    // Preamble length

extern TIME EOTDelay; // Amount of silence needed to detect the end of train 

station ExStation : SBusInterface, ClientInterface {
  Packet Preamble;
  void setup () {
    SBusInterface::configure ();
    ClientInterface::configure ();
    Preamble.fill (NONE, NONE, PrmbL);
  };
};

process EOTMonitor (ExStation) {
  Port *Bus;
  void setup () {
    Bus = S->IBus;
  };
  states {Wait, Count, Signal, Retry};
  perform;
};

process Transmitter (ExStation) {
  Packet *Preamble, *Buffer;
  Port *Bus;
  EOTMonitor *EOTrain;
  void setup (EOTMonitor *et = NULL) {
    Bus = S->OBus;
    Buffer = &(S->Buffer);
    Preamble = &(S->Preamble);
    EOTrain = et;
  };
  states {Wait, CheckBuf, Transmit, Yield, PDone, XDone, Error};
  perform;
};

process Receiver (ExStation) {
  Port *Bus;
  void setup () {
    Bus = S->IBus;
  };
  states {WPacket, Rcvd};
  perform;
};

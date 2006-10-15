#include "types.h"

identify Expressnet;

Long MinPL,    // Minimum packet length (payload)
     MaxPL,    // Maximum packet length
     FrameL,   // Header and trailer (preamble excluded)
     PrmbL;    // Preamble length

TIME EOTDelay; // Amount of silence needed to detect the end of train 

EOTMonitor::perform {
  state Wait:
    Bus->wait (EOT, Count);
  state Count:
    Timer->wait (EOTDelay, Signal);
    Bus->wait (ACTIVITY, Retry);
  state Signal:
    if (signal () != ACCEPTED) excptn ("End of train signal not accepted");
  transient Retry:
    skipto Wait;
};

Transmitter::perform {
  state Wait:
    Bus->wait (EOT, CheckBuf);
    if (EOTrain != NULL)
      EOTrain->wait (SIGNAL, Transmit);    // On end of train
  state CheckBuf:
    if (S->ready (MinPL, MaxPL, FrameL))
      proceed Transmit;
    else
      skipto Wait;
  state Transmit:
    Bus->transmit (Preamble, PDone);
    Bus->wait (COLLISION, Yield);
  state Yield:
    Bus->abort ();
    skipto Wait;
  state PDone:
    if (S->ready (MinPL, MaxPL, FrameL)) {
      Bus->abort ();  // The preamble won't trigger EOC
      Bus->transmit (Buffer, XDone);
      Bus->wait (COLLISION, Error);
    } else {
      Bus->stop ();   // Forced preamble - must trigger EOC
      skipto Wait;
    }
  state XDone:
    Bus->stop ();
    Buffer->release ();
    skipto Wait;
  state Error:
    excptn ("Illegal collision");
};
      
Receiver::perform {
  state WPacket:
    Bus->wait (EMP, Rcvd);
  state Rcvd:
    Client->receive (ThePacket, ThePort);
    skipto (WPacket);
};

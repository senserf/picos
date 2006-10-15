#include "board.h"

// Traffic generator interface

station Source : SUnit {
  Box Buffer;
  DISTANCE Space;             // Box spacing
  Long *NXmBoxes,             // Total number of transmitted boxes
       *NXmCmtrs;             // And their combined length in cm (per TP)
  void setup (int, double);
  void updateCount (Box*);    // Do some bookkeeping
};

station Sink : SUnit {
  Alert *Exception;
  Long *NRcvBoxes,            // Total number of received boxes per TP
       *NRcvCmtrs;            // And their combined length in cm (per TP)
  int BoxType;                // Expected box type
  void setup (int, int);
  void updateCount (Box*);    // Do some bookkeeping
};

traffic BoxGen (Message, Box) {
  // Standard attributes:
  //                      MIT  - in seconds (automatic)
  //                      MLE  - in umetres (converted from input)
  //                      BIT  - in seconds (automatic)
  //                      BSI  - count (natural)
};

process SourceTransmitter : Overrideable (Source) {
  // The process is overrideable, meaning that we can stop it and resume.
  // However, we won't stop this way the incoming traffic that will be
  // piling up in the queue.
  Box *Buffer,   // Private copy
      *OutgoingBox;
  Process *RP;   // The recipient
  states {NextBox, OverrideResume, SpaceDelay, ProcessOverride};
  void setup ();
  perform;
};

process SinkReceiver : Overrideable (Sink) {
  states {WaitBox, NewBox, ProcessOverride};
  void setup ();
  perform;
};

extern int NSources, NSinks;

extern Source **Sources;

extern Sink **Sinks;

void initIo (), startIo (), outputIo ();

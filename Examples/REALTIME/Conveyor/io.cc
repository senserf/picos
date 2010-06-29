#include "io.h"

int NSources = 0,           // The number of inputs
    NSinks = 0;             // The number of outputs

Source **Sources;
Sink   **Sinks;

void Source::setup (int sn, double spacing) {
  Sources [NSources++] = this;
  SUnit::setup (sn);    // Plug it in
  Space = (DISTANCE) (Second * spacing);
  NXmBoxes = NXmCmtrs = NULL;
  create SourceTransmitter;
};

void Sink::setup (int sn, int bt) {
  Sinks [NSinks++] = this;
  SUnit::setup ();      // Initialize the SUnit part
  SSegments [sn] -> NU = this;
  NRcvBoxes = NRcvCmtrs = NULL;  // Will be created upon first reception
  RP = create SinkReceiver;
  BoxType = bt;
  Exception = create Alert (form ("+Sink#%1d", NSinks-1));
};

void Source::updateCount (Box *b) {
  int i;
  if (NXmBoxes == NULL) {
    NXmBoxes = new Long [NTraffics];
    NXmCmtrs = new Long [NTraffics];
    for (i = 0; i < NTraffics; i++) NXmBoxes [i] = NXmCmtrs [i] = 0;
  }
  NXmBoxes [b->TP] ++;
  NXmCmtrs [b->TP] += b->TLength / 10000;  // This is in cm
};

void Sink::updateCount (Box *b) {
  int i;
  if (NRcvBoxes == NULL) {
    NRcvBoxes = new Long [NTraffics];
    NRcvCmtrs = new Long [NTraffics];
    for (i = 0; i < NTraffics; i++) NRcvBoxes [i] = NRcvCmtrs [i] = 0;
  }
  NRcvBoxes [b->TP] ++;
  NRcvCmtrs [b->TP] += b->TLength / 10000;  // This is in cm
};

void SourceTransmitter::setup () {
  Overrideable::setup (form ("+TrfGen@%s", S->getOName ()));
  Buffer = &(S->Buffer);
  RP = S->NU->RP;
  OutgoingBox = NULL;
};

void SinkReceiver::setup () {
  Overrideable::setup (form ("+TrfSnk@%s", S->getOName ()));
};

SourceTransmitter::perform {

  int res;

  state NextBox:

    onOverride (ProcessOverride);

    if (OutgoingBox == NULL) {
      if (!Client->getPacket (Buffer)) {
        // Have to wait for a box arrival
        Client->wait (ARRIVAL, NextBox);
        sleep;
      }
      // Note that Buffer is only used for acquisition. We must copy the
      // packet into a separate data structure before we can dispatch it.
      OutgoingBox = create Box;
      *OutgoingBox = *Buffer;
      S->updateCount (OutgoingBox);
      Buffer->release ();
    }

    // We have got a new outgoing packet

    if (RP->isSignal ()) {
      // The recipient is busy
      RP->wait (CLEAR, NextBox);
      sleep;
    }

    res = RP->signal (OutgoingBox);
    Assert (res != REJECTED, "SourceTransmitter: receiving belt not ready");
    // This should not happen

    OutgoingBox = NULL;

  transient OverrideResume:

    // Wait until the box is absorbed
    onOverride (ProcessOverride);
    RP->wait (CLEAR, SpaceDelay);

  state SpaceDelay:

    Timer->wait (S->Space, NextBox);

  state ProcessOverride:

    overrideAcknowledge ();
    switch (overrideAction ()) {
      case OVR_HOLD:
        // Wait until unblocked
        onOverride (ProcessOverride);
        sleep;
      case OVR_RESUME:
      default:
        proceed OverrideResume;
    }
};

SinkReceiver::perform {

  Box *b;

  state WaitBox:

    onOverride (ProcessOverride);
    this->wait (SIGNAL, NewBox);

  state NewBox:

    b = (Box*)TheSignal;
    S->updateCount (b);
    Client->receive (b);
    if (b->TP != S->BoxType)
      S->Exception->notify (X_WARNING, form ("Illegal box received, type %1d",
        b->TP));
    // Deallocate the box
    delete (b);

    this->erase ();

    proceed WaitBox;

  state ProcessOverride:

    overrideAcknowledge ();
    // Override signals are ignored at present and exist at all for
    // compatibility with segments/diverters, because a unit may
    // occasionally force an override action for its successor.
    proceed WaitBox;
};

void initIo () {
  int s1, i, j, nt, np, apdesc, nsources, nsinks;
  double trpar [8], minsp;
  BoxGen *tr;

  readIn (nsources);
  readIn (nsinks);

  Sources = new Source* [nsources];
  for (i = 0; i < nsources; i++) {
    readIn (s1); // The segment where the source is interfaced
    readIn (minsp);   // Minimum box spacing in seconds
    Assert (s1 >= 0 && s1 < NSegments, "initIo: illegal output segment number");
    Sources [i] = create Source, form ("BoxSrc#%1d", i), (s1, minsp);
  }
  Sinks = new Sink* [nsinks];
  for (i = 0; i < nsinks; i++) {
    readIn (s1);      // The segment from where boxes are being received
    Assert (s1 >= 0 && s1 < NSegments, "initIo: illegal input segment number");
    Assert (SSegments [s1] -> NU == NULL,
                                    "initIo: input segment already connected");
    Sinks [i] = create Sink, form ("BoxSnk#%1d", i), (s1, i);
  }
  readIn (nt);   // The number of box types
  for (i = 0; i < nt; i++) {         // Create traffic pattern #i (box type #i)
    readIn (apdesc);
        // =====================================================
        // Arrival process descriptor, flags copied from SMURPH:
        // =====================================================
        //              MIT_exp
        //              MIT_unf
        //              MIT_fix
        //              MLE_exp
        //              MLE_unf
        //              MLE_fix
        //              BIT_exp
        //              BIT_unf
        //              BIT_fix
        //              BSI_exp
        //              BSI_unf
        //              BSI_fix
        //              SCL_on
        //              SCL_off
        //              SPF_on
        //              SPF_off
    np = 0;
    readIn (trpar [np++]);       // Mean interarrival time
    if (apdesc & MIT_unf) readIn (trpar [np++]);    // Second value
    readIn (trpar [np]);         // Length
    trpar [np++] *= Second;      // Turn this into umetres
    if (apdesc & MLE_unf) {
      readIn (trpar [np]);       // Second value
      trpar [np++] *= Second;
    }
    if (apdesc & (BIT_exp | BIT_unf | BIT_fix)) {   // Bursty
      readIn (trpar [np++]);     // Burst interarrival time
      if (apdesc & BIT_unf) readIn (trpar [np++]);    // Second value
      readIn (trpar [np++]);     // Burst size
      if (apdesc & BSI_unf) readIn (trpar [np++]);    // Second value
    }
    // Standard client ON, performance measures ON
    tr = create BoxGen (apdesc, trpar [0], trpar [1], trpar [2], trpar [3],
                                trpar [4], trpar [5], trpar [6], trpar [7]);
    // Distribution of senders
    readIn (np);                 // The number of senders
    for (j = 0; j < np; j++) {
      readIn (s1);               // Next sender
      // This must be a legitimate Source number
      Assert (s1 >= 0 && s1 < NSources, "initIo: illegal source number");
      tr->addSender (Sources [s1] -> getId ());
    }
    // All sinks are legitimate receivers, because we don't know where the
    // box is going to end up. Of course, we may check it upon reception.
    for (j = 0; j < NSinks; j++)
      tr->addReceiver (Sinks [j] -> getId (), BROADCAST);
    // We are done with this traffic pattern
  }
  Client->suspend ();
};

void startIo () {
  Client->resume ();
  Client->resetSPF ();
};

void outputIo () {
  // Standard measures go first
  Source *src;
  Sink *snk;
  int i, j;
  Long bx, mx, bc, mc;
  if (NTraffics > 1) {
    // Individual statistics per traffic pattern
    for (i = 0; i < NTraffics; i++) idToTraffic (i) ->printPfm ();
  }
  Client->printPfm ();

  // Sent boxes per Source

  print ("Boxes expedited per source/traffic pattern:\n\n");

  for (i = 0; i < NSources; i++) {
    src = Sources [i];
    Ouf << "    Source "
          << form ("%2d", i) << ":  Traffic    Boxes     Metres\n";
    bc = mc = 0;
    for (j = 0; j < NTraffics; j++) {
      if (src->NXmBoxes != NULL) {
        bx = src->NXmBoxes [j];
        mx = src->NXmCmtrs [j];
      } else
        bx = mx = 0;
      bc += bx;
      mc += mx;
      Ouf << "               " << form ("%8d", j) << form (" %8d", bx)
          << form (" %10d\n", mx);
    }
    Ouf << "                      ---------------------\n";
    Ouf << "                       " << form (" %8d", bc)
                                     << form (" %10d\n\n", mc);  // Total
  }

  print ("Boxes received per sink/traffic pattern:\n\n");

  for (i = 0; i < NSinks; i++) {
    snk = Sinks [i];
    Ouf << "      Sink " 
          << form ("%2d", i) << ":  Traffic    Boxes     Metres\n";
    bc = mc = 0;
    for (j = 0; j < NTraffics; j++) {
      if (snk->NRcvBoxes != NULL) {
        bx = snk->NRcvBoxes [j];
        mx = snk->NRcvCmtrs [j];
      } else 
        bx = mx = 0;
      bc += bx; 
      mc += mx;
      Ouf << "               " << form ("%8d", j) << form (" %8d", bx)
          << form (" %10d\n", mx);
    }
    Ouf << "                      ---------------------\n";
    Ouf << "                       " << form (" %8d", bc)
                                     << form (" %10d\n\n", mc);  // Total
  }
};

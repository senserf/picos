#include "ether.h"
#include "lbus.h"
#include "utraffic.h"
#include "etherstn.h"
#include "etherrcv.h"

extern TIME SlotLength,      // The length of a virtual slot (2L + epsilon)
            GuardDelay,      // To compensate for non-zero clock tolerance
            TPSpace,         // Precomputed TIME versions of PSpace and JamL
            TJamL;

extern RATE TRate;           // Time granularity (read from data file)

process Transmitter (EtherStation) {
  Port *Bus;          // A copy of the bus port
  Packet *Buffer;     // Packet buffer pointer
  Boolean TournamentInProgress,
          Transmitting,
          Competing;
  int Ply,            // The level at which the station is competing
      DelayCount,     // Waiting time (in slots) for a loser
      DeferCount;     // Waiting time for a 'permanent loser'
  Boolean loser ();   // Tells whether the station loses in the current move
  void setup () {
    Bus = S->Bus;
    Buffer = &(S->Buffer);
    TournamentInProgress = Transmitting = Competing = NO;
  };
  states {NPacket, Transmit, Busy, XDone, SenseEOT, SenseCollision, EndJam,
          NewSlot, EndSlot};
  perform;
};

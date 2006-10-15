#ifndef __mring_h__
#define __mring_h__

// Type announcements for 'mring.cc': Metaring configuration consisting of four
// independent rings. Two of those rings represent virtual channels used for
// SAT communicates.

#define CWRing 0   // Clockwise ring
#define CCRing 1   // Counter-clockwise ring

station MRingInterface virtual {
  Port *IRing [2], *ORing [2], *ISat [2], *OSat [2];
  void configure ();
};

void initMRing (RATE, TIME, int);

#endif

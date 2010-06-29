#ifndef __fddi_h__
#define __fddi_h__

// Parameters of commercial FDDI

#define MaxPL    36000    // Maximum payload length in bits (frame excluded)
#define FrameL     160    // The combined length of packet header and trailer
#define PrmbL       64    // The length of a packet preamble
#define TokL        24    // Token length, preamble excluded

#define CTolerance 0.0001 // Clock tolerance

#endif

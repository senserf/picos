#ifndef __dqdb_h__
#define __dqdb_h__

// Constants associated with commercial DQDB

#define SlotML 8                    // Slot marker length in bits
#define SegmPL 384                  // Segment payload length in bits
#define SegmFL 32                   // Segment header length in bits
#define SegmWL (SegmPL+SegmFL+2)    // Segment window length in bits

#define CTolerance  0.0001          // Clock tolerance

#define SLOT NONE       // The type of the packet representing slot markers
#define FULL PF_usr0    // The full/empty status of the slot
#define RQST PF_usr1    // The request flag in the slot marker

#endif



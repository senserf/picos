#define MinPL      368    // Minimum payload length in bits (frame excluded)
#define MaxPL    12000    // Maximum payload length in bits (frame excluded)
#define FrameL     208    // The combined length of packet header and trailer

#define PSpace      96    // Inter-packet space length in bits
#define JamL        32    // Length of the jamming signal in bits
#define TwoL       512    // Maximum round-trip delay in bits

#define CTolerance 0.0001 // Clock tolerance

#define TRate        1    // Transmission rate: 1 ITU per bit

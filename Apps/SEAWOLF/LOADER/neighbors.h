#ifndef	__neighbors_pg_h
#define	__neihgbors_pg_h

#include "sysio.h"
#include "params.h"

typedef struct {

	lword esn;	// ESN of the neighbor
	word when;	// Lower 16 bit of the second when last heard

} neighbor_t;

#define	NAGFREQ		4096	// Run frequency for the neighbor ager

// Cleanups flags
#define	CLEAN_NEI	0x01	// Own table
#define	CLEAN_NEI_LIST	0x02	// List received from elsewhere

// Receives HELLO packets
void hello_in (address);

void neighbors_status (address);
void neighbors_clean (byte);

// Neighbor ager thread
procname (nager);

// Transaction plug-in
extern const fun_pak_t nei_pak;


#endif
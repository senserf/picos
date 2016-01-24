#ifndef __network_h__
#define __network_h__

extern int channel_type;
extern sxml_t xml_data;

#define	CTYPE_SHADOWING		0
#define	CTYPE_SAMPLED		1
#define	CTYPE_NEUTRINO		2

#define NPTABLE_SIZE    	(3*2*128)

Long initNetwork ();

void setrfpowr (Transceiver*, unsigned short);
void setrfrate (Transceiver*, unsigned short);
void setrfchan (Transceiver*, unsigned short);

#endif

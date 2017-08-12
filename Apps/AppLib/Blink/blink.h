#ifndef	__pg_blink_h
#define	__pg_blink_h

//
// Blink a LED asynchronously in the background:
//
//	Arguments:
//
//		- the led to blink (0-3)
//		- how many times (up to 255)
//		- on time in msecs
//		- off time in msecs
//		- space time in msecs after completing the cycle
//
void blink (byte, byte, word, word, word);

//+++ "blink.cc"

#endif

#include "pins_sys.h"

// ============================================
// Function to access output ports by pin index
// ============================================

volatile byte *__port_out (word p) {

	switch (p) {

#ifdef __PORT0_PRESENT__
		case 0: return &P0OUT;
#endif
#ifdef __PORT1_PRESENT__
		case 1: return &P1OUT;
#endif
#ifdef __PORT2_PRESENT__
		case 2: return &P2OUT;
#endif
#ifdef __PORT3_PRESENT__
		case 3: return &P3OUT;
#endif
#ifdef __PORT4_PRESENT__
		case 4: return &P4OUT;
#endif
#ifdef __PORT5_PRESENT__
		case 5: return &P5OUT;
#endif
#ifdef __PORT6_PRESENT__
		case 6: return &P6OUT;
#endif
#ifdef __PORT7_PRESENT__
		case 7: return &P7OUT;
#endif
#ifdef __PORT8_PRESENT__
		case 8: return &P8OUT;
#endif
#ifdef __PORT9_PRESENT__
		case 9: return &P9OUT;
#endif
#ifdef __PORT10_PRESENT__
		case 10: return &P10OUT;
#endif
#ifdef __PORT11_PRESENT__
		case 11: return &P11OUT;
#endif
	}

	return NULL;
}

#include "pins_sys.h"

// ===========================================
// Function to access input ports by pin index
// ===========================================

byte __port_in (word p) {

	switch (p) {

#ifdef __PORT0_PRESENT__
		case 0: return P0IN;
#endif
#ifdef __PORT1_PRESENT__
		case 1: return P1IN;
#endif
#ifdef __PORT2_PRESENT__
		case 2: return P2IN;
#endif
#ifdef __PORT3_PRESENT__
		case 3: return P3IN;
#endif
#ifdef __PORT4_PRESENT__
		case 4: return P4IN;
#endif
#ifdef __PORT5_PRESENT__
		case 5: return P5IN;
#endif
#ifdef __PORT6_PRESENT__
		case 6: return P6IN;
#endif
#ifdef __PORT7_PRESENT__
		case 7: return P7IN;
#endif
#ifdef __PORT8_PRESENT__
		case 8: return P8IN;
#endif
#ifdef __PORT9_PRESENT__
		case 9: return P9IN;
#endif
#ifdef __PORT10_PRESENT__
		case 10: return P10IN;
#endif
#ifdef __PORT11_PRESENT__
		case 11: return P11IN;
#endif
	}

	return 0;
}

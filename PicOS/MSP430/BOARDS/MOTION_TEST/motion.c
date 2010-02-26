#include "sysio.h"

word	MotionCounter;

word motion_status () {

	word res;

	cli;
	res = MotionCounter;
	MotionCounter = 0;
	sti;

	return res;
}

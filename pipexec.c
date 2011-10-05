#include <unistd.h>

main (int argc, char *const argv []) {

	// Replicate stderr over stdout
	close (2);
	dup (1);

	execv (argv [1], argv + 1);
}

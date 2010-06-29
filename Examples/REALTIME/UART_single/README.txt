Compile with:

	mks -R -D -S

This program shows you how to work (asynchronously) with a UART. It spawns a
thread which asks you for the UART device name and then spawns another thread.
The old thread reads stuff from the TTY (standard input) echoing it to the
UART, while the new thread takes care of the opposite end, i.e., copying stuff
from the UART to the terminal.

The UART is open as a RAW device assuming the baud rate of 9600, no parity, one
stop bit.

Remember that UARTs on Cygwin are named /dev/com1, /dev/com2, ...

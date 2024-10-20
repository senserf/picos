This should be run in ez-chronos with the AP praxis (see a neighboring
directory) in a nearby Warsaw node. The two will communicate using XRS over RF.
The AP praxis reads straightforward commands (lines) from its UART and echoes
the watch's responses to the UART thus acting as a UART proxy for ez-chronos.

Note that the first command sent by AP to ez-chronos may be ignored.

The praxis presents no HELP menu.

===============================================================================

Here is the list of commands:

s n

Starts sensor reports, n is the interval in milliseconds. The reports will
arrive automatically at the specified interval.

A single report looks like this:

	MO: D E, PR: P T

All values: D, E, P, T are unsigned decimals, P is long (i.e., a lword).

	D  is the timestamp of the last movement event as the number of
	   seconds ago

	E  is the number of movement events observed since last time the
	   sensor was read (note that D is zero if E is zero)

	P  is the pressure readout; the actual pressure in Pascals is equal
	   to P divided by 4

	T  is the 14-bit temperature readout; the value in degrees Celsius
	   is calculated as follows:

		if T is less than 0x2000 (i.e., the most significant bit is
		zero), the temperature is nonnegative and equal to T/20;

		otherwise, the readaout is 2's complement negative; the temp
		is obtained by xorring in 0xC000, treating the number as int,
		and dividing it by 20.
		
-------------------------------------------------------------------------------

q

Stops sensor reports.

-------------------------------------------------------------------------------

d delay times

Enter a sleep (power down) state with the RF module switched off. Used to
measure power consumption in the sleep state. The delay argument is the amount
of time in milliseconds for which the state should be assumed, and times is the
number of times this should happen. At the end of each turn, the device will
switch on the RF module for 1000 milliseconds and then (for as long as there
are more turns to process) execute the next turn (for delay milliseconds).
After all the prescribed turns have been executed, the device returns to
normal.

-------------------------------------------------------------------------------

D nsec

Enters a "dumb" power down state for the specified number of seconds. The
device remains frozen for that much time.

-------------------------------------------------------------------------------

r year month day dow hour minute second

Sets the real-time clock. The year is supposed to be modulo 100 (only the LS
byte of the specified int is used). dow is the day of the week, e.g., 0 meaning
Sunday, 1 meaning Monday, and so on.

If the number of arguments is less than 7 (e.g., zero), the command ignores
them and instead displays the current readings of the clock.

-------------------------------------------------------------------------------

b time

Activates the buzzer for the prescribed number of milliseconds.

-------------------------------------------------------------------------------

fr a

Read FIM word number a (specified as unsigned decimal)

-------------------------------------------------------------------------------

fw a b

Write word value b (unsigned decimal) at FIM location a (usigned decimal)

-------------------------------------------------------------------------------

fe a

Erase FIM block containing location a

-------------------------------------------------------------------------------

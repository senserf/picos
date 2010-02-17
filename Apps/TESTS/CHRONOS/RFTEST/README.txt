This should be run in ez-chronos with the AP praxis (see a neighboring
directory) in a nearby Warsaw node. The two will communicate using XRS over RF.
The AP praxis reads straightforward commands (lines) from its UART and echoes
the watch's responses to the UART thus acting as a UART proxy for ez-chronos.

Note that the first command sent by AP to ez-chronos may be ignored.

Commands:

	s n	- start reporting sensor data at n msec intervals

	q	- stop reporting sensor data

	d n c	- enter low-power mode for c turns n msecs each
		  sending a short message at every turn (switching on the
		  radio for 1 sec)

	r ...   - show (no arguments) or set the RTC; in the latter case,
		  the arguments are: year (last two digits), month, day,
		  day of week, hour, minute, second

===============================================================================

Sensor value reports:

	MO: D E, PR: P T

All values: D, E, P, T are unsigned decimals, P is long (i.e., a lword).

	D  is the timestamp of the last movement event as the number of
	   seconds ago

	E  is the number of movement events observed since last time the
	   sensor was read (note that D is zero if E is zero)

	P  is the pressure readout; the actual pressure in Pascals is equal
	   to P divided by 4

	T  is the temperature; the value in degrees Celsius is equal to
	   T divided by 20

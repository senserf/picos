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

	MO: DDDD EEEE, PR: PPPP QQQQ TTTT

All values: DDDD, EEEE, PPPP, QQQQ, TTTT are hex numbers:

	DDDD   is the timestamp of the last movement event as the number of
	       seconds ago

	EEEE   is the number of movement events observed since last time the
	       sensor was read (note that DDDD is zero if EEEE is zero)

	PPPP   is the more significant part of the pressure readout

	QQQQ   is the less significant part of the pressure readout; the
	       actual pressure in Pascals is equal to the combined number
	       divided by 4

	TTTT   is the temperature; the value in degrees Celsius is equal to
	       that number divided by 20

	

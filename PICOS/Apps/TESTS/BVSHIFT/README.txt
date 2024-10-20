Compile with the WARSAW board. Before compilation, edit app.cc and look at the
constants at the top named BATTERY_... Select the pin for controlling the
voltage regulator according to the table in BOARDS/WARSAW/board_pins.h. This
is how it looks:

#define	PIN_LIST	{	\
	PIN_DEF	(P6, 0),	\
	PIN_DEF	(P6, 1),	\
	PIN_DEF	(P6, 2),	\
	PIN_DEF	(P6, 3),	\
	PIN_DEF	(P6, 4),	\
	PIN_DEF	(P6, 5),	\
	PIN_DEF	(P6, 6),	\
	PIN_DEF	(P6, 7),	\
	PIN_DEF	(P1, 6),	\
	PIN_DEF	(P1, 7),	\
	PIN_DEF	(P2, 2),	\
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P4, 0),	\
	PIN_DEF	(P4, 4),	\
	PIN_DEF	(P4, 5),	\
	PIN_DEF	(P4, 6),	\
	PIN_DEF	(P4, 7),	\
	PIN_DEF (P5, 4)		\
}

So the default setting of BATTERY_PIN (9) corresponds to P1.7 (the 9-th pin in
the list counting from zero).

The setting BATTERY_ARMED = YES means that the pin is initially "armed", i.e.,
set to high, and that it will be automatically set to low when the battery
"status", i.e., voltage (the internal voltage sensor indication), goes below
BATTERY_THRESHOLD.

BATTERY_INTERVAL tells the initial (default) setting of the check interval in
seconds. It can be anything between 0 and 60. 0 means "disabled". Note that the
default setting is 4 seconds.

To communicate with the praxis, execute:

	oss

in THIS directory, then hit "Connect". You should be seeing status reports
every 4 seconds (BATTERY_INTERVAL). This command:

	battery -threshold n -interval k -pin p

changes the parameters. You can do it individually, e.g.,

	battery -interval 16

This command:

	battery -arm

arms the regulator, i.e., sets the pin to high. Note that the pin will
immediately go down if status (the voltage) is less than threshold, so you
have to make sure first that the voltage is higher than threshold. For example,
you can do:

	battery -arm -threshold 2222

to arm the regulator and set the threshold at the same time.

The praxis has been derived from EXAMPLES/OSS, so it includes the RF ping
functionality described in oss.pdf (in PICOS/Docs). Note that the praxis uses
the new OSS interface (described in that document). You have to "deploy" PICOS
to be able to invoke oss (as prescribed above).

Note that the way to interpret status (voltage indications) is:

	Voltage = status * 0.001221

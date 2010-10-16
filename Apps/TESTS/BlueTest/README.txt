Testing LinkMatik 2.0 using a laptop with BlueTooth capability:
===============================================================

Start a terminal emulator, connect to the board at 115200. This is the
target rate of the BlueTooth module, and it makes sense to keep the
standard UART rate the same. WARSAW_BLUE has 115200 as the default UART
rate.

Whatever the BlueTooth module receives is immediately echoed to the UART.

You should be able to enter these commands:

w string
--------

This will write the string to the module. If the module is in CONNECTED state,
the line will go over the BlueTooth link to the other party, otherwise, it
only makes sense to use w to write a set command to the module.

Digression: here is a manual describing the command set:

http://www.flexipanel.com/Docs/LinkMatik%202.0%20Reference%20DS389.pdf

You don't need to know much of it. I don't understand most of those
commands. You don't want to look at this manual at all, unless something
goes wrong and you are desperate.

r rate (e.g., r 96)
-------------------

This sets the UART rate for the module. It only makes sense, if you have
previously issued a set command to the module to reset the rate, e.g.,

	w SET CONTROL BAUD 9600,8n1
	r 96

Following the initialization (see below) you don't ever have to do that.

t rate (e.g., t 96)
-------------------

Changes the rate for UART. The UART rate need not be the same as the BlueTooth
rate.

e (0 or 1)
----------

Controls the ESCAPE signal. The default is 0.

a
-

Shows the value of the ATTENTION signal (which is input).

s
-

Resets the module.

-------------------------------------------------------------------------------

The first time around (for a brand new LinkMatik module that you are checking
for the first time) do this:

	r 96

This is because the default (factory) rate of the module is 9600 bps. Then
check if the module responds by sending this:

	w set

in response to which you should see some list of SET values. If this doesn't
work, there is something wrong with the connection. If it does, issue this
next:

	w set CONTROL BAUD 115200,8n1

which should change the module's rate to 115200, followed by:

	r 1152

to make sure that the second UART's rate has been changed as well to match.
Now try again:

	w set

which should produce the same list of SET values as before except for the
modified bit rate. Now issue these commands:

	w set BT NAME DUPA1		[can be something else]
	w set BT CLASS 301f00
	w set BT AUTH * 1234

The assigned name is the name under which the module will be visible by other
BlueTooth devices. The settings will stay when you disconnected the module, and
you will see the same settings when you switch it back on.

Switch on the laptop and make sure that BlueTooth is on. I am assuming XP.

Right click on the BlueTooth mini-icon in the bottom right corner (on the task
bar), select "Add a BlueTooth device".

When you see the device, double click on it. On my laptop, the service window
shows no services, but when I cancel it, I get another window that asks me to
pair the device by entering the code (1234 - see above). When I do that, I get
a new COM port (in the device list) labeled BlueTooth Communication Port. When
the module gets paired, it says (on the node's UART):

	CONNECT 0 RFCOMM ...

Now you can open a terminal emulator on the laptop's new UART (make sure the
rate is 115200. Whatever you write there, shows up on the module, and vice
versa.

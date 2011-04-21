Section 1: Testing LinkMatik 2.0 using a laptop with BlueTooth capability:
===============================================================================

Note: a board using the LinkMatik module must be compiled with the LINKMATIC
symbol set, e.g.,

#define LINKMATIC

Put this definition at the very front of the praxis's options.sys file.

The board is WARSAW_BLUE.

===============================================================================

Start a terminal emulator, connect to the board at 115200. This is the
target rate of the BlueTooth module, and it makes sense to keep the
standard UART rate the same. WARSAW_BLUE with LINKMATIC defined has 115200 as
the default UART rate.

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

Section 2: Testing BT-182
===============================================================================

Note: a board using the BT-182 module must be compiled with the LINKMATIC
symbol undefined. Make sure #define LINKMATIC is commented out in options.sys.

The board is WARSAW_BLUE (the same as for LinkMatik).

===============================================================================

Everything is basically the same, except that the commands to set the module up
are different (they resemble a conversation with an old modem). The standard
UART rate assumed by the praxis is 19200, which coincides with the default UART
rate for BT-182.

Issue these commands:

w ATC0			[to disable hardware flow control]
w ATP=1234		[to set up a PID for connections]
w ATN=Seawolf Dongle	[to set up a device identifier]
w ATX0			[to disable the +++ escape sequence]
w ATL5			[to change the UART rate to 115200]

Note that following the last command, you will have to execute

t 1152

Here is the way to list pretty much all the relevant settings:

w ATI1

You get:

ATI1
OK
ATC=0, NONE FLOW CONTROL
ATD=0000-DR
000000, NEVER SET BLUETOOTH ADR
ATE=1, ECHO CHARACTERS
ATG=1, ENABLE ALL PAGE AND INQUIRY SCAN
ATH=1, DISCOVERABLE
ATK=0, ONE STOP BIT
ATL=5, BAUD RATE is 115200
ATM=0, NONE PARITY_BIT
ATN=Seawolf Dongle, LOCAL NAME
ATO=0, ENABLE  AUTO CONNECTING
ATP=1234, PIN CODE 
ATQ=0, SEND RESULT CODE 
ATR=1, SPP SLAVE ROLE 
ATS=1, ENVRLE AUTO-POWERDOWN OF RS232 DRIVRATX=0, NEVER CHECK '+++' 

This is what I get after the prescribed initialization sequence. Following that,
everything works exactly as for LinkMatik.

A potentially useful command is ATZ0 (i.e., w ATZ0), which reverts the module
to factory setting (retaining the name and PIN, however).

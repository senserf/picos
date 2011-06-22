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

e [n]
----------

Controls the ESCAPE signal. Without an argument, sets the escape signal for 10
ms, which should be a sufficient interval to carry out the normal escape
function. With an argument, e.g., e 5, sets the signal for the specified number
of seconds. For BTM-182, e 5 will reset the baud rate to the factory setting of
19200 (ATL2).

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

My settings:

SET BT BDADDR 00:07:80:81:12:8a
SET BT NAME cipka10
SET BT CLASS 001f00
SET BT AUTH * 1234
SET BT LAP 9e8b33
SET BT PAGEMODE 4 2000 1
SET BT ROLE 0 f 7d00
SET BT SNIFF 0 20 1 8
SET CONTROL BAUD 115200,8n1
SET CONTROL CD 04 0
SET CONTROL ECHO 4
SET CONTROL ESCAPE 0 00 1

If you see something like:

SET CONTROL AUTOCALL 1101 5000

disable it by writing:

w SET CONTROL AUTOCALL

Section 2: Testing BTM-182
===============================================================================

Note: a board using the BTM-182 module must be compiled with the LINKMATIC
symbol undefined. Make sure #define LINKMATIC is commented out in options.sys.

The board is WARSAW_BLUE (the same as for LinkMatik).

===============================================================================

Everything is basically the same, except that the commands to set the module up
are different (they resemble a conversation with an old modem). The standard
UART rate assumed by the praxis is 19200, which coincides with the default UART
rate for BTM-182.

Issue these commands:

w ATC0			[to disable hardware flow control]
w ATP=1234		[to set up a PID for connections]
w ATN=Seawolf Dongle	[to set up a device identifier]
w ATX0			[to disable the +++ escape sequence]
w ATL5			[to change the UART rate to 115200]
w ATE1			[echo off]
w ATQ1			[no result codes (so they won't be diagnosed as bad
			 commands by the praxis using the UART transparently)]

Note that following the last command, you will have to execute

r 1152

Here is the way to list pretty much all the relevant settings:

w ATI1

You get:

AATI1
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
ATQ=1, NEVER SEND RESULT CODE 
ATR=1, SPP SLAVE ROLE 
ATS=1, ENVRLE AUTO-POWERDOWN OF RS232 DRIVRATX=0, NEVER CHECK '+++' 

This is what I get after the prescribed initialization sequence. Following that,
everything works exactly as for LinkMatik.

A potentially useful command is ATZ0 (i.e., w ATZ0), which reverts the module
to factory setting (retaining the name and PIN, however).

If you accidentally set the baud rate to above 115200 (i.e., to more what PicOS
can handle), you can revert to 19200 by holding the escape signal (PIO 4 on the
board) for 3+ seconds. You can accomplish this from BlueTest by executing:

e 5

===============================================================================

Some reminiscences from my experiments:

I have been using an XP (SP3) laptop with a built-in BlueTooth module. I have
been able to run mm_demo on boards equipped with BTM-182 as well as LinkMatik,
but the connection procedure is significantly different in both cases.

The way I try to communicate with the praxis from the laptop is to open a
piter terminal emulator pointing to the COM port assigned to the USB RFCOMM
channel.

For LinkMatik, I am able to pair the device, assign a COM port to it, and
then I can just open and close that port (e.g., by calling piter) to establish
a connection. This doesn't work for BTM-182 where I have to resort to Windows
GUI to set up a connection.

For BTM-182, when I connect first (using the Windows Bluetooth GUI) and then
open the COM port (from piter), the connection gets immediately dropped. When
I open the COM port first (invoke piter) and then connect to the device from
Windows, everything is OK. However, when (while connected) I close the COM
port (exit piter), the connection gets dropped in a weird way that requires
resetting BTM-182 on the board (you can power it down and up again, but what
matters is just resetting the BTM-182 module).

While you can reset BTM-182 from BlueTest, there are no hooks to do it from
mm_demo, so you have to power the board down and up again. It isn't clear to
me who is responsible for this problem, but my bet would go to BTM-182. Also,
the problem may not be present in other version of Windows.

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

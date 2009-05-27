You are receiving two tests:

Image_9600_d.a43    (UART preset to 9600, EEPROM/SD direct interface)
Image_115200_s.a43  (UART preset to 115200, EEPROM/SD SPI interface)

They are functionally identical.

You can use either one, but it may be safer to start with the first one
(as it is less stressful to the board).

===============================================================================

Enter 'E' (upper case) to get to the detailed EEPROM test page.
If the page doesn't show up (you see this message:

Failed to open EEPROM

followed by the original startup menu), it means that either
EEPROM is broken or the switch doesn't work. Assuming you see
the EEPROM test page, run the erase-write-read test:

w 0 524288

You should see a sequence of lines like:

WRITTEN ...

followed by

WRITE_COMPLETE, 0 ERRORS

and then:

READ ...

followed by 

READ COMPLETE, 0 ERRORS, 0 MISREADS

This test takes a while. If you are pressed for time, there
is no need to test the entire EEPROM, e.g.,

w 200000 10000

will test a small piece somewhere in the middle

When done, enter 'q' to get to the startup page.

===============================================================================

Put in the SD card, enter 'S', you should see the card's size
reported which will be followed by the SD Test page. If successful,
do the write-read test, e.g.,

w 1000000 100000

for a write-read test, you should see a similar output to the EEPROM
test.

When done, enter 'q' to get to the startup page.

===============================================================================

Enter 'G' to get to the GPS test. Then, enter 'e'. You should immediately
begin to see lines of text sent by the GPS module, looking like:

...
$GPRMC,183238.596,V,,,,,,,220509,,*2A
$GPGGA,183239.596,,,,,0,00,,,M,0.0,M,,0000*5E
$GPGSA,A,1,,,,,,,,,,,,,,,*1E
$GPRMC,183239.596,V,,,,,,,220509,,*2B
...

Enter q to quit and return to the startup page.

===============================================================================

Enter 'L' to get to the LCD test. Then enter:

o 1
d 0 dupa

You should see the entered text show up on the LCD.

Enter 'q' to return to the startup page.

===============================================================================

Enter 'T' to test the RT clock. Enter date, time, e.g.,

s 09 5 22 5 12 26 00

The numbers are: year, month, day, day of the week (0-sun, 1-mon, etc), hour
minute, second. You should receive this reply:

Status = 0

Now (a while later) check the time:

r

You should see something like this:

Status = 0 // 9 5 22 5 12 26 4

The time should advance accordingly.

Enter 'q' to return to the startup page.

===============================================================================

Set up two boards and execute 'r' on them. You should see radio packets being
transmitted and received. Execute 'q' to stop the RF test.

Note: this will also test all three LEDs.

===============================================================================

Testing the switches and current consumption.

Reset the board. 

Current consumption on my board is 290uA immediately after reset (with
the startup page displayed). Note that the switches are OFF in this state.

Test power down mode:

Enter 'D', then freeze the board for a few seconds, e.g.,

f 60  

This is one minute. Current on my board (measured with a simple meter)
is about 9uA. On yours it may be a bit more (I would estimate it at
13-15uA (because of the reset circuit).

To test the switches, use the 'o' and 'f' commands from the startup
page. Each command can switch on/off multiple switches (represented by
bits). They are:

LCD    bit       0       (value 1)
EEPROM bit       1       (value 2)
SD     bit       2       (value 4)
GPS    bit       3       (value 8)

Note that this test cannot test too much because the switches are handled
implicitly by the respective drivers, e.g., you cannot switch on EEPROM
merely by flipping the switch. But you can do this:

Switch on GPS:

o 8

Even though it is formally off, you
should see a jump in current consumption 290uA->1.5mA (this is how much
GPS takes when standing by). Then do

f 8

... and the current should go back to normal.

You can also try:

o 1
f 1

Marginal increase (about 380uA in my case).

o 2
f 2

Increase to about 3.5mA. Note that normally EEPROM does not take that much
current when idle, but the driver is not pulling CS high, which means that
the chip operates in a weird setup. Don't worry if you see something
different.

For SD (o 4), you may get any weirdness at all, depending on the card.

===============================================================================

You may want to test whether the UART works with high rates (we increased
the resistors in its path). For that you can run the 115200 version of the
test, or do this from the 9600 test (while at the startup page):

u 1152

which will set the UART rate to 115,200. Then change the rate in the
terminal.

Next try 'h' to see if things work. You may enter 'U' (for UART echo test).
Then try entering long lines and see if they are echoed OK.

You will exit this test by entering 'q'

===============================================================================

To tests sensors, enter 'V' (while at the startup page). You will see
the known menu from the sensor test praxis. The sensors are numbered:

0    - SHT TMP
1    - SHT HUM
2    - PAR/PYR on P6.0
3    - EC5 on P6.7
4    - PAR/PYR on P6.3
5    - PAR/PYR on P6.4
6    - PAR/PYR on P6.5

The test praxis has no sensor detection capability at present (can be
easily built in, if needed).

===============================================================================








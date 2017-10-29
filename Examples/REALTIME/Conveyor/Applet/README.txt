To recompile:

  rm Water.java
  make
  javac Water.java

------------------------------------------------------------------------------

To make the applet work, make sure that the parameters in index.html are
OK. Read README.txt in SOURCES/DSD about accessing the applet from your
browser. You need the tiny web server (SOURCES/NWEB) + the right setting of
Java permissions.

Each data set (files ../data_*.txt) includes the port number of the operator
mailbox on the simulator's side. This port is now the same (3346) for all
data sets. You may change this number (e.g., if you want to run more than
one example on the same host at the same time), but then you will have to
change the port number parameter in index.html.

If the windows don't come out right, try resizing them. Different platforms
may introduce different top and bottom margins, which makes it impossible to
display correctly narrow windows (like the belts) on all of them.
If this bothers you, edit Conveyor.src and redefine TOFFSET and BOFFSET
in class StatusDisplay. Then you will have to rebuild the applet classes
with your java compiler.

The "Terminate" button is disabled in the present version of the applet (it
behaves as "Disconnect"). To enable it, locate in Conveyor.src two comment
lines that look like this

                        // @@@ TERMINATE @@@ //

and remove the next source line from each place.

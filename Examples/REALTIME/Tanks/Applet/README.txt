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
mailbox on the simulator's side. This port is now the same (3345) for all
data sets. You may change this number (e.g., if you want to run more than
one example on the same host at the same time), but then you will have to
change the port number parameter in index.html.

To create the java 1.1.x version:

  rm Water.java
  make      (or make VERSION=11)

To create the java 1.0.x version:

  rm Water.java
  make VERSION=10

------------------------------------------------------------------------------

To make the applet work, you have to edit index.htm and change the host
name to point to the host on which the simulator is running. Then you will
be able to invoke the applet by loading index.htm in your java enabled
browser or in your applet viewer.

Each data set (files ../data_*.txt) includes the port number of the operator
mailbox on the simulator's side. This port is now the same (3341) for all
data sets. You may change this number (e.g., if you want to run more than
one example on the same host), but then you will have to change the port
number parameter in index.htm.

If the name of your source files end with "jav" instead of "java"
(this may happen if you are on Windows), rename them to "---.java"
before trying to compile them.

Compile all java source files, e.g., "javac *.java".

Note that the browser may restrict applet communication to the host from which
the applet page (i.e., index.htm) was fetched. In such a case, the simulator
must run on a host equipped with a web server, and the applet code (all .class
(.cla) and .gif files) as well as index.htm must be fetched from that host.
Alternatively, you may run the applet from appletviewer (but you have to
choose unlimited network and class access from its properties menu).

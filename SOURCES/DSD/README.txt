This file contains instructions on compiling the DSD applet `by hand.'

The actual source code of the applet is in file DSD.src. This file must be
preprocessed by cpp to be turned into a Java sources.

When you execute:

                      make

in this directory, you will create file DSD.java, which you can compile by
executing:

                      javac DSD.java

File index.html is the anchor html file for invoking the DSD applet. This
file is automatically edited by maker to insert the name of the monitor host
(parameter host) and the monitor port number (parameter port) according to
the user's selection. You may change these values by hand if needed.

Appletviewer is considered deprecated these days (I couldn't get anywhere with
it on Windows, although it does seem to work on Linux [Ubuntu] with the proper
addition to java.policy file). On Windows [Cygwin], the DSD applet can be run
from a browser, i.e., IE.

When you install Java on Windows, then IE is automatically enabled to run the
Java plugin. Your mileage with other browsers may vary.

You will have to do something about the (restrictive by default) permissions.
Here is what should be done on Windows 10 assuming that DSD is run from
localhost, i.e., localhost was specified as the monitor host when the package
was installed with maker (see ../):

1. Run "Configure Java". This is the program named javacpl in the bin directory
of the JRE installation. Click on the "Security" tab and the the host to the
list at the bottom. The purpose of this operation is to let you invoke the DSD
applet from the browser connecting to the trivial web server pointing to the
DSD applet. Specifically, add something like this:

	http://localhost:8001/

to the list (which is initially empty). Ignore warnings, etc. Just click OK.

Note that the port number is the port on which the trivial web server will be
listening when you start it (see below). You can change it, but then make sure
to start the server with the right port number. 8001 is the default.

2. Locate the java policy file. It is probably here:

	.../lib/security/java.policy

where the three dots stand for the installation directory of JRE. On Linux,
it is probably somewhere under /etc. Edit it (this requires Admin privileges)
to make sure that a line like this:

 permission java.net.SocketPermission "127.0.0.1:*", "connect,accept,resolve";

appears in the grant section. This directory (DSD) contains a sample
java.policy file with the right line. That line is needed, so the DSD applet
can talk (over a socket) to the monitor running on localhost.

Before you start an experiment that you want to monitor with DSD, make sure
that:

1. The monitor is running. You start the monitor by moving to directory MONITOR
and executing:

	./monitor standard.t

If you add -b as the second argument, the monitor will become a daemon, i.e.,
it will switch to the background and become independent of the invoking shell.
Note that on Windows/Cygwin the only way to kill such a process is from the
Task Manager.

2. If you can get Appletviewer (Linux) to work, then you won't be needing the
tiny web server. You just execute Appletviewer in the DSD directory, like this:

	appletviewer index.html

You will have to set "Class access" to Unrestricted in Applet->Properties.
Otherwise, to run the applet from the browser, you have to make sure it is
running. To start it, move to directory NWEB and execute:

	./nweb24 ../DSD

The argument is the root directory for the web service (we set it to the
location of the applet). Note that the default port is 8001. If you want to
be able to run other applets, like those in Examples/REALTIME/*, then it may
make sense to run the web server in the root directory of the package (which
is the closest superdirectory of them all), e.g., ../../ (from NWEB). Note
that -b has the same effect as in the case of monitor, i.e., it turns nweb24
into a daemon. Here is a sample call with the full set of arguments:

	./nweb24 -d ../../ -p 8001 -l logfile.txt -b

The log file will be crated in NWEB, i.e., in the directory where the program
has been called. Needless to say, you can specify any path (which, if it is not
absolute, will be interpreted relative to NWEB). Without -l, nweb24 produces no
log.

To run the applet, point your browser to:

	http://localhost:8001/SOURCES/DSD/index.html

(assuming that the tiny web server has been invoked as shown above).

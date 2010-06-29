This file contains instructions on compiling the DSD applet `by hand.'

The actual source code of the applet is in file DSD.src. This file must be
preprocessed by cpp to be turned into a Java sources.

When you execute

                      make

in this directory, you will create file DSD.java, which you can compile by
executing

                      javac DSD.java

On Windows, you may get DSD.jav instead of DSD.java, in which case you will
have to rename the file to DSD.java before it can be compiled.

By default, DSD.java contains code compliant with JDK1.2.x. To create 1.0.x
compliant code, execute

                      rm DSD.java    (or del DSD.java on Windows)
                      make VERSION=10

and then compile the resultant file as before.

File index.htm is the anchor html file for invoking the DSD applet. This
file is automatically edited by maker to insert the name of the monitor host
(parameter host) and the monitor port number (parameter port) according to
the user's selection. You may change these values by hand if needed.

To invoke the DSD applet, move to its directory and execute

                      appletviewer index.htm

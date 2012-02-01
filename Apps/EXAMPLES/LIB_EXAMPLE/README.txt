This example illustrates how to build and use libraries "manually", i.e.,
invoking mkmk by hand, rather than using PIP. Note that the PIP document
explains how to use libraries from PIP.

==============================================================================

Execute in this directory:

	mkmk WARSAW -l
	make

then cd to APP and execute:

	mkmk -M ../LIBRARY
	make

to build the praxis.

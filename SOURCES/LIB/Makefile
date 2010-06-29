# Creates the private library

COMP= g++
PCOMP= gcc
TCPIPCP = prtid.cc lib.cc socklib.cc
SXMLCP = sxml.c
SXMLCI = sxml.h
RANLIB =

libtcpip.a: $(TCPIPCP) $(TCPIPCC) $(TCPIPAS) $(SXMLCP) $(SXMLCI)
	rm -rf libtcpip.a
	$(COMP) -c -O2 $(TCPIPCP)
	$(PCOMP) -c -Wall -O2 $(SXMLCP)
	ar cq libtcpip.a *.o
	$(RANLIB)
	rm -rf *.o

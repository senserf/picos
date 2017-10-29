# Creates the monitor

PROG= monsel.cc
COMP= g++
OPTI= 
PLIBS= -L../LIB
LIBS=
SLPATH= -ltcpip

monitor: monitor.o
	$(COMP) $(OPTI) -o monitor monitor.o $(PLIBS) $(LIBS) $(SLPATH)

monitor.o:	$(PROG) monitor.h ../version.h ../KERNEL/dshared.cc ../KERNEL/sockbuf.cc
	$(COMP) $(OPTI) -c $(PROG) -o monitor.o

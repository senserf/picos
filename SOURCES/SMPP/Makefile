# Creates the preprocessor

COMP= g++
OPTI=
OLPATH= -L../LIB -ltcpip

smpp: smpp.o
	$(COMP) $(OPTI) -o smpp smpp.o $(OLPATH)

smpp.o:	smpp.cc smpp.h ../version.h
	$(COMP) $(OPTI) -c smpp.cc

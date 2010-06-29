# Creates the DSD java source. You may have to change PROCESSOR to the path
# of your cpp.

VERSION= 11
PROCESSOR= /lib/cpp

DSD.java: DSD.src
	$(PROCESSOR) -C -P -Dv$(VERSION) < DSD.src > DSD.java

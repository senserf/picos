# Creates the java source

VERSION= 11
PROCESSOR= /lib/cpp

Senscom.java: Status.java Senscom.src
	$(PROCESSOR) -C -P -Dv$(VERSION) < Senscom.src > Senscom.java

Status.java: Status.src
	$(PROCESSOR) -C -P -Dv$(VERSION) < Status.src > Status.java

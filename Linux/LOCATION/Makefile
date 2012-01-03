#
#
libloc.a:	locdbase.o locengine.o params.o sxml.o
		rm -f libloc.a
		ar q libloc.a locdbase.o locengine.o params.o sxml.o
		ranlib libloc.a

locengine.o:	locengine.c locengine.h ntypes.h params.h sxml.h
		gcc -Wall -O2 -c locengine.c

locdbase.o:	locdbase.c locengine.h ntypes.h params.h sxml.h
		gcc -Wall -O2 -c locdbase.c

params.o:	params.c locengine.h ntypes.h params.h sxml.h
		gcc -Wall -O2 -c params.c

sxml.o:		sxml.c sxml.h
		gcc -Wall -O2 -c sxml.c

clean:
		rm -f *.o

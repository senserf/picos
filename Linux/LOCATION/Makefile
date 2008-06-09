#
#
libloc.a:	locdbase.o locengine.o
		rm -f libloc.a
		ar q libloc.a locdbase.o locengine.o
		ranlib libloc.a

locengine.o:	locengine.c locengine.h ntypes.h
		gcc -g -c locengine.c

locdbase.o:	locdbase.c locengine.h ntypes.h
		gcc -g -c locdbase.c

clean:
		rm -f *.o

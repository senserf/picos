#
#
all:			gen_random_db query

gen_random_db:		gen_random_db.o ../libloc.a
			gcc -o gen_random_db gen_random_db.o -lm -L.. -lloc

gen_random_db.o:	gen_random_db.c
			gcc -Wall -c gen_random_db.c -I..

query:			query.o ../libloc.a
			gcc -o query query.o -lm -L.. -lloc

query.o:		query.c
			gcc -Wall -c query.c -I..

clean:
			rm -f *.o

cleanall:
			rm -f *.o gen_random_db query

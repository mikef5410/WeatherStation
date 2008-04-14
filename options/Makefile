

CC=cc
CFLAGS= -O2


options.o: options.c 
	$(CC) $(CFLAGS) -c $<

test: test.o options.o
	$(CC) $(CFLAGS) -o test $< options.o

depend:
	makedep -E'^/usr/' *.c *.h > Dependencies

clean:
	rm -f *~ \#* *.o test

.INCLUDE: Dependencies

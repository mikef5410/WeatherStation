
OS = $(shell uname -s)


ifeq ($(strip $(OS)),Linux)
CC = gcc
TESTFILE = /usr/doc/bind-4.9.3-BETA9/doc/file.lst
#PROFILE_CFLAGS = -pg -static
CFLAGS = $(PROFILE_CFLAGS)  -ffast-math -ansi -D_REENTRENT -Dlinux -DDEBUG_LEVEL=9
LPTHREAD = -lpthread
#PROFILE_LIBS = -lc_p
LIBS = 
FUPDATE = echo
endif

ifeq ($(strip $(OS)),HP-UX)
CC = cc
PROFILE_CFLAGS = -G -Wl,-a,archive
CFLAGS = $(PROFILE_CFLAGS) -g -Ae
TESTFILE = /usr/share/doc/10.20RelNotes
LPTHREAD = -lcma
PROFILE_LIBS = -lgmon
LIBS =
endif

debug.o: debug.c
	$(CC) $(CFLAGS) -c $<

depend:
	makedep -E'^/usr/' *.c *.h > Dependencies

clean:
	rm -f *~ \#* *.o

.INCLUDE: Dependencies

# Make the circular buffer library routine, and test program
#
 
OS = $(shell uname -s)


ifeq ($(strip $(OS)),Linux)
CC = gcc
TESTFILE = /usr/doc/bind-4.9.3-BETA9/doc/file.lst
#PROFILE_CFLAGS = -pg -static
CFLAGS = $(PROFILE_CFLAGS) -g -O3 -finline-functions -ffast-math -ansi -D_REENTRENT -Dlinux -DDEBUG_LEVEL=0 
LPTHREAD = -lpthread 
#PROFILE_LIBS = -lc_p 
LIBS = $(LPTHREAD) $(PROFILE_LIBS)
endif

ifeq ($(strip $(OS)),HP-UX)
HMSHOST=rtfm
HMSDIR=/MFmisc/cbuf
FUPDATE=/usr/local/bin/fupdate

CC = cc
PROFILE_CFLAGS = -G -Wl,-a,archive
CFLAGS = $(PROFILE_CFLAGS) -g -Ae
TESTFILE = /usr/share/doc/10.20RelNotes
LPTHREAD = -lcma
PROFILE_LIBS = -lgmon
LIBS = $(LPTHREAD) $(PROFILE_LIBS)
endif

VPATH=.:./RCS

#SHELL = /usr/local/bin/bash

SRCS = test_cbuf.c cbuf.c
HDRS = cbuf.h


test_cbuf   : test_cbuf.o cbuf.o debug.o
	$(CC) $(CFLAGS) -o test_cbuf test_cbuf.o cbuf.o debug.o $(LIBS)


test_cbuf.o : test_cbuf.c cbuf.h debug.h

cbuf.o : cbuf.c cbuf.h

debug.o: debug.c debug.h


$(SRCS) $(HDRS): update

update:	fmrc
	-$(FUPDATE) -RM
.PHONY:	fmrc clean test

fmrc:
	@echo \#Update this with \'make fmrc\' > fmrc
	@echo \#set your FMRC environment var to \'./fmrc\' >> fmrc
	@echo usehost $(HMSHOST) $(HMSDIR) >> fmrc
	@echo mapdir `/bin/pwd` $(HMSDIR) >> fmrc


clean: 
	rm -f *.o test_cbuf *~ \#* test.out core

.IGNORE:
test:	test_cbuf
	time ./test_cbuf < $(TESTFILE) > test.out
	diff -c $(TESTFILE) test.out


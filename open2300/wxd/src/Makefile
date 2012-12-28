OS = $(shell uname -s)

LDEBUG=debug/debug.o
IDEBUG=-Idebug

LOPTIONS=options/options.o
IOPTIONS=-Ioptions

LLINUX2300=../../linux2300.o
LRW2300=../../rw2300.o

SUBDIRS = debug options

ifeq ($(strip $(OS)),Linux)
CC = gcc
LPGSQL=  -lpq -lcrypt
IPGSQL=  -I/usr/include/pgsql

#PROFILE_CFLAGS = -pg -static
CFLAGS =  $(PROFILE_CFLAGS) -g -D_REENTRENT -Dlinux -DDEBUG_LEVEL=9

#PROFILE_LIBS = -lc_p
LIBS = $(LOPTIONS) $(LCBUF) $(LDEBUG) $(LRW2300) $(LLINUX2300) $(LPTHREAD) $(PROFILE_LIBS) $(LPGSQL) -lm
FUPDATE = echo
endif


VPATH=.


INCLUDES=$(IOPTIONS) $(ICBUF) $(IDEBUG) $(IPGSQL) -I../../
CFLAGS += $(INCLUDES) 

wxd: wxd.c wxd.h debug/debug.h cfg_read.o dbif_pg.o $(LDEBUG) $(LCBUF) $(LRW2300) $(LLINUX2300) $(LOPTIONS)
	$(CC) -o wxd $(CFLAGS) wxd.c cfg_read.o dbif_pg.o $(LIBS)

cfg_read.o:
	$(CC) -c cfg_read.c $(CFLAGS)

dbif_pg.o:
	$(CC) -c dbif_pg.c $(CFLAGS)

$(LDEBUG):
	(cd debug; $(MAKE) )

$(LOPTIONS):
	(cd options; $(MAKE))

$(LLINUX2300):
	(cd ../../; $(MAKE) linux2300.o)

$(LRW2300):
	(cd ../../; $(MAKE) rw2300.o)

tags: $(SRCS) $(HDRS)
	etags $(SRCS) $(HDRS)

clean:
	rm -f *.o *~ \#* wxd
	(cd options; make clean)
	(cd debug; make clean)



#include ./Dependencies


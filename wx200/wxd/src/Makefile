OS = $(shell uname -s)

LDEBUG=debug/debug.o
IDEBUG=-Idebug

LCBUF=buffer/cbuf.o
ICBUF=-Ibuffer

LOPTIONS=options/options.o
IOPTIONS=-Ioptions

MAKEDEP = /usr/local/bin/makedep

SUBDIRS = debug options buffer

ifeq ($(strip $(OS)),Linux)
CC = gcc
LPGSQL=  -lpq -lcrypt
IPGSQL=  -I/usr/include/pgsql
TESTFILE = /usr/doc/bind-4.9.3-BETA9/doc/file.lst
#PROFILE_CFLAGS = -pg -static
CFLAGS =  $(PROFILE_CFLAGS) -g -ansi -D_REENTRENT -Dlinux -DDEBUG_LEVEL=9
LPTHREAD = -lpthread
#PROFILE_LIBS = -lc_p
LIBS = $(LOPTIONS) $(LCBUF) $(LDEBUG) $(LPTHREAD) $(PROFILE_LIBS) $(LPGSQL) -lm
FUPDATE = echo
endif


ifeq ($(strip $(OS)),HP-UX)
HMSHOST=rtfm
HMSDIR=/MFmisc/cbuf
FUPDATE=/usr/local/bin/fupdate -RM
LPGSQL= -L/opt/pgsql/lib -lpq
IPGSQL= -I/opt/pgsql/include
CC = cc
PROFILE_CFLAGS = -G -Wl,-a,archive
CFLAGS = $(PROFILE_CFLAGS) -g -Ae
TESTFILE = /usr/share/doc/10.20RelNotes
LPTHREAD = -lcma
PROFILE_LIBS = -lgmon
LIBS = $(LOPTIONS) $(LCBUF) $(LDEBUG) $(LPTHREAD) $(PROFILE_LIBS) $(LPGSQL) -lm
endif


VPATH=.


INCLUDES=$(IOPTIONS) $(ICBUF) $(IDEBUG) $(IPGSQL)
CFLAGS += $(INCLUDES) 

wxd: wxd.c wxd.h buffer/cbuf.h debug/debug.h cfg_read.o parse.o dbif_pg.o serial.o $(LDEBUG) $(LCBUF) $(LOPTIONS)
	$(CC) -o wxd $(CFLAGS) wxd.c cfg_read.o parse.o dbif_pg.o serial.o $(LIBS)

cfg_read.o:
	$(CC) -c cfg_read.c $(CFLAGS)

dbif_pg.o:
	$(CC) -c dbif_pg.c $(CFLAGS)

parse.o:
	$(CC) -c parse.c $(CFLAGS)

serial.o:
	$(CC) -c serial.c $(CFLAGS)


$(LDEBUG):
	(cd debug; $(MAKE) )

$(LBUFFER):
	(cd buffer; $(MAKE))

$(LOPTIONS):
	(cd options; $(MAKE))


#Dependencies: $(SRCS) $(HDRS)
#	$(MAKEDEP) -I./ -I./options -I./debug -I./buffer -E'^/usr/' *.c >Dependencies

depend: Dependencies

tags: $(SRCS) $(HDRS)
	etags $(SRCS) $(HDRS)

$(SRCS) $(HDRS): update

update: 
	cvs update


clean:
	rm -f *.o *~ \#* wxd
	(cd options; make clean)
	(cd buffer; make clean)
	(cd debug; make clean)



#include ./Dependencies


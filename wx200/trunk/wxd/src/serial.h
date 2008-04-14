#ifndef _SERIAL_INCLUDED
#define _SERIAL_INCLUDED

#ifdef GLOBAL_SERIAL
#define GLOBAL
#else
#ifdef __cplusplus
#define GLOBAL extern "C"
#else
#define GLOBAL extern
#endif
#endif

GLOBAL int serial_open(char *wxtty);
GLOBAL int serial_close(char *wxtty);
GLOBAL int setupio(int ttyfd);
GLOBAL void serial_reader(cbuf * iobuf);

#undef GLOBAL
#endif				/* _SERIAL_INCLUDED */

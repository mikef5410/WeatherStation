#ifndef _PARSE_INCLUDED
#define _PARSE_INCLUDED

#ifdef GLOBAL_PARSE
#define GLOBAL
#else
#ifdef __cplusplus
#define GLOBAL extern "C"
#else
#define GLOBAL extern
#endif
#endif

#define xDtoint(a) ((int)((a) & 0x0F))
#define DDtoint(a) ((int)(((a) & 0x0F) + ((((a) & 0xF0)>>4)*10)))
#define Dxtoint(a) ((int)(((a) & 0x0F)>>4))

#define SIZE8F 35
#define SIZE9F 34
#define SIZEAF 31
#define SIZEBF 14
#define SIZECF 27

GLOBAL void parse_input(cbuf * iobuf);
GLOBAL void dump_buf(char *buffer, int len);

#undef GLOBAL
#endif				/* _PARSE_INCLUDED */

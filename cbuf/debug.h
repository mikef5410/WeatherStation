#ifndef _DEBUG_INCLUDED

#ifdef _GLOBAL_DEBUG
#define GLOBAL
#else
#ifdef __cplusplus
#define GLOBAL extern "C"
#else
#define GLOBAL extern
#endif
#endif


#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#ifndef DEBUG_LEVEL
#define errlog(a,b) 
#define errlog(level, fmt)
#define errlog1(level,fmt, a1)
#define errlog2(level,fmt, a1, a2)
#define errlog3(level,fmt, a1, a2, a3)
#define errlog4(level,fmt, a1, a2, a3, a4)
#define errlog5(level,fmt, a1, a2, a3, a4, a5)
#define DEBUG_LEVEL 0

#else

GLOBAL void ferrlog(FILE *log_fp, int level, char *filename, int lineno, char *msg_fmt, ...);


#define errlog(level, fmt) ferrlog(stderr, level, __FILE__, __LINE__, fmt)
#define errlog1(level,fmt, a1) ferrlog(stderr, level, __FILE__, __LINE__, fmt, a1)
#define errlog2(level,fmt, a1, a2) ferrlog(stderr, level, __FILE__, __LINE__, fmt, a1, a2)
#define errlog3(level,fmt, a1, a2, a3) ferrlog(stderr, level, __FILE__, __LINE__, fmt, a1, a2, a3)
#define errlog4(level,fmt, a1, a2, a3, a4) ferrlog(stderr, level, __FILE__, __LINE__, fmt, a1, a2, a3, a4)
#define errlog5(level,fmt, a1, a2, a3, a4, a5) ferrlog(stderr, level, __FILE__, __LINE__, fmt, a1, a2, a3, a4, a5)

#endif

#endif  /* _DEBUG_INCLUDED */

/*#include <varargs.h>*/
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

#define _GLOBAL_DEBUG
#include "debug.h"

int logOpened=0;

/*
 *  error should be called using the form:
 *      error(level, file_name, line, format, arg1, arg2...);
 */

/*VARARGS0*/
void
errorlog(int level, ...)
{
    va_list ap;
    char *fmt, *file;
    int  line;
    char message[1024];


    UseSyslog=0;

    va_start(ap, level);

    if (level > DebugLevel ) return;

    file=va_arg(ap,char *);
    line=va_arg(ap,int);
    fmt = va_arg(ap, char *);

    if (UseSyslog) {
       if (!logOpened) {
	  openlog(myName, 0, LogFacility);
	  logOpened=1;
       }
       vsprintf(message, fmt, ap);
       syslog(level, "%s", message);
    } else {
    (void)fprintf(stderr, "%s (%d): ", file, line);
    (void)vfprintf(stderr, fmt, ap);
    (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
    }
    va_end(ap);
}

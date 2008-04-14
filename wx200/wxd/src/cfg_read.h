
#include "options.h"
#include <syslog.h>
#include "debug.h"


#ifndef _CFGREAD_INCLUDED
#define _CFGREAD_INCLUDED

#ifdef GLOBAL_CFGREAD
#define GLOBAL
#else
#ifdef __cplusplus
#define GLOBAL extern "C"
#else
#define GLOBAL extern
#endif
#endif


#define CPNULL (char *)0

typedef struct facilitytab {
  char facility[32];
  int  ifacility;
} facilitytab;

facilitytab Facility[]
#ifdef GLOBAL_CFGREAD
=
{
   {"LOG_KERN", LOG_KERN},
   {"LOG_USER", LOG_USER},
   {"LOG_MAIL", LOG_MAIL},
   {"LOG_DAEMON", LOG_DAEMON},
   {"LOG_AUTH", LOG_AUTH},
   {"LOG_SYSLOG", LOG_SYSLOG},
   {"LOG_LPR", LOG_LPR},
   {"LOG_NEWS", LOG_NEWS},
   {"LOG_UUCP", LOG_UUCP},
   {"LOG_CRON", LOG_CRON},
#ifdef LOG_AUTHPRIV  
   {"LOG_AUTHPRIV", LOG_AUTHPRIV},
#endif
   {"LOG_LOCAL0", LOG_LOCAL0},
   {"LOG_LOCAL1", LOG_LOCAL1},
   {"LOG_LOCAL2", LOG_LOCAL2},
   {"LOG_LOCAL3", LOG_LOCAL3},
   {"LOG_LOCAL4", LOG_LOCAL4},
   {"LOG_LOCAL5", LOG_LOCAL5},
   {"LOG_LOCAL6", LOG_LOCAL6},
   {"LOG_LOCAL7", LOG_LOCAL7},
   {NULL}
}
#endif				/*GLOBAL_CFGREAD */
;

GLOBAL struct {
   int debug;
   int useSyslog;
   char logFacility[128];
   char ttydev[255];
   char dbhost[255];
   char dbport[64];
   char dboptions[1024];
   char dbtty[255];
   char dbname[255];
   int write_interval;
} opts

#ifdef GLOBAL_CFGREAD
= {
   9, 0, "", "/dev/ttyS0", "", "", "", "", "weather", (15 * 60)
}

#endif				/*GLOBAL_CFGREAD */
;


Options optlist[]
#ifdef GLOBAL_CFGREAD
=
{
   {"debug", MoosOptInt, &opts.debug},
   {"syslog", MoosOptInt, &opts.useSyslog},
   {"logfacility", MoosOptStr, opts.logFacility, 128},
   {"wxtty", MoosOptStr, opts.ttydev, 255},
   {"dbhost", MoosOptStr, opts.dbhost, 255},
   {"dbport", MoosOptStr, &opts.dbport, 64},
   {"dboptions", MoosOptStr, opts.dboptions, 1024},
   {"dbtty", MoosOptStr, opts.dbtty, 255},
   {"dbname", MoosOptStr, opts.dbname, 255},
   {"write_interval", MoosOptInt, &opts.write_interval},
   {NULL}
}
#endif				/*GLOBAL_CFGREAD */
;

GLOBAL void cfg_read(char *, char *, char *);

#undef GLOBAL
#endif				/* _CGFREAD_INCLUDED */

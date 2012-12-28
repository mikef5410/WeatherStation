#ifndef _DBIF_INCLUDED
#define _DBIF_INCLUDED

#ifdef GLOBAL_DBIF
#define GLOBAL
#else
#ifdef __cplusplus
#define GLOBAL extern "C"
#else
#define GLOBAL extern
#endif
#endif

#include "libpq-fe.h"

GLOBAL PGconn *dbconn;
GLOBAL PGresult *dbres;

GLOBAL void db_getout(PGconn * conn);
GLOBAL int db_connect(void);
GLOBAL char *db_getval(int tuple, char *fname);
GLOBAL void db_makedatetime(char *rval, int mo, int dd, int yy, int hh, int mm, int ss);
GLOBAL void db_curtime(char *retval);
GLOBAL void write_obs(void);
GLOBAL void db_writer(void);
GLOBAL void sane_defaults(void);
GLOBAL int init_db(void);

#undef GLOBAL
#endif				/* _DBIF_INCLUDED */

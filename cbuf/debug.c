#define _GLOBAL_DEBUG
#include "debug.h"


/* VARARGS4 -- for lint */
GLOBAL void ferrlog(FILE *log_fp, int level, char *filename, int lineno, char *msg_fmt, ...)
{
    va_list ap;
    char newfmt[2048];

    if (level > DEBUG_LEVEL ) {
       return;
    }

    sprintf(newfmt,"%s: %d: %s\n", filename, lineno, msg_fmt);
   /*
    * Get pointer to first variable argument so that we can
    * pass it on to v_print_log().  We do this so that
    * v_print_log() can access the variable arguments passed
    * to this routine.
    */
    va_start(ap, msg_fmt);
    v_print_log(log_fp, newfmt, ap);
    va_end(ap);
}


/* VARARGS2 -- for lint */
int v_print_log(FILE *log_fp, char *fmt, va_list ap)
{
    /*
     * If "%Y" is the first two characters in the format string,
     * a second file pointer has been passed in to print general
     * message information to.  The rest of the format string is
     * a standard  printf(3S) format string.
     */
    if ((*fmt == '%') && (*(fmt + 1) == 'Y'))
    {
        FILE *other_fp;

        fmt += 2;

        other_fp = (FILE *) va_arg(ap, char *);
        if (other_fp != (FILE *) NULL)
        {
            /*
             * Print general message information to additional stream.
             */
            (void) vfprintf(other_fp, fmt, ap);
            (void) fflush(other_fp);
        }
    }
    /*
     * Now print it to the log file.
     */
    (void) vfprintf(log_fp, fmt, ap);
    (void) fflush(log_fp);
}

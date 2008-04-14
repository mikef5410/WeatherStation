


#include <pthread.h>

#ifndef _CBUF_INCLUDED
#ifdef GLOBAL_CBUF
#define GLOBAL
#else
#ifdef __cplusplus
#define GLOBAL extern "C"
#else
#define GLOBAL extern
#endif
#endif

#ifdef linux
#define _POSIX_SOURCE
#define pthread_attr_default (const pthread_attr_t *)0
#define pthread_mutexattr_default  (const pthread_mutexattr_t *)0
#define pthread_condattr_default (const pthread_condattr_t *)0
/*#define pthread_startroutine_t  (void *(*)(void *))*/
#define pthread_startroutine_t void *(*)(void *)
typedef void *  pthread_addr_t;
#endif

typedef short buf_t;

typedef struct {
    pthread_cond_t *cond_readable;
    pthread_mutex_t *readable_lock;
    char readable;
    pthread_cond_t *cond_writeable;
    pthread_mutex_t *writeable_lock;
    char writeable;
    pthread_mutex_t *writelock;	/*allow multiple writer threads */
    pthread_mutex_t *readlock;	/*allow multiple reader threads */
    int refcnt;			/*writers increment, then decrement when done (eof) */
    int head;			/*index of first valid item */
    int tail;			/*index of last valid item */
    int bufsize;		/*size of the buffer */
    int termstyle;		/*How do we terminate? */
    int reason;			/*The termination reason */
    buf_t termchr;		/*Termination character */
    buf_t ungetc;		/*the unget character */
    int have_unget;		/*do we have an unget character? */
    buf_t *buf;			/*the buffer! */
} cbuf;

#define CBUF_NOTERM 0x0
#define CBUF_ENDTERM 0x1
#define CBUF_TERMCHR 0x2

GLOBAL cbuf *make_cbuf(int);	/*Make a circular buffer */
GLOBAL void release_cbuf(cbuf *);	/*Get rid of one */
GLOBAL int enter_cbuf(cbuf *, buf_t *, int);	/*Put items on a circular buffer */
GLOBAL int remove_cbuf(cbuf *, buf_t *, int);	/*Remove items from a circular buffer */
GLOBAL int read_cbuf(cbuf *, buf_t *, int);	/*Remove items from a circular buffer, sorta non-blocking */
GLOBAL int lookahead_cbuf(cbuf *, buf_t *, int);	/*lookahead items from a circular buffer without removing them */
GLOBAL int unget_cbuf(cbuf *, buf_t);	/*"unget" a character on the circular buffer */
#endif				/* _CBUF_INCLUDED */

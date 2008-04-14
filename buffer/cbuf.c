/* *************
 * Circular buffer facility targeted primarily at multi-threaded
 * use where one thread reads from the buffer while another writes.
 * 
 * Mike Ferrara  5/27/97
 * ************* */

#define GLOBAL_CBUF
#include "cbuf.h"

#ifdef linux
#define circ(a,b) (((a)<0) ? ((a)%(b))+(b)  : ((a)%(b)))
#else
/* Circular index management. Allows arithmetic on an index to 
   be adjusted to fit a circular buffer */
static int circ(int a, int bsize)
{
    if (a < 0)
	return ((a % bsize) + bsize);
    else
	return (a % bsize);
}
#endif

/* Create and initialize a circular buffer with size bytes of payload */
cbuf *make_cbuf(int size)
{
    cbuf *cbufp;

    cbufp = (cbuf *) malloc(sizeof(cbuf));
    cbufp->buf = (buf_t *) malloc(sizeof(buf_t) * size);

    cbufp->readable = 0;
    cbufp->readable_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(cbufp->readable_lock, pthread_mutexattr_default);

    cbufp->cond_readable = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_cond_init(cbufp->cond_readable, pthread_condattr_default);

    cbufp->writeable = 1;
    cbufp->writeable_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(cbufp->writeable_lock, pthread_mutexattr_default);

    cbufp->cond_writeable = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_cond_init(cbufp->cond_writeable, pthread_condattr_default);

    cbufp->writelock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(cbufp->writelock, pthread_mutexattr_default);

    cbufp->readlock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(cbufp->readlock, pthread_mutexattr_default);

    cbufp->refcnt = 1;		/*assume we'll have at least ONE writer */
    cbufp->bufsize = size;
    cbufp->head = 1;		/*We do the initial write at buf[1] NOT buf[0] */
    cbufp->tail = 0;		/*tail points to the LAST VALID item */
    cbufp->have_unget = 0;
    cbufp->reason = 0;
    cbufp->termstyle = CBUF_NOTERM;
    cbufp->termchr = -1;
    return (cbufp);
}

/* get rid of a circular buffer in a nice orderly fashion */

void release_cbuf(cbuf * cbufp)
{
    free(cbufp->readlock);
    free(cbufp->writelock);
    free(cbufp->cond_readable);
    free(cbufp->readable_lock);
    free(cbufp->cond_writeable);
    free(cbufp->writeable_lock);
    free(cbufp->buf);
    free(cbufp);
}

/*Put count items into the circular buffer
   the "enter" will NOT return until it completes.
   this may mean that it waits until more items are
   "removed" from the buffer to make room.
 */
int enter_cbuf(cbuf * buffer, buf_t * bytes, int count)
{
    int j, actual;

    actual = 0;
    pthread_mutex_lock(buffer->writelock);
    for (j = (buffer->tail + 1) % (buffer->bufsize); actual < count; j = (j + 1) % (buffer->bufsize)) {
	pthread_mutex_lock(buffer->writeable_lock);
	buffer->writeable = (j != circ((buffer->head - 1), buffer->bufsize));
	while (!buffer->writeable) {
	    pthread_mutex_lock(buffer->readable_lock);
	    buffer->readable = 1;
	    pthread_cond_signal(buffer->cond_readable);
	    pthread_mutex_unlock(buffer->readable_lock);
	    pthread_cond_wait(buffer->cond_writeable, buffer->writeable_lock);
	}
	pthread_mutex_unlock(buffer->writeable_lock);
	(buffer->buf)[j] = bytes[actual];
	actual++;
	buffer->tail = (buffer->tail + 1) % (buffer->bufsize);
    }
    if (actual > 0) {
	pthread_mutex_lock(buffer->readable_lock);
	buffer->readable = 1;
	pthread_cond_signal(buffer->cond_readable);
	pthread_mutex_unlock(buffer->readable_lock);
    }
    pthread_mutex_unlock(buffer->writelock);
    return (actual);
}

/* Remove count items from the circular buffer,
   if we've exhausted the buffer AND, the refcnt is zero, then
   exit. Else exit only if COUNT bytes is satisfied. 
   If there aren't enough bytes available, then wait for
   more to come in. SIDE EFFECT: buffer->ungetc is set to the last
   char removed, so an unget can be done by simply setting the 
   have_unget flag true. */

int remove_cbuf(cbuf * buffer, buf_t * bytes, int count)
{
    int j, actual;

    actual = 0;
    buffer->reason=CBUF_NOTERM;
    pthread_mutex_lock(buffer->readlock);
    if (buffer->have_unget) {
	bytes[actual] = buffer->ungetc;
	actual++;
	buffer->have_unget = 0;
    }
    for (j = buffer->head; actual < count; j = (j + 1) % (buffer->bufsize)) {
	pthread_mutex_lock(buffer->readable_lock);
	buffer->readable = (j != (buffer->tail + 1) % (buffer->bufsize));
	while (!buffer->readable) {
	    if (buffer->refcnt) {
		pthread_mutex_lock(buffer->writeable_lock);
		buffer->writeable = 1;
		pthread_cond_signal(buffer->cond_writeable);
		pthread_mutex_unlock(buffer->writeable_lock);
		pthread_cond_wait(buffer->cond_readable, buffer->readable_lock);
	    } else {
		pthread_mutex_unlock(buffer->readable_lock);
		goto GET_OUT;
	    }
	}
	pthread_mutex_unlock(buffer->readable_lock);
	bytes[actual] = buffer->buf[j];
	actual++;
	buffer->head = (buffer->head + 1) % (buffer->bufsize);
	if ((buffer->termstyle & CBUF_TERMCHR) && (bytes[actual-1] == buffer->termchr)) {
	    buffer->reason = CBUF_TERMCHR;
	    break;
	}
	if ((buffer->termstyle & CBUF_ENDTERM) && (bytes[actual-1] & 0x100)) {
	    buffer->reason = CBUF_ENDTERM;
	    break;
	}
    }
  GET_OUT:
    buffer->ungetc = bytes[count - 1];	/*save last char for unget */
    if (actual > 0) {
	pthread_mutex_lock(buffer->writeable_lock);
	buffer->writeable = 1;
	pthread_cond_signal(buffer->cond_writeable);
	pthread_mutex_unlock(buffer->writeable_lock);
    }
    pthread_mutex_unlock(buffer->readlock);
    return (actual);
}

/* read the contents of a cbuf, working more like the unix system
   read() ... block until at least one character is readable, then
   return all you can get, or what was asked for ... which ever is
   smaller, and return the number of characters read.
 */

int read_cbuf(cbuf * buffer, buf_t * bytes, int count)
{
    int j, actual;

    actual = 0;
    buffer->reason=CBUF_NOTERM;
    /* First get the "unget" character */
    pthread_mutex_lock(buffer->readlock);
    if (buffer->have_unget) {
	bytes[actual] = buffer->ungetc;
	actual++;
	buffer->have_unget = 0;
    }
    /* Now, wait till there's something to read */
    j = buffer->head;
    pthread_mutex_lock(buffer->readable_lock);
    buffer->readable = (j != (buffer->tail + 1) % (buffer->bufsize));
    while (!buffer->readable) {
	if (buffer->refcnt) {
	    pthread_mutex_lock(buffer->writeable_lock);
	    buffer->writeable = 1;
	    pthread_cond_signal(buffer->cond_writeable);
	    pthread_mutex_unlock(buffer->writeable_lock);
	    pthread_cond_wait(buffer->cond_readable, buffer->readable_lock);
	} else {
	    pthread_mutex_unlock(buffer->readable_lock);
	    goto GET_OUT;
	}
    }
    pthread_mutex_unlock(buffer->readable_lock);

    /*got here, so there's something to read, so read all there is,
       or what was asked for, and leave */
    for (j = buffer->head; actual < count; j = (j + 1) % (buffer->bufsize)) {
	if (j == (buffer->tail + 1) % (buffer->bufsize)) {
	    break;
	}
	bytes[actual] = buffer->buf[j];
	actual++;
	buffer->head = (buffer->head + 1) % (buffer->bufsize);
	if ((buffer->termstyle & CBUF_TERMCHR) && (bytes[actual-1] == buffer->termchr)) {
	    buffer->reason = CBUF_TERMCHR;
	    break;
	}
	if ((buffer->termstyle & CBUF_ENDTERM) && (bytes[actual-1] & 0x100)) {
	    buffer->reason = CBUF_ENDTERM;
	    break;
	}
    }
  GET_OUT:
    buffer->ungetc = bytes[count - 1];	/*save last char for unget */
    if (actual > 0) {
	pthread_mutex_lock(buffer->writeable_lock);
	buffer->writeable = 1;
	pthread_cond_signal(buffer->cond_writeable);
	pthread_mutex_unlock(buffer->writeable_lock);
    }
    pthread_mutex_unlock(buffer->readlock);
    return (actual);
}


/* Lookahead in the circular buffer. Same rules and behavior as remove_cbuf,
   except the cbuf->head pointer does not move. CAVEAT: the size of the
   lookahead is assumed to be a small fraction of the buffer size 

   No "unget" is implemented. "unget" in a multithreaded environment such
   as this would be potentially disastrous. Lookahead should provide a
   reasonable alternative to "unget".
 */
int lookahead_cbuf(cbuf * buffer, buf_t * bytes, int count)
{
    int j, phead, actual;

    actual = 0;
    pthread_mutex_lock(buffer->readlock);
    phead = buffer->head;
    for (j = phead; actual < count; j = (j + 1) % (buffer->bufsize)) {
	while (j == (buffer->tail + 1) % (buffer->bufsize)) {
	    if (buffer->refcnt) {
		sched_yield();
	    } else {
		pthread_mutex_unlock(buffer->readlock);
		return (actual);
	    }
	}
	bytes[actual] = buffer->buf[j];
	actual++;
	phead = (phead + 1) % (buffer->bufsize);
    }
    pthread_mutex_unlock(buffer->readlock);
    return (actual);
}

/* "unget" a character in the circular buffer. You are limited to
   ONE character of "unget" before a "remove". Implemented by keeping
   an out-of-band character and stuffing it back at "remove" time.
 */

int unget_cbuf(cbuf * buffer, buf_t byte)
{
    int j, actual;

    if (buffer->have_unget)
	return (0);		/* no room at the inn */
    buffer->have_unget = 1;
    buffer->ungetc = byte;
    return (1);
}

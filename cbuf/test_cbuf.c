/* test the circular buffer routines */
/* Mike Ferrara 5/27/97 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "cbuf.h"
#include "debug.h"


#define BSIZE 128
cbuf *iobuf;
pthread_t reader, writer;


double rnum(double range)
{
   double r;
   double rmax = ( (1<<31) - 1);

   r=random();
   errlog1(5,"random returns: %g",r);
   r=r * (BSIZE/rmax) + 1;
   errlog1(5,"rnum returns: %g",r);
   return(r);
}

void test_lookahead(cbuf * thebuf)
{
    int got, lachars;
    buf_t destbuf[BSIZE];

    lachars = (int)rnum(BSIZE);
    if (lachars > 0.10 * BSIZE)
	return;			/*only test if chars < 10% of buffer */
    got = lookahead_cbuf(thebuf, destbuf, lachars);
}

void do_write(buf_t * buffer, int len)
{
    FILE *fptr;
    char obuf[BSIZE];
    int j;

    fptr = stdout;
    for (j = 0; j < len; j++) {
	obuf[j] = (char) buffer[j];
    }
        fwrite(obuf, 1, len, fptr);
        fflush(fptr);
}

void test_read(cbuf * thebuf)
{
    double shouldwe;
    int got, thislen;
    buf_t destbuf[BSIZE];
    FILE *fptr;

    fptr = stdout;
    shouldwe = rnum(1);
    if (shouldwe > 0.10)
	return;			/* only test 10% of the time */
    thislen = (int)rnum(BSIZE);
    got = read_cbuf(thebuf, destbuf, thislen);
    errlog2(5,"test_read asked: %d  got: %d\n",thislen, got);
    do_write(destbuf, got);
}

void test_unget(cbuf * thebuf)
{
    double shouldwe;
    int rval;
    buf_t c;

    shouldwe = rnum(1);
    if (shouldwe > 0.10)
	return;			/* only test 10% of the time */
    c = (int) rnum(BSIZE);
    rval = unget_cbuf(thebuf, c);
    if (rval == 0) {
	fprintf(stderr, "!");
	fflush(stderr);
    } else {
	rval = remove_cbuf(thebuf, &c, 1);
    }
}

/* Read from stdin, filling the circular buffer. Use random
   sizes for the read/fill operation */
void fill_buf(cbuf * thebuf)
{
    int j = 0, thislen, retlen;
    char srcbuf[BSIZE], k = 0;
    buf_t src[BSIZE];
    FILE *fptr;

    errlog(5,"Reader starting");
    fptr = stdin;
    while (!feof(fptr)) {
	thislen = (int)rnum(BSIZE);
	thislen = fread(srcbuf, 1, thislen, fptr);
	for (j = 0; j < thislen; j++)
	    src[j] = (buf_t) srcbuf[j];
	retlen = enter_cbuf(thebuf, src, thislen);
    }
    thebuf->refcnt--;		/* End of file, we're done. Decrement the ref count */
}


/* empty the circular buffer by issuing random sized remove_cbuf
   calls, then write the result to stdout */
void empty_buf(cbuf * thebuf)
{
    int want, got, j;
    char last = 0;
    buf_t destbuf[BSIZE];
    FILE *fptr;

    errlog(5,"Writer starting");
    fptr = stdout;
    got = 1;
    while (got) {
	want = (int)rnum(BSIZE);
	test_lookahead(thebuf);
	test_unget(thebuf);
	test_read(thebuf);
	got = remove_cbuf(thebuf, destbuf, want);
	do_write(destbuf, got);
    }

}


main(int argc, char **argv)
{
    int r;
    iobuf = make_cbuf(BSIZE);
    srandom(time(NULL));
    pthread_create(&reader, pthread_attr_default, (pthread_startroutine_t) empty_buf, (pthread_addr_t) iobuf);
    pthread_create(&writer, pthread_attr_default, (pthread_startroutine_t) fill_buf, (pthread_addr_t) iobuf);
    pthread_join(writer, NULL);
    pthread_join(reader, NULL);
    release_cbuf(iobuf);
    exit(0);
}


#define _POSIX_SOURCE 1
#include <termios.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define GLOBAL_SERIAL
#include "cbuf.h"
#include "serial.h"
#include "wxd.h"
#include "debug.h"
#include "cfg_read.h"
#include "dbif_pg.h"

int 
serial_open(char *wxtty)
{
   int fd;
   /*check and make lock file */
   fd = open(wxtty, O_RDWR /*| O_NDELAY */ );
   if (fd == -1) {
      errlog2(1, "Unable to open serial port %s - %s", wxtty, strerror(errno));
   }
   return (fd);
}


int 
serial_close(char *wxtty)
{
   close(wxin);
   /*remove lock file */
}

int 
setupio(int ttyfd)
{
   struct termios attr;
   char temp;

   if (tcgetattr(ttyfd, &attr) != 0)
      return (-1);
   if (cfsetispeed(&attr, B9600) != 0)
      return (-1);
   if (cfsetospeed(&attr, B9600) != 0)
      return (-1);

   /* Disable the interrupt character */
#if !defined(_POSIX_VDISABLE) || (_POSIX_VDISABLE == -1)
   attr.c_cc[VINTR] = _POSIX_VDISABLE;
#else
   errno = 0;
   temp = fpathconf(ttyfd, _PC_VDISABLE);
   if (temp != -1)
      attr.c_cc[VINTR] = temp;
   else if (errno != 0) {	/* bad news, can't disable, set intr char 
				   to some low liklihood char */
      attr.c_cc[VINTR] = 0377;
   }
#endif

   attr.c_cc[VMIN] = 1;
   attr.c_cc[VTIME] = 0;

   attr.c_cflag &= (~PARENB /*& ~PARODD */ );
   attr.c_cflag |= (CLOCAL | CREAD | CS8);

   attr.c_lflag &= (~ICANON & ~ECHO);

   attr.c_iflag &= (~IXOFF & ~INPCK & ~ISTRIP /*& ~PARMRK /*& ~IGNPAR */ );
   attr.c_iflag |= (IGNBRK);

   /*attr.c_oflag &= ~CRTSCTS; */
   /*attr.c_oflag |= CRTSCTS; */
   if (tcsetattr(ttyfd, /*TCSAFLUSH */ TCSANOW, &attr) != 0)
      return (-1);

   attr.c_iflag &= ~(BRKINT | IGNPAR | PARMRK | INPCK | ISTRIP | INLCR |
		     IGNCR | ICRNL | IXON);
   attr.c_iflag |= IGNBRK;

   attr.c_oflag &= ~(OPOST);

   attr.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON | ISIG |
		     NOFLSH | TOSTOP);
   attr.c_lflag |= PENDIN;

   attr.c_cflag &= (CSIZE | HUPCL | CSTOPB | PARENB);
   attr.c_cflag |= (CLOCAL | CREAD | CS8);


   if (tcsetattr(ttyfd, /*TCSAFLUSH */ TCSANOW, &attr) != 0)
      return (-1);
   tcflush(ttyfd, TCIFLUSH);

   return (0);
}

/* Fill the circular buffer with serial input */
void 
serial_reader(cbuf * iobuf)
{
   int j = 0, thislen, retlen;
   char buf[BSIZE];
   buf_t src[BSIZE];
   int k = 0;
   int read_input = 1;

   pthread_mutex_lock(&terminate_lock);
   if (terminate)
      read_input = 0;
   pthread_mutex_unlock(&terminate_lock);
   while (read_input) {
      thislen = read(wxin, buf, BSIZE);
      for (j = 0; j < thislen; j++)
	 src[j] = (buf_t) (buf[j] & 0xFF);
#if DEBUG_READ
      dump_buf(buf, thislen);
#endif
      if (thislen)
	 retlen = enter_cbuf(iobuf, src, thislen);

      pthread_mutex_lock(&terminate_lock);
      if (terminate)
	 read_input = 0;
      pthread_mutex_unlock(&terminate_lock);
   }
   iobuf->refcnt--;		/* we're done ... tell the circular buffer code */
   errlog(8, "Serial reader exits.");
   pthread_exit(0);
}


#define _POSIX_SOURCE 1
#include <termios.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define GLOBAL_PARSE
#include "cbuf.h"
#include "parse.h"
#include "wxd.h"
#include "debug.h"
#include "cfg_read.h"
#include "dbif_pg.h"



const char *month[13] =
{"   ", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
 "Aug", "Sep", "Oct", "Nov", "Dec"};

#define SIZE8F 35
#define SIZE9F 34
#define SIZEAF 31
#define SIZEBF 14
#define SIZECF 27


int safe_read(int fd, char *buf, int len);


void 
parse_input(cbuf * iobuf)
{
   int j = 0, got = 0;
   unsigned char buf[BSIZE], inch;
   buf_t dest[BSIZE];
   int read_input = 1;

   pthread_mutex_lock(&terminate_lock);
   if (terminate)
      read_input = 0;
   pthread_mutex_unlock(&terminate_lock);
   while (read_input) {
#if USE_CBUF
      got = remove_cbuf(iobuf, dest, 1);
      inch = (unsigned char) dest[0];
#else
      j = safe_read(wxin, &inch, 1);
#endif
      switch (inch) {
      case 0x8f:
	 buf[0] = inch;
#if USE_CBUF
	 got = remove_cbuf(iobuf, dest, SIZE8F - 1);
	 for (j = 0; j < SIZE8F; j++) {
	    buf[j + 1] = (char) dest[j];
	 }
#else
	 j = safe_read(wxin, buf + 1, SIZE8F - 1);
#endif
#if DEBUG_READ
	 errlog(9, "");
#endif
	 parse8f(&buf);
	 break;
      case 0x9f:
	 buf[0] = inch;
#if USE_CBUF
	 got = remove_cbuf(iobuf, dest, SIZE9F - 1);
	 for (j = 0; j < SIZE9F; j++) {
	    buf[j + 1] = (char) dest[j];
	 }
#else
	 j = safe_read(wxin, buf + 1, SIZE9F - 1);
#endif
#if DEBUG_READ
	 errlog(9, "");
#endif
	 parse9f(&buf);
	 break;
      case 0xaf:
	 buf[0] = inch;
#if USE_CBUF
	 got = remove_cbuf(iobuf, dest, SIZEAF - 1);
	 for (j = 0; j < SIZEAF; j++) {
	    buf[j + 1] = (char) dest[j];
	 }
#else
	 j = safe_read(wxin, buf + 1, SIZEAF - 1);
#endif
#if DEBUG_READ
	 errlog(9, "");
#endif
	 parseaf(&buf);
	 break;
      case 0xbf:
	 buf[0] = inch;
#if USE_CBUF
	 got = remove_cbuf(iobuf, dest, SIZEBF - 1);
	 for (j = 0; j < SIZEBF; j++) {
	    buf[j + 1] = (char) dest[j];
	 }
#else
	 j = safe_read(wxin, buf + 1, SIZEBF - 1);
#endif
#if DEBUG_READ
	 errlog(9, "");
#endif
	 parsebf(&buf);
	 break;
      case 0xcf:
	 buf[0] = inch;
#if USE_CBUF
	 got = remove_cbuf(iobuf, dest, SIZECF - 1);
	 for (j = 0; j < SIZECF; j++) {
	    buf[j + 1] = (char) dest[j];
	 }
#else
	 j = safe_read(wxin, buf + 1, SIZECF + 1);
#endif
#if DEBUG_READ
	 errlog(9, "");
#endif
	 parsecf(&buf);
	 break;
      default:
	 break;
      }

      pthread_mutex_lock(&current_obs_lock);
      if (ch_flag) {
	 pthread_mutex_lock(&cond_update_lock);
         pthread_cond_signal(&cond_update);
	 pthread_mutex_unlock(&cond_update_lock);
      }
      pthread_mutex_unlock(&current_obs_lock);
       
      pthread_mutex_lock(&terminate_lock);
      if (terminate)
	 read_input = 0;
      pthread_mutex_unlock(&terminate_lock);
   }
   errlog(8, "Parser exits.");
   pthread_exit(0);
}


int 
parse8f(char *buf)
{
   int hour, min, sec, day, mon;
   double hum_in, hum_out;
   if (!sumcheck(buf, SIZE8F)) {
      errlog(8, "Checksum failure for 8F block.");
      return (-1);
   }
   sec = DDtoint(buf[1]);
   min = DDtoint(buf[2]);
   hour = DDtoint(buf[3]);
   mon = xDtoint(buf[5]);
   day = DDtoint(buf[4]);
   hum_in = DDtoint(buf[8]);
   hum_out = DDtoint(buf[20]);

   current_obs.sec = sec;
   current_obs.min = min;
   current_obs.hour = hour;
   current_obs.month = mon;
   current_obs.day = day;

   correct(hum_in, current_cal.rh_in_mul, current_cal.rh_in_offs);
   correct(hum_out, current_cal.rh_out_mul, current_cal.rh_out_offs);

   update(current_obs.rh_in, hum_in, HUMIN);
   update(current_obs.rh_out, hum_out, HUMOUT);

   errlog5(9, "%s %d %.2d:%.2d:%.2d", month[mon], day, hour, min, sec);
   errlog2(9, "Indoor hum: %g%%, Outdoor hum: %g%%", hum_in, hum_out);
}

int 
parse9f(char *buf)
{
   double temp_in, temp_out;

   if (!sumcheck(buf, SIZE9F)) {
      errlog(8, "Checksum failure for 9F block.");
      return (-1);
   }
   temp_in = ((xDtoint(buf[2] & 0x07) * 100) + (DDtoint(buf[1]))) / 10;
   if (buf[2] & 0x08) {
      temp_in = -1.0 * temp_in;
   }
   temp_out = ((xDtoint(buf[17] & 0x07) * 100) + (DDtoint(buf[16]))) / 10;
   if (buf[17] & 0x08) {
      temp_out = -1.0 * temp_out;
   }

   if ((temp_out < -50.0) || (temp_in < -50.0)) return; /*Bad data*/

   correct(temp_in, current_cal.temp_in_mul, current_cal.temp_in_offs);
   correct(temp_out, current_cal.temp_out_mul, current_cal.temp_out_offs);

   update(current_obs.temp_in, temp_in, TEMPIN);
   update(current_obs.temp_out, temp_out, TEMPOUT);

   errlog2(9, "Indoor temp: %gC, Outdoor temp: %gC", temp_in, temp_out);
}

int 
parseaf(char *buf)
{
   double baro, dewpt_in, dewpt_out;

   if (!sumcheck(buf, SIZEAF)) {
      errlog(8, "Checksum failure for AF block.");
      return (-1);
   }
   baro = DDtoint(buf[3]) + (DDtoint(buf[4]) * 100.0) + (xDtoint(buf[5]) * 10000.0);
   baro /= 10;

   dewpt_in = DDtoint(buf[7]);
   dewpt_out = DDtoint(buf[18]);

   correct(dewpt_in, current_cal.dp_in_mul, current_cal.dp_in_offs);
   correct(dewpt_out, current_cal.dp_out_mul, current_cal.dp_out_offs);
   correct(baro, current_cal.barometer_mul, current_cal.barometer_offs);

   update(current_obs.dp_in, dewpt_in, DEWIN);
   update(current_obs.dp_out, dewpt_out, DEWOUT);
   update(current_obs.barometer, baro, BARO);


   errlog1(9, "Barometer: %g mb", baro);
   errlog2(9, "Indoor dewpoint: %gC, Outdoor dewpoint: %gC", dewpt_in, dewpt_out);

}

int 
parsebf(char *buf)
{
   double rainrate, raintot;

   if (!sumcheck(buf, SIZEBF)) {
      errlog(8, "Checksum failure for BF block.");
      return (-1);
   }
   rainrate = (xDtoint(buf[2]) * 100) + DDtoint(buf[1]);
   raintot = (xDtoint(buf[6]) * 100) + DDtoint(buf[5]);

   correct(rainrate, current_cal.rain_rate_mul, current_cal.rain_rate_offs);
   correct(raintot, current_cal.rain_tot_mul, current_cal.rain_tot_offs);

   pthread_mutex_lock(&current_obs_lock);
   if (raintot < current_obs.rain_tot) {
      current_obs.rtot_offset += current_obs.rain_tot;
   }
   pthread_mutex_unlock(&current_obs_lock);

   update(current_obs.rain_rate, rainrate, RAINRATE);
   update(current_obs.rain_tot, raintot, RAINTOT);

   errlog2(9, "Rain rate %g mm/hr, Rain tot %g mm", rainrate, raintot);
}

int 
parsecf(char *buf)
{
   double gust, avg, chill, hum_out;
   double avg_dir, gust_dir;

   if (!sumcheck(buf, SIZECF)) {
      errlog(9, "Checksum failure for CF block.");
      return (-1);
   }
   gust = ((xDtoint(buf[2]) * 100) + DDtoint(buf[1])) / 10;
   gust_dir = (Dxtoint(buf[2])) + (DDtoint(buf[3]) * 10);

   avg = ((xDtoint(buf[5]) * 100) + DDtoint(buf[4])) / 10;
   avg_dir = (Dxtoint(buf[5])) + (DDtoint(buf[6]) * 10);

   chill = DDtoint(buf[16]);
   if (buf[21] & 0x20) {
      chill = -1.0 * chill;
   }

#if 0
   pthread_mutex_lock(&current_obs_lock);
   if (((chill - current_obs.temp_out) > 0.5) && (avg > 0 || gust > 0)) {
      chill = -1.0 * chill;
   }
   pthread_mutex_unlock(&current_obs_lock);
#endif

   correct(chill, current_cal.chill_mul, current_cal.chill_offs);
   correct(gust, current_cal.gust_spd_mul, current_cal.gust_spd_offs);

   correct(gust_dir, current_cal.gust_dir_mul, current_cal.gust_dir_offs);
   correct(avg_dir, current_cal.wavg_dir_mul, current_cal.wavg_dir_offs);

   gust_dir = (gust_dir < 0) ? 360 + ((int) gust_dir % 360) : (int) gust_dir % 360;

   correct(avg, current_cal.wavg_spd_mul, current_cal.wavg_spd_offs);
   avg_dir = (avg_dir < 0) ? 360 + ((int) avg_dir % 360) : (int) avg_dir % 360;


   update(current_obs.gust, gust, GUST);
   update(current_obs.wavg, avg, WAVG);
   update(current_obs.gust_dir, gust_dir, GUSTDIR);
   update(current_obs.wavg_dir, avg_dir, WAVGDIR);
   update(current_obs.chill, chill, WCHILL);

   errlog2(9, "Wind gust %g m/s, direction: %g", gust, gust_dir);
   errlog2(9, "Wind avg  %g m/s, direction: %g", avg, avg_dir);
   errlog1(9, "Wind chill %g C", chill);
}


int 
sumcheck(char *buf, int size)
{
   int j, sum = 0;

   for (j = 0; j < size - 1; j++) {
      sum += (unsigned int) buf[j];
   }
   return ((sum & 0xFF) == (unsigned char) buf[size - 1]);
}

void 
dump_buf(char *buffer, int len)
{
   int j, k;
   k = 0;
   for (j = 0; j < len; j++) {
      fprintf(stderr, "%.2X ", buffer[j] & 0xff);
      k++;
      if (k >= 16) {
	 fprintf(stderr, "\n");
	 k = 0;
      }
   }
}

int safe_read(int fd, char *buf, int len)
{
   int got=0;
   int siz=0;

   while(len>0 && got >= 0) {
      got=read(fd,buf+siz,len);
      if (got>0) {
	 len -= got;
	 siz += got;
      }
   }
   /*   dump_buf(buf,siz);*/
   return(siz);
}

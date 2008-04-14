#ifndef _WXD_INCLUDED
#define _WXD_INCLUDED
#include <time.h>
#include <math.h>
#include "cbuf.h"

#ifdef GLOBAL_WXD
#define GLOBAL
#else
#ifdef __cplusplus
#define GLOBAL extern "C"
#else
#define GLOBAL extern
#endif
#endif


#define CONFIG_FILE "/etc/wxd.conf"

#define TTYDEV "/dev/ttyS0"
#define BSIZE 512

GLOBAL cbuf *iobuf;

GLOBAL pthread_t reader, parser, writer, sighandler;
GLOBAL int wxin;
GLOBAL int terminate;		/* Global is used to signal the threads to quit and exit */


#define TERM_RESTART 1		/*valid values for terminate */
#define TERM_TERM 2
#define TERM_NOTERM 0

GLOBAL time_t last_write_time;
#define WRITE_INTERVAL opts.write_interval

typedef struct {
   int month;
   int day;
   int hour;
   int min;
   int sec;
   double temp_in;
   double temp_out;
   double dp_in;
   double dp_out;
   double chill;
   int rh_in;
   int rh_out;
   int rain_rate;
   int rain_tot;		/* as set directly from wx200 */
   int rtot_offset;		/* offset to compensate for random resets */
   double gust;
   int gust_dir;
   double wavg;
   int wavg_dir;
   double barometer;
} obs_t;

typedef struct {
   double temp_in_mul, temp_in_offs;
   double temp_out_mul, temp_out_offs;
   double rh_in_mul, rh_in_offs;
   double rh_out_mul, rh_out_offs;
   double dp_in_mul, dp_in_offs;
   double dp_out_mul, dp_out_offs;
   double chill_mul, chill_offs;
   double rain_tot_mul, rain_tot_offs;
   double rain_rate_mul, rain_rate_offs;
   double gust_spd_mul, gust_spd_offs;
   double gust_dir_mul, gust_dir_offs;
   double wavg_spd_mul, wavg_spd_offs;
   double wavg_dir_mul, wavg_dir_offs;
   double barometer_mul, barometer_offs;
} curcal_t;

GLOBAL obs_t current_obs;	/* Current observation values */
GLOBAL int ch_flag;		/* What changed? */
GLOBAL curcal_t current_cal;


GLOBAL pthread_mutex_t current_obs_lock;
GLOBAL pthread_mutex_t current_cal_lock;
GLOBAL pthread_mutex_t terminate_lock;
GLOBAL pthread_cond_t cond_update;
GLOBAL pthread_mutex_t cond_update_lock;


/* Bit positions for the changed flag */
#define TEMPIN  0
#define TEMPOUT 1
#define DEWIN   2
#define DEWOUT  3
#define WCHILL  4
#define HUMIN   5
#define HUMOUT  6
#define RAINRATE 7
#define RAINTOT  8
#define GUST     9
#define GUSTDIR  10
#define WAVG     11
#define WAVGDIR  12
#define BARO     13

#define set_bit(var,bit) (var) = ((var) | (1<<(bit)))
#define test_bit(var,bit) (var) & (1<<(bit))
#define clear_bit(var,bit) (var) = ((var) & ~(1<<(bit)))

#define USE_UPDATE 1
#if USE_UPDATE
#define update(obs, var, bit)  if ( (obs) != (var) && finite(var) /*&& fabs(((obs)-(var))/(var)) < 0.10*/ ) { \
   pthread_mutex_lock(&current_obs_lock); \
   (obs) = (var); \
   ch_flag = ch_flag | (1<<(bit)); \
   pthread_mutex_unlock(&current_obs_lock); \
}
#else
#define update(obs, var, bit)
#endif

#define USE_CORRECT 1
#if USE_CORRECT
#define correct(obs, mul, offs) pthread_mutex_lock(&current_cal_lock); \
   (obs) *= (mul); \
   (obs) += (offs); \
   pthread_mutex_unlock(&current_cal_lock)
#else
#define correct(obs, mul, offs)
#endif


#define USE_CBUF 1
#define DEBUG_READ 0
#define SCREENOUT 0

#undef GLOBAL
#endif				/* _WXD_INCLUDED */

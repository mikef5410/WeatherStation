
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>

#include "linux2300.h"
#include "rw2300.h"


#define GLOBAL_WXD
#include "wxd.h"
#include "debug.h"
#include "cfg_read.h"
#include "dbif_pg.h"

/* forward decls */
void screen_writer(void);
void daemonize(void);
void hupcatch(int pid);
void termcatch(int pid);
void do_opts(int argc, char **argv);

int foreground = 0;
char *myName;

main(int argc, char **argv)
{
   WEATHERSTATION ws2300;
   
   int go = 1;
   struct config_type config;

   struct sigaction ignoreChildDeath;
   ignoreChildDeath.sa_handler=NULL;
   ignoreChildDeath.sa_flags= (SA_NOCLDSTOP | SA_RESTART);
   
   struct sigaction hupAction;
   hupAction.sa_handler = &hupcatch;
   hupAction.sa_flags=SA_RESTART;

   struct sigaction termAction;
   termAction.sa_handler = &termcatch;
   termAction.sa_flags = SA_RESTART;
   
   
   myName=argv[0];
   ch_flag = 0;
   terminate = 0;

   fprintf(stderr,"%s daemon start.\n", myName);

   sigaction(SIGCHLD, &ignoreChildDeath, NULL);		/*zombie protection */
   sigaction(SIGHUP, &hupAction, NULL);
   sigaction(SIGTERM, &termAction, NULL);

   if (terminate == TERM_TERM) {
      go = 0;
   } else {
      go = 1;
   }

   while (go) {			/*The main restart loop */
      terminate = 0;
      
      cfg_read(CONFIG_FILE, "", ""); //read wxd config file here 

      do_opts(argc, argv);       // do command line option processing, they override config file opts 
      if (!foreground) {
	 daemonize(); //safely daemonize myself
      }


      get_configuration(&config, "/etc/open2300.conf"); //read open2300 configuration
      ws2300 = open_weatherstation(config.serial_device_name);

      last_write_time = time(NULL);
      if (ws2300 > 0) {
	 sane_defaults();
	 init_db();

         while(terminate==0) {
             
             if (get_curwx(ws2300)) {
                 db_writer();
                 sleep(10);
             } else {
                 sleep(30); //bad sensor data
             }
         
             if (terminate == TERM_TERM) {
                 errlog1(1, "%s: TERMINATE.", argv[0]);
                 go = 0;
             }
             if (terminate == TERM_RESTART) {
                 errlog1(1, "%s: RESTART.", argv[0]);
                 go = 1;
             }
         } // 'restart' loop
      } else {
	 go = 0;
	 errlog2(1, "%s: cannot open weatherstation on: %s", myName, config.serial_device_name);
      }
   } // 'go' loop
   errlog1(1,"%s daemon shutdown.", myName);
   exit(0);
}

int get_curwx(WEATHERSTATION ws)
{
    double temp_in, temp_out, dewpt_in, dewpt_out, chill, rh_in, rh_out,
        rain_rate, rain_tot, gust, wavg, wavg_dir, barometer;
    int gust_dir,iwavg_dir;
    time_t curtime = 0;
    struct tm *thetime;
    int outdoor_good = 1;

    static time_t lastrain = 0;
    static double rravg = 0.0;
    static double K = 2.0f/(1.0f+9.0f); //9 point EMA


    
    if (sensor_status(ws)) {
        outdoor_good=1;

        //outdoor temperature
        temp_out=temperature_outdoor(ws,CELCIUS);
        correct(temp_out, current_cal.temp_out_mul, current_cal.temp_out_offs);
        if ((temp_out > 60.0) || (temp_out < -45.5)) {
          outdoor_good = 0;
        } else {
          update(current_obs.temp_out, temp_out, TEMPOUT);
        }
        
        //indoor temperature
        temp_in = temperature_indoor(ws,CELCIUS);
        correct(temp_in,current_cal.temp_in_mul, current_cal.temp_in_offs);
        if ((temp_in <= 60.0) && (temp_in >= -45.5)) {
        update(current_obs.temp_in, temp_in, TEMPIN);
        }

        //outdoor dewpoint
        dewpt_out=dewpoint(ws,CELCIUS);
        correct(dewpt_out, current_cal.dp_out_mul, current_cal.dp_out_offs);
        if (outdoor_good && (dewpt_out <= 60.0) && (dewpt_out >= -45.5)) {
          update(current_obs.dp_out, dewpt_out, DEWOUT);
        }

        //outdoor relative humidity, percent
        rh_out = humidity_outdoor(ws);
        correct(rh_out, current_cal.rh_out_mul, current_cal.rh_out_offs);
        if (outdoor_good && (rh_out >= 0.0) && (rh_out <= 150.0))  {
          update(current_obs.rh_out, rh_out, HUMOUT);
        }

        //indoor relative humidity, percent
        rh_in = humidity_indoor(ws);
        correct(rh_in, current_cal.rh_in_mul, current_cal.rh_in_offs);
        if ((rh_in >= 0.0) && (rh_in <= 150.0))  {
          update(current_obs.rh_in, rh_in, HUMIN);
        }
        
        //indoor dewpoint is calculated here. Lacrosse doesn't report it
        dewpt_in=log(rh_in/100.0) + 17.67 *  (temp_in / (243.5 + temp_in)); //wikipedia "dew point"
        update(current_obs.dp_in, dewpt_in, DEWIN);

        //Rain ... instantaneous rain rate isn't reported by Lacrosse
        //so we calculate it here
        curtime=time(NULL);
	if ( lastrain == 0 ) lastrain=curtime;
        
        rain_tot=rain_total(ws,MILLIMETERS); //raw rain total from weather station
        correct(rain_tot, current_cal.rain_tot_mul, current_cal.rain_tot_offs); //corrected for cal factor

        if (outdoor_good) {
          if (rain_tot < current_obs.rain_tot) { //wx station reset?
            current_obs.rtot_offset += current_obs.rain_tot;
          }

	  time_t dt = curtime - lastrain; // delta-t in seconds
	  double dr = rain_tot - current_obs.rain_tot; //millimeters
          update(current_obs.rain_tot, rain_tot, RAINTOT);

	  if  ((dt>0 && dr>0)||(dt>=300)) {
	    rain_rate =  3600.0 * dr / (double)dt;
	    rain_rate = (rain_rate<1.0E-4)?0.0:rain_rate;
	    // exponential moving average.
	    rravg = (K * (rain_rate - rravg)) + rravg;
	    rravg = (rravg < 0.0) ? 0.0 : rravg;
	    //update(current_obs.rain_rate,(rravg<=1e-4)?0.0:rravg,RAINRATE);
	    update(current_obs.rain_rate, rravg, RAINRATE);
	    lastrain=curtime;
	  }
        }
        
        thetime=localtime(&curtime);
        current_obs.sec = thetime->tm_sec;
        current_obs.min = thetime->tm_min;
        current_obs.hour = thetime->tm_hour;
        current_obs.month = 1 + thetime->tm_mon;
        current_obs.day = thetime->tm_mday;
        
        // Wind chill outside
        chill=windchill(ws,CELCIUS);
        correct(chill, current_cal.chill_mul, current_cal.chill_offs);
        if (outdoor_good) {
          update(current_obs.chill, chill, WCHILL);
        }

        //Wind.
        if (outdoor_good) {
          wavg = wind_current(ws,METERS_PER_SECOND,&wavg_dir);

          correct(wavg, current_cal.wavg_spd_mul, current_cal.wavg_spd_offs);
          correct(wavg_dir, current_cal.wavg_dir_mul, current_cal.wavg_dir_offs);

          iwavg_dir = lround(wavg_dir);
          iwavg_dir = (iwavg_dir < 0) ? 360 + ((int) iwavg_dir % 360) : (int) iwavg_dir % 360;

          //use max wind as gust
          gust = wind_minmax(ws,METERS_PER_SECOND,NULL,NULL,NULL,NULL);
          wind_reset(ws,RESET_MIN|RESET_MAX);
        
          correct(gust, current_cal.gust_spd_mul, current_cal.gust_spd_offs);
        
          gust_dir = iwavg_dir;

          update(current_obs.gust, gust, GUST);
          update(current_obs.wavg, wavg, WAVG);
          update(current_obs.gust_dir, gust_dir, GUSTDIR);
          update(current_obs.wavg_dir, iwavg_dir, WAVGDIR);
        }
        
        barometer = rel_pressure(ws,MILLIBARS);
        correct(barometer, current_cal.barometer_mul, current_cal.barometer_offs);
        update(current_obs.barometer, barometer, BARO);

        
    } else { // sensors ok
        return(0);
    }


    return(1);
}

    


void 
do_opts(int argc, char **argv)
{
   int c, errflg = 0;
   extern char *optarg;
   extern int optinf, optopt;

   while ((c = getopt(argc, argv, "d:i:f")) != -1) {
      switch (c) {
      case 'f':
	 foreground++;
	 break;
      case 'd':
	 opts.debug = atoi(optarg);
	 DebugLevel = opts.debug;
	 break;
      case 'i':
	 opts.write_interval = atoi(optarg);
	 break;
      case ':':		/* -f or -o without arguments */
	 fprintf(stderr, "Option -%c requires an argument\n",
		 optopt);
	 errflg++;
	 break;
      case '?':
	 fprintf(stderr, "Unrecognized option: - %c\n",
		 optopt);
	 errflg++;
	 break;
      }
   }
   if (errflg) {
      fprintf(stderr, "usage: %s [-f] [-d debug_level] [-i write_interval]\n", argv[0]);
      exit(2);
   }
}

/*runs before threads start up, and before we start logging to syslog */
/*stolen from Linux Journal, Mar 1998 pp 46-51 */
void 
daemonize(void)
{
   struct rlimit resourceLimit = {0};
   int status = -1;
   int fileDesc = -1;
   int i;

   status = fork();
   switch (status) {
   case -1:
      perror("fork()");
      exit(1);
   case 0:			/* child */
      break;
   default:			/*parent */
      exit(0);
   }
   /* child continues on here ... */
   resourceLimit.rlim_max = 0;
   status = getrlimit(RLIMIT_NOFILE, &resourceLimit);
   if (status == -1) {
      perror("getrlimit()");
      exit(1);
   }
   if (resourceLimit.rlim_max == 0) {
      fprintf(stderr, "No file descriptors avaliable! Exiting.");
      exit(1);
   }
   /* close all open fd's */
   for (i = 0; i < resourceLimit.rlim_max; i++)
      (void) close(i);

   if (setsid() == -1) {
      perror("setsid()");
      exit(1);
   }
   status = fork();
   switch (status) {
   case -1:
      perror("fork()");
      exit(1);
   case 0:			/* second child */
      break;
   default:			/*parent */
      exit(0);
   }

   chdir("/");
   umask(0);
   fileDesc = open("/dev/null", O_RDWR);	/*stdin */
   (void) dup(fileDesc);	/*stdout */
   (void) dup(fileDesc);	/*stderr */

   /* don't dump core */
   status = getrlimit(RLIMIT_CORE, &resourceLimit);
   resourceLimit.rlim_max = 0;
   status = setrlimit(RLIMIT_CORE, &resourceLimit);
   if (status == -1) {
      perror("setrlimit()");
      exit(1);
   }
   return;
}

void 
hupcatch(int pid)
{
   terminate = TERM_RESTART;
   return;
}

void 
termcatch(int pid)
{
   terminate = TERM_TERM;
   return;
}


void 
screen_writer(void)
{
   double rain;
   int write_screen = 1;

   if (terminate)
      write_screen = 0;;
   while (write_screen) {
      if (ch_flag != 0) {
	 if (test_bit(ch_flag, RAINTOT)) {
	    rain = current_obs.rain_tot + current_obs.rtot_offset;
	    errlog1(8, "Rain total: %d mm", rain);
	    clear_bit(ch_flag, RAINTOT);
	 }
	 if (test_bit(ch_flag, RAINRATE)) {
	    errlog1(8, "Rain rate: %d mm/hr", current_obs.rain_rate);
	    clear_bit(ch_flag, RAINRATE);
	 }
	 if (test_bit(ch_flag, TEMPIN)) {
	    errlog1(8, "Indoor temp: %gC", current_obs.temp_in);
	    clear_bit(ch_flag, TEMPIN);
	 }
	 if (test_bit(ch_flag, TEMPOUT)) {
	    errlog1(8, "Outdoor temp: %gC", current_obs.temp_out);
	    clear_bit(ch_flag, TEMPOUT);
	 }
	 if (test_bit(ch_flag, WCHILL)) {
	    errlog1(8, "Wind chill: %gC", current_obs.chill);
	    clear_bit(ch_flag, WCHILL);
	 }
	 if (test_bit(ch_flag, DEWIN)) {
	    errlog1(8, "Indoor dewpoint: %gC", current_obs.dp_in);
	    clear_bit(ch_flag, DEWIN);
	 }
	 if (test_bit(ch_flag, DEWOUT)) {
	    errlog1(8, "Outdoor dewpoint: %gC", current_obs.dp_out);
	    clear_bit(ch_flag, DEWOUT);
	 }
	 if (test_bit(ch_flag, HUMIN)) {
	    errlog1(8, "Indoor humidity: %d%%", current_obs.rh_in);
	    clear_bit(ch_flag, HUMIN);
	 }
	 if (test_bit(ch_flag, HUMOUT)) {
	    errlog1(8, "Outdoor humidity: %d%%", current_obs.rh_out);
	    clear_bit(ch_flag, HUMOUT);
	 }
	 if (test_bit(ch_flag, BARO)) {
	    errlog1(8, "Barometer %g mb", current_obs.barometer);
	    clear_bit(ch_flag, BARO);
	 }
	 if (test_bit(ch_flag, GUSTDIR) && test_bit(ch_flag, GUST)) {
	    errlog1(8, "Wind gust direction: %d deg", current_obs.gust_dir);
	    clear_bit(ch_flag, GUSTDIR);
	 }
	 if (test_bit(ch_flag, GUST)) {
	    errlog1(8, "Wind gust: %g m/s", current_obs.gust);
	    clear_bit(ch_flag, GUST);
	 }
	 if (test_bit(ch_flag, WAVGDIR) && test_bit(ch_flag, WAVG)) {
	    errlog1(8, "Wind avg direction: %d deg", current_obs.wavg_dir);
	    clear_bit(ch_flag, WAVGDIR);
	 }
	 if (test_bit(ch_flag, WAVG)) {
	    errlog1(8, "Wind avg: %g m/s", current_obs.wavg);
	    clear_bit(ch_flag, WAVG);
	 }
      }
      if (terminate)
	 write_screen = 0;;
      /*sleep(1);*/
   }
}

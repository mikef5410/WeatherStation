
#define _POSIX_SOURCE 1
#include <termios.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#define __USE_MISC
#include <signal.h>

#define GLOBAL_WXD
#include "cbuf.h"
#include "serial.h"
#include "parse.h"
#include "wxd.h"
#include "debug.h"
#include "cfg_read.h"
#include "dbif_pg.h"

/* forward decls */
void screen_writer(void);
void do_opts(int argc, char **argv);
void daemon(void);
void hupcatch(int pid);
void termcatch(int pid);

int foreground = 0;
char *myName;

main(int argc, char **argv)
{
   pthread_attr_t sched_attr;
   int maxprio, minprio;
   int go = 1;
   struct sched_param fifo_param;
   struct sigaction ignoreChildDeath =
   {NULL, 0, (SA_NOCLDSTOP | SA_RESTART), NULL};
   struct sigaction hupAction =
   {hupcatch, 0, SA_RESTART, NULL};
   struct sigaction termAction =
   {termcatch, 0, SA_RESTART, NULL};

   myName=argv[0];
   ch_flag = 0;
   terminate = 0;

   fprintf(stderr,"%s daemon start.\n", myName);

   pthread_attr_init(&sched_attr);
   pthread_attr_setinheritsched(&sched_attr, PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&sched_attr, /*SCHED_RR */ SCHED_FIFO);

   maxprio = sched_get_priority_max(SCHED_FIFO);
   minprio = sched_get_priority_min(SCHED_FIFO);

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
      /*read config file here */
      cfg_read(CONFIG_FILE, "", "");
      /* do command line option processing, they override config file opts */
      do_opts(argc, argv);
      if (!foreground) {
	 daemon();
      }
      wxin = serial_open(opts.ttydev);
      last_write_time = time(NULL);
      if (wxin > 0) {
	 sane_defaults();
	 init_db();
	 if (setupio(wxin) < 0) {
	    errlog(9, "setupio bailed.");
	 }
	 iobuf = make_cbuf(BSIZE);
	 pthread_mutex_init(&current_obs_lock, pthread_mutexattr_default);
	 pthread_mutex_init(&current_cal_lock, pthread_mutexattr_default);
	 pthread_mutex_init(&terminate_lock, pthread_mutexattr_default);
	 pthread_mutex_init(&cond_update_lock, pthread_mutexattr_default);
	 pthread_cond_init(&cond_update, pthread_condattr_default); 

#if USE_CBUF

	 fifo_param.sched_priority = (minprio + maxprio) / 2;
	 pthread_attr_setschedparam(&sched_attr, &fifo_param);

	 pthread_create(&reader, /*pthread_attr_default */ &sched_attr, (pthread_startroutine_t) serial_reader, (pthread_addr_t) iobuf);
	 pthread_create(&parser, &sched_attr, (pthread_startroutine_t) parse_input, (pthread_addr_t) iobuf);
#if SCREENOUT
	 pthread_create(&writer, &sched_attr, (pthread_startroutine_t) screen_writer, (pthread_addr_t) NULL);
#else				/* Make a db writer thread */
	 pthread_create(&writer, &sched_attr, (pthread_startroutine_t) db_writer, (pthread_addr_t) NULL);
#endif


	 pthread_join(reader, NULL);
	 pthread_join(parser, NULL);
	 pthread_join(writer, NULL);


#else
	 parse_input(iobuf);
#endif
	 release_cbuf(iobuf);
	 serial_close(opts.ttydev);
	 if (terminate == TERM_TERM) {
	    errlog1(1, "%s: TERMINATE.", argv[0]);
	    go = 0;
	 }
	 if (terminate == TERM_RESTART) {
	    errlog1(1, "%s: RESTART.", argv[0]);
	    go = 1;
	 }
#if SCREENOUT
#else
	 db_getout(dbconn);
#endif
      } else {
	 go = 0;
	 errlog2(1, "%s: cannot open serial port: %s", argv[0], opts.ttydev);
      }
   }
   errlog1(1,"%s daemon shutdown.", myName);
   exit(0);
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
daemon(void)
{
   struct rlimit resourceLimit =
   {0};
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
   pthread_mutex_lock(&terminate_lock);
   terminate = TERM_RESTART;
   pthread_mutex_unlock(&terminate_lock);
   return;
}

void 
termcatch(int pid)
{
   pthread_mutex_lock(&terminate_lock);
   terminate = TERM_TERM;
   pthread_mutex_unlock(&terminate_lock);
   return;
}


void 
screen_writer(void)
{
   double rain;
   int write_screen = 1;

   pthread_mutex_lock(&terminate_lock);
   if (terminate)
      write_screen = 0;;
   pthread_mutex_unlock(&terminate_lock);
   while (write_screen) {
      pthread_mutex_lock(&cond_update_lock);
      pthread_cond_wait(&cond_update, &cond_update_lock);
      pthread_mutex_unlock(&cond_update_lock);
      pthread_mutex_lock(&current_obs_lock);
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
      pthread_mutex_unlock(&current_obs_lock);
      pthread_mutex_lock(&terminate_lock);
      if (terminate)
	 write_screen = 0;;
      pthread_mutex_unlock(&terminate_lock);
      /*sleep(1);*/
   }
   errlog(8, "Screenwriter exits.");
   pthread_exit(0);
}


/* PostgreSQL Interface for the wxd weather daemon */
/* Uses libpq */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define GLOBAL_DBIF
#include "dbif_pg.h"
#include "wxd.h"
#include "cfg_read.h"
#include "debug.h"

#define TRACE(a) fprintf(stderr,a)

void 
db_getout(PGconn * conn)
{
   PQfinish(conn);
}

int 
db_connect(void)
{
   TRACE("db_connect\n");
   /*   dbconn=PQsetdb(opts.dbhost, opts.dbport, opts.dboptions, opts.dbtty, opts.dbname); */
   dbconn = PQsetdb(NULL, NULL, NULL, NULL, opts.dbname);
   if (PQstatus(dbconn) == CONNECTION_BAD) {
      errlog2(1, "Connection to database: %s on host: %s failed.", opts.dbname, opts.dbhost);
      db_getout(dbconn);
      return (-1);
   }
   /*do an initial transation */
   dbres = PQexec(dbconn, "BEGIN; END;");
   if (PQresultStatus(dbres) != PGRES_COMMAND_OK) {
      errlog(1, "Database BEGIN command failed.");
      PQclear(dbres);
      db_getout(dbconn);
      return (-1);
   }
   PQclear(dbres);
   return (1);			/* OK ... database is connected and working */
}

char *
db_getval(int tuple, char *fname)
{
   char *rval;

   rval = PQgetvalue(dbres, tuple, PQfnumber(dbres, fname));
   fprintf(stderr,"db_getval: %d: %s = %s\n",tuple,fname,rval);
   return (rval);
}

void 
db_makedatetime(char *rval, int mo, int dd, int yy, int hh, int mm, int ss)
{

   if (yy < 70)
      yy += 2000;
   if (yy >= 70 && yy < 100)
      yy += 1900;
   sprintf(rval, "%d/%d/%d %2d:%2d:%2d", mo, dd, yy, hh, mm, ss);
   return;
}

void 
db_curtime(char *retval)
{
   time_t now;
   struct tm *t;

   now = time((time_t *) NULL);
   t = localtime(&now);
   strftime(retval, 128, "%m/%d/%Y %H:%M:%S", t);
   return;
}


void 
db_writer(void)
{
   char sql_str[2048], comma;
   char timestr[128];
   short didone = 0;
   short ix = 0;
   short need_write = 0;
   time_t curtime;
   int write_db = 1;

   TRACE("db_writer\n");
   pthread_mutex_lock(&terminate_lock);
   if (terminate)
      write_db = 0;;
   pthread_mutex_unlock(&terminate_lock);
   while (write_db) {
      pthread_mutex_lock(&cond_update_lock);
      pthread_cond_wait(&cond_update, &cond_update_lock);
      pthread_mutex_unlock(&cond_update_lock);
      pthread_mutex_lock(&current_obs_lock);
      if (ch_flag != 0) {
	 sprintf(sql_str, "BEGIN; UPDATE current_obs SET ");
	 ix = strlen(sql_str);

	 if (test_bit(ch_flag, RAINTOT)) {
	    sprintf(&sql_str[ix], "rain_tot = %d, rtot_offset = %d ", current_obs.rain_tot, current_obs.rtot_offset);
	    ix = strlen(sql_str);
	    didone++;
	    need_write++;
	    clear_bit(ch_flag, RAINTOT);
	 }
	 if (test_bit(ch_flag, RAINRATE)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c rain_rate = %d ", comma, current_obs.rain_rate);
	    ix = strlen(sql_str);
	    didone++;
	    need_write++;
	    clear_bit(ch_flag, RAINRATE);
	 }
	 if (test_bit(ch_flag, TEMPIN)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c temp_in = %g ", comma, current_obs.temp_in);
	    ix = strlen(sql_str);
	    didone++;
	    clear_bit(ch_flag, TEMPIN);
	 }
	 if (test_bit(ch_flag, TEMPOUT)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c temp_out = %g ", comma, current_obs.temp_out);
	    ix = strlen(sql_str);
	    didone++;
	    clear_bit(ch_flag, TEMPOUT);
	 }
	 if (test_bit(ch_flag, WCHILL)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c chill = %g ", comma, current_obs.chill);
	    ix = strlen(sql_str);
	    didone++;
	    clear_bit(ch_flag, WCHILL);
	 }
	 if (test_bit(ch_flag, DEWIN)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c dp_in = %g ", comma, current_obs.dp_in);
	    ix = strlen(sql_str);
	    didone++;
	    clear_bit(ch_flag, DEWIN);
	 }
	 if (test_bit(ch_flag, DEWOUT)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c dp_out = %g ", comma, current_obs.dp_out);
	    ix = strlen(sql_str);
	    didone++;
	    clear_bit(ch_flag, DEWOUT);
	 }
	 if (test_bit(ch_flag, HUMIN)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c rh_in = %d ", comma, current_obs.rh_in);
	    ix = strlen(sql_str);
	    didone++;
	    clear_bit(ch_flag, HUMIN);
	 }
	 if (test_bit(ch_flag, HUMOUT)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c rh_out = %d ", comma, current_obs.rh_out);
	    ix = strlen(sql_str);
	    didone++;
	    clear_bit(ch_flag, HUMOUT);
	 }
	 if (test_bit(ch_flag, BARO)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c barometer = %g ", comma, current_obs.barometer);
	    ix = strlen(sql_str);
	    didone++;
	    clear_bit(ch_flag, BARO);
	 }
	 if (test_bit(ch_flag, GUSTDIR) && test_bit(ch_flag, GUST)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c gust_dir = %d ", comma, current_obs.gust_dir);
	    ix = strlen(sql_str);
	    didone++;
	    clear_bit(ch_flag, GUSTDIR);
	 }
	 if (test_bit(ch_flag, GUST)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c gust_spd = %g ", comma, current_obs.gust);
	    ix = strlen(sql_str);
	    didone++;
	    need_write++;
	    clear_bit(ch_flag, GUST);
	 }
	 if (test_bit(ch_flag, WAVGDIR) && test_bit(ch_flag, WAVG)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c wavg_dir = %d ", comma, current_obs.wavg_dir);
	    ix = strlen(sql_str);
	    didone++;
	    clear_bit(ch_flag, WAVGDIR);
	 }
	 if (test_bit(ch_flag, WAVG)) {
	    comma = didone ? ',' : ' ';
	    sprintf(&sql_str[ix], "%c wavg_spd = %g ", comma, current_obs.wavg);
	    ix = strlen(sql_str);
	    didone++;
	    need_write++;
	    clear_bit(ch_flag, WAVG);
	 }
	 if (didone) {
	    db_curtime(timestr);
	    sprintf(&sql_str[ix], ", obstime = '%s' ", timestr);
	    ix = strlen(sql_str);
	    didone++;
	    sprintf(&sql_str[ix], " WHERE selector = 1; END; ");
	    dbres = PQexec(dbconn, sql_str);
	    errlog(1, sql_str);
	    if (PQresultStatus(dbres) != PGRES_COMMAND_OK) {
	       errlog(1, "Database UPDATE command failed.");
	       PQclear(dbres);
	    }
	    PQclear(dbres);
	 }
	 didone = 0;
      }
      pthread_mutex_unlock(&current_obs_lock);
      curtime = time(NULL);
      if (curtime - last_write_time > WRITE_INTERVAL || need_write) {
	 write_obs();
	 last_write_time = time(NULL);
	 need_write = 0;
      }
      pthread_mutex_lock(&terminate_lock);
      if (terminate)
	 write_db = 0;;
      pthread_mutex_unlock(&terminate_lock);
      /*sleep(1);*/
   }
   errlog(8, "dbwriter exits.");
   pthread_exit(0);
}

void 
write_obs(void)
{
   char sql_str[1024];
   char datime[128];
   int rain;

   TRACE("write_obs\n");
   db_curtime(datime);
   pthread_mutex_lock(&current_obs_lock);
   rain = current_obs.rain_tot + current_obs.rtot_offset;
   sprintf(sql_str, "BEGIN; INSERT INTO wxobs VALUES ('%s',%g,%g,%d,%d,%g,%g,%g,%d,%d,%g,%d,%g,%d,%g); END;",
	   datime,
	   current_obs.temp_in, current_obs.temp_out,
	   current_obs.rh_in, current_obs.rh_out,
	   current_obs.dp_in, current_obs.dp_out,
	   current_obs.chill,
	   rain, current_obs.rain_rate,
	   current_obs.gust, current_obs.gust_dir,
	   current_obs.wavg, current_obs.wavg_dir,
	   current_obs.barometer);

   pthread_mutex_unlock(&current_obs_lock);
   dbres = PQexec(dbconn, sql_str);
   errlog(1, sql_str);
   if (PQresultStatus(dbres) != PGRES_COMMAND_OK) {
      errlog(1, "Database INSERT command failed.");
      PQclear(dbres);
   }
   PQclear(dbres);
}

void 
sane_defaults(void)
{
   pthread_mutex_lock(&current_obs_lock);
   current_obs.temp_in = 0;
   current_obs.temp_out = 0;
   current_obs.rh_in = 0;
   current_obs.rh_out = 0;
   current_obs.dp_in = 0;
   current_obs.dp_out = 0;
   current_obs.chill = 0;
   current_obs.rtot_offset = 0;
   current_obs.rain_tot = 0;
   current_obs.rain_rate = 0;
   current_obs.gust = 0;
   current_obs.gust_dir = 0;
   current_obs.wavg = 0;
   current_obs.wavg_dir = 0;
   current_obs.barometer = 0;
   pthread_mutex_unlock(&current_obs_lock);

   pthread_mutex_lock(&current_cal_lock);

   current_cal.temp_in_mul = 1.0;
   current_cal.temp_in_offs = 0.0;

   current_cal.temp_out_mul = 1.0;
   current_cal.temp_out_offs = 0.0;

   current_cal.rh_in_mul = 1.0;
   current_cal.rh_in_offs = 0.0;

   current_cal.rh_out_mul = 1.0;
   current_cal.rh_out_offs = 0.0;

   current_cal.dp_in_mul = 1.0;
   current_cal.dp_in_offs = 0.0;

   current_cal.dp_out_mul = 1.0;
   current_cal.dp_out_offs = 0.0;

   current_cal.chill_mul = 1.0;
   current_cal.chill_offs = 0.0;

   current_cal.rain_tot_mul = 1.0;
   current_cal.rain_tot_offs = 0.0;

   current_cal.rain_rate_mul = 1.0;
   current_cal.rain_rate_offs = 0.0;

   current_cal.gust_spd_mul = 1.0;
   current_cal.gust_spd_offs = 0.0;

   current_cal.gust_dir_mul = 1.0;
   current_cal.gust_dir_offs = 0.0;

   current_cal.wavg_spd_mul = 1.0;
   current_cal.wavg_spd_offs = 0.0;

   current_cal.wavg_dir_mul = 1.0;
   current_cal.wavg_dir_offs = 0.0;

   current_cal.barometer_mul = 1.0;
   current_cal.barometer_offs = 0.0;

   pthread_mutex_unlock(&current_cal_lock);
}

int 
init_db(void)
{
   int tup, field;
   char *ret_val;
   char **endptr;
   char *xx;

   TRACE("init_db\n");

   if (db_connect() != 1) {
      errlog(1, "Database error. Bailing out.");
      return (-1);
   }
   /* Load previous current_obs into current_obs */
   dbres = PQexec(dbconn, "select * from current_obs where selector = 1;");

   tup = PQntuples(dbres) - 1;
   /* There should only be one row in this table, but take the last one
      just in case */
   pthread_mutex_lock(&current_obs_lock);

   /*On first use, the database could have NO rows in 
     current_obs. Initialize the values to 0.0 */
   if (tup >= 0) {
   current_obs.temp_in = atof(db_getval(tup, "temp_in"));
   current_obs.temp_out = atof(db_getval(tup, "temp_out"));
   current_obs.rh_in = atoi(db_getval(tup, "rh_in"));
   current_obs.rh_out = atoi(db_getval(tup, "rh_out"));
   current_obs.dp_in = atof(db_getval(tup, "dp_in"));
   current_obs.dp_out = atof(db_getval(tup, "dp_in"));
   current_obs.chill = atof(db_getval(tup, "chill"));
   current_obs.rtot_offset = atoi(db_getval(tup, "rtot_offset"));
   current_obs.rain_tot = atoi(db_getval(tup, "rain_tot"));
   current_obs.rain_rate = atoi(db_getval(tup, "rain_rate"));
   current_obs.gust = atof(db_getval(tup, "gust_spd"));
   current_obs.gust_dir = atoi(db_getval(tup, "gust_dir"));
   current_obs.wavg = atof(db_getval(tup, "wavg_spd"));
   current_obs.wavg_dir = atoi(db_getval(tup, "wavg_dir"));
   current_obs.barometer = atof(db_getval(tup, "barometer"));
   } else {
   current_obs.temp_in = 0.0;
   current_obs.temp_out = 0.0;
   current_obs.rh_in = 0.0;
   current_obs.rh_out = 0.0;
   current_obs.dp_in = 0.0;
   current_obs.dp_out = 0.0;
   current_obs.chill = 0.0;
   current_obs.rtot_offset = 0.0;
   current_obs.rain_tot = 0.0;
   current_obs.rain_rate = 0.0;
   current_obs.gust = 0.0;
   current_obs.gust_dir = 0.0;
   current_obs.wavg = 0.0;
   current_obs.wavg_dir = 0.0;
   current_obs.barometer = 0.0;
   }

   pthread_mutex_unlock(&current_obs_lock);
   PQclear(dbres);

   dbres = PQexec(dbconn, "select * from cal_fact;");
   tup = PQntuples(dbres) - 1;	/*Use the last tuple (most recent) */
   pthread_mutex_lock(&current_cal_lock);

   current_cal.temp_in_mul = atof(db_getval(tup, "temp_in_mul"));
   current_cal.temp_in_offs = atof(db_getval(tup, "temp_in_offs"));

   current_cal.temp_out_mul = atof(db_getval(tup, "temp_out_mul"));
   current_cal.temp_out_offs = atof(db_getval(tup, "temp_out_offs"));

   current_cal.rh_in_mul = atof(db_getval(tup, "rh_in_mul"));
   current_cal.rh_in_offs = atof(db_getval(tup, "rh_in_offs"));

   current_cal.rh_out_mul = atof(db_getval(tup, "rh_out_mul"));
   current_cal.rh_out_offs = atof(db_getval(tup, "rh_out_offs"));

   current_cal.dp_in_mul = atof(db_getval(tup, "dp_in_mul"));
   current_cal.dp_in_offs = atof(db_getval(tup, "dp_in_offs"));

   current_cal.dp_out_mul = atof(db_getval(tup, "dp_out_mul"));
   current_cal.dp_out_offs = atof(db_getval(tup, "dp_out_offs"));

   current_cal.chill_mul = atof(db_getval(tup, "chill_mul"));
   current_cal.chill_offs = atof(db_getval(tup, "chill_offs"));

   current_cal.rain_tot_mul = atof(db_getval(tup, "rain_tot_mul"));
   current_cal.rain_tot_offs = atof(db_getval(tup, "rain_tot_offs"));

   current_cal.rain_rate_mul = atof(db_getval(tup, "rain_rate_mul"));
   current_cal.rain_rate_offs = atof(db_getval(tup, "rain_rate_offs"));

   current_cal.gust_spd_mul = atof(db_getval(tup, "gust_spd_mul"));
   current_cal.gust_spd_offs = atof(db_getval(tup, "gust_spd_offs"));

   current_cal.gust_dir_mul = atof(db_getval(tup, "gust_dir_mul"));
   current_cal.gust_dir_offs = atof(db_getval(tup, "gust_dir_offs"));

   current_cal.wavg_spd_mul = atof(db_getval(tup, "wavg_spd_mul"));
   current_cal.wavg_spd_offs = atof(db_getval(tup, "wavg_spd_offs"));

   current_cal.wavg_dir_mul = atof(db_getval(tup, "wavg_dir_mul"));
   current_cal.wavg_dir_offs = atof(db_getval(tup, "wavg_dir_offs"));

   current_cal.barometer_mul = atof(db_getval(tup, "barometer_mul"));
   current_cal.barometer_offs = atof(db_getval(tup, "barometer_offs"));

   pthread_mutex_unlock(&current_cal_lock);
   PQclear(dbres);
}


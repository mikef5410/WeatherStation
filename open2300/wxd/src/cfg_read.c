#include <stdio.h>
#include <string.h>

#define GLOBAL_CFGREAD
#include "cfg_read.h"



void 
cfg_read(char *file, char *section, char *class)
{
  int j;
   MOptionsInit(optlist);
   MOptionsRead(file, section, class, optlist, NULL);

   LogFacility=LOG_LOCAL0;

   for (j=0; j<strlen(opts.logFacility); j++) {
     opts.logFacility[j]=toupper(opts.logFacility[j]);
   }

   for (j=0; LogFacilityList[j].facility!=NULL; j++) {
     if (!strcmp( LogFacilityList[j].facility, opts.logFacility)) {
       LogFacility=LogFacilityList[j].ifacility;
       break;
     }
   }

   UseSyslog=opts.useSyslog;
   DebugLevel=opts.debug;

   return;
}

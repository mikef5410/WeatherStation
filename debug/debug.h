#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef _GLOBAL_DEBUG
#define GLOBAL
#else
#define GLOBAL extern
#endif

extern char *myName;
GLOBAL int DebugLevel;
GLOBAL int LogFacility;
GLOBAL int UseSyslog;
GLOBAL void errorlog(int level, ...); /* params are errorlog(level,file_name,line,format,args...) */

#define errlog(lev,msg) errorlog((lev),__FILE__,__LINE__,(msg));
#define errlog1(lev,msg,a) errorlog((lev),__FILE__,__LINE__,(msg),(a));
#define errlog2(lev,msg,a,b) errorlog((lev),__FILE__,__LINE__,(msg),(a),(b));
#define errlog3(lev,msg,a,b,c) errorlog((lev),__FILE__,__LINE__,(msg),(a),(b),(c));
#define errlog4(lev,msg,a,b,c,d) errorlog((lev),__FILE__,__LINE__,(msg),(a),(b),(c),(d));
#define errlog5(lev,msg,a,b,c,d,e) errorlog((lev),__FILE__,__LINE__,(msg),(a),(b),(c),(d),(e));
#define errlog6(lev,msg,a,b,c,d,e,f) errorlog((lev),__FILE__,__LINE__,(msg),(a),(b),(c),(d),(e),(f));
#define errlog7(lev,msg,a,b,c,d,e,f,g) errorlog((lev),__FILE__,__LINE__,(msg),(a),(b),(c),(d),(e),(f),(g));
#define errlog8(lev,msg,a,b,c,d,e,f,g,h) errorlog((lev),__FILE__,__LINE__,(msg),(a),(b),(c),(d),(e),(f),(g),(h));

#undef GLOBAL
#endif

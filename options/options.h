#ifndef _OPTIONS_H
#define _OPTIONS_H

#define FIELD_SEPARATOR ':'
#define COMMENT_CHAR '#'
#define QUOTES "'\""
#define MAX_SECTION_NAME 50
#define MAX_CLASS_NAME MAX_SECTION_NAME
#define SECTION_TITLE "Module"
#define CLASS_TITLE "Configuration"

typedef char boolean;
typedef enum {MoosOptInt, MoosOptFloat, MoosOptChar, MoosOptStr, MoosOptBool}
             MoosTypes;

typedef struct Options {
  char *name;
  MoosTypes type;
  void *ptr;
  int optional;
  struct Flags {
    int specified;
  } f;
} Options;

#define OFFSETOF(STRUC, FIELD) (((STRUC *)0)->FIELD)

void MOptionsInit(Options *opts);
int MOptionsRead(char *fname, char *class, char *section, Options *optlist,
		 void *base);
void MOptionsUninit(Options *opts);

#endif

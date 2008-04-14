#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "options.h"

/*#define CASESENSITIVE */
#ifdef CASESENSITIVE
#define STRCMP strcmp
#define STRNCMP strncmp
#else
#define STRCMP strcasecmp
#define STRNCMP strncasecmp
#endif

#ifdef LACKSSTRCASE
int
strcasecmp(s1, s2)
      char *s1, *s2;
{
  while(*s1 && *s2 && toupper(*s1) == toupper(*s2))
    { s1++; s2++; }

  return (int)*s1 - (int)*s2;
}

int
strncasecmp(s1, s2, n)
     char *s1, *s2;
     int n;
{
  if(n==0) return 0;

  while(--n && *s1 && *s2 && toupper(*s1) == toupper(*s2))
    { s1++; s2++; }

  return (int)toupper(*s1) - (int)toupper(*s2);
}
#endif


/* Remove the trailing spaces and trailing comment. Yes, i didn't want to
   come up with a name for this, nor did i want to splice it in the middle
   of the main parser function.
*/
char *
clearDaDamnTrailingSpaces(p)
     char *p;
{
  char *v;

  /* Poke out the string at a a comment char if there is one. */
  if(v = strchr(p, COMMENT_CHAR))
    *v = '\0';

  for(v = p+strlen(p)-1; v>p && isspace(*v) ; --v)
    *v = '\0';

  return p;
}


/* Advances s until it points to a non-space. Returns the new s.
*/
char *
skipspaces(s)
     char *s;
{
  while(*s && isspace(*s))++s;

  return s;
}

/* Advances s until it points to a space. */
char *
jumptospace(s)
     char *s;
{
  while(*s && !isspace(*s))++s;

  return s;
}

/* Advances the pointer to the end of the net token.
   For now, a token is delimited by white spaces or by the field
   separator..
*/
char *
skipToken(s)
     char *s;
{
  s = skipspaces(s);
  while(*s && *s!=FIELD_SEPARATOR && !isspace(*s))++s;

  return s;
}


int
isin(c, lst)
     char c;
     char *lst;
{
  while(*lst &&  c!=*lst)lst++;

  return *lst;
}

/* Given a list of space separated strings (slst), determines if
   s is one of these strings.
*/
int
strisin(s, slst)
     char *s;
     char *slst;
{
  char *spc;
  int done=0;

  while(!done)
    {
      spc = jumptospace(slst);
      done = *spc == '\0';
      *spc = '\0';

      if(!STRCMP(s, slst)) return 1;
      slst = spc+1;
    }

  return 0;
}


/* MOptionsInit()
 *--------------------------------------------------------------------
 * Performs global initializations for the entire options library.
 *--------------------------------------------------------------------
 * Actually, this function is really a hoax. It doesn't do anything
 * currently. But one day it might, so call it.
*/
void
MOptionsInit(opts)
     Options *opts;
{
}

/* MOptionsUninit()
 *--------------------------------------------------------------------
 * Uninitializes the library.
 *--------------------------------------------------------------------
 * Performs any uniniting necessary when this library is no longer needed
 * for this option list. This function doesn't really do anything in this
 * version either.
 */
void
MOptionsUninit(Options *opts)
{
}

/* Performs various initializations for ReadOptions.
*/
void
InitRun(opts)
     Options *opts;
{
  Options *op;

  for(op = opts; op->name; ++op)
    {
      op->f.specified = 0;
    }
}

void
WarnUnspecified(opts)
     Options *opts;
{
  Options *op;

  for(op = opts; op->name; ++op)
    if(!op->f.specified)
      fprintf(stderr, "Warning: option \"%s\" has not been specified!\n",
	       op->name);

}


/* Sets the option name optname to value val. Converts
   val to the proper type.
*/
void
SetValue(optlist, optname, val, line_num, fname)
     Options *optlist;
     char *optname, *val;
     int line_num;
     char *fname;
{
  Options *op;

  for(op = optlist; op->name && STRCMP(op->name, optname); ++op);

  if(!op->name)
    {
#ifdef CJHASREFORMEDHISWAYS
     fprintf(stderr, "Unknown option \"%s\" on line %d in %s.\n",
	      optname, line_num, fname);
#endif
      return;
    }

  switch(op->type)
    {
    case MoosOptInt:
      *((int *)op->ptr) = atoi(val);
      break;
    case MoosOptFloat:
      *((float *)op->ptr) = atof(val);
      break;
    case MoosOptChar:
    case MoosOptStr:
      {
	int vlen = strlen(val)-1;

	if(!isin(val[0], QUOTES) || !isin(val[vlen], QUOTES))
	  {
	    fprintf(stderr, "String value for \"%s\", on line %d of file \"%s\" must be enclosed within quotes.\n",
		    op->name, line_num, fname, val);
	    return;
	  }

	val[vlen] = '\0';
	++val;

	if(op->type==MoosOptChar)
	  *((char *)op->ptr) = *val;
	else
	  {
	    strncpy((char *)op->ptr, val, op->optional>vlen ? vlen : op->optional);
	    ((char *)op->ptr)[op->optional-1] = '\0';
	  }
      }
      break;
    case MoosOptBool:
      {
	boolean v = 0;

	if(*val=='1' || toupper(*val)=='T')
	  v = 1;
	else if(*val=='0' || toupper(*val)=='F')
	  v = 0;
	else
	  fprintf(stderr, "Bad value for option \"%s\", on line %d of \
file \"%s\": \"%s\"\n", op->name, line_num, fname, val);

	*((boolean *)op->ptr) = v;
      }
      break;
    }

  op->f.specified = 1;
}


/* Adjusts offsets in the option list by adding ``base'' to all of
   the addresses.
*/
void
AdjustOffsets(optlist, base)
    Options *optlist;
    void *base;
{
    int i;

    for(i=0; optlist[i].name; ++i)
	optlist[i].ptr = (void *)((int)optlist[i].ptr + (int)base);
}


/* MOptionsRead()
 *--------------------------------------------------------------------
 * Reads options into an options list.
 *--------------------------------------------------------------------
 * Please see the <a href="README">README</a> file for information about
 * the syntax of the configuration file and the way the ``class'' and
 * ``section'' parameters are used.
 * 
 * The options list is defined as an array of @Options, the last element
 * of which has its <code>name</code> field set to NULL. Each entry has
 * associated with it this very name, a type, a location where the option's
 * value should be deposited, and some optional argument depending on the
 * type of the option.
 * The valid types are ``MoosOptInt'', ``MoosOptFloat'', ``MoosOptChar'',
 * ``MoosOptStr'', ``MoosOptBool''. For ``MoosOptStr'' options, an
 * optional value specifying the maximum length of the string must be
 * given.
 *
 * The ``base'' parameter specifies an offset by which all the addresses
 * in ``optlist'' must be adjusted. If you use the @OFFSETOF macro to
 * specify an offset location in ``optlist'', pass the address of the
 * structure in which you want to deposit the values through ``base''.
 * Otherwise, just pass NULL for ``base''.
 */
int
MOptionsRead(fname, section, class, optlist, base)
     char *fname, *class, *section;
     Options *optlist;
     void *base;
{
  FILE *db = fopen(fname, "r");
  char *p, ln[85];
  int line_number = 0;
  char *optname, *sep;
  int mySection, myClass;
  int section_title_len, class_title_len;

  AdjustOffsets(optlist, base);

  if(db == NULL)
    {
      fprintf(stderr, "%s: Error openning options file \"%s\"\n",
	      section, fname);
      return 1;
    }

  InitRun(optlist);

  /* Random initialization crap. */
  mySection = 1;
  myClass = 1;
  section_title_len = strlen(SECTION_TITLE);
  class_title_len = strlen(CLASS_TITLE);

  while(p = fgets(ln, 80, db))
    {
      char *sep;

      line_number++;

      /* Skip beginning blanks. */
      p = skipspaces(p);

      /* Handle comments. */
      if(*p == COMMENT_CHAR)
	continue;

      /* Remove the trailing \n. */
      p[strlen(p)-1] = '\0';

      /* Remove the trailing spaces. */
      clearDaDamnTrailingSpaces(p);

      /* Skip empty lines. */
      if(!*p)
	continue;

      /* Check to see if it's a special label. */
      if(!STRNCMP(p, SECTION_TITLE, section_title_len))
	{
	  /* Get the list of sections. */
	  char *sectionNames = skipspaces(p+section_title_len);

	  /* See if we've just entered the right section. */
	  mySection = strisin(section, sectionNames);

	  /* If we've just entered the right section, the current
	     position is considered to be the right class as well.
	  */
	  myClass = mySection;
	  continue;
	}
      else if(!STRNCMP(p, CLASS_TITLE, class_title_len))
	{
	  /* If we're in the right section, check to see if we have the
	     right class name.
          */
	  if(mySection)
	    {
	      char *className = skipspaces(p+class_title_len);
	      myClass = !STRCMP(className, class);
	    }
	  else
	    myClass = 0;
	  continue;
	}

      /* See if we are in a class of interest before parsing the
	 line any further. */
      if(!myClass) continue;

      /* Find the field separator on the line. */
      sep = skipToken(p);
      if(!*sep)
	{
	  fprintf(stderr, "Syntax error at line %d of %s. Missing ':'.\n",
		  line_number, fname);
	  return 1;
	}

      /* Terminate the line here. p is now the null terminated option
	 string. Way may have either clobbered a space or the field
	 separator by doing this. */
      *sep = '\0';

      optname = p;

      /* Skip to the separator or to the beginning of the value if we've
       killed the separator. */
      sep = skipspaces(sep+1);

      /* If we're at the separator, then skip it and the spaces afterwards. */
      if(*sep==FIELD_SEPARATOR) sep = skipspaces(sep+1);
      if(!*sep)
	{
	  fprintf(stderr, "Warning: No value specified for option \"%s\" on \
line %d of %s.\n", p, line_number, fname);
	  continue;
	}

      /* Set the value of the option to that specified on this line. */
      SetValue(optlist, optname, sep, line_number, fname);
    }

  /*  WarnUnspecified(optlist); */
  fclose(db);

  return 0;
}

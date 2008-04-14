#include <stdio.h>
#include "options.h"

struct {
  int a1;
  int b1;
  float f1;
  char str[50];
  boolean ff;
  char ch;
} opts = {-1, -3, -34.0, "fo!!!", 0, '-'};

Options optlist[] = {
  {"A1", MoosOptInt, &opts.a1},
  {"B1", MoosOptInt, &opts.b1},
  {"F1", MoosOptFloat, &opts.f1},
  {"STR", MoosOptStr, opts.str, 5},
  {"FF", MoosOptBool, &opts.ff},
  {"CH", MoosOptChar, &opts.ch},
  {NULL}
};



main()
{
  MOptionsInit(optlist);
  MOptionsRead("moos.cfg", "test", "new", optlist, NULL);

  printf("a1 %d\nb1 %d\nf1 = %f\nstr \"%s\"\nff %d\nch '%c'\n",
	 opts.a1, opts.b1, opts.f1, opts.str, opts.ff, opts.ch);
}





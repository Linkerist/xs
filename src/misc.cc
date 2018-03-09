#include "misc.h"

#include "xs.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace std;

void
version(void)
{
 printf("xs-%s\n", VERSION);
}

static inline void
emit_try_help(void)
{
 cerr << "Try '" << program_name << " --help' for more information." << endl;
}

void
usage(int status)
{
 if (status != EXIT_SUCCESS)
  emit_try_help();
 else {
  fprintf(stderr, "xs version 0.0.1 command:\n");

  // xs imported.
  fprintf(stderr, "Just xs, forget cd - expanding the shell built-in cd\n\n");
  fprintf(stderr, "  Usually you don't call this program directly\n");
  fprintf(stderr, "  due to we cannot realize the same effect as the shell built-in command.\n");
  fprintf(stderr, "  invoke a shell alias we have already provided.\n");
  fprintf(stderr, "\n  For general usage press 'H' while running.\n\n\n");
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  xs              \n");
  fprintf(stderr, "  xs [--noresolve] <Needle>\n");
  fprintf(stderr, "                      Needle is a filter for the list: each\n");
  fprintf(stderr, "                      entry is compared to the Needle and if it\n");
  fprintf(stderr, "                      doesn't match it won't show up.\n");
  fprintf(stderr, "                      The Needle performs some magic here. See\n");
  fprintf(stderr, "                      manpage for details.\n");
  fprintf(stderr, "                      If --noresolve is given the list will be shown\n");
  fprintf(stderr, "                      even if Needle matches a description exactly.\n");
  fprintf(stderr, "  xs <digit>      Needle is a digit: make digit the current entry\n\n");
  fprintf(stderr, "  xs [-a|--add]=[:desc:]path \n");
  fprintf(stderr, "                              add PATH to the list file using the\n");
  fprintf(stderr, "                              optional description between the colons\n");
  fprintf(stderr, "                              and exit\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Other Options\n");
  fprintf(stderr, "  -o,- -output=FILE  use FILE as result file\n"); 
  fprintf(stderr, "  -r, --nowrap       change the scrolling behaviour for long lists\n");
  fprintf(stderr, "  -b, --browse       start in BROWSE mode with the current dir\n"); 
  fprintf(stderr, "  -h, --help         print this help message and exit\n"); 
  fprintf(stderr, "  -v, --version      print version info and exit\n");
  fprintf(stderr, "\n");
 }

 exit(status);
}

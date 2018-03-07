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
 //fprintf (stderr, "Try '%s --help' for more information.\n", program_name);
 cerr << "Try '" << program_name << " --help' for more information." << endl;
}

void
usage(int status)
{
 if (status != EXIT_SUCCESS)
  emit_try_help();
 else {
  fprintf(stderr, "xs version 0.0.1 command:\n");
  fprintf(stderr, "<xs -s dir [position]>  add a directory to $__xs_history_dir.\n");
  fprintf(stderr, "if content = ./, then add current directory to $__xs_history_dir.\n");
  fprintf(stderr, "if position(1-20) is not, default value equal 1.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "<xs -d [position]> then delete a directory from $__xs_history_dir.\n");
  fprintf(stderr, "if position is not, default value equal last.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "<xs -l [position]> then list directory from $__xs_history_dir.\n");
  fprintf(stderr, "if position is not, default value equal 1,");
  fprintf(stderr, "else list all directory, then choice one position's content and enter it.");
  fprintf(stderr, "\n");
  fprintf(stderr, "<xs -e [position]> then enter a directory from $__xs_history_dir.\n");
  fprintf(stderr, "if position is not, default value equal 1.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "<xs -c> then clear $__xs_history_dir.\n");
  fprintf(stderr, "\n");

  // xs imported.
  fprintf(stderr, "expanding the shell built-in cd with bookmarks and browser\n\n");
  fprintf(stderr, "  Usually you don't call this program directly.\n");
  fprintf(stderr, "  Copy it somewhere into your path and create\n");
  fprintf(stderr, "  a shell alias (which takes arguments) as described\n");
  fprintf(stderr, "  in the file INSTALL which came with this program.\n");
  fprintf(stderr, "\n  For general usage press 'H' while running.\n\n\n");
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  xs              select from bookmarks\n");
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
  fprintf(stderr, "  -f, --file=PATH    take PATH as bookmark file\n"); 
  fprintf(stderr, "  -u, --user=USER    read (default) bookmark file of USER\n"); 
  fprintf(stderr, "  -o,- -output=FILE  use FILE as result file\n"); 
  fprintf(stderr, "  -r, --nowrap       change the scrolling behaviour for long lists\n");
  fprintf(stderr, "  -c, --cwd          make current directory the current entry if on the list\n");
  fprintf(stderr, "  -b, --browse       start in BROWSE mode with the current dir\n"); 
  fprintf(stderr, "  -h, --help         print this help message and exit\n"); 
  fprintf(stderr, "  -v, --version      print version info and exit\n");
  fprintf(stderr, "\n");
 }

 exit(status);
}

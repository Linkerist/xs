#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <string.h>
#include <getopt.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>

using namespace std;

#include "xs.h"

#include "ui.h"
#include "io.h"
#include "misc.h"

#define HISTORY_FILE ".xs_history_path"

#define BUFFER_SIZE (16 * 1024)
#define MAX_LINE_SIZE 1024

vector<pair<string, string> > cur_list, default_list;

const char * DefaultListfile = ".xsargs";
const char * DefaultResultfile = "~/.xsresult";

const char * Needle = NULL;
bool NeedleGiven = false;

int CurrPosition = 0;
map<const char*, int, ltstr> LastPositions;

// xs globals
int mode = LIST;
bool show_hidden_files = false;
bool opt_no_wrap = false;
bool opt_no_resolve = false;
bool listfile_empty = false;

string opt_resultfile = "";

typedef enum exit_values {
 total_success = 0,
 invocation_error = 1,
 xs_error = 2,
 xs_punt = 3,
 xs_fatal = 4,
 system_error = 5
} exit_values_ty;

//typedef _Bool bool;

string program_name = "xs";

string
canonify_filename(string filename)
{
 size_t pos = 0;

 // remove double slashes
 if ((pos = filename.find("//", 0)) < filename.size())
  filename.replace(pos, 2, "/");

 if (filename[0] != '~')
  return filename;

 // resolve home directory
 return string(getenv("HOME")) + filename.substr(1);
}

string
get_listfile(void)
{
 string user = "~";
 string file = DefaultListfile;
 string effective_listfile = "";

 if (user[0] == '~')
  effective_listfile = user + "/" + file;
 else
  effective_listfile = "/home" + user + "/" + file;

  return canonify_filename(effective_listfile);
}

void *
xmalloc(size_t n)
{
 void * p = malloc(n);
 if (!p && n != 0) {
  fprintf(stderr, "memory exhausted\n");
  abort();
 }

 return p;
}

void
abort_xs(void)
{
 curs_set(1); // re-show cursor (FIXME: necessary?)
 clear();
 refresh();   // ..and make sure we really see the clean screen
 //resetty();   // ??
 endwin();    // finish the curses at all
 exit(-1);
}

char *
get_cwd_as_charp(void)
{
 char * buf;
 char * result = NULL;
 size_t size = (size_t)pathconf(".", _PC_PATH_MAX);

 buf = (char *)xmalloc(size);
 result = getcwd(buf, size);
 if (result == NULL) {
  message("cannot determine current working directory.exit.");
  sleep(1);
  abort_xs();
 }

 result = strdup(buf);
 free(buf);

 return result;
}

string
get_cwd_as_string(void)
{
 char * cwd = get_cwd_as_charp();
 if (cwd == NULL)
  return "";
 else {
  string result = string(cwd);
  free(cwd);
  return result;
 }
}

bool
valid(string path, pathtype mode)
{
 struct stat buf;
 string canon_path = canonify_filename(path);

 if(mode == PATH_IS_FILE) {
  stat(canon_path.c_str(), &buf);
  if (S_ISREG(buf.st_mode))
   return true;
 } else if (mode == PATH_IS_DIR) {
  stat(canon_path.c_str(), &buf);
  if (S_ISDIR(buf.st_mode))
   return true;
 }

 return false;
}

string
get_resultfile(void)
{
 if (opt_resultfile.size() >0)
  return opt_resultfile;

 return string(DefaultResultfile);
}

void
finish(string result, bool retval)
{
 curs_set(1); // re-show cursor (FIXME: necessary?)
 clear();
 refresh();   // ..and make sure we really see the clean screen
 //resetty();   // ??
 endwin();    // finish the curses at all

 // only save if list was not filtered!
 if (!NeedleGiven)
  list_to_file();

 // is the result really a dir?
 if (!valid(result, PATH_IS_DIR)) {
  fprintf(stderr, "This is not a valid directory:\n%s\n", result.c_str());
  exit(-3);
 }
 //    string resfile = canonify_filename(Resultfile);
 string resfile = canonify_filename(get_resultfile());
 ofstream out(resfile.c_str());
 if (out) {
  out << result << endl;
  out.close();
 }

 if (!retval)
  exit(1);

 exit(0);
}

void
fatal_exit(const string msg)
{
 endwin();
 cerr << msg;
 exit(1);
}

void
terminate(int signo)
{
 endwin();

 if (signo == 11) {
  fprintf(stderr, "program received signal %d\n", signo);
  fprintf(stderr, "This should never happen.\n");
  fprintf(stderr, "Maybe you want to send an email to the author and report a bug?\n");
  fprintf(stderr, "Author: Linkerist <Linkerist@163.com>\n"); 
 }

 fprintf(stderr, "abort.\n");
 exit(1);
}

void
resizeevent(int signo)
{
 (void)signo;

 // re-connect
 signal(SIGWINCH, resizeevent);
 
 // FIXME: is this the correct way??
 endwin();
 refresh();
 init_curses();
 display_list();
}

bool
visible(int pos)
{
 if(pos < 0)
  return false;

 if(pos - yoffset < 0)
  return false;

 if(pos + yoffset > display_area_ymax)
  return false;

 return true;
}

void
toggle_hidden(void)
{
 show_hidden_files = !show_hidden_files;

 if (mode == BROWSE)
  list_from_dir();
}

int
max_yoffset(void)
{
 int len, ret;

 if (mode == BROWSE)
  len = cur_list.size() - 1;
 else
  len = default_list.size() - 1;

 ret = len - display_area_ymax;
 return ret < 0 ? 0 : ret;
}

int
main(int argc, char ** argv)
{
 int c;
 int opt_idx = 0;

 char * arg = NULL;

 string optname;
 string argument;

 const static string xs_short_opts = "a:f:u:brco:hV";

 static struct option xs_long_opts[] = {
  {"help",      no_argument, NULL, 'h'},
  {"version",   no_argument, NULL, 'V'},
 
  {"add",       required_argument, 0, 0},
  {"browse",    no_argument, 0, 0},
  {"nowrap",    no_argument, 0, 0},
  {"noresolve", no_argument, 0, 0},
  {"output",    required_argument, 0, 0},
 
  {NULL, 0, NULL, 0}
 };

 while ((c = getopt_long(argc, argv, xs_short_opts.c_str(), xs_long_opts, &opt_idx)) != -1) {
  switch (c) {
   case 0:
    optname = string(xs_long_opts[opt_idx].name);

    if (optname == "help") {
     version();
     usage(EXIT_SUCCESS);
     exit(0);
    } else if (optname == "version") {
     version();
     exit(0);
    } else if(optname == "browse")
     mode = BROWSE;
    else if (optname == "nowrap")
     opt_no_wrap = true;
    else if (optname == "noresolve")
     opt_no_resolve = true;

    break;

   case 'b':
    mode = BROWSE;
    break;
   case 'r':
    opt_no_resolve = true;
    break;
   case 'h':
    version();
    usage(EXIT_SUCCESS);
    break;
   case 'V':
    version();
    exit(0);
   default:
    usage(EXIT_FAILURE);
  }
 }

 (void)arg;

 if (optind < argc) {
  Needle = argv[optind];
  if (strlen(Needle) > 0)
   NeedleGiven = true;
  
  if (strlen(Needle) == 1 && isdigit(Needle[0])) {
   CurrPosition = atoi(Needle);
   Needle = NULL;
  }

  //usage(EXIT_FAILURE);
 }

 // leave terminal tidy
 // FIXME: what other signals do I need to catch? 
 signal(SIGINT, terminate);
 signal(SIGTERM, terminate);
 signal(SIGSEGV, terminate);

 init_curses();

 // answer to terminal reizing
 signal(SIGWINCH, resizeevent);

 /* get list from file or start in browse mode */
 if (!list_from_file())
  /* doesn't exist. browse current dir */
  mode = BROWSE;

 // if we're browsing read the CWD
 if(mode == BROWSE)
  list_from_dir();

 /* main event loop */
 /* determines the entry to use */
 while (1) {
  display_list();
  c = getch();
  if (!user_interaction(c))
   break;
 }

 finish(current_entry(), true);
 exit(1);
}

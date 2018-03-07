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

//#define VERSION "0.0.1"

/* GLOBALS */
// FIXME: this file has too many globals. Especially the
// offset/position handling is UGLY!
//

// #define false 0
// #define true 1

#define HISTORY_FILE ".xs_history_path"

#define BUFFER_SIZE (16 * 1024)
#define MAX_LINE_SIZE 1024

vector<pair<string, string> > cur_list, default_list;

const char * DefaultListfile = ".cdargs";
const char * DefaultResultfile = "~/.cdargsresult";

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
bool opt_cwd = false;

string opt_resultfile = "";
string opt_listfile = "";
string opt_user = "";

typedef enum exit_values {
 total_success = 0,
 invocation_error = 1,
 xs_error = 2,
 xs_punt = 3,
 xs_fatal = 4,
 system_error = 5
} exit_values_ty;

//typedef _Bool bool;

struct xs_option {
 bool save_path;
 bool delete_path;
 bool list_path;
 bool enter_path;
 bool clear_path;
};

string program_name = "xs";

static void
xs_option_init(struct xs_option * x)
{
 x->save_path = false;
 x->delete_path = false;
 x->list_path = false;
 x->enter_path = false;
 x->clear_path = false;
}

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

 if (opt_user.size() > 0)
  user = opt_user;

 if (opt_listfile.size() > 0)
  return canonify_filename(opt_listfile);

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

typedef std::uint64_t hash_t;

constexpr hash_t prime = 0x100000001B3ull;
constexpr hash_t basis = 0xCBF29CE484222325ull;

/*
#define hash_(__str)\
 { hash_t ret{basis};\
 while (*__str) { ret ^= *__str; ret *= prime; ++__str; }\
 ret; }\
 */

// hash_t
// hash_(char const * str)
// {
//  hash_t ret{basis};

//  while(*str){
//   ret ^= *str;
//   ret *= prime;
//   str++;
//  }

//  return ret;
// }

constexpr hash_t
hash_compile_time(char const * str, hash_t last_value = basis)
{
 return *str ? hash_compile_time(str + 1, (*str ^ last_value) * prime) : last_value;
}

int
main(int argc, char ** argv)
{
 //exit_values_ty exit_status;

 struct xs_option x;
 int c;
 int opt_idx = 0;

 char * arg = NULL;

 string optname;
 string argument;

 xs_option_init(&x);

 const static string xs_short_opts = "a:f:u:brco:sdlechV";
 // const static string xs_short_opts = "a:f:u:brco:vh";
 // const static string xs_short_opts = "sdlechV";

 static struct option xs_long_opts[] = {
  {"save",      no_argument,       NULL, 's'},
  {"delete",    no_argument, NULL, 'd'},
  {"list",      no_argument, NULL, 'l'},
  {"enter",     no_argument, NULL, 'e'},
  {"clear",     no_argument, NULL, 'c'},
 
  {"help",      no_argument, NULL, 'h'},
  {"version",   no_argument, NULL, 'V'},
 
  {"add",       required_argument, 0, 0},
  {"file",      required_argument, 0, 0},
  {"user",      required_argument, 0, 0},
  {"browse",    no_argument, 0, 0},
  {"nowrap",    no_argument, 0, 0},
  {"noresolve", no_argument, 0, 0},
  {"cwd",       no_argument, 0, 0},
  {"output",    required_argument, 0, 0},
 
  {NULL, 0, NULL, 0}
 };

 while ((c = getopt_long(argc, argv, xs_short_opts.c_str(), xs_long_opts, &opt_idx)) != -1) {
  switch (c) {
   case 0:
    optname = string(xs_long_opts[opt_idx].name);

    //switch (hash_(optname.c_str())) {
    switch (
     [&] (const char * __str) {
      hash_t ret{basis};
      while(*__str) { ret ^= *__str; ret *= prime; ++__str; }
      return ret;
     } (optname.c_str())
    ) {
     case hash_compile_time("help"):
      version();
      usage(EXIT_SUCCESS);
      exit(0);
     case hash_compile_time("version"):
      version();
      exit(0);
     case hash_compile_time("add"): {
      argument = string(optarg);
      add_to_list_file(argument);
      exit(0);
     }
     case hash_compile_time("file"):
      opt_listfile = string(optarg);
      break;
     case hash_compile_time("user"):
      opt_user = string(optarg);
      break;
     case hash_compile_time("browse"):
      mode = BROWSE;
      break;
     case hash_compile_time("nowrap"):
      opt_no_wrap = true;
      break;
     case hash_compile_time("noresolve"):
      opt_no_resolve = true;
      break;
     case hash_compile_time("cwd"):
      opt_cwd = true;
      break;
     case hash_compile_time("output"):
      opt_resultfile = string(optarg);
      break;
     default:
      ;
    }

    // if (optname == "help") {
    //  version();
    //  usage(EXIT_SUCCESS);
    //  exit(0);
    // }

    // if (optname == "version") {
    //  version();
    //  exit(0);
    // }

    // if (optname == "add") {
    //  argument = string(optarg);
    //  add_to_list_file(argument);
    //  exit(0);
    // }

    // if (optname == "file")
    //  opt_listfile = string(optarg);

    // if (optname == "user")
    //  opt_user = string(optarg);

    // if(optname == "browse")
    //  mode = BROWSE;

    // if (optname == "nowrap")
    //  opt_no_wrap = true;

    // if (optname == "noresolve")
    //  opt_no_resolve = true;

    // if (optname == "cwd")
    //  opt_cwd = true;

    // if (optname == "output")
    //  opt_resultfile = string(optarg);
    // break;

   case 'a':
    argument = string(optarg);
    add_to_list_file(argument);
    exit(0);
    break;
   case 'f':
    opt_listfile = string(optarg);
    break;
   case 'u':
    opt_user = string(optarg);
    break;
   case 'b':
    mode = BROWSE;
    break;
   case 'r':
    opt_no_resolve = true;
    break;
   // case 'c':
   //  opt_cwd = true;
   //  break;
   case 'o':
    opt_resultfile = string(optarg);
    break;

   case 's':
    x.save_path = true;
    arg = argv[optind];
    break;
   case 'd':
    x.delete_path = true;
    arg = argv[optind];
    break;
   case 'l':
    x.list_path = true;
    break;
   case 'e':
    x.enter_path = true;
    arg = argv[optind];
    break;
   case 'c':
    x.clear_path = true;
    break;
   case 'h':
    version();
    usage(EXIT_SUCCESS);
    break;
   case 'V':
    version();
    break;
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

 //if (x.enter_path)
 // arg = index;

 //xs(&x, arg);

 //return exit_status;
}

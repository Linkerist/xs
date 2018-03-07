#ifndef __XS_H__
#define __XS_H__

#include <iostream>
#include <vector>
#include <map>

#include <cstring>

using namespace std;

# define DEBUG 0

// thanks to cscope source for this:
#if (BSD || V9) && !__NetBSD__
#define TERMINFO    0   /* no terminfo curses */
#else
#define TERMINFO    1
#endif

#define DESCRIPTION_MAXLENGTH 18
#define DESCRIPTION_BROWSELENGTH 8

// Display/Browse Modes;
enum {
 LIST,
 BROWSE
};

// File or Directory?
typedef enum {
 PATH_IS_FILE,
 PATH_IS_DIR
} pathtype;

extern vector<pair<string, string> > cur_list, default_list;

// an iterator for these
typedef vector<pair<string,string> >::iterator listit;
//using listit = vector<pair<string,string> >::iterator;

// needed for map
struct ltstr {
  bool operator
  ()(const char* s1,const char* s2) const
  {
   return strcmp(s1, s2) < 0;
  }
};

//string default_list_file(void);
string get_resultfile(void);
char* get_cwd_as_charp(void);
string get_cwd_as_string(void);
string get_listfile(void);
string capitalized_last_dirname(string path);
string last_dirname(string path);
string canonify_filename(string filename);
bool valid(string path, pathtype mode);

//void scrolling(int lines);
void update_modeline(void);
void set_areas(void);
void resizeevent(int signo);
bool visible(int pos);
void toggle_hidden(void);
int max_yoffset(void);

/* Get outta here */
void fatal_exit(const string msg);
void terminate(int signo);
void finish(string result, bool retval);
void abort_xs(void);

extern const char * DefaultListfile;
extern const char * DefaultResultfile;

extern const char * Needle;
extern bool NeedleGiven;

extern int CurrPosition;
extern map<const char*, int, ltstr> LastPositions;

// xs globals
extern int mode;
extern bool show_hidden_files;
extern bool opt_no_wrap;
extern bool opt_no_resolve;
extern bool listfile_empty;
extern bool opt_cwd;

extern string opt_resultfile;
extern string opt_listfile;
extern string opt_user;

extern string program_name;

#endif

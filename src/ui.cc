#include "ui.h"

#include "xs.h"

#include "io.h"

#if defined(HAVE_NCURSES_H)
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>

// terminal coordinates and other curses stuff
int terminal_width, terminal_height;
int xmax;
int display_area_ymax;
int display_area_ymin;
int modeliney;
int msg_area_y;
int yoffset = 0;
bool curses_running = false;
int shorties_offset = 0;

void
set_areas(void)
{
 getmaxyx(stdscr, terminal_height, terminal_width); // get win-dims
 xmax = terminal_width - 1;
 display_area_ymin = 0;
 display_area_ymax = terminal_height - 3;
 modeliney = terminal_height - 2;
 msg_area_y = terminal_height - 1;
}

/* stuff to initialise the ncurses lib */
void
init_curses(void)
{
 initscr();               // init curses screen
 //savetty();             // ??
 nonl();                  // no newline
 cbreak();                // not in cooked mode: char immediately available
 noecho();                // chars are not echoed
 keypad(stdscr, true);    // Arrow keys etc
 leaveok(stdscr, TRUE);   // Don't show the...
 curs_set(0);             // ...cursor
 set_areas();

 curses_running = true;
}

bool
user_interaction(int c)
{
 int num;
 string curen;

 switch (c) {
  // ==== Exits
  case CTRL('['): // vi
  case CTRL('g'): // emacs
   abort_xs();
   break;
  case 'q':
   finish(".", false);
   break;
  case 13: // ENTER
   // choose dir at cur pos
   return false;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
   num =  c - '0' + shorties_offset;
   if(mode == LIST) {
	if (entry_nr_exists(num)) {
	 CurrPosition = num + yoffset;
	 return opt_no_resolve;
	}
   } else {
	CurrPosition = num + yoffset;
	return true;
   }
   break;

  // ==== Modes
  case '.':
   // show hidden files
   toggle_hidden();
   break;
  case '\t': // TAB
   toggle_mode();
   break;      

  // ==== Navigate The List
  case 'j':       // vi
  case CTRL('n'): // emacs
#if TERMINFO
  case KEY_DOWN:
#endif
   // navigate list++
   cur_pos_adjust(+1);
   break;
  case 'k':       // vi
  case CTRL('p'): // emacs
#if TERMINFO
  case KEY_UP:
#endif
   // navigate list--
   cur_pos_adjust(-1);
   break;

  case '^':       // vi?
  case CTRL('a'): // emacs
#if TERMINFO
  case KEY_HOME:
#endif
   // go to top
   CurrPosition = 0;
   yoffset = 0;
   break;

  case '$':       // vi
  case CTRL('e'): // emacs
# if TERMINFO
  case KEY_END:
#endif
   // go to end
   if (mode == BROWSE)
	CurrPosition = cur_list.size() - 1;
   else if (mode == LIST)
	CurrPosition = default_list.size() - 1;

   yoffset = max_yoffset();
   break;

  //==== move the shortcut digits ('shorties')
  // FIXME case ??:  //vi
  // FIXME maybe change the scrolling behaviour to not take the window but the whole
  // list? That means recentering when shorties leave the screen (adjust yoffset and
  // CurrPosition) 
  case CTRL('v'): // emacs
#if TERMINFO      
  case KEY_NPAGE:
#endif
   for(int i = 0; i < 10; i++)
	cur_pos_adjust(+1, false);

   break;

  // fixme: vi?
  //case CTRL(''): // FIXME: META(x)??
#if TERMINFO
  case KEY_PPAGE:
#endif
   for(int i = 0; i < 10; i++)
	cur_pos_adjust(-1, false);

   break;

  case 'h':       // vi
  case CTRL('b'): // emacs
#if TERMINFO
  case KEY_LEFT:
#endif
   // up dir
   if (mode == BROWSE)
	//             LastPositions[get_current_dir_name()] = CurrPosition;
	LastPositions[get_cwd_as_charp()] = CurrPosition;
   else if (mode == LIST)
	mode = BROWSE;

   list_from_dir("..");
   break;

  case 'l':       // vi
  case CTRL('f'): // emacs
#if TERMINFO
  case KEY_RIGHT:
#endif
   // descend dir at cur pos
   curen = current_entry();
   if (mode == BROWSE)
	LastPositions[get_cwd_as_charp()] = CurrPosition;
   else
	mode = BROWSE;

   list_from_dir(curen.c_str());
   break;
  case 'H':
  case '?':
   helpscreen();
   break;

  case 'd':
  case CTRL('d'): // emacs
#if TERMINFO
  case KEY_BACKSPACE:
#endif
   // delete dir acp
   if (mode == LIST && !NeedleGiven)
	delete_from_default_list(CurrPosition);
   else
	beep();

   break;

  #if TERMINFO
  case KEY_IC:
  #endif
  case 'a':
   // add dir acp (if in browse mode)
   if (!NeedleGiven) {
	if (mode == BROWSE)
	 add_to_default_list(current_entry());
	else
	 add_to_default_list(get_cwd_as_string());
   } else
	beep();

   break;

  case 'A':
   if (!NeedleGiven) {
	// add dir acp (if in browse mode) (ask for desc)
	if (mode == BROWSE)
	 add_to_default_list(current_entry(),"",true);
	else
	 add_to_default_list(get_cwd_as_string(),"",true);
   } else
	beep();

   break;
  case 'c':
   if (!NeedleGiven)
	// add the current dir in every mode
	add_to_default_list(get_cwd_as_string());
   else
	beep();

   break;
  case 'C':
   if (!NeedleGiven)
	// add the current dir in every mode (ask for desc)
	add_to_default_list(get_cwd_as_string(), "", true);
   else
	beep();

   break;

  // ==== EDIT the list
  case 'v':
  case 'e':
   if (!NeedleGiven)
	edit_list_file();
   else
	beep();

   break;

  case 'm':
   swap_two_entries(1);
   break;
  case 'M':
   swap_two_entries(-1);
   break;
  case 't':
  case 's':
   swap_two_entries(0);
   break;

  // ==== Filesystem Hotspots
  case '~':
   mode = BROWSE;
   list_from_dir(getenv("HOME"));
   break;
  case '/':
   mode = BROWSE;
   list_from_dir("/");
   break;
  default:
   beep();
   message("unknown command");
 }
 return true;
}

void
swap_two_entries(int advance_afterwards)
{
 int switch_index;
 if (advance_afterwards >= 0)
  switch_index = CurrPosition + 1;
 else
  switch_index = CurrPosition - 1;

 // ranges check:
 if (switch_index > int(default_list.size()) || switch_index < 0) {
  beep();
  return;
 }

 if (mode == LIST && !NeedleGiven) {
  pair<string, string> tmp = default_list[CurrPosition];
  default_list[CurrPosition] = default_list[switch_index];
  default_list[switch_index] = tmp;
  cur_pos_adjust(advance_afterwards);
  display_list();
 } else
  beep();
}

string
get_desc_from_user(void)
{
 char desc[DESCRIPTION_MAXLENGTH + 1];
 move(msg_area_y, 0);
 clrtoeol();
 printw("Enter description (<ENTER> to quit): ");
 refresh();
 echo();
 getnstr(desc, DESCRIPTION_MAXLENGTH);
 noecho();
 move(msg_area_y, 0);
 clrtoeol();

 return string(desc);
}

string
last_dirname(string path)
{
 if (path.at(path.size() - 1) == '/')
  path = path.substr(0, path.size() - 1);

 string dirname = path.substr(path.find_last_of('/') + 1);

 return dirname;
}

void
message(const char * msg)
{
 if (curses_running) {
  move(msg_area_y, 0);
  clrtoeol();
  printw("%s [any key to continue]", msg);
  refresh();
  getch();
  move(msg_area_y, 0);
  clrtoeol();
 } else
  printf("%s\n", msg);
}

void
display_list(void)
{
 char description_format[40];
 vector<pair<string, string> > list;

 if (mode == LIST) {
  // if the list contains just one entry (probably due to filtering by giving a Needle)
  // we are done.
  if (default_list.size() == 1 && !opt_no_resolve)
   finish(current_entry(), true);
  else
   list = default_list;
 } else
  list = cur_list;

 clear();

 update_modeline();

 int pos = display_area_ymin;
 if (CurrPosition > static_cast<int>(list.size()) - 1)
  CurrPosition = list.size() - 1;

 // Calculate actual maxlength of descriptions so we can eliminate trailing blanks.
 // We have to iterate thru the list to get the longest description
 unsigned int actual_maxlength = 0;
 if(mode == LIST) {
  for (listit li = list.begin() + yoffset; li != list.end(); ++li) {
   if (strlen(li->first.c_str()) > actual_maxlength)
	actual_maxlength = strlen(li->first.c_str());

   // Don't let actual_maxlength > DESCRIPTION_MAXLENGTH
   if ( actual_maxlength > DESCRIPTION_MAXLENGTH)
	actual_maxlength = DESCRIPTION_MAXLENGTH;
  }
 } else
  actual_maxlength = DESCRIPTION_BROWSELENGTH;

 string cwd  = get_cwd_as_string();

 for (listit li = list.begin() + yoffset; li != list.end(); ++li) {
  //string desc = li->first.substr(0, (DESCRIPTION_MAXLENGTH + 1));
  string desc = li->first.substr(0, actual_maxlength);
  string path = li->second.substr(0, xmax - 16);
  string fullpath = li->second;
  char validmarker = ' ';

  if(!valid(fullpath, PATH_IS_DIR))
   validmarker = '!';

  if (pos > display_area_ymax)
   break;

  move(pos, 0);

  if (pos == CurrPosition - yoffset)
   attron(A_STANDOUT);

  if (fullpath == cwd)
   attron(A_BOLD);

  if (pos >= shorties_offset && pos < 10 + shorties_offset)
   printw("%2d", pos - shorties_offset);
  else
   printw("  ");

  // Compose format string for printw. Notice %% to represent literal %
  sprintf(description_format, " [%%-%ds] %%c%%s", actual_maxlength);

  printw(description_format, desc.c_str(), validmarker,path.c_str());

  if (pos == CurrPosition - yoffset)
   attroff(A_STANDOUT);

  if (fullpath == cwd)
   attroff(A_BOLD);

  pos++;
 }
}

void
helpscreen(void)
{
#define PAGERENV "PAGER"
 const char * __pager = getenv(PAGERENV);
 string pager = __pager == NULL ? "" : string(__pager);
#undef PAGERENV
 if (pager.empty()) {
  // FIXME:  how to detect debian and the /usr/bin/pager rule?
  struct stat buf;
  if (stat("/usr/bin/pager", &buf) == 0)
   pager = "/usr/bin/pager";
  else {
   if (stat("/usr/bin/less", &buf) == 0)
	pager = "/usr/bin/less";
   else
	pager = "more";
  }
 }

 endwin();
 FILE * help = popen(pager.c_str(), "w");
 // FIXME: error checking this doesn't work, er?
 if (help == NULL) {
  init_curses();
  message("could not open pager");
  return;
 }

 int l = 0;
 string help_lines[] = {
  "cdargs (c) 2001-2003 S. Kamphausen <http://www.skamphausen.de>",
  "<UP>/<DOWN>  move selection up/down and scroll",
  "             please see manpage for alternative bindings!",
  "<ENTER>      select current entry",
  "<TAB>        toggle modes: LIST or BROWSE",
  "<HOME>/<END> goto first/last entry in list if available in terminal",
  "c            add current directory to list",
  "C            same as 'c' but ask for a description",
  "<PgUP/Dn>    scroll the list 10 lines up or down",
  "e,v          edit the list in $EDITOR",
  "H,?          show this help",
  "~,/          browse home/root directory",
  "q            quit - saving the list",
  "C-c,C-g,C-[  abort - don't save the list",
  "             ",
  "Commands in BROWSE-mode:",
  "<LEFT>       descent into current directory",
  "<RIGHT>      up one directory",
  "[num]        make [num] the highlighted entry",
  "a            add current entry to list",
  "A            same as 'a' but ask for a description",
  ".            toggle display of hidden files",
  "             ",
  "Commands in LIST-mode:",
  "[num]        select and resolve entry [num] if displayed",
  "<LEFT>       descent into the current entry",
  "<RIGHT>      up one directory from current dir",
  "d            delete current entry from list",
  "a            add current directory to list",
  "s,t          swap(transpose) two lines in the list",
  "M/m          move an entry up down in the list",
  ""
 };

 int help_lines_size = 31;
 while (l < help_lines_size) {
  fprintf(help, "%s\n", help_lines[l].c_str());
  ++l;
 }
 fprintf(help, "\n");

 pclose(help);

 if (strstr(pager.c_str(), "more"))
  sleep(1);

 refresh();
 init_curses();
}

void
toggle_mode(void)
{
 if (mode == LIST) {
  mode = BROWSE;
  list_from_dir(".");
 } else {
  // list was empty at start
  if (listfile_empty) {
   if (!default_list.empty()) {
	// but isn't now
	list_to_file();
	list_from_file();
	mode = LIST;
   } else {
	// ok, we have nothing to show here
	message("No List entry. Staying in BROWSE mode");
	mode = BROWSE;
	list_from_dir(".");
   }
  } else {
   // the "normal" case;
   // disable Needle and reload the list!
   NeedleGiven = false;
   Needle = NULL;
   list_from_file();
   yoffset = 0;
   shorties_offset = 0;
   CurrPosition = 0;
   mode = LIST;
  }
 }
}

void
update_modeline(void)
{
 move(modeliney, 0);
 clrtoeol();
 move(modeliney, 0);
 attron(A_REVERSE);

 string curmode;
 char modechar;

 if (mode == LIST) {
  curmode = "List";
  modechar = 'L';
 } else {
  curmode = "Browse";
  modechar = 'B';
 }

 string cwd = get_cwd_as_string();
 attron(A_BOLD);
 printw("%c: %s ", modechar, cwd.c_str());
 //   printw(" yoff: %d shoff: %d currpos: %d ", yoffset,shorties_offset,CurrPosition);
 attroff(A_BOLD);
 hline(' ', xmax);
 attroff(A_REVERSE);
}

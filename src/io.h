#ifndef __IO_H__
#define __IO_H__

#include <iostream>

using namespace std;

#if defined(HAVE_NCURSES_H)
#include <ncurses.h>
#else
#include <curses.h>
#endif

bool list_from_file(void);
void list_from_dir(const char * name = ".");
void list_to_file(void);
bool dont_show(const char * name);
void cur_pos_adjust(int n = 0, bool wraparound = true); 
bool entry_nr_exists(unsigned int nr);
string current_entry(void);
void add_to_default_list(string path, string description = "", bool ask_for_desc = false);
void delete_from_default_list(int pos);
void edit_list_file(void);

#endif

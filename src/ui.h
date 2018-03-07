#ifndef __UI_H__
#define __UI_H__

#include <iostream>

using namespace std;

#define CTRL(x) (x & 037)

/* Curses and Display Stuff */
void init_curses(void);
bool user_interaction(int c);
void swap_two_entries(int advance_afterwards);
string get_desc_from_user(void);
void message(const char * msg);
void display_list(void);
void helpscreen(void);
void toggle_mode(void);
void update_modeline(void);

extern int terminal_width, terminal_height;
extern int xmax;
extern int display_area_ymax;
extern int display_area_ymin;
extern int modeliney;
extern int msg_area_y;
extern int yoffset;
extern bool curses_running;
extern int shorties_offset;

#endif

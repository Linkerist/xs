#include "io.h"

#include "xs.h"
#include "ui.h"

#include <fstream>
#include <algorithm>

// unistd.h is not part of standard C so standard C++ in turn doesn't include it with the other c... headers.
#include <unistd.h>
#include <dirent.h>

#include <sys/stat.h>

bool
list_from_file(void)
{
 string desc;
 string path;
 int linecount = 0;

 default_list.clear();

 ifstream listfile(get_listfile().c_str());

 if (!listfile)
  return false;

 string cwd = get_cwd_as_string();

 while (!listfile.eof()) {
  string line;

  getline(listfile, line);

  if (line == "")
   continue;

  // comments ... are not saved later so we simply don't allow
  // them
  //       if (line[0] == '#') {
  //          continue;
  //       }
  // detect path and description at the leading slash of the
  // absolute path:

  desc = line.substr(0, line.find('/') - 1);
  path = line.substr(line.find('/'));

  if (cwd == path)
   CurrPosition = linecount;

  // counting the lines: if only one, no resolving should take place
  ++linecount;

  // if we got an exact match and not opt_no_resolve the first entry is the result!
  if (NeedleGiven && Needle && Needle == desc && !opt_no_resolve)
   finish(path, true);

  // filtered?
  if (dont_show(desc.c_str()) && dont_show(path.c_str()))
   continue;

  default_list.push_back(make_pair(desc, path));
 }

 listfile.close();

 switch(linecount) {
  case 0:
   listfile_empty = true;
   break;
  case 1:
   opt_no_resolve = true;
   break;
 }

 shorties_offset = 0;
 if (default_list.empty())
  return false;

 return true;
}

// default: name="."
void
list_from_dir(const char * name)
{
 string previous_dir = "";
 //if(name == "..") {
 if (!strcmp(name, ".."))
  previous_dir = get_cwd_as_string();

 // Checking, Changing and Reading
 if (chdir(name) < 0) {
  string msg = "couldn't change to dir: ";
  msg += name;
  message(msg.c_str());
 }
   
 DIR * THISDIR;
 string cwd = get_cwd_as_string();
 THISDIR = opendir(cwd.c_str());

 if (THISDIR == NULL) {
  string msg = "couldn't read dir: ";
  msg += cwd;
  message(msg.c_str());
  sleep(1);
  abort_xs();
 }
 yoffset = 0;
 shorties_offset = 1;
 struct dirent * en;
 struct stat buf;

 // cleanup
 cur_list.clear();

 CurrPosition = 1;

 // put the current directory on top ef every listing
 string desc = ".";
 cur_list.push_back(make_pair(desc,cwd));

 // read home dir
 while ((en = readdir(THISDIR)) != NULL) {
  /* filter dirs */
  if (dont_show(en->d_name))
   continue;
  string fullname;
  fullname = cwd + string("/") + string(en->d_name);

  fullname = canonify_filename(fullname);

  stat(fullname.c_str(), &buf);
  if (!S_ISDIR(buf.st_mode))
   continue;

  string lastdir = last_dirname(fullname);
  cur_list.push_back(make_pair(lastdir, fullname));
 }
 // empty directory listing: .. and .
 desc = "..";
 string path=get_cwd_as_string();
 if(cur_list.size() == 1) {
  path = path.substr(0, path.find(last_dirname(path)));
  cur_list.push_back(make_pair(desc,path));
 }

 sort(cur_list.begin(), cur_list.end());

 // have we remembered a current position for this dir?
 if (LastPositions.count(cwd.c_str()))
  CurrPosition = LastPositions[cwd.c_str()];
 else {
  //CurrPosition = 1;
  if(previous_dir.length() > 0) {
   int count = 0;
   for(listit it = cur_list.begin(); it != cur_list.end(); ++it, count++) {
    if(it->second == previous_dir) {
     CurrPosition = count;
     break;
    }
   }
  }
 }

 if(!visible(CurrPosition))
  yoffset = CurrPosition - 1;

 closedir(THISDIR);
}

void
list_to_file(void)
{
 // don't write empty files FIXME: unlink file?
 string __listfile = get_listfile();

 if (default_list.empty()) {
  unlink(__listfile.c_str());
  return;
 }

 struct stat buf;
 if (stat(__listfile.c_str(), &buf) == 0) {
  string backup_file = __listfile + "~";
  if (rename(__listfile.c_str(), backup_file.c_str()) == -1)
   fprintf(stderr,"warning: could not create backupfile\n");
 }

 ofstream listfile(__listfile.c_str());
 if (!listfile)
  fatal_exit("couldn't write listfile");

 //for (vector<pair<string, string> > li = default_list.begin(); li != default_list.end(); ++li)
 for (listit li = default_list.begin(); li != default_list.end(); ++li)
  listfile << li->first << " " << li->second << endl;

 listfile.close();
}

bool
dont_show(const char * name)
{
 /* don't show current, up, hidden if not flag */
 if (mode == BROWSE) {
  if (strcmp(name, "..") == 0)
   return true;
  
  if (!show_hidden_files && name[0] == '.')
   return true;
 } else {
  if (!NeedleGiven)
   return false;

  if (!Needle)
   return false;

  /* FIXME case-insensitive comparison */
  //if (strcasecmp(name, Needle)) {
  return !strstr(name, Needle);
 }

 return false;
}

// default: n=0,
//          wraparound=true
void
cur_pos_adjust(int n, bool wrap_around)
{
 int newpos = CurrPosition + n;
 int max, min;

 if (mode == LIST)
  max = default_list.size() - 1;
 else
  max = cur_list.size() - 1;

 min = 0;

 if (newpos < min) {
  if (opt_no_wrap || !wrap_around)
   return;

  newpos = max;
  yoffset = max_yoffset();
 }

 if (newpos > max) {
  if (opt_no_wrap || !wrap_around)
   return;

  newpos = min;
  yoffset = 0;
 }

 CurrPosition = newpos;
 // scrolling...
 if (CurrPosition-yoffset >= display_area_ymax)
  ++yoffset;

 if (yoffset > 0 && CurrPosition == yoffset)
  --yoffset;
}

bool
entry_nr_exists(unsigned int nr)
{
 if (mode == LIST)
  return nr < default_list.size();
 else if (mode == BROWSE)
  return nr < cur_list.size();

 return false;
}

string
current_entry(void)
{
 string res;

 if (mode == LIST) {
  if (default_list.empty())
   return ".";

  res = default_list[CurrPosition].second;
 } else if (mode == BROWSE) {
  if (cur_list.empty())
   return string(".");

  res = cur_list[CurrPosition].second;
 }

 return res == "" ? "." : res;
}

// default: description = ""
// default: ask_for_desc = false
void
add_to_default_list(string path, string desc, bool ask_for_desc)
{
 if (cur_list.empty() && curses_running)
  path = get_cwd_as_string();

 // get the description (either from user or generic)
 string __desc;

 if (desc.empty()) {
  if (ask_for_desc && curses_running) {
   __desc = get_desc_from_user();
   if (__desc.size() == 0)
    return;
  } else
   __desc = last_dirname(path);
 } else
  __desc = desc;

 string msg = "added :" + __desc + ":" + path;
 default_list.push_back(make_pair(__desc, path));
 message(msg.c_str());
}

void
add_to_list_file(string path)
{
 // get rid of leading = if there
 if (path.at(0) == '=')
  path = path.substr(1);

 // the syntax for passing descriptions from the command line is:
 // --add=:desc:/absolute/path
 string desc;
 if (path.at(0) == ':') {
  int colon2_at;
  colon2_at = path.find(":", 1);
  if (colon2_at > DESCRIPTION_MAXLENGTH + 1) {
   fprintf(stderr, "description too long! max %d chars.\n", DESCRIPTION_MAXLENGTH);
   exit(-4);
  }

  desc = path.substr(1, colon2_at - 1);
  path = path.substr(colon2_at + 1);
 }

 // FIXME: check for existance here?
 if (path.at(0) != '/') {
  fprintf(stderr,"this is not an absolute path:\n%s\n.", path.c_str());
  exit(-2);
 }

 list_from_file();
 add_to_default_list(path, desc, false);
 list_to_file();
}

void
delete_from_default_list(int pos)
{
 int count = 0;
 for (listit li = default_list.begin();li != default_list.end(); ++li) {
  if (count == pos) {
   default_list.erase(li);
   cur_pos_adjust(0, false);
   opt_no_resolve = true;
   return;
  } else
   ++count;
 }
}

void
edit_list_file(void)
{
 string editor = getenv("EDITOR");
 if (editor.empty()) {
  // FIXME: how to detect debian and the /usr/bin/editor rule?
  struct stat buf;
  if (stat("/usr/bin/editor", &buf) == 0)
   editor = "/usr/bin/editor";
  else
   editor = "vi";
 }

 endwin();
 list_to_file();
 if(system((editor + " \"" + get_listfile() + "\"").c_str())){};
 list_from_file();
 refresh();
 init_curses();
 display_list();
}

//unsigned int long
//lc_file(const char * f)
//{
// unsigned int long line = 0;
// int fd;
// size_t bytes_read;
// char buf[BUFFER_SIZE + 1];
//
// fd = open(f, O_RDONLY);
// if (fd == -1) {
//  fprintf(stderr, "Error: open fd for %s", f);
//  exit(1);
// }
//
// while ((bytes_read = read(fd, buf, BUFFER_SIZE)) > 0) {
//  char * p = buf;
//  char * idx_buf = buf + bytes_read;
//
//  while (p = memchr(p, '\n', idx_buf - p)) {
//   ++p;
//   ++line;
//  }
// }
//
// if (close(fd) != 0) {
//  fprintf(stderr, "Error: close fd for %s", f);
//  exit(1);
// }
//
// return line;
//}
//
//char *
//get_history_filename(void)
//{
// char * home_dir = getenv("HOME");
//
// /* one for '/', one for '\0' */
// char * history_filename = (char *)xmalloc(strlen(home_dir) + strlen(HISTORY_FILE) + 2);
//
// sprintf(history_filename, "%s/%s", home_dir, HISTORY_FILE);
//
// return history_filename;
//}
//
//exit_values_ty
//save_path(const char * arg_path)
//{
// exit_values_ty exit_status = total_success;
//
// char * history_filename = get_history_filename();
//
// int nr_path;
//
// if (!access(history_filename, F_OK))
//  nr_path = lc_file(history_filename);
//
// char * save_path = realpath(arg_path, NULL);
//
// FILE * fp = fopen(history_filename, "a+");
// if (fp == NULL) {
//  fprintf(stderr, "cannot open %s for appending", history_filename);
//  exit_status = system_error;
//  exit(EXIT_FAILURE);
// }
//
// fprintf(fp, "%d %s\n", ++nr_path, save_path);
//
// fclose(fp);
//
// free(save_path);
// free(history_filename);
//
// return exit_status;
//}
//
//exit_values_ty
//list_path(void)
//{
// exit_values_ty exit_status = total_success;
//
// char * f = get_history_filename();
// struct stat statbuf;
//
// int fd = open(f, O_RDONLY);
// if (fd < 0) {
//  fprintf(stderr, "Error: open fd for %s\n", f);
//  free(f);
//  exit(1);
// }
//
// if (fstat(fd, &statbuf) < 0) {
//  fprintf(stderr, "Error: fstat for fd of %s\n", f);
//  free(f);
//  exit(1);
// }
//
// off_t fsize = statbuf.st_size;
//
// char * buf = (char *)xmalloc(fsize);
//
// ssize_t n_read, n_write;
//
// while(true) {
//  n_read = read(fd, buf, (size_t)fsize);
//  if (n_read < 0) {
//   fprintf(stderr, "Error: read for fd of %s\n", f);
//   exit_status = system_error;
//   break;
//  }
//
//  if (n_read == 0)
//   break;
//
//  n_write = n_read;
//
//  if (write(STDOUT_FILENO, buf, n_write) != n_write) {
//   fprintf(stderr, "Error: write for fd of %s\n", f);
//   exit_status = system_error;
//   break;
//  }
// }
//
// if (close(fd) != 0) {
//  fprintf(stderr, "Error: close fd for %s\n", f);
//  exit_status = system_error;
// }
//
// free(buf);
// free(f);
//
// return exit_status;
//}
//
//bool
//yesno(void)
//{
// bool yes;
//
// int c = getchar();
//
// yes = c == 'y' || c == 'Y';
//
// while (c != '\n' && c != EOF)
//  c = getchar();
//
// return yes;
//}
//
//exit_values_ty
//clear_path()
//{
// exit_values_ty exit_status = total_success;
// char choice;
//
// fprintf(stderr, "Are you sure to clear %s? (y/Y/n/N) ", HISTORY_FILE);
// if (!yesno())
//  return exit_status;
//
// char * f = get_history_filename();
//
// int fd = open(f, O_RDWR);
// if (fd < 0) {
//  fprintf(stderr, "Error: open %s\n", f);
//  free(f);
//  exit(1);
// }
//
// if (ftruncate(fd, 0) < 0) {
//  fprintf(stderr, "Error: truncate %s\n", f);
//  free(f);
//  exit(1);
// }
//
// if (lseek(fd, 0, SEEK_SET) < 0) {
//  fprintf(stderr, "Error: lseek %s\n", f);
//  free(f);
//  exit(1);
// }
//
// close(fd);
// free(f);
//
// return exit_status;
//}
//
//exit_values_ty
//delete_path(const char * arg_index)
//{
// exit_values_ty exit_status = total_success;
// char * f = get_history_filename();
// int nr_path;
//
// if (!access(f, F_OK))
//  nr_path = lc_file(f);
//
// int index = atoi(arg_index);
//
// if (index > nr_path) {
//  fprintf(stderr, "In %s, there were only %d path!\n", f, nr_path);
//  free(f);
//  exit(1);
// }
//
// fpos_t pos_w, pos_r, pos;
// int i, k;
// char * one_line;
//
// FILE * fin = fopen(f, "rb+");
// if (fin == NULL) {
//  fprintf(stderr, "Error: fopen for %s\n", f);
//  exit(1);
// }
//
// one_line = (char *)xmalloc(MAX_LINE_SIZE * sizeof(char));
//  
// for (i = 0; i < index; i++)
//  fgets(one_line, MAX_LINE_SIZE, fin);
//  
// fgetpos(fin, &pos_w);
// fgets(one_line, MAX_LINE_SIZE, fin);
// fgetpos(fin, &pos_r);
// //pos = pos_r;
//  
// while (1) {
//  pos = pos_r;
//  fsetpos (fin, &pos);
//  if (fgets(one_line, MAX_LINE_SIZE - 1, fin) == NULL)
//   break;
//  fgetpos (fin, &pos_r);
//  pos = pos_w;
//  fsetpos (fin, &pos);
//  fprintf(fin, "%s", one_line);
//  fgetpos (fin, &pos_w);
// }
//
// truncate(f, *(off_t *)&pos_w);
// fseek(fin, *(long *)&pos_w, SEEK_SET);
// fclose(fin);
//
// free(one_line);
// free(f);
//
// return exit_status;
//}
//
//exit_values_ty
//enter_path(const char * arg_index)
//{
// exit_values_ty exit_status = total_success;
// char * f = get_history_filename();
// int nr_path;
//
// if (access(f, F_OK)) {
//  fprintf(stderr, "history file does not exist.\n");
//  free(f);
//  exit(1);
// }
//
// nr_path = lc_file(f);
//
// int index = atoi(arg_index);
//
// if (index > nr_path) {
//  fprintf(stderr, "In %s, there were only %d path!\n", f, nr_path);
//  free(f);
//  exit(1);
// }
//
// fprintf(stderr, "enter path %d\n", index);
//
// FILE * fp = fopen(f, "r");
// char * one_line;
// int i;
// char str_index[10], str_path[96];
//
// if (fp == NULL) {
//  fprintf(stderr, "Error: fopen for %s\n", f);
//  exit(1);
// }
//
// one_line = (char *)xmalloc(MAX_LINE_SIZE * sizeof(char));
//
// for (i = 0; i < index; ++i)
//  fgets(one_line, MAX_LINE_SIZE, fp);
//
// fscanf(fp, "%s %s", str_index, str_path);
// fprintf(stderr, "%s %s\n", str_index, str_path);
//
// //chdir(str_path);
// //system("cd /");
// pid_t pid;
//
// pid = fork();
// if (pid < 0) {
//  printf("fail to fork\n");
//  exit(1);
// } else if (pid == 0) {
//  if (execvp("cd", NULL) < 0) {
//   printf("fail to exec\n");
//   exit(0);
//  }
// }
//
// free(one_line);
// free(f);
//
// return exit_status;
//}
//
//exit_values_ty
//xs(const struct xs_option * x, const char * arg)
//{
// exit_values_ty exit_status = total_success;
//
//
// if (x->delete_path) {
//  delete_path(arg);
// }
//
// if (x->list_path) {
//  list_path();
// }
//
// if (x->enter_path) {
//  enter_path(arg);
// }
//
// if (x->clear_path) {
//  clear_path();
// }
//
// return exit_status;
//}

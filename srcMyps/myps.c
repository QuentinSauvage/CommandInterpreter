#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>
#include "myps.h"

long int t_mem;
struct winsize w;
time_t start;

FILE *openFile(const char *fname) {
  FILE *f;
  if ((f = fopen(fname, R_MODE)) == NULL)
    syserror(OPEN);
  return f;
}

/*2:lnk, 1:dir, 0:others*/
int is_dir(char *fname) {
  struct stat st;
  if (lstat(fname, &st) == ERR)
    syserror(ST);
  if (S_ISDIR(st.st_mode))
    return 1;
  return 0;
}

int compare(const void *a, const void *b) {
  char c = (*(char *)a);
  char d = (*(char *)b);
  if (c == ch(HIGH) || c == ch(LOW))
    return -1;
  if (d == ch(HIGH) || d == ch(LOW))
    return 1;
  if (c == PLUS)
    return 1;
  if (d == PLUS)
    return -1;
  if (IS_UPPER(c) && IS_LOWER(d))
    return c;
  if (IS_UPPER(d) && IS_LOWER(c))
    return d;
  return c < d;
}

/*started time in stat -> timestamp*/
time_t cvt_start(time_t t1, char *i_start) {
  time_t t2 = atol(i_start) / CLK_TCK;
  return time(NULL) - t1 + (t2);
}

void print_infos(infos i) {
  int res1, res2;
  char buf[BUFFER_SIZE];
  /*started time*/
  time_t t = cvt_start(start, i[START]);
  struct tm *tt = localtime(&t);
  strftime(buf, 6, TIME_FORMAT, tt);
  /*truncate command*/
  if (strlen(i[COMMAND]) > ((unsigned int)w.ws_col - TTY_LIM))
    i[COMMAND][w.ws_col - TTY_LIM] = ch(END);
  qsort(i[STAT] + 1, strlen(i[STAT]) - 1, sizeof(char), compare);
  /*get time*/
  res2 = atoi(i[TIME]) / CLK_TCK;
  res1 = res2 / SND_VALUE;
  res2 %= SND_VALUE;
  /*cpu and mem format*/
  if (i[CPU][1] == ch(END))
    strcpy(i[CPU], NULL_FORMAT);
  else
    i[CPU][NB_LIM] = ch(END);
  if (i[MEM][1] == ch(END))
    strcpy(i[MEM], NULL_FORMAT);
  else
    i[MEM][NB_LIM] = ch(END);
  printf("%-8s %5s %4s %4s %6s %5s %-5s\t", i[USER], i[PID], i[CPU], i[MEM],
         i[VSZ], i[RSS], i[TTY]);
  printf("%-4s %5s %3d:%.2d %s", i[STAT], buf, res1, res2, i[COMMAND]);
  putchar('\n');
}

int read_user(infos i) {
  struct passwd *pass = getpwuid(atoi(i[USER]));
  strncpy(i[USER], pass->pw_name, strlen(pass->pw_name));
  if (((i[USER][BASE_LIM])) != ch(END)) {
    i[USER][BASE_LIM - 1] = ch(PLUS);
    i[USER][BASE_LIM] = ch(END);
  }
  return 0;
}

void read_cmd(char *fname, infos i) {
  FILE *f;
  int cpt;
  char path[PATH_SIZE], c;
  PPATH(path, fname, path(CMD_PATH));
  f = openFile(path);
  for (cpt = 0, c = fgetc(f); c != EOF && cpt < BUFFER_SIZE; c = fgetc(f))
    i[COMMAND][cpt++] = (c == ch(END)) ? ch(SPACE) : c;
  if (fclose(f) == EOF)
    syserror(CLOSE);
}

/*reads item in status file*/
int read_status_it(FILE *f, char *ind, char *str) {
  int cpt = 0, found = 0;
  char token[TOKEN_SIZE], c;
  while ((c = fgetc(f)) != EOF)
    if (READABLE(c))
      token[cpt++] = c;
    else {
      token[cpt] = ch(END);
      if (found && READABLE(token[0])) {
        strncpy(str, token, BUFFER_SIZE);
        return 1;
      } else if (!strcmp(ind, token))
        found = 1;
      memset(token, 0, TOKEN_SIZE);
      cpt = 0;
    }
  fseek(f, 0, SEEK_SET);
  strcpy(str, "0");
  return 0;
}

/*reads status file*/
void read_status(char *fname, infos i) {
  FILE *f;
  char path[PATH_SIZE], buf[BUFFER_SIZE], tmp[BUFFER_SIZE];
  long int len = strlen(i[STAT]), rss;
  double f_rss;
  PPATH(path, fname, path(STATUS_PATH));
  f = openFile(path);
  if (!strlen(i[COMMAND])) {
    i[COMMAND][0] = ch(L_BRACKET);
    read_status_it(f, idx(NAME_IDX), i[COMMAND] + 1);
    i[COMMAND][strlen(i[COMMAND])] = ch(R_BRACKET);
  }
  read_status_it(f, idx(UID_IDX), i[USER]);
  read_status_it(f, idx(SS_IDX_1), buf);
  read_status_it(f, idx(SS_IDX_2), tmp);
  if (!strcmp(buf, tmp)) /*session loader*/
    i[STAT][len++] = ch(SESSION);
  buf[0] = ch(END);
  read_status_it(f, idx(VSZ_IDX), i[VSZ]);
  read_status_it(f, idx(LCK_IDX), buf);
  if (strlen(buf) > 1) /*locked pages*/
    i[STAT][len] = ch(LCK);
  read_status_it(f, idx(RSS_IDX), i[RSS]);
  rss = atoi(i[RSS]);
  f_rss = (rss * RATIO) / t_mem;
  gcvt(f_rss, 2, i[MEM]);
  if (fclose(f) == EOF)
    syserror(CLOSE);
}

/*skip non-used items in stat file*/
void read_begin_stat(FILE *f) {
  char c;
  int end = 0;
  for (c = fgetc(f); 1; c = fgetc(f)) {
    if (c == ch(CLOSE_P))
      end = 1;
    else if (end && c != ch(CLOSE_P))
      return;
  }
}

/*reads the stat file*/
void read_stat(char *fname, infos i) {
  FILE *f;
  int cpt = 0, nice = 0, thread = 0, session = 0, bg = 0, cpu;
  char state;
  long int stt, utime, stime, tty;
  char path[PATH_SIZE];
  PPATH(path, fname, path(STAT_PATH));
  f = openFile(path);
  read_begin_stat(f);
  fscanf(f, STAT_FORMAT, &state, &session, &tty, &bg, &utime, &stime, &nice,
         &thread, &stt);
  i[STAT][cpt++] = state;
  if (bg > 0)
    i[STAT][cpt++] = ch(PLUS);
  if (nice < 0)
    i[STAT][cpt++] = ch(HIGH);
  else if (nice)
    i[STAT][cpt++] = ch(LOW);
  if (thread > 1)
    i[STAT][cpt++] = ch(THREAD);
  if (tty) {
    int a = tty, b = tty;
    a = tty;
    sprintf(i[TTY], "%s%d", (a & HIGH_TTY) ? SLAVE_0 : SLAVE_1, b % LOW_TTY);
  } else
    i[TTY][0] = ch(Q_MARK);
  sprintf(i[TIME], "%ld", utime + stime);
  sprintf(i[START], "%ld", stt);
  stt = (start - (stt / 100));
  cpu = (stt) ? ((utime + stime) * 10) / (start - (stt / 100)) : 0;
  sprintf(i[CPU], "%d%c%d", cpu / 10, ch(POINT), cpu % 10);
  if (fclose(f) == EOF)
    syserror(CLOSE);
}

/*init process's infos*/
void memset_infos(infos i) {
  int j;
  for (j = 0; j < SZ_INFOS; j++)
    memset(i[j], 0, BUFFER_SIZE);
}

/*reads infos related to the process*/
void read_proc(struct dirent *file) {
  infos i;
  memset_infos(i);
  if (!is_dir(file->d_name) || !atoi(file->d_name))
    return;
  strcpy(i[PID], file->d_name);
  read_cmd(file->d_name, i);
  read_stat(file->d_name, i);
  read_status(file->d_name, i);
  read_user(i);
  print_infos(i);
}

/*reads the total memory of the system*/
void get_tmem() {
  FILE *f;
  char path[PATH_SIZE];
  PATH(path, path(MEM_PATH));
  f = openFile(path);
  fscanf(f, "%*s %ld", &t_mem);
  if (fclose(f) == EOF)
    syserror(CLOSE);
}

/*reads the boot time of the system*/
int read_boot(FILE *f, char *buf) {
  int cpt = 0;
  char token[TOKEN_SIZE], c;
  while ((c = fgetc(f)) != EOF)
    if (READABLE(c))
      token[cpt++] = c;
    else {
      token[cpt] = ch(END);
      strncpy(buf, token, BUFFER_SIZE);
      return 1;
    }
  return -1;
}

/*open uptime file*/
void get_boot_time() {
  FILE *f;
  char path[PATH_SIZE], buf[BUFFER_SIZE];
  PATH(path, path(BOOT_PATH));
  f = openFile(path);
  if (!read_boot(f, buf))
    fprintf(stderr, "%s", message[READ]), exit(READ);
  if (fclose(f) == EOF)
    syserror(CLOSE);
  start = atol(buf);
}

/*reads each process in /proc*/
void get_proc() {
  DIR *dir;
  struct dirent *f;
  if ((dir = opendir(path(PROC_PATH))) == NULL)
    syserror(OPEN);
  get_tmem();
  get_boot_time();
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); /*tty dimension*/
  HEADER();
  while ((f = readdir(dir)) != NULL) {
    read_proc(f);
    if (chdir(path(PROC_PATH)) == ERR)
      syserror(CHDIR);
  }
  if (closedir(dir) == ERR)
    syserror(CLOSE);
}

int main() {
  get_proc();
  exit(0);
}


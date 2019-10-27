#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include "myls.h"

void read_subdir(MYDIR *dir, Arguments *args, char *cwd);

MYDIR *create_dir() {
  MYDIR *dir = NULL;
  int i;
  dir = malloc(sizeof(MYDIR));
  dir->nb_files = 0;
  dir->files = malloc(sizeof(char *) * MAX_DIR);
  for (i = 0; i < MAX_DIR; i++)
    (dir->files)[i] = malloc(sizeof(char) * FILENAME_SIZE);
  return dir;
}

void free_dir(MYDIR *dir) {
  int i, size = MAX_DIR * ((dir->nb_files / MAX_DIR) + 1);
  for (i = 0; i < size; i++)
    free((dir->files)[i]);
  free(dir->files);
  free(dir);
}

Command_line *create_cmd_line() {
  Command_line *cmd_line = NULL;
  cmd_line = malloc(sizeof(Command_line));
  cmd_line->args = malloc(sizeof(Arguments));
  cmd_line->args->a = 0;
  cmd_line->args->r = 0;
  cmd_line->dir = create_dir();
  return cmd_line;
}

void free_cmd_line(Command_line *cmd_line) {
  free(cmd_line->args);
  free_dir(cmd_line->dir);
  free(cmd_line);
}

void myrealloc(MYDIR *mydir) {
  int i;
  mydir->files =
      realloc(mydir->files, (mydir->nb_files + 1 + MAX_DIR) * sizeof(char *));
  for (i = mydir->nb_files + 1; i < mydir->nb_files + 1 + MAX_DIR; i++)
    (mydir->files)[i] = malloc(sizeof(char) * FILENAME_SIZE);
}

int mystrcmp(char *a, char *b) {
  int i, j, point_a = 0, point_b = 0, offset_a = 0, offset_b = 0, tmp1, tmp2;
  for (i = 0, j = 0; a[i] != '\0' && b[j] != '\0'; i++, j++) {
    if (a[i] == POINT[0] && !point_a)
      offset_a++;
    else
      point_a = 1;
    if (b[j] == POINT[0] && !point_b)
      offset_b++;
    else
      point_b = 1;
    tmp1 = a[i + offset_a];
    tmp2 = b[j + offset_b];
    if ((IS_LOWER(tmp1)) && (IS_UPPER(tmp2)))
      tmp1 = TO_UPPER(tmp1);
    if ((IS_LOWER(tmp2)) && (IS_UPPER(tmp1)))
      tmp2 = TO_UPPER(tmp2);
    if (tmp1 < tmp2)
      return -1;
    if (tmp1 > tmp2)
      return 1;
  }
  if (a[i + offset_a] == '\0' && b[j + offset_b] != a[i + offset_a])
    return -1;
  if (b[j + offset_b] == '\0' && b[j + offset_b] != a[i + offset_a])
    return 1;
  return 0;
}

int compare(const void *a, const void *b) {
  return mystrcmp(*(char **)a, *(char **)b);
}

void print_modes(mode_t mode) {
  if (S_ISFIFO(mode) || S_ISDIR(mode))
    putchar(modes[D]);
  else if (S_ISLNK(mode))
    putchar(modes[L]);
  else
    putchar(modes[D]);
  printf("%c", (mode & S_IRUSR) ? modes[R] : modes[M]);
  printf("%c", (mode & S_IWUSR) ? modes[W] : modes[M]);
  printf("%c", (mode & S_IXUSR) ? modes[X] : modes[M]);
  printf("%c", (mode & S_IRGRP) ? modes[R] : modes[M]);
  printf("%c", (mode & S_IWGRP) ? modes[W] : modes[M]);
  printf("%c", (mode & S_IXGRP) ? modes[X] : modes[M]);
  printf("%c", (mode & S_IROTH) ? modes[R] : modes[M]);
  printf("%c", (mode & S_IWOTH) ? modes[W] : modes[M]);
  printf("%c", (mode & S_IXOTH) ? modes[X] : modes[M]);
}

void print_type(mode_t mode, char *file) {
  char buf[BUFFER_SIZE];
  int readed, i;
  if (S_ISDIR(mode))
    BLUE(file);
  else if (S_ISFIFO(mode))
    YELLOW(file);
  else if (S_ISLNK(mode)) {
    CYAN(file);
    printf(LNK);
    if ((readed = readlink(file, buf, BUFFER_SIZE)) == ERR)
      syserror(READ);
    for (i = 0; i < readed; i++)
      putchar(buf[i]);
  } else if (S_ISSOCK(mode))
    MAGENTA(file);
  else if (mode & S_IXUSR)
    GREEN(file);
  else if (S_ISREG(mode))
    printf("%s ", file);
  else
    BLACK(file);
}

void print_date(time_t time) {
  char buf[TIME_SIZE];
  struct tm *convert = localtime(&time);
  strftime(buf, TIME_SIZE, HOUR_FORMAT, convert);
  buf[0] = tolower(buf[0]);
  printf(" %s.  ", buf);
  strftime(buf, sizeof(buf), TIME_FORMAT, convert);
  printf("%s ", buf);
}

void print_owners(int uid, int gid) {
  struct passwd *pwd;
  struct group *grp;
  if ((pwd = getpwuid(uid)) == NULL)
    syserror(USR);
  if ((grp = getgrgid(gid)) == NULL)
    syserror(GRP);
  printf("%s %s ", pwd->pw_name, grp->gr_name);
}

/*2:lnk, 1:dir, 0:others*/
int is_dir(char *filename) {
  struct stat st;
  if (lstat(filename, &st) == ERR)
    syserror(STAT);
  if (S_ISLNK(st.st_mode))
    return 2;
  if (S_ISDIR(st.st_mode))
    return 1;
  return 0;
}

void print_file(char *file) {
  struct stat st;
  if (lstat(file, &st) == ERR)
    syserror(STAT);
  print_modes(st.st_mode);
  printf(" %ld ", st.st_nlink);
  print_owners(st.st_uid, st.st_gid);
  printf("%5ld", st.st_size);
  print_date(st.st_mtime);
  print_type(st.st_mode, file);
  putchar('\n');
}

void print_files(MYDIR *dir, Arguments *args) {
  int i;
  char buf[BUFFER_SIZE], *cwd;
  long int total = 0;
  struct stat st;
  MYDIR *sub_dir = create_dir();
  qsort(dir->files, dir->nb_files, sizeof(dir->files), compare);
  for (i = 0; i < dir->nb_files; i++)
    if (((dir->files)[i][0] == POINT[0] && args->a) ||
        (dir->files)[i][0] != POINT[0]) {
      if (lstat((dir->files)[i], &st) == ERR)
        syserror(STAT);
      total += st.st_blocks;
    }
  printf("%s %ld\n", TOTAL, total >> 1);
  for (i = 0; i < dir->nb_files; i++) {
    if ((dir->files[i][0] == POINT[0] && args->a) ||
        dir->files[i][0] != POINT[0]) {
      print_file((dir->files)[i]);
      if (args->r && is_dir(dir->files[i]) == 1 &&
          strcmp(dir->files[i], POINT) != 0 &&
          strcmp(dir->files[i], PARENT) != 0) {
        if (!((sub_dir->nb_files + 1) % MAX_DIR))
          myrealloc(sub_dir);
        strcpy(sub_dir->files[sub_dir->nb_files++], dir->files[i]);
      }
    }
  }
  if (args->r) {
    if ((cwd = getcwd(buf, BUFFER_SIZE)) == NULL)
      syserror(GET);
    read_subdir(sub_dir, args, cwd);
  }
  free_dir(sub_dir);
}

void read_files(DIR *dir, Arguments *args) {
  struct dirent *file;
  MYDIR *mydir=NULL;
  mydir = create_dir(mydir);
  if(mydir != NULL) {
    while ((file = readdir(dir)) != NULL) {
      if (!((mydir->nb_files + 1) % MAX_DIR))
        myrealloc(mydir);
      strcpy(mydir->files[mydir->nb_files++], file->d_name);
    }
    print_files(mydir, args);
  }
  free_dir(mydir);
}

/*divides command line into files and directories*/
void copy_argfiles(MYDIR *f, MYDIR *d, int nb, char **list) {
  int i;
  struct stat st;
  for (i = 0; i < nb; i++) {
    if (lstat(list[i], &st) == ERR)
      syserror(STAT);
    if (!S_ISLNK(st.st_mode) && !S_ISDIR(st.st_mode)) {
      if (!((f->nb_files + 1) % MAX_DIR))
        myrealloc(f);
      strcpy(f->files[f->nb_files++], list[i]);
    } else {
      if (!((d->nb_files + 1) % MAX_DIR))
        myrealloc(d);
      strcpy(d->files[d->nb_files++], list[i]);
    }
  }
}

/*open a dir and read its files*/
void read_argdir(char *dirname, Arguments *args, char *parent) {
  char buf[BUFFER_SIZE], *cwd;
  DIR *dir;
  if (chdir(dirname) == ERR)
    syserror(CHDIR);
  if ((cwd = getcwd(buf, BUFFER_SIZE)) == NULL)
    syserror(GET);
  if (args->r)
    printf("%s/%s:\n", parent, dirname);
  if ((dir = opendir(cwd)) == NULL)
    syserror(OPEN);
  read_files(dir, args);
  if (closedir(dir) == ERR)
    syserror(CLOSE);
}

/*call the function to read a dir in another dir*/
void read_subdir(MYDIR *dir, Arguments *args, char *cwd) {
  int i;
  for (i = 0; i < dir->nb_files; i++) {
    read_argdir(dir->files[i], args, cwd);
    if (chdir(cwd) == ERR)
      syserror(CHDIR);
    if (i < dir->nb_files - 1)
      putchar('\n');
  }
}

/*reads the current dir*/
void read_currentdir(Arguments *args) {
  DIR *dir;
  char buf[BUFFER_SIZE], *cwd;
  if ((cwd = getcwd(buf, BUFFER_SIZE)) == NULL)
    syserror(GET);
  if (args->r)
    printf("%s:\n", POINT);
  if ((dir = opendir(cwd)) == NULL)
    syserror(OPEN);
  read_files(dir, args);
  if (closedir(dir) == ERR)
    syserror(CLOSE);
}

/*reads the files and directories on the command line*/
void get_infodir(Command_line *cmd_line) {
  char buf[BUFFER_SIZE], *cwd;
  int i;
  MYDIR *files = create_dir(), *dir = create_dir();
  if ((cwd = getcwd(buf, BUFFER_SIZE)) == NULL)
    syserror(GET);
  copy_argfiles(files, dir, cmd_line->dir->nb_files, cmd_line->dir->files);
  qsort(files->files[0], files->nb_files, FILENAME_SIZE, compare);
  for (i = 0; i < files->nb_files; i++)
    printf("%s", files->files[i]);
  if (i)
    putchar('\n');
  read_subdir(dir, cmd_line->args, cwd);
  free_dir(files);
  free_dir(dir);
  if (!cmd_line->dir->nb_files)
    read_currentdir(cmd_line->args);
}

/*reads program options*/
int get_arg_value(char c, Command_line *cmd_line) {
  if (c == LOWER_A) {
    (cmd_line->args)->a = 1;
    return 1;
  }
  if (c == UPPER_R) {
    (cmd_line->args)->r = 1;
    return 1;
  }
  fprintf(stderr, message[CMD], c);
  return 0;
}

/*reads the command line arguments*/
int read_args(int argc, char **argv, Command_line *cmd_line) {
  int len, i, j;
  for (i = 1; i < argc; i++)
    if (argv[i][0] == TIRET) {
      len = strlen(argv[i]);
      for (j = 1; argv[i][j] != ' ' && j < len; j++)
        if (!get_arg_value(argv[i][j], cmd_line))
          return -1;
    } else {
      if (!((cmd_line->dir->nb_files + 1) % MAX_DIR))
        myrealloc(cmd_line->dir);
      strcpy((cmd_line->dir->files)[cmd_line->dir->nb_files++], argv[i]);
    }
  return 0;
}

void read_cmd_line(int argc, char **argv) {
  Command_line *cmd_line = create_cmd_line();
  if (cmd_line) {
    if (read_args(argc, argv, cmd_line) >= 0) {
      get_infodir(cmd_line);
    }
    free_cmd_line(cmd_line);
  }
}

int main(int argc, char **argv) {
  read_cmd_line(argc, argv);
  exit(0);
}


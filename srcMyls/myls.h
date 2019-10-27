#define BLACK(m) printf("\033[01;30m%s \033[0m",m)
#define RED(m) printf("\033[01;31m%s \033[0m",m)
#define GREEN(m) printf("\033[01;32m%s \033[0m",m)
#define YELLOW(m) printf("\033[01;33m%s \033[0m",m)
#define BLUE(m) printf("\033[01;34m%s \033[0m",m)
#define MAGENTA(m) printf("\033[01;35m%s \033[0m",m)
#define CYAN(m) printf("\033[01;36m%s \033[0m",m)

#define BUFFER_SIZE 512
#define FILENAME_SIZE 256
#define TIME_SIZE 20
#define MAX_DIR 40
#define ERR -1
#define POINT "."
#define STAR "*"
#define PARENT ".."
#define LNK "-> "
#define TIME_FORMAT "%e %H:%M"
#define HOUR_FORMAT "%h"
#define TOTAL "total"
#define DIRECTORY 'd'
#define TIRET '-'
#define LOWER_A 'a'
#define UPPER_R 'R'
#define IS_LOWER(x) x >= 'a' && x <= 'z'
#define IS_UPPER(x) x >= 'A' && x <= 'Z'
#define TO_UPPER(x) x - 'a' + 'A'

char *message[] = {
  "Error while getting current working directory",
  "Error while changing current working directory",
  "Error while opening directory",
  "Error while closing directory",
  "Error while stating file",
  "User id error",
  "Groupe id error",
  "Error while getting link target",
  "Error -%c is not valid. Use -a or -R\n",
};

char modes[6] = {'d', '-', 'r', 'w', 'x', 'l'};

typedef struct arguments{
	unsigned int a : 1;
	unsigned int r : 1;
} Arguments;

typedef struct mydir{
  char **files;
  int nb_files;
} MYDIR;

typedef struct{
  struct arguments *args;
  MYDIR *dir;
} Command_line;

typedef enum {GET=0,CHDIR,OPEN,CLOSE,STAT,USR,GRP,READ,CMD} Err_code;
typedef enum {D,M,R,W,X,L} Modes;

#define syserror(n) perror(message[n]), exit(n)

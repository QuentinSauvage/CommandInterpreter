#define ERR -1
#define SZ_INFOS 11
#define BUFFER_SIZE 200
#define TOKEN_SIZE 512
#define PATH_SIZE 30
#define BASE_LIM 8
#define NB_LIM 3
#define OFF_LIM 11
#define STAT_LIM 4
#define TTY_LIM 66
#define CMD_LIM 23
#define RATIO 100.0
#define CLK_TCK sysconf(_SC_CLK_TCK)
#define SND_VALUE 60
#define BEGIN_YEAR 1970
#define YEAR(st) (st/60/60/24/365)
#define DAY() (60*60*24*365)
#define DAY_VALUE 60/60/24
#define NB_DAY(st) ((st%30)+1)
#define NB_MONTH(st) ((st/30)+1)
#define NULL_FORMAT "0.0"
#define TIME_FORMAT "%H:%M"
#define LOW_TTY 128
#define HIGH_TTY 0x8000
#define SLAVE_0 "pts/"
#define SLAVE_1 "tty"
#define CMD_INDEX 1
#define R_MODE "r"
#define STAT_FORMAT "%c %*d %*d %d %ld %d %*d %*d %*d %*d %*d %ld %ld %*d %*d %*d %d %d %*d %ld"
#define HEADER() printf("USER       PID %sCPU %sMEM    VSZ   RSS TTY      STAT START   TIME COMMAND\n","%","%");

#define READABLE(c) c&&c!=' '&&c!='\t'&&c!='\n'&&c!='\r'
#define PATH(buf,n) sprintf(buf,"%s/%s","/proc",n);
#define PPATH(buf,n,f) sprintf(buf,"%s/%s/%s","/proc",n,f);
#define IS_UPPER(c) (c>='A'&&c<='Z')
#define IS_LOWER(c) (c>='a'&&c<='z')
#define IS_LETTER(c) IS_LOWER(c)||IS_UPPER(c)
#define syserror(n) perror(message[n]), exit(n)
#define ch(c) characters[c]
#define idx(index) indexes[index]
#define path(p) pathes[p]

char *message[] = {
	"Error while changing current working directory",
	"Error while opening file",
	"Error while closing file",
	"Error while stating file",
	"Error while reading",
};
char *indexes[] = {"Name:","Uid:","VmSize:","VmLck:","VmRSS:",
	"MemTotal:","NSpid:","NSsid:"
};
char characters[] = {' ','\t','\n','\r','+','s','N','<','l','\0','L','[',
	']',':','?','.',')'
};
char *pathes[] = {"/proc","meminfo","uptime","stat","status","cmdline"};

typedef char infos[11][BUFFER_SIZE];
typedef enum {USER=0,PID,CPU,MEM,VSZ,RSS,TTY,STAT,START,TIME,COMMAND} Infos_name;
typedef enum {CHDIR,OPEN,CLOSE,ST,READ} Err_code;
typedef enum {SPACE,TAB,NEWLINE,RETURN,PLUS,SESSION,LOW,HIGH,THREAD,END,LCK,
	L_BRACKET,R_BRACKET,COMA,Q_MARK,POINT,CLOSE_P} Characters;
typedef enum {NAME_IDX,UID_IDX,VSZ_IDX,LCK_IDX,RSS_IDX,MEM_IDX,
	SS_IDX_1,SS_IDX_2} Indexes;
typedef enum {PROC_PATH,MEM_PATH,BOOT_PATH,STAT_PATH,STATUS_PATH,CMD_PATH} Pathes;
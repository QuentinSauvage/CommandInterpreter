#ifndef INTERPRETER_H
#define INTERPRETER_H

#define SIZELGCMD 4096
#define SIZEPROMPT 512
#define SIZEARG 256
#define SIZE 128
#define MAX_REDIRECTION 16
#define PROMPT_CHAR '>'
#define NONE_SEQUENCER ';'
#define AND_SEQUENCER '&'
#define OR_SEQUENCER '|'
#define VAR_CH '$'
#define HOME_CH '~'
#define INNER_CMD_EXIT "exit"
#define INNER_CMD_CD "cd"
#define INNER_CMD_STATUS "status"
#define INNER_CMD_MYJOBS "myjobs"
#define INNER_CMD_MYFG "myfg"
#define INNER_CMD_MYBG "mybg"
#define EXIT_QUESTION "Are you sure you want to exit?"
#define SET "set"
#define UNSET "unset"
#define SET_ENV "setenv"
#define UNSET_ENV "unsetenv"

#define SHOULD_EXEC(seq, stat) ((seq == NONE || seq == PIPE || (seq == AND && !stat) || (seq == OR && stat))

typedef enum { NONE, AND, OR, PIPE } SeqCond;

typedef enum { FG_MODE, BG_MODE } ExecMode;

typedef struct {
  char *tabcmd[SIZE];
  int argCount;
  SeqCond sequence;
  Redirection redirList[MAX_REDIRECTION];
  int nbRedirect;
} Command;

struct status {
  char cmd[SIZELGCMD];
  int exit_num;
};

typedef struct status *Status;

/**Handler for SIGINT*/
void int_handler(int);
/**Handler for SIGTSTP (Ctrl + Z*/
void tstp_handler(int sig);
/**Initialize a Command*/
void init_cmd(Command *);
/**reads the users input*/
void run();
/**reads the command typed*/
void runLine(char *, Command *, Status, ExecMode);
/**reads the sequencers*/
char *sequence_action(SeqCond, Command *, int, char *, char *);
/**reads special characters in the command line*/
char *fill_cmd(Command *, char *);
/**wildcards*/
void replace_regex(Command *, char *);
/**exec the command line*/
void execution(Command *, Status, int, int, ExecMode);
/**Do all the rediretion for the Command*/
void doRedirect(Command *);
/**Execute external Command*/
int exec_cmd(Command *, int, int);
/**Execute cd*/
int exec_cd(Command *);
/**Execute status*/
int exec_status(Status);
/**Execute myjobs*/
int exec_myJobs();
/**Execute myfg*/
int exec_myFg(Command *);
/**Execute mybg*/
int exec_myBg(Command *);
/**Concat all the string in the array to a single one*/
void concat(char *, char **);
/**free a Command*/
void freeCommand(Command *);

#endif


#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <signal.h>
#include <regex.h>
#include <dirent.h>
#include "message.h"
#include "redirection.h"
#include "interpreter.h"
#include "background.h"
#include "variables.h"

extern int kill(pid_t, int);

/**pid of the current foreground process (or 0)*/
static pid_t foreground_pid = 0;

static char *yesList[] = {"yes", "y", "oui", "o", NULL};
static char *noList[] = {"no", "n", "non", NULL};
static char *home_dir;
static int home_dir_len;

static void printPrompt() {
  int offset=0;
  char prompt[SIZEPROMPT];
  getcwd(prompt, SIZEPROMPT);
  if(!strncmp(home_dir,prompt,home_dir_len)) {
    prompt[home_dir_len-1]=HOME_CH;
    offset=home_dir_len-1;
  }
  printf("%s%c", &prompt[offset], PROMPT_CHAR);
  fflush(stdout);
}

static int promptExit() {
  char answer[SIZELGCMD];
  int i;
  for (;;) {
    printf(EXIT_QUESTION);
    scanf("%s", answer);
    for (i = 0; yesList[i]; i++)
      if (!strcmp(answer, yesList[i]))
        return 1;
    for (i = 0; noList[i]; i++)
      if (!strcmp(answer, noList[i]))
        return 0;
  }
}

/**Handler for SIGINT*/
void int_handler(int sig) {
  if(sig == SIGINT) {
    putchar('\n');
    if (!foreground_pid) {
      if (promptExit()) {
        closeJobs();
        free_local();
        free_env();
        exit(NO_ERROR);
      }
      printPrompt();
    } else
      kill(foreground_pid, SIGINT);
  }
}

/**Handler for SIGTSTP (Ctrl + Z)*/
void tstp_handler(int sig) {
  if(sig == SIGTSTP) {
    if (foreground_pid) {
      putchar('\n');
      kill(foreground_pid, SIGTSTP);
    }
  }
}

/**Initialize a Command*/
void init_cmd(Command *cmd) {
  int i;
  for (i = 0; i < SIZE; i++) {
    cmd->tabcmd[i] = (char *)malloc(sizeof(char) * SIZEARG);
  }
  cmd->nbRedirect = 0;
}

/**reads the users input*/
void run(char **envp) {
  struct passwd *pw;
  char lgcmd[SIZELGCMD], backcmd[SIZELGCMD], *s;
  Command cmd;
  Status last_stat = (Status)malloc(sizeof(struct status));
  last_stat->exit_num = ABNORNAL_EXIT;
  pw = getpwuid(getuid());
  home_dir=pw->pw_dir;
  home_dir_len=strlen(home_dir);
  init_env(envp);
  init_cmd(&cmd);
  s = lgcmd;
  for (;;) {
    printPrompt();
    fgets(lgcmd, SIZELGCMD - 1, stdin);
    printFinishedBackground();
    s = lgcmd;
    s = checkBackground(s, backcmd);
    for (; *backcmd;) {
      runBackground(backcmd);
      s = checkBackground(s, backcmd);
    }
    runLine(s, &cmd, last_stat, FG_MODE);
  }
}

/**reads the command typed*/
void runLine(char *start, Command *next, Status last_stat, ExecMode mode) {
  SeqCond last_seq = NONE;
  char *tmp;
  int oldPipeOut = ERR, newPipe[2];
  newPipe[0] = ERR;
  newPipe[1] = ERR;
  for (; *start != '\0';) {
    start = fill_cmd(next, start);
    if (next->argCount) {
      if (SHOULD_EXEC(last_seq, last_stat->exit_num))) {
          if (next->sequence == PIPE) {
            if (pipe(newPipe) == ERR)
              fatalsyserror(3);
          }
          tmp = next->tabcmd[next->argCount];
          next->tabcmd[next->argCount] = NULL;
          execution(next, last_stat, oldPipeOut, newPipe[1], mode);
          next->nbRedirect = 0;
          next->tabcmd[next->argCount] = tmp;
          close(newPipe[1]);
          newPipe[1] = -1;
          close(oldPipeOut);
          oldPipeOut = newPipe[0];
        }
      last_seq = next->sequence;
    }
  }
}

/**reads the sequencers*/
char *sequence_action(SeqCond seq, Command *cmd, int before, char *start,
                      char *end) {
  if (before) {
    *end = '\0';
    replace_regex(cmd, start);
  }
  cmd->sequence = seq;
  if (seq == NONE || seq == PIPE)
    end++;
  else
    end += 2;
  return end;
}

/**handles the redirection*/
void addRedirection(Command *cmd, RedirType type, char **start) {
  char *s;
  for (; isspace(**start); (*start)++)
    ;
  s = *start;
  for (; !isspace(**start); (*start)++)
    ;
  *(*start++) = '\0';
  cmd->redirList[cmd->nbRedirect].type = type;
  strncpy(cmd->redirList[cmd->nbRedirect++].filename, s,
          SIZE_REDIRECTION_FILENAME);
}

/**reads special characters in the command line*/
char *fill_cmd(Command *cmd, char *start) {
  char *ps, *sRedir;
  int redirType;
  cmd->argCount = 0;
  cmd->sequence = NONE;
  for (; isspace(*start); start++)
    ;
  ps = start;
  while (*start) {
    redirType = ERR;
    while (!isspace(*ps) && *ps != NONE_SEQUENCER && *ps != AND_SEQUENCER &&
           *ps != OR_SEQUENCER &&
           (redirType = isRedirOperator(ps, &sRedir)) == ERR)
      ps++;
    if (*ps == NONE_SEQUENCER)
      return sequence_action(NONE, cmd, 1, start, ps);
    if (*ps == AND_SEQUENCER && *(ps + 1) == AND_SEQUENCER)
        return sequence_action(AND, cmd, 1, start, ps);
    if (*ps == OR_SEQUENCER) {
      if (*(ps + 1) == OR_SEQUENCER)
        return sequence_action(OR, cmd, 1, start, ps);
      return sequence_action(PIPE, cmd, 1, start, ps);
    }
    *ps++ = '\0';
    replace_regex(cmd, start);
    if (redirType != ERR) {
      addRedirection(cmd, redirType, &sRedir);
      ps = sRedir + 1;
    }
    while (isspace(*ps))
      ps++;
    if (*ps == NONE_SEQUENCER)
      return sequence_action(NONE, cmd, 0, start, ps);
    if (*ps == AND_SEQUENCER && *(ps + 1) == AND_SEQUENCER)
        return sequence_action(AND, cmd, 0, start, ps);
    if (*ps == OR_SEQUENCER) {
      if (*(ps + 1) == OR_SEQUENCER)
        return sequence_action(OR, cmd, 0, start, ps);
      return sequence_action(PIPE, cmd, 0, start, ps);
    }
    if ((redirType = isRedirOperator(ps, &sRedir)) != ERR) {
      addRedirection(cmd, redirType, &sRedir);
      ps = sRedir + 1;
    }
    start = ps;
  }
  return start;
}

/**prepare the replacements with wildcards*/
int create_regex(regex_t *reg, char *src) {
  char convert[SIZEARG];
  int ic, is;
  convert[0] = '^';
  for (ic = 1, is = 0; src[is]; ic++, is++) {
    if (src[is] == '?')
      convert[ic] = '.';
    else if (src[is] == '*')
      convert[ic++] = '.', convert[ic] = '*';
    else if (src[is] == '.')
      convert[ic++] = '\\', convert[ic] = '.';
    else
      convert[ic] = src[is];
  }
  convert[ic++] = '$';
  convert[ic] = '\0';
  return regcomp(reg, convert, REG_EXTENDED | REG_NOSUB);
}

/**wildcards*/
void replace_regex(Command *cmd, char *arg) {
  regex_t reg;
  int matching = 0;
  DIR *dir;
  struct dirent *file;
  if (!create_regex(&reg, arg)) {
    dir = opendir(".");
    while ((file = readdir(dir)) != NULL) {
      if (regexec(&reg, file->d_name, 0, NULL, 0) != REG_NOMATCH) {
        matching++;
        strcpy(cmd->tabcmd[cmd->argCount++], file->d_name);
      }
    }
    closedir(dir);
  }
  if (!matching)
    strcpy(cmd->tabcmd[cmd->argCount++], arg);
  regfree(&reg);
}

void replace_var(Command *cmd) {
  int i,len;
  char *res;
  for(i=0;cmd->tabcmd[i]!=NULL;i++) {
    if(cmd->tabcmd[i][0]==HOME_CH) {
      len=strlen(cmd->tabcmd[i]);
      if(len-1+home_dir_len<SIZE) {
        res=malloc((len-1)*sizeof(char));
        strcpy(res,&cmd->tabcmd[i][1]);
        sprintf(cmd->tabcmd[i],"%s%s",home_dir,res);
        free(res);
      }
    } else if(cmd->tabcmd[i][0]==VAR_CH) {
      res=get_local(&cmd->tabcmd[i][1]);
      if(!res)
        res=get_env(&cmd->tabcmd[i][1]);
      if(res) strcpy(cmd->tabcmd[i],res);
    }
  }
}

/**exec the command line*/
void execution(Command *cmd, Status stat, int inFd, int outFd, ExecMode mode) {
  /*need  to check for inner cmd with limited nb arg*/
  /*manage redirection for inner cmd ?*/
  if (!strcmp(cmd->tabcmd[0], INNER_CMD_EXIT)) {
    free_local();
    free_env();
    exit(NO_ERROR);
  }
  if (!strcmp(cmd->tabcmd[0], SET)) {
    if (cmd->tabcmd[1])
      split_arg(cmd->tabcmd[1], 0);
    else
      print_all_local();
  } else if (!strcmp(cmd->tabcmd[0], UNSET) && cmd->tabcmd[1]) {
    if(cmd->tabcmd[1][0] == VAR_CH)
      unset(&cmd->tabcmd[1][1]);
  } else if (!strcmp(cmd->tabcmd[0], SET_ENV)) {
    if (cmd->tabcmd[1])
      split_arg(cmd->tabcmd[1], 1);
    else
      print_all_env();
  } else if (!strcmp(cmd->tabcmd[0], UNSET_ENV) && cmd->tabcmd[1]) {
    if(cmd->tabcmd[1][0] == VAR_CH)
      unset_env(&cmd->tabcmd[1][1]);
  } else {
    replace_var(cmd);
    if (!strcmp(cmd->tabcmd[0], INNER_CMD_CD)) {
      stat->exit_num = exec_cd(cmd);
    } else if (!strcmp(cmd->tabcmd[0], INNER_CMD_STATUS)) {
      stat->exit_num = exec_status(stat);
    } else if (!strcmp(cmd->tabcmd[0], INNER_CMD_MYJOBS)) {
      stat->exit_num = exec_myJobs();
    } else if (!strcmp(cmd->tabcmd[0], INNER_CMD_MYFG)) {
      stat->exit_num = exec_myFg(cmd);
    } else if (!strcmp(cmd->tabcmd[0], INNER_CMD_MYBG)) {
      stat->exit_num = exec_myBg(cmd);
    } else if (mode == BG_MODE) {
      stat->exit_num = exec_bg_cmd(cmd, inFd, outFd);
    } else {
      stat->exit_num = exec_cmd(cmd, inFd, outFd);
    }
  }
  concat(stat->cmd, cmd->tabcmd);
}

/**Do all the rediretion for the Command*/
void doRedirect(Command *cmd) {
  int i;
  for (i = 0; i < cmd->nbRedirect; i++)
    doRedirection(cmd->redirList[i]);
}

static int waitForeground(pid_t foreground, char *cmdLine, pid_t subPid) {
  int status, jobIndex;
  foreground_pid = foreground;
  while (waitpid(foreground, &status, WUNTRACED) == ERR)
    ;
  if (WIFSTOPPED(status)) {
    jobIndex = addJob(foreground, subPid, cmdLine, STOPPED);
    printStopped(jobIndex, cmdLine);
  }
  foreground_pid = 0;
  if (WIFEXITED(status))
    status = WEXITSTATUS(status);
  else {
    status = ABNORNAL_EXIT;
  }
  return status;
}

/**Execute external Command*/
int exec_cmd(Command *cmd, int inFd, int outFd) {
  pid_t p;
  char cmdLine[SIZELGCMD];
  concat(cmdLine, cmd->tabcmd);
  if ((p = fork()) == ERR)
    fatalsyserror(1);
  if (p)
    return waitForeground(p, cmdLine, p);
  else {
    p = getpid();
    setpgid(p, p);
    doRedirect(cmd);
    if (inFd != ERR)
      close(0), dup(inFd);
    if (outFd != ERR)
      close(1), dup(outFd);
    execvpe(*(cmd->tabcmd), cmd->tabcmd, get_envp());
    syserror(EXEC);
    exit(FAILEDEXEC);
  }
}

/**Execute cd*/
int exec_cd(Command *cmd) {
  int res;
  if (cmd->tabcmd[1] == NULL) {
    return chdir(home_dir);
  }
  if ((res = chdir(cmd->tabcmd[1])) == ERR) {
    perror(cmd->tabcmd[1]);
  }
  return res;
}

/**Execute status*/
int exec_status(Status stat) {
  if (stat->exit_num == ABNORNAL_EXIT) {
    puts(STATUS_MSG_ABNORMAL);
  } else {
    printf(STATUS_MSG_BEGIN);
    printf("%s", stat->cmd);
    printf(STATUS_MSG_END, stat->exit_num);
    putchar('\n');
  }
  return 0;
}

/**Execute myjobs*/
int exec_myJobs() {
  printJobList();
  return 0;
}

/**Execute myfg*/
int exec_myFg(Command *cmd) {
  int jobIndex, numJob;
  pid_t pid, subPid;
  char cmdLine[SIZELGCMD];
  if (cmd->tabcmd[1] == NULL)
    jobIndex = getHighestJobIndex(0);
  else {
    numJob = atoi(cmd->tabcmd[1]);
    jobIndex = getJobIndex(numJob, 0);
  }
  if (jobIndex == ERR) {
    error(FG_JOB);
    return ERR;
  }
  pid = jobList[jobIndex]->jobPid;
  subPid = jobList[jobIndex]->cmdPid;
  strcpy(cmdLine, jobList[jobIndex]->cmdLine);
  kill(pid, SIGCONT);
  kill(subPid, SIGCONT);
  free(jobList[jobIndex]);
  jobList[jobIndex] = NULL;
  return waitForeground(pid, cmdLine, subPid);
}

/**Execute mybg*/
int exec_myBg(Command *cmd) {
  int jobIndex, numJob;
  if (cmd->tabcmd[1] == NULL)
    jobIndex = getHighestJobIndex(1);
  else {
    numJob = atoi(cmd->tabcmd[1]);
    jobIndex = getJobIndex(numJob, 1);
  }
  if (jobIndex == ERR) {
    error(BG_JOB);
    return ERR;
  }
  kill(jobList[jobIndex]->jobPid, SIGCONT);
  kill(jobList[jobIndex]->cmdPid, SIGCONT);
  jobList[jobIndex]->currentState = RUNNING;
  return 0;
}

/**Concat all the string in the array to a single one*/
void concat(char *dest, char **tabcmd) {
  char **ps;
  int len = 0;
  memset(dest, 0, SIZELGCMD);
  for (ps = tabcmd; *ps; ps++) {
    strcpy(dest + len, *ps);
    len += strlen(*ps);
    *(dest + len++) = ' ';
  }
}
/**free a Command*/
void freeCommand(Command *cmd) {
  int i;
  for (i = 0; i < SIZE; i++) {
    free(cmd->tabcmd[i]);
  }
}


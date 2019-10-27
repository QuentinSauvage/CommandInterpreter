#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include "message.h"
#include "redirection.h"
#include "interpreter.h"
#include "variables.h"
#include "background.h"

extern int kill(pid_t, int);

/**Index of the highest job in the list*/
static int highestJob = 0;
static char *jobStateList[] = {"En cours d'exécution", "Stoppé"};
/**Pipe used to give to the main prosess the new job pid*/
static int getPidPipe[2];

/**Initialize the jobList with NULL jobs*/
void initJobList() {
  int i;
  for (i = 0; i < MAX_JOB; i++)
    jobList[i] = NULL;
}

/**Check if cmdLine contains cmd to execute in background*/
char *checkBackground(char *start, char *buf) {
  char *s, *cpBuf;
  for (s = start, cpBuf = buf; *s != '\0'; s++) {
    if (*s == '&') {
      if (*(++s) != '&') {
        *cpBuf = '\0';
        return s;
      } else
        *(cpBuf++) = *(s - 1);
    }
    *(cpBuf++) = *s;
  }
  *buf = '\0';
  return start;
}

/**Add a new job to the job list.
Returns the job index in the jobList (not the numJob)*/
int addJob(pid_t jobPid, pid_t cmdPid, char *cmdLine, JobState state) {
  int i, h = ERR;
  for (i = 0; i < MAX_JOB; i++)
    if (jobList[i])
      h = i;
  if (h == ERR)
    highestJob = 0;
  jobList[highestJob] = (Job)malloc(sizeof(struct str_job));
  jobList[highestJob]->numJob = highestJob + 1;
  jobList[highestJob]->jobPid = jobPid;
  jobList[highestJob]->cmdPid = cmdPid;
  strncpy(jobList[highestJob]->cmdLine, cmdLine, JOB_CMD_SIZE - 1);
  jobList[highestJob]->cmdLine[JOB_CMD_SIZE - 1] = '\0';
  jobList[highestJob]->currentState = state;
  return highestJob++;
}

/**get a job in the list with the jobpid*/
int getJobsByPid(pid_t jobPid) {
  int i;
  for (i = 0; i < MAX_JOB; i++) {
    if (jobList[i] && jobList[i]->jobPid == jobPid)
      return i;
  }
  return ERR;
}

/**Print all the finished jobs in background*/
void printFinishedBackground() {
  pid_t child = 1;
  int status, jobIndex, stopped = 0;
  while (child > 0) {
    child = waitpid(-1, &status, WNOHANG | WUNTRACED);
    if (WIFSTOPPED(status)) {
      stopped = 1;
    } else if (WIFEXITED(status)) {
      if ((status = WEXITSTATUS(status)) == ABNORNAL_EXIT)
        status = ERR;
    } else
      status = ERR;
    if ((jobIndex = getJobsByPid(child)) != ERR) {
      if (stopped)
        jobList[jobIndex]->currentState = STOPPED;
      else {
        printf(FINISHED_JOB_PRINT, jobList[jobIndex]->cmdLine,
               jobList[jobIndex]->numJob, jobList[jobIndex]->cmdPid, status);
        free(jobList[jobIndex]);
        jobList[jobIndex] = NULL;
      }
    }
  }
}

/**Print all the jobs in the list*/
void printJobList() {
  int i, status;
  for (i = 0; i < MAX_JOB; i++) {
    if (jobList[i]) {
      if(waitpid(jobList[i]->cmdPid, &status, WNOHANG | WUNTRACED | WCONTINUED) != ERR) {
        if(WIFSTOPPED(status))
          jobList[i]->currentState = STOPPED;
        else if(WIFCONTINUED(status))
          jobList[i]->currentState = RUNNING;
      }
      printf(MYJOBS_PRINT_LINE, jobList[i]->numJob, jobList[i]->cmdPid,
             jobStateList[jobList[i]->currentState], jobList[i]->cmdLine);
    }
  }
}

/**Print when a Command is stopped*/
void printStopped(int jobIndex, char *cmdLine) {
  printf(STOPPED_PRINT, jobList[jobIndex]->numJob, cmdLine,
         jobStateList[STOPPED]);
}

static void transferHandler(int sig) {
  kill(0, sig);
  signal(sig, transferHandler);
}

/**Run the given cmdLine in background*/
void runBackground(char *cmdLine) {
  pid_t pidBg;
  Command cmd;
  int jobIndex, p;
  Status last_stat;
  if (pipe(getPidPipe) == ERR)
    fatalsyserror(3);
  if ((pidBg = fork()) == ERR)
    fatalsyserror(1);
  if (pidBg) {
    close(getPidPipe[1]);
    read(getPidPipe[0], &p, sizeof(int));
    close(getPidPipe[0]);
    jobIndex = addJob(pidBg, p, cmdLine, RUNNING);
    printf(STARTED_JOB_PRINT, jobList[jobIndex]->numJob, p);
  } else {
    signal(SIGSTOP, transferHandler);
    signal(SIGTSTP, transferHandler);
    signal(SIGINT, transferHandler);
    pidBg = getpid();
    setpgid(pidBg, pidBg);
    close(getPidPipe[0]);
    last_stat = (Status)malloc(sizeof(struct status));
    last_stat->exit_num = ABNORNAL_EXIT;
    init_cmd(&cmd);
    runLine(cmdLine, &cmd, last_stat, BG_MODE);
    freeCommand(&cmd);
    p = last_stat->exit_num;
    free(last_stat);
    exit(p);
  }
}

/**Send the new process pid and execute the Command*/
int exec_bg_cmd(Command *cmd, int inFd, int outFd) {
  pid_t p;
  int status;
  if ((p = fork()) == ERR)
    fatalsyserror(1);
  if (p) {
    if (getPidPipe[1] != ERR) {
      write(getPidPipe[1], &p, sizeof(int));
      close(getPidPipe[1]);
      getPidPipe[1] = ERR;
    }
    while (waitpid(p, &status, WUNTRACED) == ERR)
      ;
    while (WIFSTOPPED(status)) {
      kill(getpid(), SIGSTOP);
      waitpid(p, &status, WUNTRACED);
    }
    if (WIFEXITED(status))
      status = WEXITSTATUS(status);
    else
      status = ABNORNAL_EXIT;
  } else {
    close(getPidPipe[1]);
    doRedirect(cmd);
    if (inFd != ERR)
      close(0), dup(inFd);
    if (outFd != ERR)
      close(1), dup(outFd);
    execvpe(*(cmd->tabcmd), cmd->tabcmd, get_envp());
    syserror(2);
    exit(FAILEDEXEC);
  }
  return status;
}

/**Get the highest job's index*/
int getHighestJobIndex(int stopped) {
  int i;
  for (i = highestJob - 1; i >= 0; i--) {
    if (jobList[i] && (!stopped || jobList[i]->currentState == STOPPED))
      return i;
  }
  return ERR;
}

/**Get a job's index*/
int getJobIndex(int numJob, int stopped) {
  int i;
  if (numJob > 0) {
    for (i = highestJob - 1; i >= 0; i--) {
      if (jobList[i] && jobList[i]->numJob == numJob) {
        if (!stopped || jobList[i]->currentState == STOPPED)
          return i;
        break;
      }
    }
  }
  return ERR;
}

/**Close all the jobs*/
void closeJobs() {
  int i;
  for (i = 0; i < MAX_JOB; i++) {
    if (jobList[i]) {
      kill(jobList[i]->jobPid, SIGKILL);
      kill(jobList[i]->cmdPid, SIGKILL);
    }
  }
}


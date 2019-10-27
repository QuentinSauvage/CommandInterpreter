#ifndef BACKGROUND_H
#define BACKGROUND_H

#define MAX_JOB 64
#define JOB_CMD_SIZE 4096

#define STARTED_JOB_PRINT "[%d] %d\n"
#define FINISHED_JOB_PRINT "%s (jobs=[%d], pid=%d) terminée avec status=%d\n"
#define MYJOBS_PRINT_LINE "[%d] %d %s %s\n"
#define STOPPED_PRINT "[%d] %s %s\n"

typedef enum { RUNNING, STOPPED } JobState;

struct str_job {
  pid_t jobPid;
  pid_t cmdPid;
  int numJob;
  char cmdLine[JOB_CMD_SIZE];
  JobState currentState;
};

typedef struct str_job *Job;

Job jobList[MAX_JOB];

/**Initialize the jobList with NULL jobs*/
void initJobList();
/**Check if cmdLine contains cmd to execute in background*/
char *checkBackground(char *, char *);
/**Add a new job to the job list*/
int addJob(pid_t, pid_t, char *, JobState);
/**Print all the finished jobs in background*/
void printFinishedBackground();
/**Print all the jobs in the list*/
void printJobList();
/**Print when a Command is stopped*/
void printStopped(int, char *);
/**Run the given cmdLine in background*/
void runBackground(char *);
/**Send the new process pid and execute the Command*/
int exec_bg_cmd(Command *, int, int);
/**Get the highest job's index*/
int getHighestJobIndex(int);
/**Get a job's index*/
int getJobIndex(int, int);
/**Close all the jobs*/
void closeJobs();
#endif


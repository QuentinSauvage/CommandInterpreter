#ifndef MESSAGE_H
#define MESSAGE_H

#define FAILEDEXEC 127
#define ABNORNAL_EXIT 128
#define ERR -1

#define error(x) fprintf(stderr, "%s\n", errormsg[x]);
#define syserror(x) perror(errormsg[x])
#define fatalsyserror(x) syserror(x), exit(x)
#define RED(m) "\033[01;31m" m "\033[0m"
#define GREEN(m) "\033[01;32m" m "\033[0m"
#define STATUS_MSG_ABNORMAL RED("Abnormal exit")
#define STATUS_MSG_BEGIN GREEN("exit status of[")
#define STATUS_MSG_END GREEN("\b]=%d")

extern char *errormsg[];

typedef enum {NO_ERROR=0,FORK,EXEC,PIPE_EX,BG_JOB,FG_JOB,
	SHMGET,SHMCTL,SHMAT,SHMDT,SEMGET,SEMCTL,FTOK,V_EX,P_EX} Errors;


#endif

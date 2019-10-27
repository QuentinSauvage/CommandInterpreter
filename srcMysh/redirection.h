#ifndef REDIRECTION_H
#define REDIRECTION_H

#define SIZE_REDIRECTION_FILENAME 128
#define FILE_CREATION_MASK 0755
#define OPEN_FLAGS_TRUNC O_WRONLY | O_TRUNC | O_APPEND | O_CREAT
#define OPEN_FLAGS_APPEND O_WRONLY | O_APPEND | O_CREAT

typedef enum { OUT, OUTAPP, ER, ERAPP, OUTER, OUTERAPP, IN } RedirType;

extern char *redirOperators[];

typedef struct {
  char filename[SIZE_REDIRECTION_FILENAME];
  RedirType type;
} Redirection;

/**Test for redirection operator*/
int isRedirOperator(char *, char **);
/**Close and open filedescriptors corresponding to the rediretion*/
void doRedirection(Redirection);

#endif


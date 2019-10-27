#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "message.h"
#include "redirection.h"

char *redirOperators[] = {">", ">>", "2>", "2>>", ">&", ">>&", "<"};

/**Test for redirection operator*/
int isRedirOperator(char *start, char **end) {
  int i, size;
  for (i = IN; i >= OUT; i--) {
    size = strlen(redirOperators[i]);
    if (!strncmp(redirOperators[i], start, size)) {
      *end = start + size;
      return i;
    }
  }
  return ERR;
}

/**Close and open filedescritors corresponding to the rediretion*/
void doRedirection(Redirection r) {
  if (r.type == IN)
    close(0), open(r.filename, O_RDONLY);
  else if (r.type == OUT)
    close(1), open(r.filename, OPEN_FLAGS_TRUNC, FILE_CREATION_MASK);
  else if (r.type == OUTAPP)
    close(1), open(r.filename, OPEN_FLAGS_APPEND, FILE_CREATION_MASK);
  else if (r.type == ER)
    close(2), open(r.filename, OPEN_FLAGS_TRUNC, FILE_CREATION_MASK);
  else if (r.type == ERAPP)
    close(2), open(r.filename, OPEN_FLAGS_APPEND, FILE_CREATION_MASK);
  else if (r.type == OUTER) {
    close(1), open(r.filename, OPEN_FLAGS_TRUNC, FILE_CREATION_MASK);
    close(2), open(r.filename, OPEN_FLAGS_TRUNC, FILE_CREATION_MASK);
  } else if (r.type == OUTERAPP) {
    close(1), open(r.filename, OPEN_FLAGS_APPEND, FILE_CREATION_MASK);
    close(2), open(r.filename, OPEN_FLAGS_APPEND, FILE_CREATION_MASK);
  }
}


#include "message.h"

char *errormsg[] = {"No error",
                    RED("Impossible to fork process"),
                    RED("Exec failed"),
                    RED("Error when creating the pipe"),
                    RED("Unable to put job in background"),
                    RED("Unable to put job in foreground"),
                    RED("Error when getting shared memory"),
                    RED("Error when controlling shared memory"),
                    RED("Error when attaching shared memory"),
                    RED("Error when detaching shared memory"),
                    RED("Error when getting semaphore"),
                    RED("Error when controlling semaphore"),
                    RED("Error when ftoking"),
                    RED("Error during V operation"),
                    RED("Error during P operation"),
                    RED("Unknown error")};

#include <signal.h>

extern void run();
extern void int_handler(int);
extern void tstp_handler(int);
extern void initJobList();

int main(int argc, char **argv, char **envp) {
  signal(SIGINT, int_handler);
  signal(SIGTSTP, tstp_handler);
  initJobList();
  run(envp);
  return 0;
}


#include "rlimit.h"
#include "common.h"
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>

int rlimit_demo_cpu(void) {
  /* Step 1: Define CPU limits: soft limit = 1 sec, hard limit = 2 sec */
  struct rlimit limit = {.rlim_cur = 1, .rlim_max = 2};
  struct rlimit before;
  int status;

  printf("\n=== Resource limit: setrlimit(RLIMIT_CPU) ===\n");
  if (getrlimit(RLIMIT_CPU, &before) == 0) {
    printf("[Parent] current RLIMIT_CPU: soft=%ld hard=%ld (RLIM_INFINITY = "
           "unlimited)\n",
           (long)before.rlim_cur, (long)before.rlim_max);
  }

  /* Step 2: Fork a child process to enforce the limit */
  pid_t pid = fork_flushed();
  if (pid < 0) {
    perror("fork");
    return 1;
  }

  if (pid == 0) {
    /* Child applies the limit to itself */
    if (setrlimit(RLIMIT_CPU, &limit) != 0) {
      perror("setrlimit");
      child_exit(1);
    }
    printf("  [Child %d] limit set: soft=%ld s, hard=%ld s. Starting infinite "
           "CPU loop...\n",
           getpid(), (long)limit.rlim_cur, (long)limit.rlim_max);
    fflush(stdout);

    /* Step 3: Runaway CPU loop: consumes 100% CPU until kernel terminates it */
    volatile unsigned long long spin = 0;
    while (spin < ~0ULL)
      spin++;
  }

  /* Step 4: Parent waits for child termination */
  printf("[Parent %d] limit applied only to child %d; waiting\n", getpid(),
         pid);
  while (waitpid(pid, &status, 0) == -1) {
    if (errno != EINTR) {
      perror("waitpid");
      return 1;
    }
  }

  /* Step 5: Verify kernel delivered SIGXCPU signal */
  if (WIFSIGNALED(status) && WTERMSIG(status) == SIGXCPU) {
    printf("[Parent] child terminated by signal %d (%s).\n", SIGXCPU,
           strsignal(SIGXCPU));
    printf("Observed effect: after 1 second of CPU time the kernel sent "
           "SIGXCPU and stopped the runaway loop.\n");
    return 0;
  }
  printf(
      "[Parent] unexpected child result (status 0x%x); limit not observed.\n",
      status);
  return 1;
}

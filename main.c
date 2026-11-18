#include "banker.h"
#include "common.h"
#include "ipc.h"
#include "logger.h"
#include "rlimit.h"
#include "scheduler.h"
#include "worker.h"
#include <sys/wait.h>

static Job jobs[MAX_JOBS];
static int num_jobs;
static const char *cli_arg; /* argument following a flag, e.g. --jobs FILE */

/* Uses the CLI argument if given, otherwise prompts for it. */
static const char *get_arg(const char *prompt, char *buf, size_t n) {
  if (cli_arg)
    return cli_arg;
  printf("%s", prompt);
  if (!fgets(buf, (int)n, stdin))
    return NULL;
  buf[strcspn(buf, "\r\n")] = '\0';
  return buf[0] ? buf : NULL;
}

typedef struct {
  const char *flag; /* command-line form: ./campuscompute --<flag> */
  const char *label;
  int (*run)(void); /* 0 = success */
  bool in_demo;     /* included in the integrated demonstration */
} Command;

static int cmd_input(void) { return scheduler_input_jobs(jobs, &num_jobs); }
static int cmd_default(void) {
  scheduler_load_default_jobs(jobs, &num_jobs);
  printf("Loaded %d default jobs.\n", num_jobs);
  return 0;
}
static int cmd_jobs_file(void) {
  char buf[256];
  const char *p = get_arg("Path to jobs file: ", buf, sizeof(buf));
  return p ? scheduler_load_file(jobs, &num_jobs, p) : 1;
}
static int cmd_random(void) {
  char buf[64];
  const char *p = get_arg("Number of random jobs (1-20): ", buf, sizeof(buf));
  if (!p)
    return 1;
  int count = atoi(p);
  unsigned seed = 0;
  if (cli_arg && getenv("SEED"))
    seed = (unsigned)strtoul(getenv("SEED"), NULL, 10);
  int r = scheduler_generate_random(jobs, &num_jobs, count, seed);
  if (r == 0)
    scheduler_display_jobs(jobs, num_jobs);
  return r;
}
static int cmd_banker_file(void) {
  char buf[256];
  const char *p = get_arg("Path to Banker state file: ", buf, sizeof(buf));
  return p ? banker_from_file(p) : 1;
}
static int cmd_add(void) { return scheduler_add_job(jobs, &num_jobs); }
static int cmd_edit_pri(void) {
  return scheduler_edit_job_priority(jobs, num_jobs);
}
static int cmd_quantum(void) { return scheduler_input_quantum(); }
static int cmd_show(void) {
  scheduler_display_jobs(jobs, num_jobs);
  return 0;
}
static int cmd_fcfs(void) { return scheduler_run_fcfs(jobs, num_jobs); }
static int cmd_rr(void) { return scheduler_run_rr(jobs, num_jobs); }
static int cmd_priority(void) { return scheduler_run_priority(jobs, num_jobs); }
static int cmd_compare(void) { return scheduler_compare_all(jobs, num_jobs); }
static int cmd_demo(void);
static int cmd_cleanup(void);

static const Command commands[] = {
    {"input", "Input custom jobs (replace current list)", cmd_input, false},
    {"jobs", "Load jobs from a file (arrival burst priority)", cmd_jobs_file,
     false},
    {"random", "Generate random jobs (env SEED=n to reproduce)", cmd_random,
     false},
    {"default", "Load default workload (5 jobs)", cmd_default, false},
    {"add-job", "Add a new job to current workload", cmd_add, false},
    {"edit-priority", "Change an existing job's priority", cmd_edit_pri, false},
    {"set-quantum", "Set Round Robin time quantum", cmd_quantum, false},
    {"show", "Display current jobs", cmd_show, true},
    {"fcfs", "FCFS scheduling", cmd_fcfs, true},
    {"rr", "Round Robin scheduling", cmd_rr, true},
    {"priority", "Priority scheduling (non-preemptive)", cmd_priority, true},
    {"compare", "Compare all scheduling policies", cmd_compare, true},
    {"logger", "Producer-consumer logger (default benchmark)", logger_demo,
     true},
    {"logger-custom", "Producer-consumer logger (custom threads)",
     logger_custom_run, false},
    {"logger-msg", "Logger: submit custom message to queue",
     logger_submit_event, false},
    {"ipc", "Shared memory + semaphore IPC (benchmark)", ipc_demo, true},
    {"ipc-custom", "Shared memory IPC: send custom job packet", ipc_custom,
     false},
    {"banker-safe", "Banker's algorithm: safe state", banker_demo_safe, true},
    {"banker-unsafe", "Banker's algorithm: unsafe state", banker_demo_unsafe,
     true},
    {"banker-file", "Banker's algorithm: load state from file",
     cmd_banker_file, false},
    {"banker-input", "Banker's algorithm: enter your own state",
     banker_input_and_check, false},
    {"worker-ok", "Worker process: normal completion", worker_demo_normal,
     true},
    {"worker-fail", "Worker process: simulated failure", worker_demo_failure,
     true},
    {"worker-custom", "Worker process: custom simulated exit code",
     worker_custom, false},
    {"rlimit", "Resource limit demo (RLIMIT_CPU)", rlimit_demo_cpu, true},
    {"demo", "Integrated demonstration", cmd_demo, false},
    {"cleanup", "Verify cleanup of threads/IPC/processes", cmd_cleanup, false},
};
#define NUM_COMMANDS ((int)(sizeof(commands) / sizeof(commands[0])))

static int cmd_cleanup(void) {
  logger_shutdown();
  ipc_cleanup();

  int reaped = 0;
  while (waitpid(-1, NULL, WNOHANG) > 0)
    reaped++;
  bool no_children = (waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD);
  int left = ipc_objects_remaining();

  printf("\n=== Cleanup verification ===\n");
  printf("Logger thread: stopped and joined, log file closed\n");
  printf("Child processes: %d stray reaped, none remaining: %s\n", reaped,
         no_children ? "yes" : "NO");
  printf("IPC objects (shared memory + semaphores) remaining: %d\n", left);
  return (no_children && left == 0) ? 0 : 1;
}

static int cmd_demo(void) {
  int result[NUM_COMMANDS];
  int failures = 0;

  printf("\n=== Integrated demonstration ===\n");
  for (int i = 0; i < NUM_COMMANDS; i++) {
    if (commands[i].in_demo)
      result[i] = commands[i].run();
  }
  int cleanup_result = cmd_cleanup();

  printf("\n=== Summary ===\n");
  for (int i = 0; i < NUM_COMMANDS; i++) {
    if (!commands[i].in_demo)
      continue;
    printf("  %-14s %s\n", commands[i].flag, result[i] == 0 ? "PASS" : "FAIL");
    failures += result[i] != 0;
  }
  printf("  %-14s %s\n", "cleanup", cleanup_result == 0 ? "PASS" : "FAIL");
  failures += cleanup_result != 0;
  printf("%d check(s) failed.\n", failures);
  return failures != 0;
}

static void print_banner(void) {
  printf("\nCampusCompute - Job Scheduling and Resource Management | %s\n",
         get_srn());
}

static void print_menu(void) {
  printf("\nCommands (type the number at the prompt, or run ./campuscompute "
         "--<flag>):\n");
  for (int i = 0; i < NUM_COMMANDS; i++) {
    printf("  %2d  %-42s --%s\n", i + 1, commands[i].label, commands[i].flag);
  }
  printf("   0  Clean up and exit\n   ?  Show this menu\n");
}

static void run_interactive(void) {
  char line[128];

  print_banner();
  print_menu();
  while (1) {
    printf("\nCampusCompute [%s] >> ", get_srn());
    if (!fgets(line, sizeof(line), stdin))
      break;
    line[strcspn(line, "\r\n")] = '\0';
    if (line[0] == '\0')
      continue;
    if (strcmp(line, "?") == 0 || strcmp(line, "help") == 0) {
      print_menu();
      continue;
    }

    char *end;
    long choice = strtol(line, &end, 10);
    if (*end != '\0' || choice < 0 || choice > NUM_COMMANDS) {
      printf("[Error] Invalid command \"%s\". Enter a number 0-%d or '?' for "
             "the menu.\n",
             line, NUM_COMMANDS);
      continue;
    }
    if (choice == 0)
      break;
    commands[choice - 1].run();
  }
  cmd_cleanup();
  printf("Goodbye.\n");
}

int main(int argc, char *argv[]) {
  num_jobs = 0;
  scheduler_load_default_jobs(jobs, &num_jobs);

  if (argc == 1) {
    run_interactive();
    return 0;
  }
  if (argc == 2 &&
      (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    print_banner();
    print_menu();
    return 0;
  }
  if (argc >= 2 && strncmp(argv[1], "--", 2) == 0) {
    if (strcmp(argv[1] + 2, "quantum") == 0 ||
        strcmp(argv[1] + 2, "set-quantum") == 0) {
      if (argc >= 3) {
        int q = atoi(argv[2]);
        if (q >= 1 && q <= 100) {
          scheduler_set_quantum(q);
          printf("Round Robin time quantum set to %d.\n", q);
          return 0;
        }
      }
      return cmd_quantum();
    }
    if (strcmp(argv[1] + 2, "rr") == 0 && argc >= 3) {
      int q = atoi(argv[2]);
      if (q >= 1 && q <= 100)
        scheduler_set_quantum(q);
      return cmd_rr();
    }
    /* Options: --jobs FILE | --random N | --banker-file FILE, optionally
     * followed by --<command> to run on that data in the same invocation,
     * e.g.  ./campuscompute --jobs my.txt --compare */
    int i = 1;
    while (i < argc && strncmp(argv[i], "--", 2) == 0) {
      const char *name = argv[i] + 2;
      int k;
      for (k = 0; k < NUM_COMMANDS; k++)
        if (strcmp(name, commands[k].flag) == 0)
          break;
      if (k == NUM_COMMANDS)
        break;
      bool takes_arg = strcmp(name, "jobs") == 0 ||
                       strcmp(name, "random") == 0 ||
                       strcmp(name, "banker-file") == 0;
      cli_arg = NULL;
      if (takes_arg) {
        if (i + 1 >= argc) {
          fprintf(stderr, "[Error] --%s needs an argument\n", name);
          return 1;
        }
        cli_arg = argv[++i];
      }
      int rc = commands[k].run();
      cli_arg = NULL;
      if (rc != 0 || !takes_arg || i + 1 >= argc)
        return rc;
      i++; /* chain: data option consumed, run the next command */
    }
  }
  fprintf(stderr, "[Error] Unknown option or arguments: '%s'\n",
          argc > 1 ? argv[1] : "");
  fprintf(
      stderr,
      "Usage: %s [--help | --<command>]   (see --help for the command list)\n",
      argv[0]);
  return 1;
}

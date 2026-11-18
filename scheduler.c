#include "scheduler.h"
#include "logger.h"
#include <time.h>

#define MAX_SLICES 2048

typedef struct {
  int job_id, start, end;
} Slice; /* job_id 0 = CPU idle */
typedef struct {
  Slice s[MAX_SLICES];
  int count;
} Timeline;
typedef struct {
  int items[128];
  int head, tail;
} Queue;
typedef enum { FCFS, PRIORITY, RR } Policy;

static const char *policy_names[] = {"FCFS", "Priority (non-preemptive)",
                                     "Round Robin"};
static int current_quantum = TIME_QUANTUM;

int scheduler_set_quantum(int q) {
  if (q < 1 || q > 100)
    return 1;
  current_quantum = q;
  return 0;
}

int scheduler_get_quantum(void) { return current_quantum; }

int scheduler_input_quantum(void) {
  printf("Current Round Robin time quantum: %d\n", current_quantum);
  int q;
  if (!read_int("Enter new time quantum (1-100): ", 1, 100, &q))
    return 1;
  current_quantum = q;
  printf("Round Robin time quantum set to %d.\n", current_quantum);
  return 0;
}

/* ---------- workload ---------- */

void scheduler_load_default_jobs(Job jobs[], int *n) {
  static const int data[5][4] = {
      {1, 0, 5, 2}, {2, 1, 3, 1}, {3, 2, 8, 4}, {4, 3, 6, 2}, {5, 4, 4, 2}};
  for (int i = 0; i < 5; i++) {
    jobs[i] = (Job){data[i][0], data[i][1], data[i][2], data[i][3], "READY"};
  }
  *n = 5;
}

/* Loads jobs from a text file: one job per line "arrival burst priority".
 * Blank lines and '#' comments are ignored. Replaces the current workload
 * only if the whole file is valid. */
int scheduler_load_file(Job jobs[], int *n, const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    printf("[Error] Cannot open \"%s\": %s\n", path, strerror(errno));
    return 1;
  }
  Job tmp[MAX_JOBS];
  int count = 0, lineno = 0;
  char line[256];
  while (fgets(line, sizeof(line), f)) {
    lineno++;
    char *hash = strchr(line, '#');
    if (hash)
      *hash = '\0';
    int a, b, p;
    char extra;
    int got = sscanf(line, " %d %d %d %c", &a, &b, &p, &extra);
    if (got <= 0)
      continue; /* blank or comment-only line */
    if (got != 3 || a < 0 || a > MAX_ARRIVAL || b < 1 || b > MAX_BURST ||
        p < 1 || p > 99) {
      printf("[Error] %s:%d: expected \"arrival(0-%d) burst(1-%d) "
             "priority(1-99)\"\n",
             path, lineno, MAX_ARRIVAL, MAX_BURST);
      fclose(f);
      return 1;
    }
    if (count >= MAX_JOBS) {
      printf("[Error] %s: more than %d jobs\n", path, MAX_JOBS);
      fclose(f);
      return 1;
    }
    tmp[count] = (Job){count + 1, a, b, p, "READY"};
    count++;
  }
  fclose(f);
  if (count == 0) {
    printf("[Error] %s contains no jobs\n", path);
    return 1;
  }
  memcpy(jobs, tmp, count * sizeof(Job));
  *n = count;
  printf("Loaded %d jobs from %s.\n", count, path);
  return 0;
}

/* Generates n random jobs. seed 0 = time-based, so every run differs. */
int scheduler_generate_random(Job jobs[], int *n, int count, unsigned seed) {
  if (count < 1 || count > MAX_JOBS) {
    printf("[Error] Job count must be 1-%d\n", MAX_JOBS);
    return 1;
  }
  if (seed == 0)
    seed = (unsigned)time(NULL) ^ (unsigned)getpid();
  srand(seed);
  for (int i = 0; i < count; i++)
    jobs[i] = (Job){i + 1, rand() % 10, 1 + rand() % 12, 1 + rand() % 5,
                    "READY"};
  *n = count;
  printf("Generated %d random jobs (seed %u - reuse it to reproduce).\n",
         count, seed);
  return 0;
}

int scheduler_input_jobs(Job jobs[], int *n) {
  Job tmp[MAX_JOBS];
  int count;
  char prompt[64];

  if (!read_int("Number of jobs (1-20): ", 1, MAX_JOBS, &count))
    return 1;
  for (int i = 0; i < count; i++) {
    printf("Job %d\n", i + 1);
    tmp[i].id = i + 1;
    snprintf(prompt, sizeof(prompt), "  Arrival time (0-%d): ", MAX_ARRIVAL);
    if (!read_int(prompt, 0, MAX_ARRIVAL, &tmp[i].arrival))
      return 1;
    snprintf(prompt, sizeof(prompt), "  CPU burst (1-%d): ", MAX_BURST);
    if (!read_int(prompt, 1, MAX_BURST, &tmp[i].burst))
      return 1;
    if (!read_int("  Priority (1-99, lower = higher priority): ", 1, 99,
                  &tmp[i].priority))
      return 1;
    strcpy(tmp[i].status, "READY");
  }
  memcpy(jobs, tmp, count * sizeof(Job));
  *n = count;
  printf("%d jobs registered.\n", count);
  return 0;
}

int scheduler_add_job(Job jobs[], int *n) {
  if (*n >= MAX_JOBS) {
    printf("Cannot add job: maximum capacity of %d jobs reached.\n", MAX_JOBS);
    return 1;
  }
  Job j;
  j.id = (*n > 0) ? (jobs[*n - 1].id + 1) : 1;
  for (int i = 0; i < *n; i++) {
    if (jobs[i].id >= j.id)
      j.id = jobs[i].id + 1;
  }

  printf("\nAdding Job %d:\n", j.id);
  char prompt[64];
  snprintf(prompt, sizeof(prompt), "  Arrival time (0-%d): ", MAX_ARRIVAL);
  if (!read_int(prompt, 0, MAX_ARRIVAL, &j.arrival))
    return 1;
  snprintf(prompt, sizeof(prompt), "  CPU burst (1-%d): ", MAX_BURST);
  if (!read_int(prompt, 1, MAX_BURST, &j.burst))
    return 1;
  if (!read_int("  Priority (1-99, lower = higher priority): ", 1, 99,
                &j.priority))
    return 1;
  strcpy(j.status, "READY");

  jobs[*n] = j;
  (*n)++;
  printf("Job %d added successfully (total workload: %d jobs).\n", j.id, *n);
  scheduler_display_jobs(jobs, *n);
  return 0;
}

int scheduler_edit_job_priority(Job jobs[], int n) {
  if (n <= 0) {
    printf("No jobs available. Load or input jobs first.\n");
    return 1;
  }
  scheduler_display_jobs(jobs, n);
  int target_id;
  if (!read_int("\nEnter Job ID to modify priority: ", 1, 1000, &target_id))
    return 1;
  int idx = -1;
  for (int i = 0; i < n; i++) {
    if (jobs[i].id == target_id) {
      idx = i;
      break;
    }
  }
  if (idx == -1) {
    printf("Job %d not found in current workload.\n", target_id);
    return 1;
  }
  printf("Current priority of Job %d is %d.\n", target_id, jobs[idx].priority);
  int new_pri;
  if (!read_int("Enter new priority (1-99, lower = higher priority): ", 1, 99,
                &new_pri))
    return 1;
  jobs[idx].priority = new_pri;
  printf("Priority of Job %d updated to %d.\n", target_id, new_pri);
  scheduler_display_jobs(jobs, n);
  return 0;
}

void scheduler_display_jobs(const Job jobs[], int n) {
  if (n <= 0) {
    printf("No jobs loaded.\n");
    return;
  }
  printf("\n| Job | Arrival | Burst | Priority | Status    |\n");
  for (int i = 0; i < n; i++) {
    printf("| %-3d | %-7d | %-5d | %-8d | %-9s |\n", jobs[i].id,
           jobs[i].arrival, jobs[i].burst, jobs[i].priority, jobs[i].status);
  }
}

/* ---------- simulation ---------- */

static void add_slice(Timeline *t, int id, int start, int end) {
  t->s[t->count++] = (Slice){id, start, end};
}

static void finish(Job *j, Metrics *m, int now) {
  strcpy(j->status, "COMPLETED");
  m->completion = now;
  m->turnaround = now - j->arrival;
  m->waiting = m->turnaround - j->burst;
  char buf[LOG_MSG_LEN];
  snprintf(buf, sizeof(buf), "[Scheduler] Job %d completed (CT=%d, TAT=%d, WT=%d)",
           j->id, m->completion, m->turnaround, m->waiting);
  logger_log(buf);
}

/* Earliest arrival among jobs whose skip[] flag is false. */
static int earliest_arrival(const Job jobs[], const bool skip[], int n) {
  int best = -1;
  for (int i = 0; i < n; i++) {
    if (!skip[i] && (best < 0 || jobs[i].arrival < best))
      best = jobs[i].arrival;
  }
  return best;
}

/* Tie-break: priority (Priority policy only), then earlier arrival, then lower
 * job ID. */
static bool better(const Job *a, const Job *b, Policy p) {
  if (p == PRIORITY && a->priority != b->priority)
    return a->priority < b->priority;
  if (a->arrival != b->arrival)
    return a->arrival < b->arrival;
  return a->id < b->id;
}

static int pick(const Job jobs[], const bool done[], int n, int now, Policy p) {
  int best = -1;
  for (int i = 0; i < n; i++) {
    if (done[i] || jobs[i].arrival > now)
      continue;
    if (best < 0 || better(&jobs[i], &jobs[best], p))
      best = i;
  }
  return best;
}

/* Simulates non-preemptive policies (FCFS and Priority) */
static void simulate_nonpreemptive(Job jobs[], int n, Policy p, Metrics m[],
                                   Timeline *t) {
  bool done[MAX_JOBS] = {false};
  int now = 0;

  for (int finished = 0; finished < n; finished++) {
    /* Step 1: Pick the highest priority/earliest arrival job that has already
     * arrived */
    int next = pick(jobs, done, n, now, p);

    /* Step 2: If CPU is idle (no job arrived yet), advance time to earliest
     * arrival */
    if (next < 0) {
      int wake = earliest_arrival(jobs, done, n);
      add_slice(t, 0, now, wake); /* IDLE slice */
      now = wake;
      next = pick(jobs, done, n, now, p);
    }

    /* Step 3: Run the job non-preemptively until completion */
    strcpy(jobs[next].status, "RUNNING");
    add_slice(t, jobs[next].id, now, now + jobs[next].burst);
    now += jobs[next].burst;
    done[next] = true;

    /* Step 4: Record Completion Time, TAT, and Waiting Time */
    finish(&jobs[next], &m[next], now);
  }
}

static void queue_push(Queue *q, int v) {
  q->items[q->tail] = v;
  q->tail = (q->tail + 1) % 128;
}

static int queue_pop(Queue *q) {
  int v = q->items[q->head];
  q->head = (q->head + 1) % 128;
  return v;
}

/* Admits newly arrived jobs into the ready queue ordered by arrival time, then
 * Job ID */
static void admit_arrivals(const Job jobs[], int n, int now, bool admitted[],
                           Queue *q) {
  while (1) {
    int best = -1;
    for (int i = 0; i < n; i++) {
      if (!admitted[i] && jobs[i].arrival <= now) {
        if (best < 0 || jobs[i].arrival < jobs[best].arrival ||
            (jobs[i].arrival == jobs[best].arrival &&
             jobs[i].id < jobs[best].id)) {
          best = i;
        }
      }
    }
    if (best < 0)
      break;
    admitted[best] = true;
    queue_push(q, best);
  }
}

/*
 * Round Robin Scheduling:
 * Preemptive scheduling with a fixed time quantum.
 * Rule: New arrivals join the queue before a preempted job is re-queued.
 */
static void simulate_rr(Job jobs[], int n, Metrics m[], Timeline *t) {
  int remaining[MAX_JOBS];
  bool admitted[MAX_JOBS] = {false};
  Queue q = {.head = 0, .tail = 0};
  int now = 0, finished = 0;

  for (int i = 0; i < n; i++)
    remaining[i] = jobs[i].burst;

  while (finished < n) {
    /* Step 1: Admit all jobs that have arrived by current time 'now' */
    admit_arrivals(jobs, n, now, admitted, &q);

    /* Step 2: If queue is empty, jump CPU to next job's arrival time */
    if (q.head == q.tail) {
      int wake = earliest_arrival(jobs, admitted, n);
      if (wake < 0)
        break;
      add_slice(t, 0, now, wake); /* IDLE slice */
      now = wake;
      continue;
    }

    /* Step 3: Pop next job from head of ready queue */
    int cur = queue_pop(&q);
    int run =
        remaining[cur] < current_quantum ? remaining[cur] : current_quantum;

    /* Step 4: Execute job for up to current_quantum time units */
    strcpy(jobs[cur].status, "RUNNING");
    add_slice(t, jobs[cur].id, now, now + run);
    now += run;
    remaining[cur] -= run;

    /* Step 5: Admit any newly arrived jobs during this time slice */
    admit_arrivals(jobs, n, now, admitted, &q);

    /* Step 6: If job still needs CPU, re-queue at tail; otherwise mark complete
     */
    if (remaining[cur] > 0) {
      queue_push(&q, cur);
    } else {
      finished++;
      finish(&jobs[cur], &m[cur], now);
    }
  }
}

static void simulate(Job jobs[], int n, Policy p, Metrics m[], Timeline *t) {
  for (int i = 0; i < n; i++)
    strcpy(jobs[i].status, "READY");
  t->count = 0;
  if (p == RR)
    simulate_rr(jobs, n, m, t);
  else
    simulate_nonpreemptive(jobs, n, p, m, t);
}

static void averages(const Metrics m[], int n, double *tat, double *wt) {
  int total_tat = 0, total_wt = 0;
  for (int i = 0; i < n; i++) {
    total_tat += m[i].turnaround;
    total_wt += m[i].waiting;
  }
  *tat = (double)total_tat / n;
  *wt = (double)total_wt / n;
}

/* ---------- reporting ---------- */

static void print_timeline(const Timeline *t) {
  printf("\nExecution order:\n");
  for (int i = 0; i < t->count; i++) {
    if (t->s[i].job_id == 0)
      printf("[IDLE %d-%d] ", t->s[i].start, t->s[i].end);
    else
      printf("[J%d %d-%d] ", t->s[i].job_id, t->s[i].start, t->s[i].end);
  }
  printf("\n");
}

static void print_table(const Job jobs[], const Metrics m[], int n) {
  double tat, wt;
  printf("\n| Job | Arrival | Burst | Priority | Completion | Turnaround | "
         "Waiting |\n");
  for (int i = 0; i < n; i++) {
    printf("| %-3d | %-7d | %-5d | %-8d | %-10d | %-10d | %-7d |\n", jobs[i].id,
           jobs[i].arrival, jobs[i].burst, jobs[i].priority, m[i].completion,
           m[i].turnaround, m[i].waiting);
  }
  averages(m, n, &tat, &wt);
  printf("Average turnaround time: %.2f\nAverage waiting time:    %.2f\n", tat,
         wt);
}

static int run_policy(Job jobs[], int n, Policy p) {
  Metrics m[MAX_JOBS];
  Timeline t;

  if (n <= 0) {
    printf("No jobs to schedule. Load or input jobs first.\n");
    return 1;
  }
  printf("\n=== %s scheduling ===\n", policy_names[p]);
  if (p == PRIORITY) {
    printf("Tie-break: lower number = higher priority; equal priority -> "
           "earlier arrival -> lower job ID.\n");
  } else if (p == RR) {
    printf("Time quantum = %d. New arrivals join the queue before a preempted "
           "job is re-queued.\n",
           current_quantum);
  } else {
    printf("Tie-break: equal arrival -> lower job ID.\n");
  }

  simulate(jobs, n, p, m, &t);
  print_timeline(&t);
  print_table(jobs, m, n);
  return 0;
}

int scheduler_run_fcfs(Job jobs[], int n) { return run_policy(jobs, n, FCFS); }
int scheduler_run_priority(Job jobs[], int n) {
  return run_policy(jobs, n, PRIORITY);
}
int scheduler_run_rr(Job jobs[], int n) { return run_policy(jobs, n, RR); }

int scheduler_compare_all(const Job jobs[], int n) {
  Job copy[MAX_JOBS];
  Metrics m[MAX_JOBS];
  Timeline t;
  double tat[3], wt[3];
  int best = 0;

  if (n <= 0) {
    printf("No jobs to compare. Load or input jobs first.\n");
    return 1;
  }
  printf("\n=== Policy comparison (same job set, RR quantum = %d) ===\n",
         current_quantum);
  printf("| Policy                    | Avg turnaround | Avg waiting |\n");
  for (int p = 0; p < 3; p++) {
    memcpy(copy, jobs, n * sizeof(Job));
    simulate(copy, n, (Policy)p, m, &t);
    averages(m, n, &tat[p], &wt[p]);
    printf("| %-25s | %-14.2f | %-11.2f |\n", policy_names[p], tat[p], wt[p]);
    if (wt[p] < wt[best])
      best = p;
  }
  printf("Lowest average waiting time: %s\n", policy_names[best]);
  return 0;
}

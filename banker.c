#include "banker.h"
#include "common.h"

typedef int Matrix[MAX_PROCESSES][MAX_RESOURCES];

typedef struct {
  int p, r;
  Matrix alloc, max;
  int avail[MAX_RESOURCES];
} State;

static const State SAFE_STATE = {
    .p = 5,
    .r = 3,
    .alloc = {{0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}},
    .max = {{7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}},
    .avail = {3, 3, 2},
};

static const State UNSAFE_STATE = {
    .p = 4,
    .r = 3,
    .alloc = {{3, 1, 2}, {2, 2, 1}, {2, 1, 1}, {1, 1, 1}},
    .max = {{5, 4, 3}, {4, 4, 3}, {6, 3, 3}, {3, 3, 2}},
    .avail = {0, 1, 0},
};

static void print_state(const State *s, Matrix need) {
  printf("\nProcess | Allocation | Maximum | Need (= Max - Alloc)\n");
  for (int i = 0; i < s->p; i++) {
    printf("  P%-4d | ", i);
    for (int j = 0; j < s->r; j++)
      printf("%d ", s->alloc[i][j]);
    printf("     | ");
    for (int j = 0; j < s->r; j++)
      printf("%d ", s->max[i][j]);
    printf("   | ");
    for (int j = 0; j < s->r; j++)
      printf("%d ", need[i][j]);
    printf("\n");
  }
  printf("Available: ");
  for (int j = 0; j < s->r; j++)
    printf("%d ", s->avail[j]);
  printf("\n\n");
}

/*
 * Banker's Safety Algorithm:
 * Checks whether the current resource allocation state is SAFE.
 * A state is safe if there exists a Safe Sequence <P0, P1, ... Pn-1>
 * where each process can finish by obtaining its maximum claim.
 */
static bool is_safe(const State *s, Matrix need, int seq[]) {
  int work[MAX_RESOURCES];
  bool finished[MAX_PROCESSES] = {false};
  int count = 0;

  /* Step 1: Initialize Work = Available vector */
  memcpy(work, s->avail, sizeof(work));

  /* Step 2: Repeat until all processes finish or no progress can be made */
  while (count < s->p) {
    bool progress = false;

    /* Find an unfinished process whose Need can be satisfied by current Work */
    for (int i = 0; i < s->p; i++) {
      if (finished[i])
        continue;

      bool fits = true;
      for (int j = 0; j < s->r; j++) {
        if (need[i][j] > work[j])
          fits = false;
      }
      if (!fits)
        continue;

      /* Step 3: Process finishes, reclaims its allocated resources back to Work
       */
      for (int j = 0; j < s->r; j++)
        work[j] += s->alloc[i][j];
      printf("  P%d can finish (Need <= Work); Work becomes [ ", i);
      for (int j = 0; j < s->r; j++)
        printf("%d ", work[j]);
      printf("]\n");

      finished[i] = true;
      seq[count++] = i;
      progress = true;
    }

    /* If no process could proceed in this pass, system cannot make progress */
    if (!progress)
      break;
  }

  /* Step 4: If not all processes finished, report blocked processes (deadlock)
   */
  if (count < s->p) {
    printf("  No remaining process has Need <= Work. Blocked: ");
    for (int i = 0; i < s->p; i++) {
      if (!finished[i])
        printf("P%d ", i);
    }
    printf("\n");
  }

  /* Safe if and only if all processes finished */
  return count == s->p;
}

static bool analyse(const State *s) {
  Matrix need;
  int seq[MAX_PROCESSES];

  /* Step 1: Calculate Need Matrix = Maximum - Allocation */
  for (int i = 0; i < s->p; i++) {
    for (int j = 0; j < s->r; j++)
      need[i][j] = s->max[i][j] - s->alloc[i][j];
  }
  print_state(s, need);

  /* Step 2: Execute Safety Algorithm */
  printf("Safety check:\n");
  bool safe = is_safe(s, need, seq);

  /* Step 3: Display Result and Safe Sequence (if safe) */
  if (safe) {
    printf("\nRESULT: SAFE state. Safe sequence: <");
    for (int i = 0; i < s->p; i++)
      printf(" P%d", seq[i]);
    printf(" >\n");
  } else {
    printf("\nRESULT: UNSAFE state. No safe sequence exists; deadlock is "
           "possible.\n");
  }
  return safe;
}

int banker_demo_safe(void) {
  printf("\n=== Banker's algorithm: safe-state test case ===\n");
  return analyse(&SAFE_STATE) ? 0 : 1;
}

int banker_demo_unsafe(void) {
  printf("\n=== Banker's algorithm: unsafe-state test case ===\n");
  return analyse(&UNSAFE_STATE) ? 1 : 0;
}

int banker_input_and_check(void) {
  State s;
  char prompt[64];

  memset(&s, 0, sizeof(s));
  printf("\n=== Banker's algorithm: custom input ===\n");
  if (!read_int("Number of processes (1-10): ", 1, MAX_PROCESSES, &s.p))
    return 1;
  if (!read_int("Number of resource types (1-10): ", 1, MAX_RESOURCES, &s.r))
    return 1;

  for (int i = 0; i < s.p; i++) {
    for (int j = 0; j < s.r; j++) {
      snprintf(prompt, sizeof(prompt), "Allocation[P%d][R%d]: ", i, j);
      if (!read_int(prompt, 0, 100, &s.alloc[i][j]))
        return 1;
    }
  }
  for (int i = 0; i < s.p; i++) {
    for (int j = 0; j < s.r; j++) {
      snprintf(prompt, sizeof(prompt), "Maximum[P%d][R%d] (>= %d): ", i, j,
               s.alloc[i][j]);
      if (!read_int(prompt, s.alloc[i][j], 100, &s.max[i][j]))
        return 1;
    }
  }
  for (int j = 0; j < s.r; j++) {
    snprintf(prompt, sizeof(prompt), "Available[R%d]: ", j);
    if (!read_int(prompt, 0, 100, &s.avail[j]))
      return 1;
  }
  analyse(&s);
  return 0;
}

/* Reads whitespace-separated integers, skipping '#' comment lines. */
static bool next_int(FILE *f, int *out) {
  int c;
  while ((c = fgetc(f)) != EOF) {
    if (c == '#') {
      while ((c = fgetc(f)) != EOF && c != '\n')
        ;
    } else if (c == '-' || (c >= '0' && c <= '9')) {
      ungetc(c, f);
      return fscanf(f, "%d", out) == 1;
    }
  }
  return false;
}

/* File format: P R, then P rows of Allocation, P rows of Max, then Available.
 */
int banker_from_file(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    printf("[Error] Cannot open \"%s\": %s\n", path, strerror(errno));
    return 1;
  }
  State s;
  memset(&s, 0, sizeof(s));
  bool ok = next_int(f, &s.p) && next_int(f, &s.r) && s.p >= 1 &&
            s.p <= MAX_PROCESSES && s.r >= 1 && s.r <= MAX_RESOURCES;
  for (int i = 0; ok && i < s.p; i++)
    for (int j = 0; ok && j < s.r; j++)
      ok = next_int(f, &s.alloc[i][j]) && s.alloc[i][j] >= 0;
  for (int i = 0; ok && i < s.p; i++)
    for (int j = 0; ok && j < s.r; j++)
      ok = next_int(f, &s.max[i][j]) && s.max[i][j] >= s.alloc[i][j];
  for (int j = 0; ok && j < s.r; j++)
    ok = next_int(f, &s.avail[j]) && s.avail[j] >= 0;
  fclose(f);
  if (!ok) {
    printf("[Error] %s: malformed state (need P R, Alloc, Max, Available; "
           "Max >= Alloc)\n",
           path);
    return 1;
  }
  printf("\n=== Banker's algorithm: state loaded from %s ===\n", path);
  analyse(&s);
  return 0;
}

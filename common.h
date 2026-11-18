#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_SRN "PES2UG25CS606"
#define PARTNER_SRN "PES2UG25CS615"
#define MAX_JOBS 20
#define MAX_ARRIVAL 1000
#define MAX_BURST 100
#define TIME_QUANTUM 2
#define MAX_RESOURCES 10
#define MAX_PROCESSES 10
#define LOG_QUEUE_SIZE 8
#define LOG_MSG_LEN 256
#define LOG_FILE_NAME "campuscompute.log"

/* Returns SRN from environment variable if set (export SRN=...), otherwise
 * DEFAULT_SRN */
static inline const char *get_srn(void) {
  const char *env = getenv("SRN");
  return (env && *env) ? env : DEFAULT_SRN;
}

typedef struct {
  int id;
  int arrival;
  int burst;
  int priority;    /* lower number = higher priority */
  char status[16]; /* READY -> RUNNING -> COMPLETED */
} Job;

typedef struct {
  int completion;
  int turnaround; /* completion - arrival */
  int waiting;    /* turnaround - burst */
} Metrics;

/* Prompts until the user enters an integer in [lo, hi]. Returns false on EOF.
 */
bool read_int(const char *prompt, int lo, int hi, int *out);

/* A child inherits unflushed stdio buffers, so flush before fork and before
 * _exit. */
static inline pid_t fork_flushed(void) {
  fflush(stdout);
  return fork();
}

static inline void child_exit(int code) {
  fflush(stdout);
  _exit(code);
}

#endif

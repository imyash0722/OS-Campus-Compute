#include "logger.h"
#include "common.h"
#include <pthread.h>
#include <time.h>

typedef struct {
  char text[LOG_MSG_LEN];
  char time[20];
  bool stop;
} LogEvent;

static LogEvent queue[LOG_QUEUE_SIZE];
static int head, tail, count;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

static pthread_t consumer;
static FILE *log_file;
static bool active;
static int written;

/* Producer pushes an event into the bounded buffer */
static void push(const LogEvent *ev) {
  /* Step 1: Acquire mutex lock before touching shared queue */
  pthread_mutex_lock(&lock);

  /* Step 2: If queue is full, wait on not_full (releases mutex while sleeping)
   */
  while (count == LOG_QUEUE_SIZE) {
    printf("  [Producer] queue full (%d/%d), waiting for consumer\n", count,
           LOG_QUEUE_SIZE);
    pthread_cond_wait(&not_full, &lock);
  }

  /* Step 3: Insert item into circular buffer and increment count */
  queue[tail] = *ev;
  tail = (tail + 1) % LOG_QUEUE_SIZE;
  count++;

  /* Step 4: Signal consumer that queue is not empty */
  pthread_cond_signal(&not_empty);

  /* Step 5: Release mutex lock */
  pthread_mutex_unlock(&lock);
}

/* Consumer pops an event from the bounded buffer */
static LogEvent pop(void) {
  /* Step 1: Acquire mutex lock */
  pthread_mutex_lock(&lock);

  /* Step 2: If queue is empty, wait on not_empty */
  while (count == 0)
    pthread_cond_wait(&not_empty, &lock);

  /* Step 3: Remove item from circular buffer and decrement count */
  LogEvent ev = queue[head];
  head = (head + 1) % LOG_QUEUE_SIZE;
  count--;

  /* Step 4: Signal waiting producers that space is now available */
  pthread_cond_signal(&not_full);

  /* Step 5: Release mutex lock */
  pthread_mutex_unlock(&lock);
  return ev;
}

static void *consume(void *arg) {
  (void)arg;
  while (1) {
    LogEvent ev = pop();
    if (ev.stop)
      break;
    printf("  [Consumer] [%s] %s\n", ev.time, ev.text);
    fprintf(log_file, "[%s] %s\n", ev.time, ev.text);
    fflush(log_file);
    written++;
    usleep(20000); /* slow consumer, so the bounded queue can fill up */
  }
  return NULL;
}

int logger_init(void) {
  if (active)
    return 0;
  log_file = fopen(LOG_FILE_NAME, "w");
  if (!log_file) {
    perror("fopen log file");
    return -1;
  }
  head = tail = count = written = 0;
  int err = pthread_create(&consumer, NULL, consume, NULL);
  if (err != 0) {
    fprintf(stderr, "pthread_create: %s\n", strerror(err));
    fclose(log_file);
    return -1;
  }
  active = true;
  return 0;
}

void logger_log(const char *message) {
  LogEvent ev = {.stop = false};
  time_t now = time(NULL);
  struct tm tm_now;

  if (!active)
    return;
  localtime_r(&now, &tm_now);
  strftime(ev.time, sizeof(ev.time), "%Y-%m-%d %H:%M:%S", &tm_now);
  snprintf(ev.text, sizeof(ev.text), "%s", message);
  push(&ev);
}

int logger_shutdown(void) {
  if (!active)
    return written;
  LogEvent stop = {.stop = true};
  push(&stop);
  pthread_join(consumer, NULL);
  fclose(log_file);
  log_file = NULL;
  active = false;
  return written;
}

typedef struct {
  int id;
  int events;
} ProducerArg;

static void *produce(void *arg) {
  ProducerArg *pa = (ProducerArg *)arg;
  char msg[LOG_MSG_LEN];
  for (int i = 1; i <= pa->events; i++) {
    snprintf(msg, sizeof(msg), "Producer-%d: job event #%d", pa->id, i);
    logger_log(msg);
  }
  return NULL;
}

static int run_producer_consumer(int num_producers, int events_per_producer) {
  pthread_t threads[16];
  ProducerArg args[16];
  int started = 0;

  printf("\n=== Producer-consumer logger (%d producers, %d events each, queue size %d) ===\n",
         num_producers, events_per_producer, LOG_QUEUE_SIZE);
  if (logger_init() != 0)
    return 1;

  logger_log("Logger started");
  for (int i = 0; i < num_producers; i++) {
    args[i].id = i + 1;
    args[i].events = events_per_producer;
    if (pthread_create(&threads[i], NULL, produce, &args[i]) != 0)
      break;
    started++;
  }
  for (int i = 0; i < started; i++)
    pthread_join(threads[i], NULL);
  logger_log("All producers finished");

  int expected = 2 + started * events_per_producer;
  int written_events = logger_shutdown();
  printf("Producers joined, consumer joined. Events produced: %d, written to "
         "%s: %d\n",
         expected, LOG_FILE_NAME, written_events);
  return (started == num_producers && written_events == expected) ? 0 : 1;
}

int logger_demo(void) {
  return run_producer_consumer(3, 5);
}

int logger_custom_run(void) {
  int p = 3, e = 5;
  printf("\n=== Custom Multithreaded Activity Logger ===\n");
  if (!read_int("Enter number of producer threads (1-10): ", 1, 10, &p))
    return 1;
  if (!read_int("Enter events per producer thread (1-20): ", 1, 20, &e))
    return 1;
  return run_producer_consumer(p, e);
}

int logger_submit_event(void) {
  char buf[LOG_MSG_LEN];
  printf("\n=== Submit Custom Log Event to Activity Logger ===\n");
  printf("Enter log message to enqueue: ");
  if (!fgets(buf, sizeof(buf), stdin))
    return 1;
  buf[strcspn(buf, "\r\n")] = '\0';
  if (buf[0] == '\0') {
    printf("[Logger] Empty message, skipped.\n");
    return 1;
  }
  bool was_active = active;
  if (!was_active && logger_init() != 0)
    return 1;
  printf("Pushing event to bounded queue: \"%s\"\n", buf);
  logger_log(buf);
  usleep(40000); /* allow consumer thread to dequeue and print */
  if (!was_active) {
    logger_shutdown();
    printf("Log event committed to %s.\n", LOG_FILE_NAME);
  }
  return 0;
}

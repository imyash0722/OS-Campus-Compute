#ifndef WORKER_H
#define WORKER_H

/* Each returns 0 when the parent observed the expected outcome, 1 otherwise. */
int worker_demo_normal(void);
int worker_demo_failure(void);
int worker_custom(void);

#endif

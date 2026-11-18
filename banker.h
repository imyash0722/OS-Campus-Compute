#ifndef BANKER_H
#define BANKER_H

/* Each returns 0 when the result matches the expectation, 1 otherwise. */
int banker_demo_safe(void);
int banker_demo_unsafe(void);
int banker_input_and_check(void);
int banker_from_file(const char *path);

#endif

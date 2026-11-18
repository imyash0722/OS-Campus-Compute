#ifndef RLIMIT_H
#define RLIMIT_H

int rlimit_demo_cpu(
    void); /* returns 0 if the kernel stopped the child with SIGXCPU */

#endif

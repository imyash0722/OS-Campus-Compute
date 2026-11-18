# CampusCompute — Demo Walkthrough & File Guide

One file to demo from and to explain every source file.

## 1. Setup (10 seconds)
```bash
make clean && make          # builds ./campuscompute with -Wall -Wextra, no warnings
export SRN=PES2UG25CS606    # banner/prompt shows this SRN (optional; has a default)
```

## 2. Demo script (about 5 minutes, in this order)

| Step | Command | What to say | Concept |
|---|---|---|---|
| 1 | `make demo` | "Every module runs and asserts its own result; the summary is real PASS/FAIL, ending with a cleanup check." | integration |
| 2 | `./campuscompute --jobs samples/jobs_lab.txt --compare` | "Workload comes from a file, nothing hardcoded. Compare shows avg turnaround and waiting for each policy." | FR1, FR2 |
| 3 | `./campuscompute --jobs samples/jobs_lab.txt --rr 3` | "Quantum is configurable; see the Gantt chart change." Verify one row by hand on paper. | Round Robin |
| 4 | `SEED=7 ./campuscompute --random 8 --compare` | "Random workload; seed printed so it's reproducible." | FR2 |
| 5 | `./campuscompute --banker-file samples/banker_unsafe.txt` | Show UNSAFE. Edit `Available` in the file to `1 2`, rerun, and it becomes SAFE with a sequence. | FR5 |
| 6 | `./campuscompute --logger` | "Producers and a consumer share a bounded queue; mutex plus condition variables stop overflow/underflow." Show `campuscompute.log`. | FR3 |
| 7 | `./campuscompute --ipc` or `--ipc-custom` | "Parent and child exchange a packet through shared memory; semaphores order the access." | FR4 |
| 8 | `./campuscompute --worker-ok` / `--worker-fail` / `--worker-custom` | "fork + waitpid; the parent tells clean exit from failure by status." | FR6 |
| 9 | `./campuscompute --rlimit` (run `top` in another terminal) | "The child spins until the kernel sends SIGXCPU at the 1s CPU limit." | FR7 |
| 10 | `./campuscompute --cleanup` | "No threads, children, shared memory or semaphores left." Optionally `ls /dev/shm`. | FR7 |

Interactive alternative: `./campuscompute`, then type a menu number (`?` shows the menu).
Let the examiner use `input`, `add-job`, `edit-priority`, `banker-input`, `logger-msg` and `worker-custom` themselves.

## 3. What each file does

| File | Role | Key things to point at |
|---|---|---|
| `common.h` | Shared constants, `Job`/`Metrics` structs, SRN lookup, `fork_flushed()`/`child_exit()` helpers | Limits (`MAX_JOBS`, `LOG_QUEUE_SIZE`); flushing stdio before fork avoids duplicated output |
| `input.c` | `read_int()`: validated integer input with range checks and buffer flushing | Rejects letters/out-of-range and re-prompts |
| `scheduler.h/.c` | Job intake (default, file, random, interactive), FCFS, Priority, Round Robin, Gantt chart, turnaround/waiting metrics, compare | Waiting = turnaround − burst; turnaround = completion − arrival; idle slices handled |
| `logger.h/.c` | Producer-consumer logger: circular buffer, producer threads, one consumer writing `campuscompute.log` | `pthread_mutex_t` for mutual exclusion; `not_full`/`not_empty` condition variables |
| `ipc.h/.c` | Shared memory (`shm_open`+`ftruncate`+`mmap`) and named semaphores between parent and child | `sem_timedwait` (5s) prevents hanging forever; `shm_unlink`/`sem_unlink` on cleanup; `ipc_objects_remaining()` verifies |
| `banker.h/.c` | Need = Max − Alloc, safety algorithm, safe sequence, built-in safe/unsafe states, interactive and file input | `is_safe()`: Work = Available, find a process with Need ≤ Work, release its allocation |
| `worker.h/.c` | `fork()` a worker, `waitpid()` it, report exit code vs. simulated failure | `WIFEXITED`/`WEXITSTATUS`; failure is a deliberate exit(42), not a real fault |
| `rlimit.h/.c` | `setrlimit(RLIMIT_CPU)` on a child that busy-loops; parent detects `SIGXCPU` | `WIFSIGNALED`/`WTERMSIG` |
| `main.c` | Command table (flag, label, function, in-demo), interactive menu, CLI flag parsing and chaining, `--demo`, `--cleanup` | One table drives menu, flags, and demo |
| `Makefile` | `make`, `make demo`, `make run`, `make zip`, `make clean` | Flags `-Wall -Wextra -std=c99 -pthread`, links `-lrt` |
| `samples/jobs_lab.txt` | Example workload file | `arrival burst priority`, `#` comments |
| `samples/banker_unsafe.txt` | Example Banker state (unsafe) | `P R`, Allocation, Max, Available |
| `README.md` | Full documentation, architecture, syscall table, screenshot checklist | |
| `VIVA_GUIDE.md` | Top viva Q&A | |
| `PROJECT_STUDY_GUIDE.md` | Concept study guide | |

## 4. Likely questions
- **Is anything hardcoded?** Only defaults. Jobs and Banker states come from a file, `--random`, or interactive input; results are computed at run time.
- **Why flush before fork?** The child inherits unflushed stdio buffers, so output would print twice.
- **Why `sem_timedwait`?** If the child dies, a plain `sem_wait` would block forever.
- **Why is Round Robin worse on average here?** Context slicing delays completion of long jobs; it favours response time, not turnaround.

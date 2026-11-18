# CampusCompute — Comprehensive Viva Voce & Demonstration Guide
**Operating Systems (UE25CS242A) — Level 3 Mini-Project**

---

## Table of Contents
1. [Project Overview & Architecture](#1-project-overview--architecture)
2. [Source Code & System Calls Reference](#2-source-code--system-calls-reference)
3. [CPU Scheduling In-Depth (FR1, FR2)](#3-cpu-scheduling-in-depth-fr1-fr2)
4. [Concurrent Activity Logger & Synchronization (FR3)](#4-concurrent-activity-logger--synchronization-fr3)
5. [Inter-Process Communication & POSIX Semaphores (FR4)](#5-inter-process-communication--posix-semaphores-fr4)
6. [Banker's Algorithm & Resource Safety (FR5)](#6-bankers-algorithm--resource-safety-fr5)
7. [Worker Failure Detection & Process Lifecycle (FR6)](#7-worker-failure-detection--process-lifecycle-fr6)
8. [Resource Limits & System Cleanup (FR7, FR8)](#8-resource-limits--system-cleanup-fr7-fr8)
9. [Live Coding & Practical Demonstration Cheat Sheet](#9-live-coding--practical-demonstration-cheat-sheet)
10. [Top 25 Viva Voce Questions & Model Answers](#10-top-25-viva-voce-questions--model-answers)

---

## 1. Project Overview & Architecture

### Problem Statement
A university computing laboratory operates high-performance computing nodes shared by multiple student and faculty research jobs. Jobs compete for CPU time and shared hardware resources. CampusCompute is a modular C application for Linux that provides:
- Multi-policy CPU scheduling (FCFS, Round Robin, Non-preemptive Priority).
- Thread-safe activity logging using a bounded producer-consumer queue.
- Inter-process communication using POSIX shared memory and named semaphores.
- Deadlock-avoidance safety analysis via Banker's algorithm.
- Robust worker process monitoring with controlled failure detection via `waitpid()`.
- Kernel-enforced resource limits via `setrlimit(RLIMIT_CPU)`.

### Architecture Diagram

```
                              +---------------------------------------+
                              |         main.c (CLI / Menu)           |
                              |  - Dispatches CLI flags & user input  |
                              |  - Runs integrated demo & cleanup     |
                              +-------------------+-------------------+
                                                  |
         +--------------------+-------------------+--------------------+--------------------+
         |                    |                   |                    |                    |
         v                    v                   v                    v                    v
  +--------------+    +---------------+   +---------------+    +---------------+    +---------------+
  | scheduler.c  |    |   logger.c    |   |     ipc.c     |    |   banker.c    |    |   worker.c    |
  | - FCFS       |    | - POSIX Pthr  |   | - shm_open    |    | - Safety Algo |    | - fork()      |
  | - Round Robin|    | - Mutex Lock  |   | - mmap        |    | - Need Matrix |    | - waitpid()   |
  | - Priority   |    | - Cond Vars   |   | - Named Sems  |    | - Safe /      |    | - Status      |
  | - Comparison |    | - Circular Q  |   | - IPC Cleanup |    |   Unsafe Seq  |    |   Inspection  |
  +--------------+    +---------------+   +---------------+    +---------------+    +---------------+
                                                                                            |
                                                                                    +---------------+
                                                                                    |   rlimit.c    |
                                                                                    | - setrlimit() |
                                                                                    | - SIGXCPU     |
                                                                                    +---------------+
```

---

## 2. Source Code & System Calls Reference

| System Call / POSIX API | Header | Purpose in CampusCompute | Why Chosen |
|---|---|---|---|
| `fork()` | `<unistd.h>` | Creates a child worker process in `worker.c`, `ipc.c`, and `rlimit.c`. | Standard UNIX process creation mechanism. |
| `waitpid(pid, &status, options)` | `<sys/wait.h>` | Suspends parent until child changes state; retrieves exit status code or termination signal. | Allows targeted waiting on specific child and non-blocking checks (`WNOHANG`). |
| `setrlimit(resource, &limit)` | `<sys/resource.h>` | Imposes kernel CPU limit (soft=1s, hard=2s) in `rlimit.c`. | Demonstrates real kernel enforcement of CPU consumption. |
| `shm_open(name, oflag, mode)` | `<sys/mman.h>`, `<fcntl.h>` | Creates POSIX shared memory object in `/dev/shm`. | High-throughput zero-copy IPC between processes. |
| `ftruncate(fd, length)` | `<unistd.h>` | Sets size of shared memory region to `sizeof(Packet)`. | Newly created shm has size 0; must be sized before mapping. |
| `mmap(addr, len, prot, flags, fd, off)` | `<sys/mman.h>` | Maps shared memory file descriptor into process address space. | Allows direct pointer access (`pkt->job_id`) across processes. |
| `munmap(addr, len)` | `<sys/mman.h>` | Unmaps shared memory page from virtual address space. | Prevents virtual memory leaks on cleanup. |
| `shm_unlink(name)` | `<sys/mman.h>` | Removes shared memory object name from `/dev/shm`. | Ensures no persistent IPC objects leak into the OS. |
| `sem_open(name, oflag, mode, value)` | `<semaphore.h>` | Creates or opens named POSIX semaphores (`ready`, `done`). | Named semaphores allow process synchronization across address spaces. |
| `sem_timedwait(sem, &ts)` | `<semaphore.h>` | Decrements (locks) semaphore with an absolute timeout (5s). | Prevents coordinator deadlock if child crashes or hangs. |
| `sem_post(sem)` | `<semaphore.h>` | Increments (unlocks) semaphore, waking up blocked waiting processes. | Signals completion of write or read operations. |
| `sem_close(sem)` | `<semaphore.h>` | Closes semaphore descriptor in calling process. | Releases process-level semaphore handle. |
| `sem_unlink(name)` | `<semaphore.h>` | Removes named semaphore from system. | Guarantees complete cleanup. |
| `pthread_create(...)` | `<pthread.h>` | Spawns producer and consumer threads in `logger.c`. | Standard POSIX multithreading within single process memory space. |
| `pthread_join(thread, ...)` | `<pthread.h>` | Blocks until specified thread terminates. | Guarantees all logs are produced and consumed before shutdown. |
| `pthread_mutex_lock/unlock` | `<pthread.h>` | Protects bounded buffer `queue[]` critical section. | Enforces Mutual Exclusion across concurrent threads. |
| `pthread_cond_wait(&cv, &mutex)` | `<pthread.h>` | Atomically unlocks mutex and sleeps waiting on condition. | Avoids busy-waiting when queue is full or empty. |
| `pthread_cond_signal(&cv)` | `<pthread.h>` | Wakes up at least one thread blocked on condition variable. | Notifies consumer when data arrives; notifies producer when space opens. |

---

## 3. CPU Scheduling In-Depth (FR1, FR2)

### Job Attributes (`struct Job`)
- `id`: Unique integer ($1, 2, 3, \dots$).
- `arrival`: Time instant at which the job enters the ready queue ($0 \le \text{arrival} \le 1000$).
- `burst`: Total CPU execution time required ($1 \le \text{burst} \le 100$).
- `priority`: Priority level ($1 \le \text{priority} \le 99$, where lower number = higher priority).
- `status`: String state (`READY` $\rightarrow$ `RUNNING` $\rightarrow$ `COMPLETED`).

### Key Formulas
1. **Completion Time ($CT$)**: Timestamp when the job finishes its final CPU burst.
2. **Turnaround Time ($TAT$)**: Total elapsed time from arrival to completion.
   $$TAT = CT - AT$$
3. **Waiting Time ($WT$)**: Total time the job spent waiting in the ready queue without CPU execution.
   $$WT = TAT - BT$$
4. **Average Metrics**:
   $$\text{Avg } TAT = \frac{\sum_{i=1}^n TAT_i}{n}, \quad \text{Avg } WT = \frac{\sum_{i=1}^n WT_i}{n}$$

### Algorithms & Tie-Breaking Rules

#### 1. First Come First Served (FCFS)
- **Nature**: Non-preemptive.
- **Rule**: Jobs run in ascending order of arrival time ($AT$).
- **Tie-breaker**: If two jobs arrive at the exact same time, the one with lower Job ID runs first.

#### 2. Non-preemptive Priority Scheduling
- **Nature**: Non-preemptive. Once a job is allocated the CPU, it runs to completion.
- **Rule**: At any scheduling point, select the ready job with the lowest priority number.
- **Tie-breaker**:
  1. Priority value (lower number wins).
  2. If priorities are equal: Earlier arrival time wins.
  3. If arrival times are equal: Lower Job ID wins.

#### 3. Round Robin (RR)
- **Nature**: Preemptive.
- **Time Quantum ($q$)**: Configurable (default: $q = 2$).
- **Rule**: The CPU is allocated to the head of the ready queue for up to $q$ time units. If the burst exceeds $q$, the job is preempted and placed at the tail of the queue.
- **Crucial Queue Rule**: When a job's quantum expires at time $t$, **new arrivals occurring at or before time $t$ are admitted into the queue before the preempted job is re-queued**.
- **Tie-breaker for simultaneous arrivals**: Jobs arriving at the same time are queued in order of earlier arrival time, then lower Job ID.

### Benchmark Workload Execution Analysis (Default 5 Jobs)
Default Workload:
- J1: $AT=0, BT=5, Pri=2$
- J2: $AT=1, BT=3, Pri=1$
- J3: $AT=2, BT=8, Pri=4$
- J4: $AT=3, BT=6, Pri=2$
- J5: $AT=4, BT=4, Pri=2$

**Comparison Results**:
- **FCFS**: Execution order: `J1 -> J2 -> J3 -> J4 -> J5`. Avg $TAT = 13.40$, Avg $WT = 8.20$.
- **Priority**: J1 starts at 0 (only job available). At $t=5$, J2, J3, J4, J5 are ready. J2 ($Pri=1$) runs. At $t=8$, J4 ($Pri=2, AT=3$) runs ahead of J5 ($AT=4$). Then J5 runs. Finally J3 ($Pri=4$) runs.
  Execution order: `J1 -> J2 -> J4 -> J5 -> J3`. Avg $TAT = 12.20$, Avg $WT = 7.00$ (Lowest WT!).
- **Round Robin ($q=2$)**: Interleaves slices: `[J1 0-2] [J2 2-4] [J3 4-6] [J1 6-8] ...`. High responsiveness for short jobs (J2 completes at $t=13$), but higher context switching overhead: Avg $TAT = 17.80$, Avg $WT = 12.60$.
- **Round Robin ($q=4$)**: Larger quantum allows J2 ($BT=3$) and J5 ($BT=4$) to complete faster without extra preemptions, dropping Avg $TAT$ to $17.20$ and Avg $WT$ to $12.00$.

---

## 4. Concurrent Activity Logger & Synchronization (FR3)

### The Classical Bounded Buffer Problem
- **Shared resource**: Circular array `queue[LOG_QUEUE_SIZE]` (size 8).
- **Producers**: 3 worker threads producing 5 job log events each (total 15 events + 2 system events = 17 events).
- **Consumer**: 1 logger thread consuming events from the queue, printing them, and writing them to `campuscompute.log`.

### Synchronization Primitives
1. `pthread_mutex_t lock`: Ensures that only one thread accesses `queue`, `head`, `tail`, and `count` at any instant.
2. `pthread_cond_t not_full`:
   - Producers check `while (count == LOG_QUEUE_SIZE)`.
   - If full, producer prints `[Producer] queue full (8/8), waiting for consumer` and calls `pthread_cond_wait(&not_full, &lock)`.
   - Consumer signals `pthread_cond_signal(&not_full)` after popping an element.
3. `pthread_cond_t not_empty`:
   - Consumer checks `while (count == 0)`.
   - If empty, consumer calls `pthread_cond_wait(&not_empty, &lock)`.
   - Producers signal `pthread_cond_signal(&not_empty)` after pushing an element.

### Why `while` loop instead of `if` for condition variables?
To handle **spurious wakeups** and race conditions. A thread may wake up from `pthread_cond_wait` even if no signal was sent, or another thread might claim the slot first before the awakened thread re-acquires the mutex. The `while` loop re-evaluates the predicate upon wakeup.

### Orderly Termination
1. Main thread calls `pthread_join()` on all 3 producer threads.
2. Main thread calls `logger_shutdown()`, which pushes a sentinel stop event (`ev.stop = true`).
3. Consumer thread pops `ev.stop`, breaks out of the loop, flushes `campuscompute.log`, and exits.
4. Main thread joins the consumer thread with `pthread_join(consumer, NULL)`.
5. Guarantees 0 memory leaks, 0 thread leaks, and complete file flushing.

---

## 5. Inter-Process Communication & POSIX Semaphores (FR4)

### Mechanism: Shared Memory (`shm_open` + `mmap`)
- File descriptor opened in `/dev/shm` via `shm_open("/cc_shm_<SRN>", O_CREAT | O_EXCL | O_RDWR, 0600)`.
- Sized with `ftruncate(fd, sizeof(Packet))`.
- Mapped with `mmap(NULL, sizeof(Packet), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)`.
- Child inherits mapped region across `fork()`.

### Synchronization: Dual Named Semaphores
- `sem_ready` (initial value 0): Signaled by parent when job data is written.
- `sem_done` (initial value 0): Signaled by child when job computation is finished and response written.

### Workflow
1. Parent writes job details (`job_id=101`, `name="Fluid_Dynamics_Sim"`, `burst=5`) into `pkt`.
2. Parent calls `sem_post(ready)`.
3. Child, waiting on `sem_timedwait(ready, &timeout)`, wakes up and reads data from shared memory.
4. Child writes response string `"job 101 executed by child <pid>"` into `pkt->response`.
5. Child calls `sem_post(done)` and exits with status 0.
6. Parent wakes from `sem_timedwait(done, &timeout)`, reads the response, and reaps child with `waitpid()`.
7. Cleanup: `munmap()`, `close()`, `shm_unlink()`, `sem_close()`, and `sem_unlink()`.
8. `ipc_objects_remaining()` verifies 0 leftovers in `/dev/shm`.

---

## 6. Banker's Algorithm & Resource Safety (FR5)

### Mathematical Foundation
- $n$ processes ($P_0, \dots, P_{n-1}$), $m$ resource types ($R_0, \dots, R_{m-1}$).
- **Available**: Vector of length $m$. Available instances of each resource.
- **Max**: $n \times m$ matrix. Maximum demand of each process.
- **Allocation**: $n \times m$ matrix. Currently allocated resources to each process.
- **Need**: $n \times m$ matrix. Remaining resources required:
  $$\text{Need}[i][j] = \text{Max}[i][j] - \text{Allocation}[i][j]$$

### Safety Algorithm Steps
1. Let $\text{Work} = \text{Available}$ (length $m$).
2. Let $\text{Finish}[i] = \text{false}$ for $i = 0, 1, \dots, n-1$.
3. Find an index $i$ such that:
   - $\text{Finish}[i] == \text{false}$
   - $\text{Need}[i][j] \le \text{Work}[j]$ for all $j \in \{0, \dots, m-1\}$.
4. If such an $i$ exists:
   - $\text{Work} = \text{Work} + \text{Allocation}[i]$
   - $\text{Finish}[i] = \text{true}$
   - Append $P_i$ to Safe Sequence.
   - Repeat from Step 3.
5. If all $\text{Finish}[i] == \text{true}$, system is in a **SAFE state**.
   If any $\text{Finish}[i] == \text{false}$, system is in an **UNSAFE state** (deadlock possible).

### Demonstration Test Cases

#### Safe State Test Case
- 5 processes, 3 resource types (Total instances = $[10, 5, 7]$).
- $\text{Available} = [3, 3, 2]$.
- Trace:
  - $P_1$ finishes ($\text{Need} [1, 2, 2] \le [3, 3, 2]$) $\rightarrow \text{Work}$ becomes $[5, 3, 2]$.
  - $P_3$ finishes ($\text{Need} [0, 1, 1] \le [5, 3, 2]$) $\rightarrow \text{Work}$ becomes $[7, 4, 3]$.
  - $P_4$ finishes ($\text{Need} [4, 3, 1] \le [7, 4, 3]$) $\rightarrow \text{Work}$ becomes $[7, 4, 5]$.
  - $P_0$ finishes ($\text{Need} [7, 4, 3] \le [7, 4, 5]$) $\rightarrow \text{Work}$ becomes $[7, 5, 5]$.
  - $P_2$ finishes ($\text{Need} [6, 0, 0] \le [7, 5, 5]$) $\rightarrow \text{Work}$ becomes $[10, 5, 7]$.
- **Safe Sequence**: $\langle P_1, P_3, P_4, P_0, P_2 \rangle$.

#### Unsafe State Test Case
- 4 processes, 3 resource types.
- $\text{Available} = [0, 1, 0]$.
- Every process needs at least 2 units of $R_0$ and 1 unit of $R_2$, but available has $0$ units of $R_0$ and $R_2$.
- Result: No process can proceed. System reports: `UNSAFE state. No safe sequence exists; deadlock is possible.`

---

## 7. Worker Failure Detection & Process Lifecycle (FR6)

### Process Lifecycle Implementation
1. **Creation**: Parent calls `fork_flushed()` (flushes stdio buffers before fork so buffered output is not duplicated).
2. **Normal Worker**: Child performs simulation work and exits cleanly with `_exit(0)`.
3. **Simulated Failure**: Child prints `SIMULATED FAILURE (deliberate, not a real OS fault)` and terminates with exit code `42`.
4. **Parent Detection**:
   ```c
   waitpid(pid, &status, 0);
   if (WIFEXITED(status)) {
       int code = WEXITSTATUS(status);
       if (code == 0) // COMPLETED
       else if (code == 42) // FAILED (detected non-zero exit code)
   } else if (WIFSIGNALED(status)) {
       int sig = WTERMSIG(status); // Killed by signal
   }
   ```
5. **Key Viva Point**: We clearly distinguish between normal termination (`exit(0)`) and simulated failure (`exit(42)`), avoiding accidental confusion with kernel-level crashes.

---

## 8. Resource Limits & System Cleanup (FR7, FR8)

### Resource Limit Demonstration (`setrlimit`)
- `struct rlimit limit = { .rlim_cur = 1, .rlim_max = 2 };`
- Child applies limit to itself: `setrlimit(RLIMIT_CPU, &limit);`.
- Child enters CPU-bound infinite spin loop: `while (spin < ~0ULL) spin++;`.
- **Observed Kernel Effect**: Exactly upon reaching 1 second of accumulated CPU time, the Linux kernel generates `SIGXCPU` (signal 24) and forcibly terminates the runaway process.
- Parent inspects status:
  `WIFSIGNALED(status) && WTERMSIG(status) == SIGXCPU` $\rightarrow$ PASS.

### Comprehensive Cleanup Audit
- Logger: Bounded buffer emptied, thread joined, `campuscompute.log` flushed and closed.
- Child Processes: Reaped via `waitpid(-1, NULL, WNOHANG)`. Verified `ECHILD` (no remaining children).
- IPC Objects: `shm_unlink()` and `sem_unlink()` called. `ipc_objects_remaining()` inspects `/dev/shm` and confirms 0 leftover objects.

---

## 9. Live Coding & Practical Demonstration Cheat Sheet

Here are the exact commands to run during your viva when the examiner gives specific tasks:

### Task 1: "Add a new job and execute the FCFS algorithm."
**In Interactive Mode**:
```bash
./campuscompute
# Type 3 (Add a new job to current workload)
# Enter: Arrival=5, Burst=4, Priority=1
# Type 7 (FCFS scheduling)
```
**Observation**: Shows Job 6 added to the table and execution order updated to `[J1 0-5] [J2 5-8] [J3 8-16] [J4 16-22] [J5 22-26] [J6 26-30]`.

### Task 2: "Change the Round Robin time quantum and demonstrate the difference in output."
**Direct CLI Command**:
```bash
# Run with default quantum 2
./campuscompute --rr

# Run with quantum 4
./campuscompute --rr 4
```
**Explanation to Teacher**:
- With $q=2$, the timeline produces 14 time slices due to frequent preemptions.
- With $q=4$, the timeline drops to 8 time slices because shorter jobs (like J2 with $BT=3$ and J5 with $BT=4$) complete in a single quantum without preemption.
- Average turnaround time decreases from 17.80 to 17.20.

### Task 3: "Change job priorities and execute Priority Scheduling."
**In Interactive Mode**:
```bash
./campuscompute
# Type 4 (Change an existing job's priority)
# Enter Job ID: 4
# Enter New Priority: 1
# Type 9 (Priority scheduling)
```
**Observation**: Job 4's priority becomes 1. At $t=5$ (after J1 finishes), J2 and J4 both have Priority 1. J2 arrived at $t=1$, J4 at $t=3$, so J2 runs first, then J4 runs next at $t=8$ ahead of all Priority 2 jobs!

### Task 4: "Run the same job set using all three scheduling algorithms and compare results."
**Direct CLI Command**:
```bash
./campuscompute --compare
```
**Observation**: Displays a side-by-side comparison table showing average turnaround and waiting times for FCFS, Priority, and Round Robin, highlighting which policy achieved the lowest average waiting time.

### Task 5: "Demonstrate producer-consumer synchronization."
**Direct CLI Command**:
```bash
./campuscompute --logger
```
**Observation**: Points out lines showing `[Producer] queue full (8/8), waiting for consumer` to demonstrate blocking synchronization, followed by orderly thread join and log verification.

### Task 6: "Show a safe and unsafe state using Banker's Algorithm."
```bash
./campuscompute --banker-safe
./campuscompute --banker-unsafe
```
**Observation**: Shows the safe sequence $\langle P_1, P_3, P_4, P_0, P_2 \rangle$ for the safe state, and blocked processes with deadlock warning for the unsafe state.

### Task 7: "Simulate worker failure and demonstrate parent-side detection."
```bash
./campuscompute --worker-fail
```
**Observation**: Worker deliberately exits with code 42. Parent detects this using `waitpid()` and reports `FAILED (detected from non-zero exit status)`.

---

## 10. Top 25 Viva Voce Questions & Model Answers

### General & Architecture
**Q1: What is the primary objective of the CampusCompute project?**
> **Answer**: CampusCompute simulates a university computing environment to practically demonstrate fundamental Operating System concepts: CPU scheduling algorithms, multithreading synchronization with bounded buffers, inter-process communication using POSIX shared memory and semaphores, deadlock avoidance via Banker's algorithm, process lifecycle monitoring via `fork()` and `waitpid()`, and kernel resource quota enforcement using `setrlimit()`.

**Q2: What is the difference between a process and a thread?**
> **Answer**: A process is an independent execution unit with its own private virtual address space, file descriptors, and OS resources (created via `fork()`). A thread is a lightweight unit of execution within a process that shares the same address space, global variables, and open files with peer threads (created via `pthread_create()`), but maintains its own program counter, registers, and stack.

### CPU Scheduling
**Q3: What is the difference between preemptive and non-preemptive scheduling?**
> **Answer**: In non-preemptive scheduling (like our FCFS and Priority algorithms), once a process receives the CPU, it holds it until it either terminates or blocks for I/O. In preemptive scheduling (like our Round Robin algorithm), the OS can interrupt a running process and return it to the ready queue when its time quantum expires or a higher priority process arrives.

**Q4: Define Turnaround Time and Waiting Time. How are they calculated?**
> **Answer**:
> - **Turnaround Time ($TAT$)**: Total elapsed time from process arrival to its completion: $TAT = \text{Completion Time} - \text{Arrival Time}$.
> - **Waiting Time ($WT$)**: Total time a process spent waiting in the ready queue: $WT = TAT - \text{Burst Time}$.

**Q5: What happens if the Round Robin time quantum is extremely large or extremely small?**
> **Answer**: If the quantum is extremely large ($q \ge \max(BT)$), Round Robin degenerates into FCFS scheduling. If the quantum is extremely small (approaching 0), the system spends excessive CPU cycles on context switching overhead rather than productive work, leading to processor thrashing.

**Q6: What is the priority tie-breaking rule in your implementation?**
> **Answer**: In our Priority scheduling, lower number indicates higher priority. If two jobs have identical priority, the one with earlier arrival time runs first. If arrival times are also identical, the one with lower Job ID runs first.

**Q7: In Round Robin, when a job's quantum expires at time $t$ and a new job arrives at time $t$, which one is queued first?**
> **Answer**: The newly arrived job is admitted into the ready queue *before* the preempted job is re-queued at the tail. This prevents newly arriving short jobs from being starved by a running long job.

### Multithreading & Synchronization
**Q8: What is a Race Condition? How did you prevent it?**
> **Answer**: A race condition occurs when multiple threads concurrently read and write shared data, and the final result depends on the non-deterministic order of execution. In `logger.c`, we prevent race conditions by enclosing all accesses to the shared circular queue, `count`, `head`, and `tail` inside a critical section protected by `pthread_mutex_lock(&lock)` and `pthread_mutex_unlock(&lock)`.

**Q9: What is the difference between a Mutex and a Semaphore?**
> **Answer**:
> 1. **Ownership**: A mutex has ownership—only the specific thread that locked the mutex can unlock it. A semaphore has no ownership—any thread/process can signal (`sem_post`) or wait (`sem_wait`).
> 2. **Type**: A mutex is strictly binary (locked/unlocked). A semaphore can be counting (integer value $\ge 0$) or binary.
> 3. **Scope**: POSIX mutexes operate within threads of a single process. Named POSIX semaphores can synchronize separate processes across the entire OS.

**Q10: Why do you call `pthread_cond_wait` inside a `while` loop rather than an `if` statement?**
> **Answer**: To protect against **spurious wakeups** (where the OS wakes up a waiting thread without an explicit signal) and race conditions (where another thread sneaks in and consumes the buffer slot before the awakened thread re-acquires the mutex).

**Q11: What does `pthread_cond_wait(&cv, &mutex)` do internally?**
> **Answer**: It atomically releases the mutex and suspends the calling thread on the condition variable. When awakened by `pthread_cond_signal`, it automatically re-acquires the mutex before returning to the caller.

### Inter-Process Communication (IPC)
**Q12: Why did you choose Shared Memory over Pipes or Message Queues?**
> **Answer**: Shared memory is the fastest form of IPC in Linux because data is mapped directly into the address spaces of both processes. There is zero kernel copying involved during data transfer, unlike pipes or sockets where data must be copied from user space to kernel space and back to user space.

**Q13: Why is a synchronization mechanism like a semaphore needed with shared memory?**
> **Answer**: Shared memory provides no built-in synchronization. Without semaphores, parent and child processes would suffer from race conditions or overwrite data before the receiver has read it. We used `sem_ready` and `sem_done` to coordinate the exact write-then-read sequence.

**Q14: What is the role of `ftruncate` after `shm_open`?**
> **Answer**: When a shared memory object is newly created using `shm_open`, its initial size is 0 bytes. Calling `ftruncate(fd, sizeof(Packet))` allocates the physical page storage required before calling `mmap`.

**Q15: What is the difference between `shm_unlink` and `munmap`?**
> **Answer**: `munmap` unmaps the shared memory region from the virtual address space of the calling process. `shm_unlink` removes the name from the OS namespace (`/dev/shm`), ensuring that the shared memory object is destroyed once all processes have closed their file descriptors.

### Banker's Algorithm
**Q16: What is the purpose of Banker's Algorithm? Is it prevention or avoidance?**
> **Answer**: Banker's Algorithm is a **deadlock avoidance** algorithm. It tests for safety by simulating the maximum possible resource claims of all processes before granting requests, ensuring the system never enters an unsafe state.

**Q17: What is the difference between a Safe State and an Unsafe State?**
> **Answer**: A safe state is one where there exists at least one order (a Safe Sequence) in which all processes can obtain their maximum required resources and finish execution without deadlock. An unsafe state is not necessarily deadlocked yet, but may lead to deadlock if all processes simultaneously demand their maximum resource requirements.

**Q18: How is the Need matrix calculated?**
> **Answer**: $\text{Need}[i][j] = \text{Maximum}[i][j] - \text{Allocation}[i][j]$. It represents the remaining instances of resource $j$ that process $i$ may still request before completing.

**Q19: What is the condition for process $P_i$ to finish in the safety algorithm?**
> **Answer**: $\text{Finish}[i] == \text{false}$ and $\text{Need}_i \le \text{Work}$. When this condition is met, $P_i$ is assumed to run to completion and release all its currently allocated resources back to the pool: $\text{Work} = \text{Work} + \text{Allocation}_i$.

### Process Management & Worker Failure
**Q20: What does `fork()` return in parent and child?**
> **Answer**: In the parent process, `fork()` returns the Process ID (PID) of the newly created child process. In the child process, it returns `0`. If process creation fails, it returns `-1`.

**Q21: Why do we call `fflush(stdout)` before `fork()`?**
> **Answer**: Standard C library I/O buffers (`stdout`) are duplicated in the child process upon `fork()`. If there is unflushed text in the buffer, both parent and child will flush it upon termination, causing duplicate lines in output. Flushing before fork guarantees clean process separation.

**Q22: How does the parent process inspect the termination status of a child?**
> **Answer**: Using POSIX macros on the status integer returned by `waitpid()`:
> - `WIFEXITED(status)`: True if child terminated normally (via `exit` or return).
> - `WEXITSTATUS(status)`: Evaluates to the exit code passed by the child (e.g., 0 for success, 42 for simulated failure).
> - `WIFSIGNALED(status)`: True if child was terminated by an unhandled signal.
> - `WTERMSIG(status)`: Returns the signal number that killed the child (e.g., `SIGXCPU`).

**Q23: What is a zombie process and how did your program avoid it?**
> **Answer**: A zombie process occurs when a child process terminates but its parent has not yet read its exit status via `wait()` or `waitpid()`. The process table entry remains allocated. Our program always calls `waitpid()` to reap every child process immediately, and the `--cleanup` routine cleans up any stray children.

### Resource Limits & Error Handling
**Q24: What is the difference between a Soft Limit and a Hard Limit in `setrlimit`?**
> **Answer**: The soft limit (`rlim_cur`) is the active threshold enforced by the kernel. When breached, the kernel notifies the process (e.g., sends `SIGXCPU`). The hard limit (`rlim_max`) is the ceiling for the soft limit. Unprivileged processes can lower their hard limit or adjust their soft limit up to the hard limit, but only the root user (`CAP_SYS_RESOURCE`) can increase the hard limit.

**Q25: How does your program handle invalid inputs or malformed commands?**
> **Answer**:
> - In `input.c`, `read_int()` validates every input using `fgets()` and `strtol()`, safely purging trailing buffer characters and checking lower and upper bounds, re-prompting until valid input is given.
> - In `main.c`, unrecognized CLI flags print a standardized error and usage message and exit with code 1.
> - In the interactive menu, invalid numbers or non-numeric strings are caught and flagged with `[Error] Invalid command`.

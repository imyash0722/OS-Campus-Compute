# CampusCompute — Comprehensive Project & Concept Study Guide
**Course**: Operating Systems (UE25CS242A) — Level 3 Mini-Project  
**Faculty In-Charge**: Dr. Gokulakrishnan S  
**Institution**: PES University  
**Project Title**: *CampusCompute – Intelligent Job Scheduling and Resource Management*  
**Team**: Virag Minche (PES2UG25CS606) and Yashwanth Karthik (PES2UG25CS615)

---

## How to Use This Guide
This document is designed as your **all-in-one preparation manual** for your project demonstration, live coding, and technical viva voce.
- **Section 1**: Understand what the project is, why it was built, and the real-world computing lab scenario.
- **Section 2**: Deep-dive into each Operating System concept, its theoretical foundation, and **exactly how it is implemented in our C code**.
- **Section 3**: File-by-file and function-by-function code walkthrough so you can read through the source code effortlessly.
- **Section 4**: Step-by-step instructions for the 8 practical live-coding tasks your examiner may ask you to perform on the spot.
- **Section 5**: Top technical viva questions with concise, high-scoring model answers.

---

## 1. Project Scenario & Big-Picture Understanding

### What is CampusCompute?
Imagine a university computing lab equipped with expensive multi-core servers. Students and researchers submit simulation scripts, machine learning training runs, and rendering tasks (referred to as **jobs**).

If jobs are run chaotically without an Operating System manager:
1. Short, interactive jobs get stuck behind 10-hour simulations (**Convoy Effect / Starvation**).
2. Multiple processes writing to the same log file corrupt each other's data (**Race Conditions**).
3. Processes requesting RAM and GPUs simultaneously grab resources and freeze forever (**Deadlock**).
4. Buggy jobs with infinite loops consume 100% CPU forever, burning electricity and starving other users (**Resource Exhaustion**).
5. Crashed jobs leave orphan memory blocks taking up space indefinitely (**Resource Leaks**).

**CampusCompute** is a command-line Operating System simulator written in C for Linux that solves every single one of these problems using real OS primitives.

---

## 2. Core OS Concepts & How They Relate to Our Project

Here is the master concept map connecting textbook OS theory directly to our C source code.

```
+---------------------------------------------------------------------------------------------------------+
|                                              CampusCompute                                              |
+---------------------------------------------------------------------------------------------------------+
       |                  |                    |                  |                 |               |
       v                  v                    v                  v                 v               v
 [CPU Scheduling]   [Concurrency]            [IPC]          [Deadlock]         [Processes]     [Limits]
  FCFS / RR / Pri    Producer-Consumer    Shared Memory      Banker's Safety    fork/waitpid    setrlimit
  scheduler.c        logger.c             ipc.c              banker.c           worker.c        rlimit.c
  (FR1, FR2)         (FR3)                (FR4)              (FR5)              (FR6)           (FR7)
```

---

### Concept 1: Job Lifecycle & Process Representation (FR1)

#### Theory
In an OS, a **Job** or **Process** is represented by a control block (in Linux, `task_struct`; in our project, `struct Job`). A job transitions through distinct lifecycle states:
- **READY**: The job has arrived and is waiting in the ready queue for CPU time.
- **RUNNING**: The CPU is currently executing the job's instructions.
- **COMPLETED**: The job has exhausted its required CPU burst and finished.

#### How It Relates to Our Code (`common.h` & `scheduler.c`)
- **Job Attributes** defined in `common.h`:
  ```c
  typedef struct {
      int id;           // Unique integer identifier (1, 2, 3...)
      int arrival;      // Timestamp when job enters ready queue (0-1000)
      int burst;        // Total CPU execution ticks needed (1-100)
      int priority;     // Priority ranking (1-99, lower number = higher priority)
      char status[16];  // "READY" -> "RUNNING" -> "COMPLETED"
  } Job;
  ```
- **Lifecycle tracking**:
  - Initially loaded or entered: status set to `"READY"`.
  - When selected by CPU: status updated to `"RUNNING"`.
  - When burst finishes: `finish()` marks status as `"COMPLETED"`.
- **Input Validation** (`input.c`):
  - `read_int()` prevents buffer overflows and rejects non-numeric characters using `fgets()` and `strtol()`, enforcing bounds `[lo, hi]`.

---

### Concept 2: CPU Scheduling Algorithms & Metrics (FR2)

#### Theory
When multiple jobs are in the `READY` state, the OS **CPU Scheduler** must decide which job gets the CPU next, balancing **throughput**, **fairness**, and **waiting time**.

#### Scheduling Policies Implemented
1. **First-Come, First-Served (FCFS)**:
   - **Type**: Non-preemptive.
   - **Mechanism**: The CPU is allocated to jobs strictly in the order they arrive.
   - **Tie-Breaker**: Equal arrival time $\rightarrow$ lower Job ID.
   - **Drawback**: Vulnerable to the *Convoy Effect* (short jobs get stuck behind long jobs, driving up average waiting time).
2. **Non-preemptive Priority Scheduling**:
   - **Type**: Non-preemptive. Once a job starts, it runs to completion.
   - **Mechanism**: At any scheduling point, the CPU picks the ready job with the lowest priority number ($1$ is higher priority than $4$).
   - **Tie-Breaker**: Equal priority $\rightarrow$ earlier arrival time $\rightarrow$ lower Job ID.
   - **Trade-off**: High-priority jobs finish quickly, but low-priority jobs can suffer *starvation* if high-priority jobs keep arriving.
3. **Round Robin (RR)**:
   - **Type**: Preemptive.
   - **Mechanism**: Every job is given a maximum slice of CPU time called a **Time Quantum** ($q$). If the job does not finish within $q$, it is interrupted (preempted) and moved to the back of the queue.
   - **Tie-Breaker**: When a quantum expires at time $t$, **newly arriving jobs are admitted into the ready queue before the preempted job is re-queued**.
   - **Impact of Quantum**:
     - Very large quantum $\rightarrow$ behaves like FCFS.
     - Very small quantum $\rightarrow$ excessive context switching overhead.

#### Performance Metrics & Formulas
- **Completion Time ($CT$)**: The exact clock tick when the job completes its burst.
- **Turnaround Time ($TAT$)**: Total elapsed time from arrival to completion:
  $$TAT = CT - AT$$
- **Waiting Time ($WT$)**: Time spent sitting idle in the ready queue:
  $$WT = TAT - BT$$
- **Averages**:
  $$\text{Average } TAT = \frac{\sum TAT}{N}, \quad \text{Average } WT = \frac{\sum WT}{N}$$

#### Default 5-Job Benchmark Comparison

| Job | Arrival | Burst | Priority | FCFS CT | Priority CT | RR ($q=2$) CT |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **J1** | 0 | 5 | 2 | 5 | 5 | 16 |
| **J2** | 1 | 3 | 1 | 8 | 8 | 13 |
| **J3** | 2 | 8 | 4 | 16 | 26 | 26 |
| **J4** | 3 | 6 | 2 | 22 | 14 | 24 |
| **J5** | 4 | 4 | 2 | 26 | 18 | 20 |
| **Averages** | - | - | - | **TAT: 13.40 / WT: 8.20** | **TAT: 12.20 / WT: 7.00** | **TAT: 17.80 / WT: 12.60** |

*Key Takeaway*: Priority achieves the lowest average waiting time ($7.00$) because J2, J4, and J5 finish ahead of the long, low-priority J3.

---

### Concept 3: Multithreading, Producer-Consumer & Synchronization (FR3)

#### Theory
- **Process vs Thread**: A process has its own private virtual memory space. Threads exist *inside* a process and share the exact same global variables, memory heap, and file descriptors.
- **Race Condition**: When two or more threads read and write shared data concurrently, and the final outcome depends on the timing of execution.
- **Critical Section**: The section of code that accesses shared memory. It must be protected by **Mutual Exclusion**.
- **Bounded Buffer (Producer-Consumer) Problem**:
  - Producers generate log messages and insert them into a fixed-size queue (size = 8).
  - A consumer extracts messages and writes them to disk (`campuscompute.log`).
  - **Constraint 1**: A producer cannot insert into a FULL queue.
  - **Constraint 2**: A consumer cannot extract from an EMPTY queue.

#### How It Relates to Our Code (`logger.c`)
- **Shared Bounded Buffer**: `queue[LOG_QUEUE_SIZE]` of size 8.
- **Mutual Exclusion**: `pthread_mutex_t lock`. Before touching the queue, both producers and consumers must acquire this lock:
  ```c
  pthread_mutex_lock(&lock);
  // ... critical section ...
  pthread_mutex_unlock(&lock);
  ```
- **Condition Variables**:
  - `not_full`: Producers wait here when `count == LOG_QUEUE_SIZE`.
  - `not_empty`: Consumer waits here when `count == 0`.
- **The Spurious Wakeup Problem (`while` vs `if`)**:
  ```c
  while (count == LOG_QUEUE_SIZE) {
      pthread_cond_wait(&not_full, &lock);
  }
  ```
  *Why `while`?* When a thread wakes up, another thread may have grabbed the slot first, or the OS may wake the thread spuriously without any signal. The `while` loop re-verifies that the condition is truly satisfied before proceeding.
- **Orderly Shutdown**:
  When all 3 producer threads finish and are joined via `pthread_join()`, `logger_shutdown()` inserts a special sentinel event (`ev.stop = true`). The consumer pops this event, breaks its infinite loop, flushes the log file, and exits cleanly.

---

### Concept 4: Inter-Process Communication (IPC) & Named Semaphores (FR4)

#### Theory
Because distinct processes have separate virtual memory spaces, Child Process cannot read Parent's variables directly. They need **Inter-Process Communication (IPC)**.
- **Shared Memory**: The fastest form of IPC. The OS maps the exact same physical memory frames into the page tables of both processes. Once mapped, communication occurs at RAM speeds without kernel copying.
- **POSIX Semaphores**: Unlike a mutex (which is owned by a single thread), a semaphore is an integer counter managed by the kernel used to synchronize separate processes across the OS.
  - `sem_post(s)`: Increments semaphore ($V$ operation).
  - `sem_wait(s)`: Decrements semaphore ($P$ operation); blocks if value is 0.

#### How It Relates to Our Code (`ipc.c`)
1. **Shared Memory Setup**:
   - `shm_open("/cc_shm_PES2UG25CS606", O_CREAT | O_RDWR, 0600)` creates a shared memory file under `/dev/shm`.
   - `ftruncate(fd, sizeof(Packet))` sets its size.
   - `mmap(NULL, sizeof(Packet), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)` maps the buffer into virtual memory as a `Packet *pkt` pointer.
2. **Synchronization Handshake**:
   - `sem_ready` (initially 0): Parent writes job data $\rightarrow$ calls `sem_post(ready)`.
   - Child was waiting via `sem_timedwait(ready)` $\rightarrow$ wakes up, reads packet, computes response $\rightarrow$ calls `sem_post(done)`.
   - Parent, waiting on `sem_timedwait(done)`, wakes up and reads child's response.
3. **Timed Waiting vs Unbounded Waiting**:
   We use `sem_timedwait(&ts)` with a 5-second timeout instead of `sem_wait()`. If the child process crashes, the parent will not hang forever.
4. **Cleanup**:
   `munmap()`, `close()`, `shm_unlink()`, and `sem_unlink()` remove all traces from `/dev/shm`. `ipc_objects_remaining()` inspects the filesystem to guarantee 0 leaked objects.

---

### Concept 5: Deadlock Avoidance — Banker's Algorithm (FR5)

#### Theory
- **Deadlock**: A permanent blocked state where a set of processes are waiting for resources held by each other.
- **The 4 Coffman Conditions**: Mutual Exclusion, Hold and Wait, No Preemption, Circular Wait.
- **Deadlock Avoidance**: The OS analyzes every resource allocation to ensure the system never enters an **Unsafe State**.
- **Banker's Algorithm**:
  - **Available** ($1 \times m$): Free units of each resource type.
  - **Allocation** ($n \times m$): Currently allocated units.
  - **Maximum** ($n \times m$): Upper bound of units each process may claim.
  - **Need** ($n \times m$): Remaining units each process may request:
    $$\text{Need}[i][j] = \text{Maximum}[i][j] - \text{Allocation}[i][j]$$

#### Safety Algorithm Steps
1. Let $\text{Work} = \text{Available}$ and $\text{Finish}[i] = \text{false}$ for all $i$.
2. Find an unfinished process $P_i$ such that $\text{Need}_i \le \text{Work}$.
3. If found: assume $P_i$ completes and returns its resources:
   $$\text{Work} = \text{Work} + \text{Allocation}_i, \quad \text{Finish}[i] = \text{true}$$
   Add $P_i$ to the Safe Sequence and repeat Step 2.
4. If all processes finish, the state is **SAFE** (deadlock-free). If any process remains blocked, the state is **UNSAFE** (deadlock possible).

#### How It Relates to Our Code (`banker.c`)
- **Safe State Test Case**:
  5 processes, 3 resource types. $\text{Available} = [3, 3, 2]$.
  - Step 1: $P_1$ needs $[1, 2, 2] \le [3, 3, 2] \implies \text{Work} = [5, 3, 2]$.
  - Step 2: $P_3$ needs $[0, 1, 1] \le [5, 3, 2] \implies \text{Work} = [7, 4, 3]$.
  - Step 3: $P_4$ needs $[4, 3, 1] \le [7, 4, 3] \implies \text{Work} = [7, 4, 5]$.
  - Step 4: $P_0$ needs $[7, 4, 3] \le [7, 4, 5] \implies \text{Work} = [7, 5, 5]$.
  - Step 5: $P_2$ needs $[6, 0, 0] \le [7, 5, 5] \implies \text{Work} = [10, 5, 7]$.
  - **Safe Sequence**: $\langle P_1, P_3, P_4, P_0, P_2 \rangle$.
- **Unsafe State Test Case**:
  4 processes, 3 resource types. $\text{Available} = [0, 1, 0]$.
  Every process needs at least 2 units of $R_0$ and 1 unit of $R_2$. None can finish $\rightarrow$ program identifies **UNSAFE** deadlock state.

---

### Concept 6: Process Management & Worker Failure Detection (FR6)

#### Theory
- `fork()` clones the calling process, creating a new Child process with an identical address space.
  - Return in Parent: Child's PID ($> 0$).
  - Return in Child: `0`.
- **Zombie Process**: A terminated child whose exit status has not yet been read by its parent via `wait()` or `waitpid()`.
- `waitpid(pid, &status, options)`: Suspends parent until child changes state, reaping the zombie and retrieving its termination status.

#### How It Relates to Our Code (`worker.c`)
- `fork_flushed()` calls `fflush(stdout)` immediately before `fork()` so buffered text is not duplicated.
- **Normal Completion**:
  - Child runs and terminates via `child_exit(0)`.
  - Parent checks:
    ```c
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        // Child exited normally with code 0 -> COMPLETED
    }
    ```
- **Simulated Failure**:
  - Child deliberately terminates via `child_exit(42)`.
  - Parent catches it:
    ```c
    int code = WEXITSTATUS(status);
    if (code == 42) {
        // Detected controlled worker failure!
    }
    ```
  - We explicitly print: `SIMULATED FAILURE (deliberate, not a real OS fault)` so the examiner knows it is controlled testing.

---

### Concept 7: OS Resource Limits & Signals (FR7)

#### Theory
An unconstrained process can monopolize system CPU or memory. Linux provides `setrlimit()` to impose hard and soft ceilings.
- **Soft Limit (`rlim_cur`)**: The threshold actively enforced by the kernel.
- **Hard Limit (`rlim_max`)**: The maximum value to which an unprivileged process can raise its soft limit.
- **Kernel Signal Enforcement**: When a process reaches its soft `RLIMIT_CPU` limit, the Linux kernel delivers an asynchronous signal: `SIGXCPU` (Signal 24). The default action is abnormal termination.

#### How It Relates to Our Code (`rlimit.c`)
- Child sets its soft CPU limit to 1 second:
  ```c
  struct rlimit limit = { .rlim_cur = 1, .rlim_max = 2 };
  setrlimit(RLIMIT_CPU, &limit);
  ```
- Child enters an infinite spin loop: `while (spin < ~0ULL) spin++;`.
- After 1 second of accumulated CPU execution, the Linux kernel sends `SIGXCPU`.
- Parent catches this in `waitpid()`:
  ```c
  if (WIFSIGNALED(status) && WTERMSIG(status) == SIGXCPU) {
      // Child terminated by signal 24 (SIGXCPU)
  }
  ```
  This proves genuine OS-level quota enforcement, not hard-coded output.

---

### Concept 8: Error Handling & System Cleanup (FR8)

#### Theory
Production systems must reject malformed user input without crashing, and guarantee that background threads, stray processes, and IPC files are unlinked upon exit.

#### How It Relates to Our Code (`input.c` & `main.c`)
- Non-integer input or out-of-range numbers are rejected with helpful error messages.
- Command-line flags (e.g. `./campuscompute --bogus`) exit with status 1 and usage instructions.
- `--cleanup` runs an audit:
  1. Stops and joins the logger thread.
  2. Reaps stray child processes using `waitpid(-1, NULL, WNOHANG)`.
  3. Inspects `/dev/shm` to verify 0 leftover shared memory or semaphore objects.

---

## 3. Source Code Tour (File-by-File)

### 1. `common.h`
- **Purpose**: Central definitions and structures.
- **Key Elements**:
  - `Job` struct: `id`, `arrival`, `burst`, `priority`, `status`.
  - `Metrics` struct: `completion`, `turnaround`, `waiting`.
  - `get_srn()`: Returns `getenv("SRN")` if set, otherwise defaults to `"PES2UG25CS606"`.
  - `fork_flushed()`: Calls `fflush(stdout)` before `fork()`.
  - `child_exit()`: Calls `fflush(stdout)` before `_exit()`.

### 2. `input.c`
- **Purpose**: Safe integer input.
- **Function**: `bool read_int(const char *prompt, int lo, int hi, int *out)`
  - Prompts user, reads input via `fgets()`.
  - Clears any trailing input from `stdin` if buffer exceeded.
  - Converts string to integer with `strtol()`.
  - Re-prompts until input is strictly within `[lo, hi]`.

### 3. `scheduler.c` & `scheduler.h`
- **Purpose**: CPU workload management and scheduling simulation.
- **Key Functions**:
  - `scheduler_load_default_jobs()`: Loads the standard 5-job benchmark.
  - `scheduler_add_job()`: Appends a single job to the workload (Live Coding Q1).
  - `scheduler_edit_job_priority()`: Modifies priority of an existing job (Live Coding Q3).
  - `scheduler_set_quantum()`: Modifies the Round Robin quantum (Live Coding Q2).
  - `simulate_nonpreemptive()`: Runs FCFS and Priority scheduling. Advances clock through idle periods.
  - `simulate_rr()`: Implements Round Robin queue logic, executing up to `current_quantum` per slice.
  - `finish()`: Calculates $CT, TAT = CT - AT, WT = TAT - BT$.
  - `scheduler_compare_all()`: Runs all 3 algorithms on the same job set and prints a comparison table.

### 4. `logger.c` & `logger.h`
- **Purpose**: Multithreaded producer-consumer activity logger.
- **Key Functions**:
  - `push()`: Producer locks mutex, waits on `not_full`, pushes event, signals `not_empty`, unlocks mutex.
  - `pop()`: Consumer locks mutex, waits on `not_empty`, pops event, signals `not_full`, unlocks mutex.
  - `consume()`: Consumer thread loop writing formatted timestamps to `campuscompute.log`.
  - `logger_shutdown()`: Pushes sentinel `{ .stop = true }` event and calls `pthread_join(consumer)`.
  - `logger_demo()`: Launches 3 producer threads, each generating 5 events.

### 5. `ipc.c` & `ipc.h`
- **Purpose**: Zero-copy shared memory and named semaphore IPC.
- **Key Functions**:
  - `ipc_cleanup()`: Calls `shm_unlink()` and `sem_unlink()`.
  - `ipc_objects_remaining()`: Checks if `/cc_shm_<SRN>` or semaphores still exist in `/dev/shm`.
  - `child_main()`: Waits on `sem_ready`, reads shared memory packet, writes response, signals `sem_done`.
  - `ipc_demo()`: Creates shared memory and semaphores, forks child, exchanges packet, reaps child, and unlinks resources.

### 6. `banker.c` & `banker.h`
- **Purpose**: Deadlock avoidance using Banker's safety algorithm.
- **Key Functions**:
  - `is_safe()`: Implements safety check using `Work` and `Finish` vectors.
  - `analyse()`: Computes $\text{Need} = \text{Max} - \text{Alloc}$, displays matrices, and prints safe sequence.
  - `banker_demo_safe()`: Executes the 5-process safe state.
  - `banker_demo_unsafe()`: Executes the 4-process unsafe state.
  - `banker_input_and_check()`: Allows user to input custom matrices interactively.

### 7. `worker.c` & `worker.h`
- **Purpose**: Process lifecycle tracking and worker failure detection.
- **Key Functions**:
  - `run_worker(simulate_failure)`: Forks child. Child exits with code 0 (normal) or code 42 (simulated failure). Parent inspects status using `waitpid()`, `WIFEXITED()`, and `WEXITSTATUS()`.

### 8. `rlimit.c` & `rlimit.h`
- **Purpose**: Process resource quotas and signal handling.
- **Key Functions**:
  - `rlimit_demo_cpu()`: Child applies 1-second soft CPU limit via `setrlimit(RLIMIT_CPU)`. Enters infinite spin loop. Parent catches kernel-delivered `SIGXCPU` via `waitpid()` and `WTERMSIG()`.

### 9. `main.c`
- **Purpose**: Application controller, menu dispatch, CLI flags, integrated demo, and cleanup audit.
- **Key Functions**:
  - `cmd_demo()`: Runs all 14 unit demonstrations in sequence and displays a PASS/FAIL summary table.
  - `cmd_cleanup()`: Audits threads, child processes, and IPC resources to ensure zero system leaks.
  - `run_interactive()`: Interactive menu loop.
  - `main()`: Parses CLI arguments (`--fcfs`, `--rr [quantum]`, `--demo`, etc.).

---

## 4. Live Coding & Practical Question Playbook

During the viva, your instructor may ask you to execute or modify code on the spot. Here is your cheat sheet:

### Question 1: "Add a new job and execute the FCFS algorithm."
- **In Interactive Mode**:
  1. Run `./campuscompute`.
  2. Enter `3` (`Add a new job to current workload`).
  3. Enter: `Arrival time: 5`, `Burst: 4`, `Priority: 1`.
  4. Enter `7` (`FCFS scheduling`).
- **Explanation**: *"Job 6 is appended to the job table. In FCFS, jobs execute strictly in arrival order. Job 6 arrives at $t=5$, waits for earlier jobs to finish, and executes from $t=26$ to $t=30$."*

### Question 2: "Change Round Robin time quantum and show the difference."
- **Direct CLI Execution**:
  ```bash
  ./campuscompute --rr 2
  ./campuscompute --rr 4
  ```
- **Explanation**:
  - With $q=2$, the Gantt timeline produces 14 slices.
  - With $q=4$, the slices drop to 8 because short jobs (like J2 with $BT=3$ and J5 with $BT=4$) complete in a single quantum without preemption.
  - Average Turnaround Time decreases from $17.80$ to $17.20$.
- **Code Modification (if asked to edit source)**:
  Open `common.h` line 16: change `#define TIME_QUANTUM 2` to `4`. Recompile with `make`.

### Question 3: "Change job priorities and execute Priority Scheduling."
- **In Interactive Mode**:
  1. Run `./campuscompute`.
  2. Enter `4` (`Change an existing job's priority`).
  3. Enter Job ID: `4`, New Priority: `1`.
  4. Enter `9` (`Priority scheduling`).
- **Explanation**: *"Job 4 now has Priority 1 alongside Job 2. When Job 1 completes at $t=5$, both J2 and J4 are ready with Priority 1. By our tie-break rule (earlier arrival time wins), Job 2 ($AT=1$) runs first, followed immediately by Job 4 ($AT=3$) at $t=8$, executing ahead of all Priority 2 jobs."*

### Question 4: "Run the same job set on all three algorithms and compare."
- **Direct CLI Execution**:
  ```bash
  ./campuscompute --compare
  ```
- **Explanation**: Shows the side-by-side summary table: Priority achieves lowest average waiting time ($7.00$) on this workload.

### Question 5: "Demonstrate Producer-Consumer synchronization."
- **Direct CLI Execution**:
  ```bash
  ./campuscompute --logger
  ```
- **Explanation**: Point to the terminal output: `[Producer] queue full (8/8), waiting for consumer`. Explain that the producers block on `pthread_cond_wait(&not_full, &lock)`. When the consumer pops an item, it signals `not_full`, unblocking a producer.

### Question 6: "Demonstrate Banker's Algorithm safe and unsafe states."
- **Direct CLI Execution**:
  ```bash
  ./campuscompute --banker-safe
  ./campuscompute --banker-unsafe
  ```
- **Explanation**: Safe state demonstrates safe sequence $\langle P_1, P_3, P_4, P_0, P_2 \rangle$. Unsafe state shows all processes blocked because available resources $[0, 1, 0]$ cannot satisfy any process's Need.

### Question 7: "Simulate worker failure and parent detection."
- **Direct CLI Execution**:
  ```bash
  ./campuscompute --worker-fail
  ```
- **Explanation**: Child process calls `child_exit(42)`. Parent calls `waitpid()` and evaluates `WIFEXITED(status)` and `WEXITSTATUS(status)`, detecting exit code 42.

### Question 8: "Demonstrate process resource limit enforcement."
- **Direct CLI Execution**:
  ```bash
  ./campuscompute --rlimit
  ```
- **Explanation**: Demonstrates `setrlimit(RLIMIT_CPU)` with a 1-second soft limit. After 1 second of CPU consumption, the kernel delivers `SIGXCPU` (signal 24), terminating the process.

---

## 5. Top 20 Technical Viva Questions & Model Answers

**Q1: What is the difference between Preemptive and Non-preemptive scheduling?**
> **Answer**: In non-preemptive scheduling (like our FCFS and Priority implementations), once a process is given the CPU, it runs until it finishes its burst or blocks for I/O. In preemptive scheduling (like Round Robin), the OS can forcibly interrupt a running process when its time quantum expires and allocate the CPU to another ready process.

**Q2: What is the Convoy Effect?**
> **Answer**: The Convoy Effect occurs in FCFS when a CPU-bound process with a huge burst time runs first, forcing all subsequent short I/O-bound jobs to wait behind it, causing poor CPU and device utilization and high average waiting times.

**Q3: What is the difference between a Mutex and a Semaphore?**
> **Answer**: 
> 1. *Ownership*: A mutex has ownership—only the thread that locked the mutex can unlock it. A semaphore has no ownership—any thread/process can post or wait.
> 2. *Value*: A mutex is binary (locked/unlocked). A semaphore can be counting ($\ge 0$).
> 3. *Scope*: POSIX mutexes synchronize threads within a single process. Named POSIX semaphores can synchronize separate processes across the entire OS.

**Q4: Why must `pthread_cond_wait()` be called in a `while` loop rather than an `if` statement?**
> **Answer**: To guard against **spurious wakeups** and race conditions. A thread can wake up without an explicit signal, or another thread might claim the buffer slot before the awakened thread re-acquires the mutex. The `while` loop re-checks the condition upon wakeup.

**Q5: Why is Shared Memory preferred over Message Queues or Pipes for high-throughput computing?**
> **Answer**: Shared memory is zero-copy. The memory pages are mapped directly into the address spaces of both processes, allowing data transfer at RAM speed without copying data into or out of kernel space.

**Q6: Why is synchronization mandatory when using Shared Memory?**
> **Answer**: Shared memory provides no hardware synchronization. If two processes read and write to the same buffer concurrently, data corruption occurs. We use semaphores (`sem_ready` and `sem_done`) to coordinate write-then-read execution.

**Q7: What is the purpose of `ftruncate()` after `shm_open()`?**
> **Answer**: When a shared memory object is newly created via `shm_open()`, its size is 0 bytes. `ftruncate()` sets the physical size of the shared memory region before it can be mapped with `mmap()`.

**Q8: What is the difference between Deadlock Prevention and Deadlock Avoidance?**
> **Answer**: Deadlock Prevention eliminates at least one of the 4 Coffman conditions (e.g. denying Hold and Wait). Deadlock Avoidance (like Banker's Algorithm) dynamically inspects every resource allocation request using prior knowledge of maximum claims to ensure the system never enters an unsafe state.

**Q9: What is the formula for the Need matrix in Banker's Algorithm?**
> **Answer**: $\text{Need}[i][j] = \text{Maximum}[i][j] - \text{Allocation}[i][j]$. It represents the remaining instances of resource $j$ process $i$ may request before completing.

**Q10: Is an Unsafe State always Deadlocked?**
> **Answer**: No. An unsafe state is not necessarily deadlocked yet, but deadlock is possible if all processes simultaneously demand their maximum resource claims. A safe state guarantees that deadlock cannot occur.

**Q11: What does `fork()` return in parent and child?**
> **Answer**: In the parent, `fork()` returns the PID of the child ($> 0$). In the child, `fork()` returns `0`. On failure, it returns `-1`.

**Q12: Why do we call `fflush(stdout)` before `fork()`?**
> **Answer**: Standard C library I/O buffers (`stdout`) are duplicated in the child process. If unflushed text exists in the buffer, both parent and child flush it upon termination, causing duplicate output lines.

**Q13: What is the purpose of `waitpid()`?**
> **Answer**: `waitpid()` suspends the parent until the specified child terminates, reaps the child process entry from the process table (preventing zombie processes), and returns termination status information.

**Q14: How does `waitpid()` distinguish between normal exit and signal termination?**
> **Answer**:
> - `WIFEXITED(status)` is true if the child terminated normally via `exit()`. `WEXITSTATUS(status)` returns the exit code ($0-255$).
> - `WIFSIGNALED(status)` is true if the child was terminated by an unhandled signal. `WTERMSIG(status)` returns the signal number (e.g. `SIGXCPU`).

**Q15: What is the difference between `exit()` and `_exit()`?**
> **Answer**: `exit()` performs C library cleanup (flushing stdio buffers, running `atexit` handlers). `_exit()` immediately terminates the process via the kernel without touching user-space buffers. In child worker processes, we use `_exit()` to prevent accidental flushing of parent buffers.

**Q16: What is the difference between a Soft Limit and a Hard Limit in `setrlimit`?**
> **Answer**: The soft limit is the active quota enforced by the kernel. The hard limit is the ceiling for the soft limit. An unprivileged process can lower its hard limit or adjust its soft limit up to the hard limit, but only root (`CAP_SYS_RESOURCE`) can increase the hard limit.

**Q17: Which signal is generated when a process exceeds its CPU limit?**
> **Answer**: The Linux kernel sends `SIGXCPU` (Signal 24). Its default action is abnormal process termination with a core dump.

**Q18: What is the difference between `shm_unlink()` and `munmap()`?**
> **Answer**: `munmap()` unmaps the shared memory page from the process's virtual address space. `shm_unlink()` removes the shared memory filename from the `/dev/shm` filesystem so it is destroyed once all processes close it.

**Q19: How do you verify that your program cleans up all created IPC resources?**
> **Answer**: Run `./campuscompute --cleanup; ls /dev/shm | grep cc_`. The command outputs 0 remaining objects, and `ls /dev/shm` returns empty.

**Q20: Why did you write your program in modular C rather than Python or Java?**
> **Answer**: C provides direct access to POSIX kernel system calls (`fork`, `mmap`, `shm_open`, `setrlimit`, `pthread`), allowing exact control over memory addresses, hardware signals, and process tables without runtime abstraction layers.

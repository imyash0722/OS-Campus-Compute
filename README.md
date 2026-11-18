# CampusCompute — Intelligent Job Scheduling and Resource Management

[![Language](https://img.shields.io/badge/Language-C99-00599C?logo=c)](#)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%2F%20Ubuntu%20%2F%20WSL-E95420?logo=ubuntu)](https://ubuntu.com)
[![Standards](https://img.shields.io/badge/Standard-POSIX.1--2008-black)](#)
[![Build](https://img.shields.io/badge/Build-Make%20%7C%20GCC-success)](#)
[![Course](https://img.shields.io/badge/Course-UE25CS242A%20Operating%20Systems-blue)](#)

> **Level 3 Mini-Project — Operating Systems (UE25CS242A)**  
> **Course Faculty**: Dr. Gokulakrishnan S  
> **Institution**: PES University  
> **Team Members**:
> - **Virag Minche** (`PES2UG25CS606`)
> - **Yashwanth Karthik** (`PES2UG25CS615`)

---

## 1. Project Overview & Scenario

In a high-performance university shared computing facility, students and researchers run CPU-intensive simulations and multi-stage compute jobs. During peak operational periods, these jobs compete for limited CPU cores and hardware resources. 

**CampusCompute** is a robust, modular Linux command-line application implemented in C (C99) demonstrating practical core Operating System concepts:
- **FR1 — Job Lifecycle & Process Representation**: Validated job intake, unique identifiers, burst, priority, and states (`READY` $\rightarrow$ `RUNNING` $\rightarrow$ `COMPLETED`).
- **FR2 — CPU Scheduling**: FCFS, Non-preemptive Priority, and Round Robin (with configurable time quantum), complete with Gantt timelines, turnaround time, waiting time, and averages.
- **FR3 — Concurrent Activity Logger**: Multithreaded bounded producer-consumer buffer synchronized via POSIX threads, a mutual exclusion lock (`pthread_mutex_t`), and condition variables (`pthread_cond_t`).
- **FR4 — Inter-Process Communication (IPC)**: POSIX shared memory (`shm_open`, `mmap`) and named semaphores (`sem_open`, `sem_timedwait`) for data exchange between cooperating processes.
- **FR5 — Resource-Safety Analysis (Banker's Algorithm)**: Calculation of the Need matrix ($\text{Need} = \text{Max} - \text{Alloc}$) and detection of safe states (with safe sequences) vs. unsafe deadlock states.
- **FR6 — Worker Process Management & Failure Detection**: Process creation using `fork()`, status monitoring via `waitpid()`, and parent-side differentiation between clean termination and controlled simulated failures.
- **FR7 — Resource Quotas & System Cleanup**: Kernel CPU limits using `setrlimit(RLIMIT_CPU)` with `SIGXCPU` signal detection, along with automated zero-leak cleanup of all IPC objects and child processes.
- **FR8 — User Interface & Error Handling**: Comprehensive interactive CLI menu, standardized CLI flags, and robust validation of commands and integer inputs.

> [!NOTE]
> Every algorithm and metric is calculated dynamically at run time. The integrated test suite (`--demo`) executes all functional checks with real pass/fail assertions.

---

## 2. Directory Structure & Architecture

```
mini project/
├── common.h         # Shared definitions, Job & Metrics structs, dynamic SRN resolver
├── input.c          # Validated integer input routines (bounds checking & buffer flushing)
├── scheduler.h      # Scheduler interfaces (FCFS, RR, Priority, Comparison, Job management)
├── scheduler.c      # CPU scheduling implementations, Gantt timeline, metrics calculation
├── logger.h         # Logger API
├── logger.c         # Bounded circular queue, POSIX threads, mutex & condition variables
├── ipc.h            # IPC interface and cleanup verification
├── ipc.c            # POSIX shared memory & named semaphore coordination
├── banker.h         # Banker's algorithm interface
├── banker.c         # Safety algorithm, Need matrix calculation, safe/unsafe test states
├── worker.h         # Worker process interface
├── worker.c         # fork() + waitpid() process monitoring & simulated failure handling
├── rlimit.h         # Resource limits interface
├── rlimit.c         # setrlimit(RLIMIT_CPU) and SIGXCPU signal trapping
├── main.c           # CLI entry point, interactive menu, CLI flags, integrated demo
├── Makefile         # Build automation, compiler flags (-Wall -Wextra), make demo, make zip
├── README.md        # Complete project documentation & screenshot checklist
└── VIVA_GUIDE.md    # Comprehensive Viva Voce preparation guide (Top 25 Q&As)
```

### System Calls & POSIX APIs Used

| System Call / API | Header | Purpose in CampusCompute |
|---|---|---|
| `fork()` | `<unistd.h>` | Creates isolated child processes for workers, IPC receiver, and resource limit tests. |
| `waitpid()` | `<sys/wait.h>` | Suspends parent until child changes state; inspects exit status (`WIFEXITED`, `WEXITSTATUS`, `WIFSIGNALED`, `WTERMSIG`). |
| `setrlimit()` | `<sys/resource.h>` | Enforces a strict 1-second CPU limit on a child process; kernel terminates it via `SIGXCPU`. |
| `shm_open()` | `<sys/mman.h>`, `<fcntl.h>` | Creates POSIX shared memory object under `/dev/shm`. |
| `ftruncate()` | `<unistd.h>` | Sets the physical capacity of the shared memory region to `sizeof(Packet)`. |
| `mmap()` / `munmap()` | `<sys/mman.h>` | Maps and unmaps shared memory pages into the virtual address spaces of parent and child. |
| `shm_unlink()` | `<sys/mman.h>` | Removes shared memory object from the OS namespace upon completion. |
| `sem_open()` / `sem_close()` | `<semaphore.h>` | Creates, opens, and closes POSIX named semaphores for process coordination. |
| `sem_timedwait()` | `<semaphore.h>` | Decrements semaphore with a 5-second timeout to prevent deadlocks if a child process fails. |
| `sem_post()` | `<semaphore.h>` | Increments semaphore, waking up blocked waiting processes. |
| `sem_unlink()` | `<semaphore.h>` | Unlinks named semaphores from the system. |
| `pthread_create()` / `pthread_join()` | `<pthread.h>` | Spawns and joins producer and consumer threads. |
| `pthread_mutex_lock()` / `unlock()` | `<pthread.h>` | Enforces Mutual Exclusion over the bounded circular log queue. |
| `pthread_cond_wait()` / `signal()` | `<pthread.h>` | Coordinates producer (`not_full`) and consumer (`not_empty`) buffer availability. |

---

## 3. Build & Execution Instructions

### Prerequisites
Standard C compiler and POSIX libraries (available on Ubuntu 20.04/22.04/24.04 and WSL):
```bash
sudo apt update && sudo apt install -y build-essential gcc make zip unzip
```

### Environment Configuration
Before capturing screenshots or demonstrating, export your SRN to match your terminal prompt and program banner:
```bash
export SRN='PES2UG25CS606'
export PS1='[$SRN] \w\$ '
```
*(When your teammate demonstrates, they simply run `export SRN='PES2UG25CS615'`)*.

### Compilation
```bash
make clean
make
```

### Running CampusCompute
```bash
# 1. Interactive Menu
./campuscompute
# or:
make run

# 2. View Command-Line Help
./campuscompute --help

# 3. Run Automated Integrated Demonstration (all tests with PASS/FAIL summary)
./campuscompute --demo
# or:
make demo

# 4. Standalone CLI Flags
./campuscompute --fcfs              # Run FCFS scheduling
./campuscompute --rr                # Run Round Robin (default quantum 2)
./campuscompute --rr 4              # Run Round Robin with quantum 4
./campuscompute --priority          # Run Non-preemptive Priority scheduling
./campuscompute --compare           # Compare all 3 algorithms on the current workload
./campuscompute --logger            # Multithreaded producer-consumer logger
./campuscompute --ipc               # Shared memory & named semaphore IPC
./campuscompute --banker-safe       # Banker's algorithm safe state check
./campuscompute --banker-unsafe     # Banker's algorithm unsafe state check
./campuscompute --worker-ok         # Normal child process execution
./campuscompute --worker-fail       # Simulated worker failure detection
./campuscompute --rlimit            # CPU resource limit demonstration (SIGXCPU)
./campuscompute --cleanup           # Cleanup verification of processes, threads & IPC

# 5. Create Submission ZIP Archive with CRC Integrity Check
make zip
```

---

## 4. Interactive CLI Menu Reference

```
CampusCompute - Job Scheduling and Resource Management | PES2UG25CS606

Commands (type the number at the prompt, or run ./campuscompute --<flag>):
   1  Input custom jobs (replace current list)   --input
   2  Load default workload (5 jobs)             --default
   3  Add a new job to current workload          --add-job
   4  Change an existing job's priority          --edit-priority
   5  Set Round Robin time quantum               --set-quantum
   6  Display current jobs                       --show
   7  FCFS scheduling                            --fcfs
   8  Round Robin scheduling                     --rr
   9  Priority scheduling (non-preemptive)       --priority
  10  Compare all scheduling policies            --compare
  11  Producer-consumer logger (default benchmark) --logger
  12  Producer-consumer logger (custom threads)  --logger-custom
  13  Logger: submit custom message to queue     --logger-msg
  14  Shared memory + semaphore IPC (benchmark)  --ipc
  15  Shared memory IPC: send custom job packet  --ipc-custom
  16  Banker's algorithm: safe state             --banker-safe
  17  Banker's algorithm: unsafe state           --banker-unsafe
  18  Banker's algorithm: enter your own state   --banker-input
  19  Worker process: normal completion          --worker-ok
  20  Worker process: simulated failure          --worker-fail
  21  Worker process: custom simulated exit code --worker-custom
  22  Resource limit demo (RLIMIT_CPU)           --rlimit
  23  Integrated demonstration                   --demo
  24  Verify cleanup of threads/IPC/processes    --cleanup
   0  Clean up and exit
   ?  Show this menu
```

---

## 5. CPU Scheduling Algorithms & Formulas

### Metric Definitions
1. **Completion Time ($CT$)**: Instant of time when a process finishes execution.
2. **Turnaround Time ($TAT$)**: Total time elapsed between arrival and completion:
   $$TAT = CT - AT$$
3. **Waiting Time ($WT$)**: Total time a job spends waiting in the ready queue:
   $$WT = TAT - BT$$
4. **Average Turnaround Time**: $\frac{1}{n} \sum_{i=1}^n TAT_i$
5. **Average Waiting Time**: $\frac{1}{n} \sum_{i=1}^n WT_i$

### Tie-Breaking Rules
- **FCFS**: Jobs execute strictly by arrival order. If two jobs arrive at the same time, the job with the **lower Job ID** executes first.
- **Priority (Non-preemptive)**: Lower numerical value indicates higher priority (e.g., $1 > 2$).
  - Tie-break 1: Earlier arrival time wins.
  - Tie-break 2: Lower Job ID wins.
- **Round Robin (Preemptive)**:
  - Default time quantum $q = 2$ (user configurable).
  - When a job's quantum expires at time $t$, **new arrivals arriving at or before time $t$ are admitted into the ready queue before the preempted job is re-queued**.
  - Simultaneous new arrivals are queued in ascending order of arrival time, then Job ID.

### Benchmark Workload Comparison (Default 5 Jobs)
Workload: J1 ($AT=0, BT=5, Pri=2$), J2 ($AT=1, BT=3, Pri=1$), J3 ($AT=2, BT=8, Pri=4$), J4 ($AT=3, BT=6, Pri=2$), J5 ($AT=4, BT=4, Pri=2$).

| Policy | Execution Timeline | Avg Turnaround | Avg Waiting | Best Metric |
|---|---|---|---|---|
| **FCFS** | `[J1 0-5] [J2 5-8] [J3 8-16] [J4 16-22] [J5 22-26]` | 13.40 | 8.20 | Predictable order |
| **Priority** | `[J1 0-5] [J2 5-8] [J4 8-14] [J5 14-18] [J3 18-26]` | **12.20** | **7.00** | **Lowest Avg WT & TAT** |
| **Round Robin ($q=2$)** | Slices: `[J1 0-2] [J2 2-4] [J3 4-6] [J1 6-8] [J4 8-10]...` | 17.80 | 12.60 | High responsiveness |
| **Round Robin ($q=4$)** | Slices: `[J1 0-4] [J2 4-7] [J3 7-11] [J4 11-15] [J5 15-19]...` | 17.20 | 12.00 | Reduced context switches |

---

## 6. Banker's Algorithm & Resource Safety

### Safety Analysis
- **Available**: Resources currently free in the system.
- **Max**: Maximum resource claim declared by each process.
- **Allocation**: Resources currently held by each process.
- **Need**: Remaining resources required to complete:
  $$\text{Need}[i][j] = \text{Max}[i][j] - \text{Allocation}[i][j]$$

### Safe State Verification
Given 5 processes, 3 resource types, and $\text{Available} = [3, 3, 2]$:
1. $P_1$ needs $[1, 2, 2] \le [3, 3, 2] \implies \text{Work} = [3, 3, 2] + [2, 0, 0] = [5, 3, 2]$.
2. $P_3$ needs $[0, 1, 1] \le [5, 3, 2] \implies \text{Work} = [5, 3, 2] + [2, 1, 1] = [7, 4, 3]$.
3. $P_4$ needs $[4, 3, 1] \le [7, 4, 3] \implies \text{Work} = [7, 4, 3] + [0, 0, 2] = [7, 4, 5]$.
4. $P_0$ needs $[7, 4, 3] \le [7, 4, 5] \implies \text{Work} = [7, 4, 5] + [0, 1, 0] = [7, 5, 5]$.
5. $P_2$ needs $[6, 0, 0] \le [7, 5, 5] \implies \text{Work} = [7, 5, 5] + [3, 0, 2] = [10, 5, 7]$.
- **Valid Safe Sequence**: $\langle P_1, P_3, P_4, P_0, P_2 \rangle$.

### Unsafe State Verification
Given 4 processes, 3 resource types, and $\text{Available} = [0, 1, 0]$:
- Every process requires at least 2 units of $R_0$ and 1 unit of $R_2$.
- With 0 units of $R_0$ and $R_2$ available, no process can satisfy $\text{Need} \le \text{Work}$.
- Program flags the state as **UNSAFE** and warns that deadlock is possible.

---

## 7. Practical Demonstration & Live Coding Quick Guide

When the examiner asks live coding or practical questions, execute the corresponding command:

| Examiner Prompt | Action / Command | Explanation |
|---|---|---|
| **1. Add a new job and run FCFS** | Option `3` (`--add-job`), enter Arrival=5, Burst=4, Priority=1. Then Option `7` (`--fcfs`). | Adds Job 6 to the existing workload without losing previous jobs, runs FCFS, showing Job 6 executing at the end of the timeline ($t=26$ to $30$). |
| **2. Change RR quantum and show difference** | `./campuscompute --rr 2`<br>`./campuscompute --rr 4` | Demonstrates that increasing quantum from 2 to 4 drops timeline slices from 14 to 8, reducing preemption overhead and decreasing average turnaround time from 17.80 to 17.20. |
| **3. Change job priority and run Priority** | Option `4` (`--edit-priority`), pick Job 4, set priority to `1`. Then Option `9` (`--priority`). | Job 4's priority becomes 1. At $t=5$, Job 2 and Job 4 both have Priority 1. By arrival tie-break ($AT=1 < AT=3$), Job 2 runs first, then Job 4 runs next at $t=8$. |
| **4. Compare all scheduling policies** | `./campuscompute --compare` | Runs FCFS, Priority, and Round Robin on the same job set and presents a comparative summary of average turnaround and waiting times. |
| **5. Demonstrate Producer-Consumer** | `./campuscompute --logger` | Shows queue-full blocking (`queue full (8/8), waiting for consumer`), condition variable signaling, and orderly shutdown with 17 events written to `campuscompute.log`. |
| **6. Demonstrate Banker's safe & unsafe** | `./campuscompute --banker-safe`<br>`./campuscompute --banker-unsafe` | Shows complete safe sequence $\langle P_1, P_3, P_4, P_0, P_2 \rangle$ for safe state, and blocked deadlock state for unsafe state. |
| **7. Demonstrate Worker Failure Detection** | `./campuscompute --worker-fail` | Child process deliberately exits with status 42 (`SIMULATED FAILURE`). Parent catches it via `waitpid()` and reports `FAILED (detected from non-zero exit status)`. |
| **8. Demonstrate Resource Limit** | `./campuscompute --rlimit` | Demonstrates `setrlimit(RLIMIT_CPU)` soft limit of 1s. After 1s of CPU spinning, Linux kernel terminates child with `SIGXCPU` (signal 24). |

---

## 8. Official Screenshot Checklist (S01 to S24)

> [!IMPORTANT]
> Configure your prompt before taking screenshots:
> ```bash
> export SRN='PES2UG25CS606'
> export PS1='[$SRN] \w\$ '
> ```
> Combine all screenshots sequentially into a single PDF named `<YOUR_SRN>.pdf` (e.g., `PES2UG25CS606.pdf`). Each screenshot must be accompanied by the exact one-sentence explanation below.

| Screenshot # | Execution Command | Required One-Line Explanation |
|---|---|---|
| **S01** | `pwd; echo $SRN` | S01 – Environment and SRN prompt: Demonstrates the Ubuntu/WSL terminal, full SRN in the prompt, and current working directory. |
| **S02** | `gcc --version; make --version` | S02 – Compiler and build tools: Confirms availability of GCC C99 compiler and Make build utility. |
| **S03** | `ls -la` | S03 – Project files: Displays the project directory structure, source files, header files, and Makefile. |
| **S04** | `make clean; make` | S04 – Clean build: Demonstrates clean compilation from source with all compiler warnings enabled (`-Wall -Wextra`) and zero errors. |
| **S05** | `./campuscompute --help` | S05 – CLI help/usage: Displays available command-line flags, options, and usage instructions. |
| **S06** | `./campuscompute --fcfs` | S06 – FCFS scheduling: Shows FCFS execution order, completion time, turnaround time, waiting time, and calculated averages. |
| **S07** | `./campuscompute --rr` | S07 – Round Robin scheduling: Shows Round Robin execution order, time quantum of 2, and calculated scheduling metrics. |
| **S08** | `./campuscompute --priority` | S08 – Priority scheduling: Demonstrates non-preemptive priority scheduling, tie-breaking rules, and calculated metrics. |
| **S09** | `./campuscompute --compare` | S09 – Scheduling comparison: Compares FCFS, Priority, and Round Robin algorithms using the exact same job workload. |
| **S10** | `./campuscompute --logger` | S10 – Producer-consumer logger: Shows producer threads populating the bounded buffer and queue-full waiting states. |
| **S11** | `./campuscompute --logger` | S11 – Thread synchronization and termination: Demonstrates orderly consumer extraction, condition variable signaling, and clean thread joins. |
| **S12** | `./campuscompute --ipc` | S12 – Shared-memory IPC: Demonstrates inter-process communication between parent and child processes via POSIX shared memory. |
| **S13** | `./campuscompute --ipc` | S13 – Semaphore coordination and cleanup: Shows synchronization using POSIX named semaphores and unlinking of IPC resources. |
| **S14** | `./campuscompute --banker-safe` | S14 – Banker's safe state: Demonstrates Banker's safety algorithm identifying a safe resource state with a valid safe sequence. |
| **S15** | `./campuscompute --banker-unsafe`| S15 – Banker's unsafe state: Demonstrates Banker's algorithm correctly detecting an unsafe state with deadlock potential. |
| **S16** | `./campuscompute --worker-ok` | S16 – Normal worker process: Shows child process creation using `fork()` and successful normal completion with exit status 0. |
| **S17** | `./campuscompute --worker-fail` | S17 – Simulated worker failure: Demonstrates simulated worker failure exit code 42 and parent-side detection via `waitpid()`. |
| **S18** | `./campuscompute --rlimit` | S18 – Resource-limit demonstration: Demonstrates process CPU quota enforcement using `setrlimit()` and termination by kernel `SIGXCPU`. |
| **S19** | `./campuscompute --bogus` | S19 – Invalid input/command: Shows error handling and helpful usage messages when invalid command-line arguments are provided. |
| **S20** | `./campuscompute --cleanup; ls /dev/shm \| grep cc_` | S20 – Cleanup verification: Verifies that all created child processes have ended and all program IPC objects are removed from `/dev/shm`. |
| **S21** | `./campuscompute --demo` | S21 – Integrated execution: Shows the comprehensive automated test suite verifying all 14 project modules with PASS status. |
| **S22** | `cat campuscompute.log` | S22 – Generated logs: Displays the persistent timestamped log file created by the multithreaded logger module. |
| **S23** | `ls -la` | S23 – Final source and build files: Shows the final project directory with compiled binary, source files, and build assets. |
| **S24** | `make zip` | S24 – Submission ZIP: Demonstrates automated generation of the submission ZIP archive and verification of archive integrity. |

---

## 9. Submission Verification

To prepare the final submission archive:
```bash
make zip
```
Output:
```
zip -r PES2UG25CS606.zip main.c input.c scheduler.c logger.c ipc.c banker.c worker.c rlimit.c common.h scheduler.h logger.h ipc.h banker.h worker.h rlimit.h Makefile README.md VIVA_GUIDE.md PROJECT_STUDY_GUIDE.md
Verifying archive integrity:
unzip -t PES2UG25CS606.zip
No errors detected in compressed data of PES2UG25CS606.zip.
```

---

## 10. Authors & Acknowledgments

- **Virag Minche** (`PES2UG25CS606`) — Department of Computer Science and Engineering, PES University
- **Yashwanth Karthik** (`PES2UG25CS615`) — Department of Computer Science and Engineering, PES University
- **Course Instructor**: Dr. Gokulakrishnan S, Department of Computer Science and Engineering, PES University

---

## Live Demo Without Hardcoded Data

The built-in 5-job workload and Banker states are only defaults. For a live demo, feed the program data it has never seen:

```bash
# Scheduler on a workload the examiner dictates (file: "arrival burst priority" per line)
./campuscompute --jobs samples/jobs_lab.txt --compare
./campuscompute --jobs samples/jobs_lab.txt --rr 3        # chain: load, then run RR with quantum 3

# Random workload; the seed is printed so you can reproduce it, or set SEED=7 for a fixed one
./campuscompute --random 8 --compare
SEED=7 ./campuscompute --random 6 --fcfs

# Banker's algorithm on a state file (P R, Allocation, Max, Available; '#' comments allowed)
./campuscompute --banker-file samples/banker_unsafe.txt
```

Interactive menu equivalents: `jobs`, `random`, `banker-file`, `input`, `add-job`, `banker-input`, `logger-custom`, `logger-msg`, `ipc-custom`, `worker-custom`.
Suggested viva flow: let the examiner type a job list into a file or the `input` command, run all three policies, and verify one Gantt chart by hand on the board.

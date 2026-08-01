# Isolyx

Isolyx is a high-performance, low-level Linux job execution and containerization engine built natively in modern C++. It leverages raw Linux OS primitives—rather than relying on heavy abstractions or third-party container runtimes—to provide secure, time-sliced, and strictly resourced execution environments for untrusted workloads.

Status: Under Active Development (Production-ready core).

## Core Architecture & Features

Isolyx is architected using strict Object-Oriented principles and modern C++ (C++17) design patterns (Strategy, Observer) to decouple mechanisms from policies. 

### 1. Process Isolation (Linux Namespaces)
Uses the native `clone()` system call to spawn isolated processes with their own `CLONE_NEWPID`, `CLONE_NEWUTS`, `CLONE_NEWNS`, `CLONE_NEWUSER`, and `CLONE_NEWIPC` namespaces. The engine dynamically configures user/group mapping so it can run securely without requiring `root` privileges. A strictly enforced "async-signal-safe" boundary ensures safe execution between `clone()` and `execvp()`.

### 2. Hardware Resource Capping (Cgroups v2)
Employs Linux Control Groups v2 (`/sys/fs/cgroup`) using strictly enforced RAII wrappers to guarantee resource cleanup. It enforces hard caps on:
- `cpu.max`: Time-sliced CPU burst limits
- `memory.max`: Strict RAM ceilings
- `memory.swap.max`: Prevent swapping to bypass memory limits
- `pids.max`: Protection against fork bombs

### 3. System Call Filtering (Seccomp BPF)
Integrates `libseccomp` to construct Berkeley Packet Filter (BPF) denylists, blocking potentially hazardous system calls (`ptrace`, `mount`, `reboot`) at the kernel level. Malicious processes are instantaneously terminated via `SIGSYS`.

### 4. Anomaly-based Syscall Watchdog
Incorporates a heuristic-based profiler that dynamically samples `/proc/<pid>/syscall` out-of-band to detect abnormal execution patterns. If a sandbox (e.g., a simple sorting task) unexpectedly starts spamming network-related system calls, the watchdog immediately terminates it via `SIGKILL`.

### 5. Advanced CPU Scheduling
Features a pluggable scheduling system (`IScheduler` interface) decoupling the job queue from the execution context. Supported algorithms include:
- First-Come, First-Served (FCFS)
- Round Robin (Real kernel-level preemption via `SIGSTOP`/`SIGCONT`)
- Multi-Level Feedback Queue (MLFQ) with dynamic priority demotion

### 5. Decoupled Observability & Dashboard
Implements the Observer pattern to dispatch execution results. The `DashboardObserver` serializes events into JSON and streams them through a zero-dependency Linux Named Pipe (FIFO). A Node.js backend broadcasts these events via WebSockets to a minimal, responsive React frontend.

## Prerequisites

- Linux operating system (Tested on Arch Linux)
- GCC (with C++17 support)
- CMake (3.10+)
- `libseccomp` (`pacman -S libseccomp` on Arch)
- Node.js & npm (for the dashboard)

## Build Instructions

Compile the C++ core engine and tests:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

Compile the test payloads:
```bash
./tests/build_tests.sh
```

## Usage

### 1. Running the CLI Engine
Start the interactive Isolyx shell:
```bash
cd build
./isolyx
```

Inside the shell, you can dispatch background jobs to the worker pool:
```bash
isolyx> tests/cpu_hog &
isolyx> tests/memory_bomb &
isolyx> tests/denylist_syscall &
```

### 2. Running the Scheduler Benchmark
Execute the automated benchmarking suite to verify the time-slicing and preemption efficiency of the Round Robin and MLFQ schedulers against FCFS:
```bash
cd build
./isolyx_benchmark
```

### 3. Running the Live Dashboard
The web dashboard allows real-time observability of jobs passing through the Isolyx engine without polluting terminal output.

Start the API Backend (Terminal 1):
```bash
cd dashboard/api
npm install
npm start
```

Start the React Frontend (Terminal 2):
```bash
cd dashboard/web
npm install
npm run dev
```

Open your browser to `http://localhost:5173`. When you execute jobs in the Isolyx CLI, the dashboard will update automatically.

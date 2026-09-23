# Linux Kernel Module & POSIX Concurrency Engine

## Overview
This repository contains a two-part systems engineering project demonstrating low-level Linux kernel development and high-performance, multi-process concurrency. 

The system is designed to parallelize large-scale data processing across multiple child processes using POSIX shared memory, while a custom Linux kernel module dynamically tracks and extracts process control block (PCB) data directly from the kernel space.

## Architecture & Implementation

### 1. User-Space Concurrency Engine (`findmax`)
* **Multi-Processing:** Spawns dynamically configurable child processes to divide and conquer large datasets.
* **IPC & Synchronization:** Utilizes POSIX shared memory segments to aggregate results, strictly synchronized using mutexes and condition variables to prevent race conditions.
* **Resource Management:** Ensures zero memory leaks by gracefully tearing down shared memory and cleaning up intermediate I/O states upon parent termination.

### 2. Custom Linux Kernel Module (`processinfo`)
* **Kernel Space Extraction:** Intercepts process control blocks (`task_struct`) to extract virtual memory size, thread counts, priority, and state.
* **Data Structuring:** Implements the Linux kernel's native Red-Black Tree (`rbtree`) API to store and index active user-space processes by PID.
* **/proc File System Integration:** Exposes the kernel-level data to user-space applications via a custom `/proc/processinfo` endpoint.

## Build & Execution
Compiled and tested on Ubuntu 22.04 (x86-64).

```bash
# Build the user-space application and kernel module
make

# Run the concurrency engine (e.g., 4 processes, top 10 values)
./findmax -t 10 -c 4 -i input.txt -o out.txt

# Load the kernel module
sudo insmod processinfo.ko

```
## Benchmarking & Performance
The system was benchmarked on an Ubuntu 22.04 LTS (64-bit) 8-core virtual machine using a 10,000,000 integer dataset. See `Performance_Analysis.pdf` for full caching and context-switching metrics.

| Child Processes (N) | Execution Time (s) | Core Utilization Notes |
| :--- | :--- | :--- |
| 1 | 0.024020 | Sequential baseline |
| 8 | 0.013654 | Optimal parallelism (matches VM core count) |
| 10 | 0.019294 | Core oversubscription; OS context-switching overhead |
| 20 | 0.029956 | Subsystem capacity exceeded; severe cache thrashing |

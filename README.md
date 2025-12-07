# SRTF Multithreaded Scheduler 

This program implements Shortest Remaining Time First (SRTF) CPU scheduling using a multithreaded approach. Each process is represented by a worker thread that runs one time unit when signaled by a central scheduler. The scheduler maintains the current time, selects the ready process with the minimum remaining time each tick, and logs a timeline and final performance metrics.

## Notable Features

- Cross‑platform threading via compile‑time OS detection:
  - Windows: `CreateThread`, `CRITICAL_SECTION`, `CONDITION_VARIABLE`
  - macOS/Linux: `pthread_create`, `pthread_mutex_t`, `pthread_cond_t`
- Validates inputs (number of processes, arrival/burst times)
- Prints aligned timeline of events (Running, Completed, Idle)
- Computes and prints per‑process `Turnaround`, `Waiting`, `Response` times
- Renders a simple ASCII Gantt chart of the execution history

## Build

- macOS/Linux:
  - `gcc srtf_mt.c -o srtf_mt -lpthread`

- Windows (MinGW):
  - `gcc srtf_mt.c -o srtf_mt`
  - Ensure Windows SDK headers and libraries are available in your toolchain.

- Windows (MSVC):
  - Create a Console project and add `srtf_mt.c`
  - Build normally; Windows synchronization primitives are part of the OS SDK.

## Run

1. Execute the binary: `./srtf_mt`
2. Enter the number of processes (1–10)
3. Provide arrival time (≥ 0) and burst time (> 0) for each `P1..Pn`
4. Observe the aligned event log, performance summary, and the Gantt chart

## Output

- Timeline (example):
  - `Time   Process ID   Status        Remaining Time`
  - `0      P1           Running       5`
  - `1      P2           Ready         4` (if logged in your version)
  - `5      P1           Completed     0`

- Performance Results:
  - `Process i: Turnaround = X, Waiting = Y, Response = Z`
  - `Average Turnaround Time = A`
  - `Average Waiting Time = B`

- Gantt Chart:
  - ASCII bars showing contiguous runs per process, with time markers aligned under block boundaries.


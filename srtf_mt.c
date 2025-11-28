#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Creating a loop to differentiate between the user's OS and loading the respective thread libraries. 
// used wrappers to ensure compatibility between the two OSs between function names 
#ifdef _WIN32
    #include <windows.h>
    
    typedef HANDLE ThreadHandle;
    typedef CRITICAL_SECTION MutexHandle;
    typedef CONDITION_VARIABLE CondHandle;

    // Wrapper for thread function signature
    #define THREAD_FUNC_RETURN DWORD WINAPI
    #define THREAD_FUNC_ARG LPVOID

    // Mutex Operations
    void init_mutex(MutexHandle *m) { InitializeCriticalSection(m); }
    void lock_mutex(MutexHandle *m) { EnterCriticalSection(m); }
    void unlock_mutex(MutexHandle *m) { LeaveCriticalSection(m); }
    void destroy_mutex(MutexHandle *m) { DeleteCriticalSection(m); }

    // Condition Variable Operations
    void init_cond(CondHandle *c) { InitializeConditionVariable(c); }
    void wait_cond(CondHandle *c, MutexHandle *m) { SleepConditionVariableCS(c, m, INFINITE); }
    void signal_cond(CondHandle *c) { WakeConditionVariable(c); }
    void destroy_cond(CondHandle *c) { /* No op for Windows Condition Variables */ }

    // Thread Operations
    void create_thread(ThreadHandle *t, THREAD_FUNC_RETURN (*func)(THREAD_FUNC_ARG), void *arg) {
        *t = CreateThread(NULL, 0, func, arg, 0, NULL);
    }
    void join_thread(ThreadHandle t) {
        WaitForSingleObject(t, INFINITE);
        CloseHandle(t);
    }

#else
    #include <pthread.h>
    #include <unistd.h>

    typedef pthread_t ThreadHandle;
    typedef pthread_mutex_t MutexHandle;
    typedef pthread_cond_t CondHandle;

    // Wrapper for thread function signature
    #define THREAD_FUNC_RETURN void*
    #define THREAD_FUNC_ARG void*

    // Mutex Operations
    void init_mutex(MutexHandle *m) { pthread_mutex_init(m, NULL); }
    void lock_mutex(MutexHandle *m) { pthread_mutex_lock(m); }
    void unlock_mutex(MutexHandle *m) { pthread_mutex_unlock(m); }
    void destroy_mutex(MutexHandle *m) { pthread_mutex_destroy(m); }

    // Condition Variable Operations
    void init_cond(CondHandle *c) { pthread_cond_init(c, NULL); }
    void wait_cond(CondHandle *c, MutexHandle *m) { pthread_cond_wait(c, m); }
    void signal_cond(CondHandle *c) { pthread_cond_signal(c); }
    void destroy_cond(CondHandle *c) { pthread_cond_destroy(c); }

    // Thread Operations
    void create_thread(ThreadHandle *t, THREAD_FUNC_RETURN (*func)(THREAD_FUNC_ARG), void *arg) {
        pthread_create(t, NULL, func, arg);
    }
    void join_thread(ThreadHandle t) {
        pthread_join(t, NULL);
    }
#endif

struct process {
    int pid;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int start_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int response_time;
    bool is_completed;
    
    // Thread specific handles
    ThreadHandle thread_id;
    CondHandle my_cond; // Worker waits on this
};

// Global Scheduling Context
struct Scheduler {
    int n;
    int current_time;
    int completed_count;
    int active_process_index; 
    
    struct process *processes;
    
    MutexHandle lock;          // Protects all shared state
    CondHandle scheduler_cond; // Scheduler waits on this for worker to finish a tick
};

struct Scheduler sched;

// worker thread function
THREAD_FUNC_RETURN Process_Worker(THREAD_FUNC_ARG arg) {
    struct process *p = (struct process *)arg;
    int my_index = -1;

    // Find my index to verify identity against scheduler
    // (This is just a helper, ideally index is passed in args)
    lock_mutex(&sched.lock);
    for(int i=0; i<sched.n; i++) {
        if(&sched.processes[i] == p) {
            my_index = i;
            break;
        }
    }
    unlock_mutex(&sched.lock);

    while (true) {
        lock_mutex(&sched.lock);

        // Wait until Scheduler tells THIS specific process to run
        while (sched.active_process_index != my_index) {
            wait_cond(&p->my_cond, &sched.lock);
        }
      
        // If we are resumed but completed (shouldn't happen in this logic, but safe guard)
        if (p->remaining_time <= 0) {
            sched.active_process_index = -1; // Yield back
            signal_cond(&sched.scheduler_cond);
            unlock_mutex(&sched.lock);
            break;
        }

        // Check if this is the first time running
        if (p->remaining_time == p->burst_time) {
            p->start_time = sched.current_time;
        }

        // Decrement time 
        p->remaining_time--;
        
        // We set active to -1 so we don't accidentally run again without permission
        sched.active_process_index = -1; 
        signal_cond(&sched.scheduler_cond);

        // Check completion logic inside the locked region
        if (p->remaining_time == 0) {
            p->is_completed = true;
            p->completion_time = sched.current_time + 1; // +1 because current_time is at start of tick
            
            // Calculate Metrics
            p->turnaround_time = p->completion_time - p->arrival_time;
            p->waiting_time = p->turnaround_time - p->burst_time;
            p->response_time = p->start_time - p->arrival_time;
            
            // Atomic increment of global counter
            sched.completed_count++;
            
            unlock_mutex(&sched.lock);
            return 0; // Thread Exit
        }

        unlock_mutex(&sched.lock);
    }
    return 0;
}

int main() {
    // Initialization
    init_mutex(&sched.lock);
    init_cond(&sched.scheduler_cond);
    
    printf("Enter number of processes: ");
    if (scanf("%d", &sched.n) != 1 || sched.n <= 0) {
        return 1;
    }

    sched.processes = (struct process *)malloc(sizeof(struct process) * sched.n);
    sched.current_time = 0;
    sched.completed_count = 0;
    sched.active_process_index = -1; // No one is running initially

    // Input & Thread Creation
    for (int i = 0; i < sched.n; i++) {
        sched.processes[i].pid = i + 1;
        printf("Enter arrival time P%d: ", i + 1);
        scanf("%d", &sched.processes[i].arrival_time);
        printf("Enter burst time P%d: ", i + 1);
        scanf("%d", &sched.processes[i].burst_time);

        sched.processes[i].remaining_time = sched.processes[i].burst_time;
        sched.processes[i].start_time = -1;
        sched.processes[i].is_completed = false;
        
        // Initialize per-process condition variable
        init_cond(&sched.processes[i].my_cond);
        
        // Create the thread (It will immediately wait on its cond var)
        create_thread(&sched.processes[i].thread_id, Process_Worker, &sched.processes[i]);
    }

    printf("\n%-6s %-12s %-12s %-15s\n", "Time", "Process ID", "Status", "Remaining Time");

    // SRTF Dispatcher Loop
    int prev_running_pid = -1;

    while (sched.completed_count != sched.n) {
        lock_mutex(&sched.lock);

        // Find shortest job that has arrived
        int min_index = -1;
        int min_remaining = 2147483647;

        for (int i = 0; i < sched.n; i++) {
            if (!sched.processes[i].is_completed && 
                 sched.processes[i].arrival_time <= sched.current_time && 
                 sched.processes[i].remaining_time > 0) {
                
                if (sched.processes[i].remaining_time < min_remaining) {
                    min_remaining = sched.processes[i].remaining_time;
                    min_index = i;
                }
                else if (sched.processes[i].remaining_time == min_remaining) {
                    if (sched.processes[i].arrival_time < sched.processes[min_index].arrival_time) {
                        min_index = i;
                    }
                }
            }
        }

        if (min_index != -1) {
            // Found a process to run
            struct process *p = &sched.processes[min_index];

            // Print status
            if (p->remaining_time == p->burst_time || prev_running_pid != p->pid) {
                printf("%-6d P%-11d %-12s %-15d\n", sched.current_time, p->pid, "Running", p->remaining_time);
            }
            prev_running_pid = p->pid;

            // Set global flag indicating who should run
            sched.active_process_index = min_index;
            
            // Wake up THAT specific worker thread
            signal_cond(&p->my_cond);
            
            // Wait for the worker to finish its time slice
            // The scheduler sleeps here while the worker runs
            while (sched.active_process_index != -1) {
                wait_cond(&sched.scheduler_cond, &sched.lock);
            }
            
            // Worker has finished 1 tick and signaled us back
            if (p->is_completed) {
                printf("%-6d P%-11d %-12s %-15d\n", sched.current_time + 1, p->pid, "Completed", 0);
            }

        } 
        else {
            printf("%-6d %-12s %-12s %-15s\n", sched.current_time, "IDLE", "-", "-");
        }

        // Advance time
        sched.current_time++;
        unlock_mutex(&sched.lock);
    }

    // Join all threads
    for (int i = 0; i < sched.n; i++) {
        join_thread(sched.processes[i].thread_id);
        destroy_cond(&sched.processes[i].my_cond);
    }

    // Print Metrics
    printf("\nSRTF Performance Results:\n");
    double sum_tat = 0, sum_wt = 0;
    for (int i = 0; i < sched.n; i++) {
        sum_tat += sched.processes[i].turnaround_time;
        sum_wt += sched.processes[i].waiting_time;
        printf("Process %d: Turnaround = %d, Waiting = %d, Response = %d\n",
               sched.processes[i].pid,
               sched.processes[i].turnaround_time,
               sched.processes[i].waiting_time,
               sched.processes[i].response_time);
    }
    printf("\nAverage Turnaround Time = %.2f\n", sum_tat / sched.n);
    printf("Average Waiting Time = %.2f\n", sum_wt / sched.n);

    // Free resources
    free(sched.processes);
    destroy_mutex(&sched.lock);
    destroy_cond(&sched.scheduler_cond);

    return 0;
}
#include <stdio.h>
#include <stdbool.h>

struct process
{
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
};

int completed = 0;
int current_time = 0;

int main()
{
    // creates a struct to hold the process details 
    // user input for number of processes such that the struct can be created for that length
    int n;
    printf("Enter number of processes: ");
    if (scanf("%d", &n) != 1 || n <= 0)
    {
        fprintf(stderr, "Invalid number of processes\n");
        return 1;
    }

    struct process processes[n];

    // takes user input for the arrival time and burst time for each process
    for (int i = 0; i < n; i++)
    {
        processes[i].pid = i + 1;

        printf("Enter arrival time %d: ", i + 1);
        if (scanf("%d", &processes[i].arrival_time) != 1)
        {
            fprintf(stderr, "Invalid input\n");
            return 1;
        }

        printf("Enter burst time %d: ", i + 1);
        if (scanf("%d", &processes[i].burst_time) != 1)
        {
            fprintf(stderr, "Invalid input\n");
            return 1;
        }

        // calculate the default metrics for each process
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].start_time = -1;
        processes[i].completion_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].waiting_time = 0;
        processes[i].response_time = 0;
        processes[i].is_completed = false;
    }

    completed = 0;
    current_time = 0;
    int prev_running_pid = -1;
    printf("%-6s %-12s %-12s %-15s\n", "Time", "Process ID", "Status", "Remaining Time");
    while (completed != n)
    {
        int min_index = -1;
        int min_remaining = 1000000000; // just a big value
        for (int i = 0; i < n; i++)
        {
            // makes sure the process is not completed, has arrived, and has remaining time
            if (!processes[i].is_completed && processes[i].arrival_time <= current_time && processes[i].remaining_time > 0)
            {
                // finds the process with the minimum remaining time
                if (processes[i].remaining_time < min_remaining)
                {
                    min_remaining = processes[i].remaining_time;
                    min_index = i;
                }
            }
        }

        for (int i = 0; i < n; i++)
        {
            // logs the process that is ready to run
            if (processes[i].arrival_time == current_time && !processes[i].is_completed)
            {
                printf("%-6d P%-11d %-12s %-15d\n", current_time, processes[i].pid, "Ready", processes[i].remaining_time);
            }
        }

        if (min_index != -1)
        {
            int running_pid = processes[min_index].pid;

            // logs the process that is running
            if (processes[min_index].remaining_time == processes[min_index].burst_time || prev_running_pid != running_pid)
            {
                printf("%-6d P%-11d %-12s %-15d\n", current_time, running_pid, "Running", processes[min_index].remaining_time);
            }

            // checks if the process is running for the first time
            if (processes[min_index].remaining_time == processes[min_index].burst_time)
            {
                processes[min_index].start_time = current_time;
            }

            processes[min_index].remaining_time -= 1;
            current_time++;

            prev_running_pid = running_pid;

            if (processes[min_index].remaining_time == 0)
            {
                // updates the metrics and logs the processes that are completed
                processes[min_index].completion_time = current_time;
                processes[min_index].turnaround_time = processes[min_index].completion_time - processes[min_index].arrival_time;
                processes[min_index].waiting_time = processes[min_index].turnaround_time - processes[min_index].burst_time;
                processes[min_index].response_time = processes[min_index].start_time - processes[min_index].arrival_time;
                processes[min_index].is_completed = true;
                printf("%-6d P%-11d %-12s %-15d\n", current_time, running_pid, "Completed", 0);
                completed++;
            }
        }
        else
        {
            current_time++;
        }
    }

    // printing performance results
    printf("\nSRTF Performance Results:\n");
    int sum_tat = 0, sum_wt = 0;
    for (int i = 0; i < n; i++)
    {
        sum_tat += processes[i].turnaround_time;
        sum_wt += processes[i].waiting_time;
        printf("Process %d: Turnaround = %d, Waiting = %d, Response = %d\n",
               processes[i].pid,
               processes[i].turnaround_time,
               processes[i].waiting_time,
               processes[i].response_time);
    }
    printf("\nAverage Turnaround Time = %.2f\n", (double)sum_tat / n);
    printf("Average Waiting Time = %.2f\n", (double)sum_wt / n);
}






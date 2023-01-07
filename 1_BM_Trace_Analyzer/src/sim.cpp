// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// DO NOT MODIFY OR SUBMIT THIS FILE. //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

// sim.cpp
// Reads and analyzes a CPU trace file for ECE 4100/6100.
// Author: Rishov Sarkar

#include "trace.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/** Total number of instructions executed. Updated in this file. */
extern uint64_t stat_num_inst;

/**
 * Array of number of instructions executed by op type.
 *
 * For instance, stat_optype_dyn[OP_ALU] should be the number of times an ALU
 * instruction was executed in the trace.
 *
 * Updated by student code.
 */
extern uint64_t stat_optype_dyn[NUM_OP_TYPES];

/** Total number of CPU cycles. Updated by student code. */
extern uint64_t stat_num_cycle;

/**
 * Total number of unique instructions executed.
 *
 * Instructions with the same address should only be counted once.
 *
 * Updated by student code.
 */
extern uint64_t stat_unique_pc;

int read_trace(int fd);
void print_stats();

int main(int argc, char *argv[])
{
    int status;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <trace file>\n", argv[0]);
        return 2;
    }

    // Open the trace file using gunzip.
    // Uses the traditional pipe/fork/exec method.
    printf("Opening trace file with gunzip: %s\n", argv[1]);

    int pipefd[2];
    status = pipe(pipefd);
    if (status != 0)
    {
        perror("Couldn't create pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("Couldn't fork");
        return 1;
    }

    if (pid == 0)
    {
        // Child process: exec gunzip.
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execlp("gunzip", "gunzip", "-c", argv[1], NULL);
        perror("Couldn't exec gunzip");
        fprintf(stderr, "Is gunzip installed?\n");
        return 127;
    }

    // Parent process: read the trace file.
    int trace_fd = pipefd[0];
    close(pipefd[1]);
    status = read_trace(trace_fd);
    close(trace_fd);
    if (status != 0)
    {
        return 1;
    }

    // Wait for the child process to finish.
    waitpid(pid, &status, 0);
    status = WEXITSTATUS(status);
    if (status == 127)
    {
        return 1;
    }

    // Print statistics.
    print_stats();
    return 0;
}

int read_trace(int fd)
{
    TraceRec trace_record;
    while (true)
    {
        ssize_t bytes_read = read(fd, &trace_record, sizeof(trace_record));
        if (bytes_read == 0)
        {
            return 0;
        }
        if (bytes_read == -1)
        {
            perror("Couldn't read from pipe");
            return -1;
        }
        if (bytes_read != sizeof(trace_record) || trace_record.optype >= NUM_OP_TYPES)
        {
            fprintf(stderr, "Error: Invalid trace file\n");
            return -1;
        }

        // Update statistics.
        stat_num_inst++;
        analyze_trace_record(&trace_record);
    }
}

void print_stats()
{
    if (stat_num_inst == 0)
    {
        fprintf(stderr, "Warning: No instructions found in trace file. "
                        "Is this a valid trace file?\n");
        stat_num_inst = 1; // Avoid division by zero.
    }

    double cpi = (double)stat_num_cycle / (double)stat_num_inst;

    printf("\n");

    printf("LAB1_NUM_INST           \t : %10lu\n", stat_num_inst);
    printf("LAB1_NUM_CYCLES         \t : %10lu\n", stat_num_cycle);

    printf("LAB1_CPI                \t : %6.3f\n", cpi);
    printf("LAB1_UNIQUE_PC          \t : %10lu\n", stat_unique_pc);

    printf("\n");

    printf("LAB1_NUM_ALU_OP         \t : %10lu\n", stat_optype_dyn[OP_ALU]);
    printf("LAB1_NUM_LD_OP          \t : %10lu\n", stat_optype_dyn[OP_LD]);
    printf("LAB1_NUM_ST_OP          \t : %10lu\n", stat_optype_dyn[OP_ST]);
    printf("LAB1_NUM_CBR_OP         \t : %10lu\n", stat_optype_dyn[OP_CBR]);
    printf("LAB1_NUM_OTHER_OP       \t : %10lu\n", stat_optype_dyn[OP_OTHER]);

    printf("\n");

    printf("LAB1_PERC_ALU_OP        \t : %6.3f\n", 100.0 * (double)(stat_optype_dyn[OP_ALU]) / (double)(stat_num_inst));
    printf("LAB1_PERC_LD_OP         \t : %6.3f\n", 100.0 * (double)(stat_optype_dyn[OP_LD]) / (double)(stat_num_inst));
    printf("LAB1_PERC_ST_OP         \t : %6.3f\n", 100.0 * (double)(stat_optype_dyn[OP_ST]) / (double)(stat_num_inst));
    printf("LAB1_PERC_CBR_OP        \t : %6.3f\n", 100.0 * (double)(stat_optype_dyn[OP_CBR]) / (double)(stat_num_inst));
    printf("LAB1_PERC_OTHER_OP      \t : %6.3f\n\n", 100.0 * (double)(stat_optype_dyn[OP_OTHER]) / (double)(stat_num_inst));
}

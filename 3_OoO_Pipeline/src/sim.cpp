// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// You should not modify this file. //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

// sim.cpp
// Performs a timing simulation of an out-of-order pipelined CPU for ECE
// 4100/6100 & CS 4290/6290.

#include "pipeline.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * The width of the pipeline; that is, the maximum number of instructions that
 * can be processed during any given cycle in each of the issue, schedule, and
 * commit stages of the pipeline.
 * 
 * (Note that this does not apply to the writeback stage: as many as
 * MAX_WRITEBACKS instructions can be written back to the ROB in a single
 * cycle!)
 * 
 * When the width is 1, the pipeline is scalar.
 * When the width is greater than 1, the pipeline is superscalar.
 * 
 * You should not modify this value directly; it is set by the command-line
 * argument -pipewidth.
 */
uint32_t PIPE_WIDTH = 1;

/**
 * The number of entries in the ROB; that is, the maximum number of
 * instructions that can be stored in the ROB at any given time.
 * 
 * You should use only this many entries of the ROB::entries array.
 * 
 * Do not modify this value.
 */
uint32_t NUM_ROB_ENTRIES = 32;

/**
 * The number of cycles an LD instruction should take to execute.
 * 
 * This is used by the code in exeq.cpp to determine how long to wait before
 * considering the execution of an LD instruction done.
 * 
 * You should not modify this value directly; it is set by the command-line
 * argument -loadlatency.
 */
uint32_t LOAD_EXE_CYCLES = 4;

/**
 * Whether to use in-order scheduling or out-of-order scheduling.
 * 
 * The possible values are SCHED_IN_ORDER for in-order scheduling and
 * SCHED_OUT_OF_ORDER for out-of-order scheduling.
 * 
 * Your implementation of pipe_cycle_sched() in pipeline.cpp should check this
 * value and implement scheduling of instructions accordingly.
 * 
 * You should not modify this value directly; it is set by the command-line
 * argument -schedpolicy.
 */
SchedulingPolicy SCHED_POLICY = SCHED_OUT_OF_ORDER;

#define HEARTBEAT_CYCLES 10000
#define STAT_CYCLES (HEARTBEAT_CYCLES * 50)

Pipeline *pipeline;
uint64_t last_hbeat_inst = 0;

int parse_args(int argc, char *argv[], char **trace_filename);
int open_gunzip_pipe(const char *filename, int *fd, pid_t *pid);
int check_heartbeat();
void print_stats();
void print_usage(char *program_name);

int main(int argc, char *argv[])
{
    int status;

    // Parse the command-line arguments.
    char *trace_filename = NULL;
    status = parse_args(argc, argv, &trace_filename);
    if (status != 0)
    {
        return status;
    }

    // Open the trace file using gunzip.
    int trace_fd;
    pid_t pid;
    printf("Opening trace file with gunzip: %s\n", trace_filename);
    status = open_gunzip_pipe(trace_filename, &trace_fd, &pid);
    if (status != 0)
    {
        return status;
    }

    // Simulate the pipeline.
    pipeline = pipe_init(trace_fd);
    status = 0;
    while (status == 0 && !pipeline->halt)
    {
        // fprintf(stderr, "CYCLE_START\n");
        pipe_cycle(pipeline);

        // fprintf(stderr, "CYCLE_M\n");
        status = check_heartbeat();
        // fprintf(stderr, "CYCLE_END\n");

    }
    // fprintf(stderr, "AMIOUT\n");
    close(trace_fd);
    if (status != 0)
    {
        waitpid(pid, NULL, 0);
        return status;
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

int parse_args(int argc, char *argv[], char **trace_filename)
{
    *trace_filename = NULL;

    if (argc < 2)
    {
        print_usage(argv[0]);
        return 2;
    }

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            // Parse options.
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0)
            {
                print_usage(argv[0]);
                return 2;
            }
            else if (strcmp(argv[i], "-pipewidth") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to -pipewidth\n");
                    return 2;
                }

                int pipe_width = atoi(argv[i]);
                if (pipe_width < 1 || pipe_width > MAX_PIPE_WIDTH)
                {
                    fprintf(stderr, "Error: pipe width must be between 1 and %d\n", MAX_PIPE_WIDTH);
                    return 2;
                }

                PIPE_WIDTH = pipe_width;
            }
            else if (strcmp(argv[i], "-loadlatency") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to -loadlatency\n");
                    return 2;
                }

                int load_exe_cycles = atoi(argv[i]);
                if (load_exe_cycles < 1)
                {
                    fprintf(stderr, "Error: load latency must be a positive integer number of cycles\n");
                    return 2;
                }

                LOAD_EXE_CYCLES = load_exe_cycles;
            }
            else if (strcmp(argv[i], "-schedpolicy") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to -schedpolicy\n");
                    return 2;
                }

                int policy = atoi(argv[i]);
                if (policy < 0 || policy >= NUM_SCHED_POLICIES)
                {
                    fprintf(stderr, "Error: invalid argument for -schedpolicy\n");
                    return 2;
                }

                SCHED_POLICY = (SchedulingPolicy)policy;
            }
            else
            {
                fprintf(stderr, "Error: unrecognized option: %s\n", argv[i]);
            }
        }
        else
        {
            // Parse trace file name.
            if (*trace_filename != NULL)
            {
                fprintf(stderr, "Error: only one trace file may be specified\n");
                return 2;
            }

            *trace_filename = argv[i];
        }
    }

    if (*trace_filename == NULL)
    {
        fprintf(stderr, "Error: no trace file specified\n");
        return 2;
    }

    return 0;
}

int open_gunzip_pipe(const char *filename, int *fd, pid_t *pid)
{
    int status;
    int pipefd[2];

    status = pipe(pipefd);
    if (status != 0)
    {
        perror("Couldn't create pipe");
        return 1;
    }

    *pid = fork();
    if (*pid == -1)
    {
        perror("Couldn't fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    if (*pid == 0)
    {
        // Child process: exec gunzip.
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execlp("gunzip", "gunzip", "-c", filename, NULL);
        perror("Couldn't exec gunzip");
        fprintf(stderr, "Is gunzip installed?\n");
        return 127;
    }

    // Parent process: return the read end of the pipe.
    *fd = pipefd[0];
    close(pipefd[1]);
    return 0;
}

int check_heartbeat()
{
    if (pipeline->stat_num_cycle % HEARTBEAT_CYCLES == 0)
    {
        // Print a heartbeat.
        printf(".");
        fflush(stdout);

        // Check for deadlock.
        if (pipeline->stat_retired_inst == last_hbeat_inst)
        {
            fprintf(stderr, "\n");
            fprintf(stderr, "Error: pipeline is deadlocked: no instructions "
                            "committed in %u cycles\n",
                    HEARTBEAT_CYCLES);
            return 1;
        }

        // Update the heartbeat info.
        last_hbeat_inst = pipeline->stat_retired_inst;
    }

    if (pipeline->stat_num_cycle % STAT_CYCLES == 0)
    {
        // Print statistics.
        uint64_t stat_num_inst = pipeline->stat_num_cycle;
        uint64_t stat_num_cycle = pipeline->stat_retired_inst;
        double cpi = (double)stat_num_inst / (double)stat_num_cycle;

        printf("\n");
        printf("(Inst: %7lu\tCycle: %7lu\tCPI: %5.3f)\n",
               (unsigned long)stat_num_cycle, (unsigned long)stat_num_inst,
               cpi);
    }

    return 0;
}

void print_stats()
{
    unsigned long stat_num_inst = pipeline->stat_retired_inst;
    unsigned long stat_num_cycle = pipeline->stat_num_cycle;
    double cpi = (double)stat_num_cycle / (double)stat_num_inst;

    printf("\n\n");
    printf("LAB3_NUM_INST           \t : %10lu\n", stat_num_inst);
    printf("LAB3_NUM_CYCLES         \t : %10lu\n", stat_num_cycle);
    printf("LAB3_CPI                \t : %10.3f\n", cpi);
    printf("\n");
}

void print_usage(char *program_name)
{
    fprintf(stderr, "Usage: %s [options] <trace file>\n\n", program_name);
    fprintf(stderr, "Trace driven pipeline simulator\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "    -pipewidth <width>  Set width of pipeline to <width> (default: 1)\n");
    fprintf(stderr, "    -schedpolicy <num>  Set scheduling policy [0: in-order, 1: out-of-order]\n");
    fprintf(stderr, "                        (default: 1)\n");
    fprintf(stderr, "    -loadlatency <num>  Set number of cycles for LD to execute (default: 4)\n");
}

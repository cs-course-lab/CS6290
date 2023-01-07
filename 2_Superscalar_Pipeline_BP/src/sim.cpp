// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// You should not modify this file. //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

// sim.cpp
// Performs a timing simulation of a pipelined CPU for ECE 4100/6100 &
// CS 4290/6290.

#include "pipeline.h"
#include "bpred.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * The width of the pipeline; that is, the maximum number of instructions that
 * can be in each stage of the pipeline at any given time.
 * 
 * When the width is 1, the pipeline is scalar.
 * When the width is greater than 1, the pipeline is superscalar.
 * 
 * You should not modify this value directly; it is set by the command-line
 * argument -pipewidth.
 */
uint32_t PIPE_WIDTH = 1;

/**
 * A Boolean indicating whether forwarding from the Memory Access stage (MA)
 * should be simulated.
 * 
 * You should not modify this value directly; it is set by the command-line
 * argument -enablememfwd.
 */
uint32_t ENABLE_MEM_FWD = 0;

/**
 * A Boolean indicating whether forwarding from the Execute stage (EX) should
 * be simulated.
 * 
 * You should not modify this value directly; it is set by the command-line
 * argument -enableexefwd.
 */
uint32_t ENABLE_EXE_FWD = 0;

/**
 * The branch prediction policy that should be simulated.
 * 
 * Refer to the BpredPolicy enumeration in bpred.h for a description of the
 * possible values.
 * 
 * You should not modify this value directly; it is set by the command-line
 * argument -bpredpolicy.
 */
BPredPolicy BPRED_POLICY = BPRED_PERFECT;

// #define HEARTBEAT_CYCLES 100
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
        pipe_cycle(pipeline);
        status = check_heartbeat();
    }
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
            else if (strcmp(argv[i], "-enablememfwd") == 0)
            {
                ENABLE_MEM_FWD = 1;
            }
            else if (strcmp(argv[i], "-enableexefwd") == 0)
            {
                ENABLE_EXE_FWD = 1;
            }
            else if (strcmp(argv[i], "-bpredpolicy") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to -bpredpolicy\n");
                    return 2;
                }

                int policy = atoi(argv[i]);
                if (policy < 0 || policy >= NUM_BPRED_POLICIES)
                {
                    fprintf(stderr, "Error: invalid argument for -bpredpolicy\n");
                    return 2;
                }

                BPRED_POLICY = (BPredPolicy)policy;
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

    printf("LAB2_NUM_INST           \t : %10lu\n", stat_num_inst);
    printf("LAB2_NUM_CYCLES         \t : %10lu\n", stat_num_cycle);
    printf("LAB2_CPI                \t : %10.3f\n", cpi);

    if (BPRED_POLICY != BPRED_PERFECT)
    {
        unsigned long stat_num_branches = pipeline->b_pred->stat_num_branches;
        unsigned long stat_num_mispred = pipeline->b_pred->stat_num_mispred;
        double bpred_mispred_rate = 100.0 * (double)stat_num_mispred / (double)stat_num_branches;

        printf("LAB2_BPRED_BRANCHES     \t : %10lu\n", stat_num_branches);
        printf("LAB2_BPRED_MISPRED      \t : %10lu\n", stat_num_mispred);
        printf("LAB2_MISPRED_RATE       \t : %10.3f\n", bpred_mispred_rate);
    }

    printf("\n");
}

void print_usage(char *program_name)
{
    fprintf(stderr, "Usage: %s [options] <trace file>\n\n", program_name);
    fprintf(stderr, "Trace driven pipeline simulator\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "    -pipewidth <width>  Set width of pipeline to <width> (Default: 1)\n");
    fprintf(stderr, "    -enablememfwd       Enable forwarding from Memory Access (MA) stage\n");
    fprintf(stderr, "                        (disabled by default)\n");
    fprintf(stderr, "    -enableexefwd       Enable forwarding from Execute (EX) stage (disabled by\n");
    fprintf(stderr, "                        default)\n");
    fprintf(stderr, "    -bpredpolicy <num>  Set branch predictor [0: Perfect, 1: Always Taken,\n");
    fprintf(stderr, "                        2: Gshare] (Default: 0)\n");
}

///////////////////////////////////////////////////////////////////////////////
// You shouldn't need to modify this file.                                   //
///////////////////////////////////////////////////////////////////////////////

// sim.cpp
// Performs a timing simulation of a memory system for ECE 4100/6100 &
// CS 4290/6290.

#include "types.h"
#include "memsys.h"
#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define MAX_CORES 2
#define PRINT_DOTS 1
#define DOT_INTERVAL 100000

/**
 * The current mode under which the simulation is running, corresponding to
 * which part of the lab is being evaluated.
 */
Mode SIM_MODE = SIM_MODE_A;

/** The number of bytes in a cache line. */
uint64_t CACHE_LINESIZE = 64;

/** The replacement policy to use for the L1 data and instruction caches. */
ReplacementPolicy REPL_POLICY = LRU;

/** The size of the data cache in bytes. */
uint64_t DCACHE_SIZE = 32 * 1024;

/** The associativity of the data cache. */
uint64_t DCACHE_ASSOC = 8;

/** The size of the instruction cache in bytes. */
uint64_t ICACHE_SIZE = 32 * 1024;

/** The associativity of the instruction cache. */
uint64_t ICACHE_ASSOC = 8;

/** The size of the L2 cache in bytes. */
uint64_t L2CACHE_SIZE = 1024 * 1024;

/** The associativity of the L2 cache. */
uint64_t L2CACHE_ASSOC = 16;

/** The replacement policy to use for the L2 cache. */
ReplacementPolicy L2CACHE_REPL = LRU;

/**
 * For static way partitioning, the quota of ways in each set that can be
 * assigned to core 0.
 * 
 * The remaining number of ways is the quota for core 1.
 * 
 * This is used to implement extra credit part E.
 */
unsigned int SWP_CORE0_WAYS = 0;

/** The number of cores being simulated. */
unsigned int NUM_CORES = 0;

/** Which page policy the DRAM should use. */
DRAMPolicy DRAM_PAGE_POLICY = OPEN_PAGE;

/**
 * The current clock cycle number.
 * 
 * This can be used as a timestamp for implementing the LRU replacement policy.
 */
uint64_t current_cycle;

MemorySystem *memsys;
Core *core[MAX_CORES];
const char *trace_filename[MAX_CORES];
uint64_t last_printdot_cycle;

int parse_args(int argc, char **argv);
void print_dots();
void print_stats();
void print_usage(const char *program_name);

int main(int argc, char **argv)
{
    int status = parse_args(argc, argv);
    if (status != 0)
    {
        return status;
    }

    srand(42);
    memsys = memsys_new();
    for (unsigned int i = 0; i < NUM_CORES; i++)
    {
        core[i] = core_new(memsys, trace_filename[i], i);
    }

    print_dots();

    // Iterate until all cores are done.
    bool all_cores_done = false;
    while (!all_cores_done)
    {
        all_cores_done = true;

        for (unsigned int i = 0; i < NUM_CORES; i++)
        {
            core_cycle(core[i]);
            all_cores_done = all_cores_done && core[i]->done;
        }

        if (current_cycle - last_printdot_cycle >= DOT_INTERVAL)
        {
            print_dots();
        }

        current_cycle++;
    }

    print_stats();
    return 0;
}

int parse_args(int argc, char **argv)
{
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
            if (strcasecmp(argv[i], "-h") == 0 ||
                strcasecmp(argv[i], "-help") == 0)
            {
                print_usage(argv[0]);
                return 2;
            }

            else if (strcasecmp(argv[i], "-mode") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to -mode\n");
                    return 2;
                }

                int mode = atoi(argv[i]);
                if (mode < 1 || mode > 4)
                {
                    fprintf(stderr, "Error: mode must be between 1 and 4\n");
                    if (mode == 5)
                    {
                        fprintf(stderr, "Note: for part E, use -mode 4 "
                                        "-L2repl 2\n");
                    }
                    if (mode == 6)
                    {
                        fprintf(stderr, "Note: for part F, use -mode 4 "
                                        "-L2repl 3\n");
                    }
                    return 2;
                }

                SIM_MODE = (Mode)mode;
            }

            else if (strcasecmp(argv[i], "-linesize") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to -linesize\n");
                    return 2;
                }
                CACHE_LINESIZE = atoi(argv[i]);
            }

            else if (strcasecmp(argv[i], "-repl") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to -repl\n");
                    return 2;
                }

                int repl = atoi(argv[i]);
                if (repl < 0 || repl > 3)
                {
                    fprintf(stderr, "Error: repl must be between 0 and 3\n");
                    return 2;
                }

                REPL_POLICY = (ReplacementPolicy)repl;
            }

            else if (strcasecmp(argv[i], "-DsizeKB") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to -DsizeKB\n");
                    return 2;
                }
                DCACHE_SIZE = atoi(argv[i]) * 1024;
            }

            else if (strcasecmp(argv[i], "-Dassoc") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to -Dassoc\n");
                    return 2;
                }
                DCACHE_ASSOC = atoi(argv[i]);
            }

            else if (strcasecmp(argv[i], "-L2sizeKB") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to -L2sizeKB\n");
                    return 2;
                }
                L2CACHE_SIZE = atoi(argv[i]) * 1024;
            }

            else if (strcasecmp(argv[i], "-L2repl") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to -L2repl\n");
                    return 2;
                }

                int l2repl = atoi(argv[i]);
                if (l2repl < 0 || l2repl > 3)
                {
                    fprintf(stderr, "Error: L2repl must be between 0 and 3\n");
                    return 2;
                }

                L2CACHE_REPL = (ReplacementPolicy)l2repl;
            }

            else if (strcasecmp(argv[i], "-SWP_core0ways") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to "
                                    "-SWP_core0ways\n");
                    return 2;
                }
                SWP_CORE0_WAYS = atoi(argv[i]);
            }

            else if (strcasecmp(argv[i], "-dram_policy") == 0)
            {
                if (++i >= argc)
                {
                    fprintf(stderr, "Error: missing argument to "
                                    "-dram_policy\n");
                    return 2;
                }

                int dram_policy = atoi(argv[i]);
                if (dram_policy < 0 || dram_policy > 1)
                {
                    fprintf(stderr, "Error: dram_policy must be between 0 and 1\n");
                    return 2;
                }

                DRAM_PAGE_POLICY = (DRAMPolicy)dram_policy;
            }

            else
            {
                fprintf(stderr, "Error: unrecognized option: %s\n", argv[i]);
                return 2;
            }
        }
        else
        {
            // Parse trace file name.
            if (NUM_CORES >= MAX_CORES)
            {
                fprintf(stderr, "Error: too many trace files specified\n");
                return 2;
            }

            trace_filename[NUM_CORES] = argv[i];
            NUM_CORES++;
        }
    }

    if (NUM_CORES == 0)
    {
        fprintf(stderr, "Error: no trace file specified\n");
        return 2;
    }

    return 0;
}

void print_dots()
{
    unsigned int LINE_INTERVAL = 50 * DOT_INTERVAL;
    last_printdot_cycle = current_cycle;

    if (!PRINT_DOTS)
    {
        return;
    }

    if (current_cycle % LINE_INTERVAL == 0)
    {
        if (current_cycle != 0)
        {
            printf("\n");
        }
        printf("%4llu M\t", (unsigned long long)current_cycle / 1000000);
        fflush(stdout);
    }
    else
    {
        printf(".");
        fflush(stdout);
    }
}

void print_stats()
{
    printf("\n\n");
    printf("CYCLES              \t\t : %10llu\n",
           (unsigned long long)current_cycle);

    for (unsigned int i = 0; i < NUM_CORES; i++)
    {
        core_print_stats(core[i]);
    }

    memsys_print_stats(memsys);
}

void print_usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s [-option <value>] trace_0 <trace_1>\n",
            program_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Trace driven memory system simulator\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    -mode <num>             Set mode of the simulator "
                    "[1: part A, 2: part B,\n");
    fprintf(stderr, "                            3: part C, 4: part D/E/F] "
                    "(default: 1)\n");
    fprintf(stderr, "    -linesize <num>         Set cache line size in bytes "
                    "for all caches\n");
    fprintf(stderr, "                            (default: 64)\n");
    fprintf(stderr, "    -repl <num>             Set replacement policy for "
                    "L1 cache [0: LRU,\n");
    fprintf(stderr, "                            1: random, 2: SWP, 3: DWP] "
                    "(default: 0)\n");
    fprintf(stderr, "    -DsizeKB <num>          Set capacity in KB of the L1 "
                    "dcache (default: 32 KB)\n");
    fprintf(stderr, "    -Dassoc <num>           Set associativity of the L1 "
                    "dcache (default: 8)\n");
    fprintf(stderr, "    -L2sizeKB <num>         Set capacity in KB of the "
                    "unified L2 cache\n");
    fprintf(stderr, "                            (default: 512 KB)\n");
    fprintf(stderr, "    -L2repl <num>           Set replacement policy for "
                    "L2 cache [0: LRU,\n");
    fprintf(stderr, "                            1: random, 2: SWP, 3: DWP] "
                    "(default: 0)\n");
    fprintf(stderr, "    -SWP_core0ways <num>    Set static quota for core 0 "
                    "in SWP (default: 1)\n");
    fprintf(stderr, "    -dram_policy <num>      Set DRAM page policy "
                    "[0: open-page, 1: close-page]\n");
    fprintf(stderr, "                            (default: 0)\n");
}

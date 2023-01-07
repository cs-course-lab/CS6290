// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// You should not modify this file. //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

// exeq.cpp
// Implements the execution queue.

#include "exeq.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * Allocate and initialize a new EXEQ.
 * 
 * @return a pointer to a newly allocated EXEQ
 */
EXEQ *exeq_init()
{
    EXEQ *exeq = (EXEQ *)calloc(1, sizeof(EXEQ));
    for (unsigned int i = 0; i < MAX_EXEQ_ENTRIES; i++)
    {
        exeq->entries[i].valid = false;
    }
    return exeq;
}

/**
 * Print out the state of the EXEQ for debugging purposes.
 * 
 * This function is called automatically in pipe_print_state(), but you may
 * also use it to help you debug. If you choose to do so, please remove calls
 * to this function before submitting the lab.
 * 
 * @param exeq the EXEQ
 */
void exeq_print_state(EXEQ *t)
{
    printf("Current EXEQ state:\n");
    printf("Entry  Valid  Inst  Wait Cycles\n");
    for (unsigned int i = 0; i < MAX_EXEQ_ENTRIES; i++)
    {
        printf("%5d ::  %d ", i, t->entries[i].valid);
        printf("%5d \t", (int)t->entries[i].inst.inst_num);
        printf("%5d \n", t->entries[i].inst.exe_wait_cycles);
    }
    printf("\n");
}

/**
 * Simulate one cycle of the execution queue.
 * 
 * @param exeq the EXEQ
 */
void exeq_cycle(EXEQ *exeq)
{
    for (unsigned int i = 0; i < MAX_EXEQ_ENTRIES; i++)
    {
        if (exeq->entries[i].valid)
        {
            exeq->entries[i].inst.exe_wait_cycles--;
        }
    }
}

/**
 * Add an instruction to the execution queue if there is space.
 * 
 * @param exeq the EXEQ
 * @param inst the instruction to add
 * @return true if the instruction was added, false if the queue was full
 */
bool exeq_insert(EXEQ *exeq, InstInfo inst)
{
    for (unsigned int i = 0; i < MAX_EXEQ_ENTRIES; i++)
    {
        if (!exeq->entries[i].valid)
        {
            exeq->entries[i].valid = true;
            exeq->entries[i].inst = inst;
            exeq->entries[i].inst.exe_wait_cycles = 1;

            // Override wait time for LD instructions
            if (exeq->entries[i].inst.op_type == OP_LD)
            {
                exeq->entries[i].inst.exe_wait_cycles = LOAD_EXE_CYCLES;
            }

            return true;
        }
    }

    return false;
}

/**
 * Check if any instructions have completed execution.
 * 
 * @param exeq the EXEQ
 * @return true if any instructions have completed execution, false otherwise
 */
bool exeq_check_done(EXEQ *exeq)
{
    for (unsigned int i = 0; i < MAX_EXEQ_ENTRIES; i++)
    {
        if (exeq->entries[i].valid)
        {
            if (exeq->entries[i].inst.exe_wait_cycles == 0)
            {
                return true;
            }
        }
    }
    return false;
}

/**
 * Get the next instruction that has completed execution and remove it from the
 * queue.
 * 
 * @param exeq the EXEQ
 * @return an instruction that has completed execution.
 */
InstInfo exeq_remove(EXEQ *exeq)
{
    for (unsigned int i = 0; i < MAX_EXEQ_ENTRIES; i++)
    {
        if (exeq->entries[i].valid &&
            exeq->entries[i].inst.exe_wait_cycles == 0)
        {
            exeq->entries[i].valid = false;
            return exeq->entries[i].inst;
        }
    }

    fprintf(stderr, "Warning: Trying to remove from empty EXEQ!\n");
    InstInfo dummy;
    return dummy;
}

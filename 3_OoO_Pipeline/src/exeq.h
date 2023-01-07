// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// You should not modify this file. //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

// exeq.h
// Declares the struct for the execution queue.

#ifndef _EXEQ_H_
#define _EXEQ_H_

#include "trace.h"
#include <inttypes.h>

/**
 * The maximum number of instructions that can be in the execution queue at
 * once.
 */
#define MAX_EXEQ_ENTRIES 16

/**
 * The number of cycles an LD instruction should take to execute.
 * 
 * This is used by the code in exeq.cpp to determine how long to wait before
 * considering the execution of an LD instruction done.
 */
extern uint32_t LOAD_EXE_CYCLES;

/** An execution queue entry. */
typedef struct EXEQEntryStruct
{
    /** Is this entry valid? */
    bool valid;
    /** The instruction to execute. */
    InstInfo inst;
} EXEQEntry;

/** The execution queue. */
typedef struct EXEQStruct
{
    /** An array of execution queue entries. */
    EXEQEntry entries[MAX_EXEQ_ENTRIES];
} EXEQ;

/**
 * Allocate and initialize a new EXEQ.
 * 
 * @return a pointer to a newly allocated EXEQ
 */
EXEQ *exeq_init();

/**
 * Print out the state of the EXEQ for debugging purposes.
 * 
 * This function is called automatically in pipe_print_state(), but you may
 * also use it to help you debug. If you choose to do so, please remove calls
 * to this function before submitting the lab.
 * 
 * @param exeq the EXEQ
 */
void exeq_print_state(EXEQ *exeq);

/**
 * Simulate one cycle of the execution queue.
 * 
 * @param exeq the EXEQ
 */
void exeq_cycle(EXEQ *exeq);

/**
 * Add an instruction to the execution queue if there is space.
 * 
 * @param exeq the EXEQ
 * @param inst the instruction to add
 * @return true if the instruction was added, false if the queue was full
 */
bool exeq_insert(EXEQ *t, InstInfo inst);

/**
 * Check if any instructions have completed execution.
 * 
 * @param exeq the EXEQ
 * @return true if any instructions have completed execution, false otherwise
 */
bool exeq_check_done(EXEQ *t);

/**
 * Get the next instruction that has completed execution and remove it from the
 * queue.
 * 
 * @param exeq the EXEQ
 * @return an instruction that has completed execution.
 */
InstInfo exeq_remove(EXEQ *exeq);

#endif

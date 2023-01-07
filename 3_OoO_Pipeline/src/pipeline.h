// ----------------------------------------------------------------------- //
// You should not need to modify this file.                                //
// If you do, you may add any declarations you may need, but do not modify //
// or remove the existing declarations.                                    //
// ----------------------------------------------------------------------- //

// pipeline.h
// Declares the pipeline struct, as well as functions, data structures, and
// enums related to it.

#ifndef _PIPELINE_H_
#define _PIPELINE_H_

#include "trace.h"
#include "rat.h"
#include "rob.h"
#include "exeq.h"
#include <inttypes.h>

/**
 * [Internal] The maximum allowed width of the pipeline.
 * 
 * This is an implementation detail that defines the array size of most
 * Pipeline::*_latch arrays; you should not have to use this value directly.
 * 
 * You may need to use the global variable PIPE_WIDTH instead.
 */
#define MAX_PIPE_WIDTH 8

/**
 * The maximum number of instructions that can be written back in a single
 * cycle.
 * 
 * Your implementation of pipe_cycle_writeback() must be able to handle the
 * writeback of at most this many instructions in a single cycle, which may be
 * more than PIPE_WIDTH.
 */
#define MAX_WRITEBACKS 256

/** How the pipeline schedules instructions for execution. */
typedef enum SchedulingPolicyEnum
{
    SCHED_IN_ORDER,     // Instructions are scheduled in-order.
    SCHED_OUT_OF_ORDER, // Instructions are scheduled out-of-order.
    NUM_SCHED_POLICIES
} SchedulingPolicy;

/**
 * One of the latches in the pipeline. Each one of these can contain one
 * instruction to be processed by the next pipeline stage.
 */
typedef struct PipelineLatchStruct
{
    /**
     * Is this instruction valid?
     * 
     * If false, this latch does not contain an instruction, but rather a
     * bubble in the pipeline, and so it should not be processed further. Any
     * other fields in this structure should be ignored.
     * 
     * Your code may set this to false at any time to create a bubble.
     */
    bool valid;

    /**
     * Should this instruction be stalled?
     * 
     * Your code may set this to stall or unstall an instruction as needed.
     * 
     * While this is set to true, the stage that feeds into this latch will
     * keep the instruction in place rather than overwriting it by moving the
     * next instruction into the latch.
     */
    bool stall;

    /**
     * A structure containing information about this instruction, such as what
     * type of instruction it is, what registers it reads and writes, and so
     * on.
     */
    InstInfo inst;
} PipelineLatch;

/**
 * The data structure for an out-of-order pipelined processor.
 */
typedef struct Pipeline
{
    /**
     * The pipeline latch holding fetched instructions.
     * The FE (fetch) stage writes instructions to this latch.
     * The ID (instruction decode) stage reads instructions from this latch.
     * 
     * Not all MAX_PIPE_WIDTH entries of this array will be used. Refer to the
     * global variable PIPE_WIDTH to see how many of these will be used.
     */
    PipelineLatch FE_latch[MAX_PIPE_WIDTH];

    /**
     * The pipeline latch holding decoded instructions.
     * The ID (instruction decode) stage writes instructions to this latch.
     * The issue stage reads instructions from this latch.
     * 
     * Not all MAX_PIPE_WIDTH entries of this array will be used. Refer to the
     * global variable PIPE_WIDTH to see how many of these will be used.
     */
    PipelineLatch ID_latch[MAX_PIPE_WIDTH];

    /**
     * The pipeline latch holding scheduled instructions.
     * The SC (scheduling) stage writes instructions to this latch.
     * The EX (execution) stage reads instructions from this latch.
     * 
     * Not all MAX_PIPE_WIDTH entries of this array will be used. Refer to the
     * global variable PIPE_WIDTH to see how many of these will be used.
     */
    PipelineLatch SC_latch[MAX_PIPE_WIDTH];

    /**
     * The pipeline latch holding instructions that have completed execution.
     * The EX (execution) stage writes instructions to this latch.
     * The WB (writeback) stage reads instructions from this latch.
     * 
     * Note that this array can contain up to MAX_WRITEBACKS instructions to be
     * written back, which may be more than PIPE_WIDTH!
     */
    PipelineLatch EX_latch[MAX_WRITEBACKS];

    /**
     * The re-order buffer, containing instructions that have been issued but
     * have not yet been committed.
     * 
     * You must implement several ROB functions in rob.cpp in part A of the
     * assignment.
     */
    ROB *rob;

    /**
     * The register alias table, containing information on which architectural
     * registers are aliased to which instructions in the ROB.
     * 
     * You must implement several RAT functions in rat.cpp in part A of the
     * assignment.
     */
    RAT *rat;

    /**
     * The execution queue for instructions that take multiple cycles to
     * execute.
     * 
     * This has been implemented for you.
     */
    EXEQ *exeq;

    /**
     * The total number of committed instructions.
     * 
     * This counter is already updated for you by the provided code in
     * pipe_commit_inst(), which you should call for every instruction you
     * commit in your implementation of pipe_cycle_commit().
     */
    uint64_t stat_retired_inst;

    /**
     * The total number of simulated CPU cycles.
     * 
     * This counter is already updated for you by the provided code in
     * pipe_cycle().
     */
    uint64_t stat_num_cycle;

    /** [Internal] The file descriptor from which to read trace records. */
    int trace_fd;
    /** [Internal] The last inst_num assigned. */
    uint64_t last_inst_num;
    /** [Internal] The inst_num of the last instruction in the trace. */
    uint64_t halt_inst_num;
    /** [Internal] Whether the pipeline is done. */
    bool halt;
} Pipeline;

/**
 * Allocate and initialize a new pipeline.
 * 
 * You should not modify this function.
 * 
 * @param trace_fd the file descriptor from which to read trace records
 * @return a pointer to a newly allocated pipeline
 */
Pipeline *pipe_init(int trace_fd);

/**
 * Simulate one cycle of all stages of a pipeline.
 * 
 * You should not need to modify this function except for debugging purposes.
 * If you add code to print debug output in this function, remove it or comment
 * it out before you submit the lab.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle(Pipeline *p);

/**
 * Simulate one cycle of the fetch stage of a pipeline.
 * 
 * This function is implemented for you. You should not modify it.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_fetch(Pipeline *p);

/**
 * Simulate one cycle of the instruction decode stage of a pipeline.
 * 
 * This function is implemented for you. You should not modify it.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_decode(Pipeline *p);

/**
 * Simulate one cycle of the issue stage of a pipeline: insert decoded
 * instructions into the ROB and perform register renaming.
 * 
 * You must implement this function in pipeline.cpp in part B of the
 * assignment.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_issue(Pipeline *p);

/**
 * Simulate one cycle of the scheduling stage of a pipeline: schedule
 * instructions to execute if they are ready.
 * 
 * You must implement this function in pipeline.cpp in part B of the
 * assignment.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_schedule(Pipeline *p);

/**
 * Simulate one cycle of the execute stage of a pipeline. This handles
 * instructions that take multiple cycles to execute.
 * 
 * This function is implemented for you. You should not modify it.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_exe(Pipeline *p);

/**
 * Simulate one cycle of the writeback stage of a pipeline: update the ROB
 * with information from instructions that have finished executing.
 * 
 * You must implement this function in pipeline.cpp in part B of the
 * assignment.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_writeback(Pipeline *p);

/**
 * Simulate one cycle of the commit stage of a pipeline: commit instructions
 * in the ROB that are ready to commit.
 * 
 * You must implement this function in pipeline.cpp in part B of the
 * assignment.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_commit(Pipeline *p);

/**
 * Commit the given instruction.
 * 
 * This updates counters and flags on the pipeline.
 * 
 * This function is implemented for you. You should not modify it.
 * 
 * @param p the pipeline to update.
 * @param inst the instruction to commit.
 */
void pipe_commit_inst(Pipeline *p, InstInfo inst);

/**
 * Print out the state of the pipeline for debugging purposes.
 * 
 * You may use this function to help debug your pipeline implementation, but
 * please remove calls to this function before submitting the lab.
 * 
 * @param p the pipeline
 */
void pipe_print_state(Pipeline *p);

#endif

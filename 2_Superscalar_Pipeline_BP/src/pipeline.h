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
#include "bpred.h"
#include <inttypes.h>

/**
 * [Internal] The maximum allowed width of the pipeline.
 * 
 * This is an implementation detail that defines the array size of
 * Pipeline::pipe_latch; you should not have to use this value directly.
 */
#define MAX_PIPE_WIDTH 8

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
extern uint32_t PIPE_WIDTH;

/**
 * A Boolean indicating whether forwarding from the Memory Access stage (MA)
 * should be simulated.
 * 
 * You should not modify this value directly; it is set by the command-line
 * argument -enablememfwd.
 */
extern uint32_t ENABLE_MEM_FWD;

/**
 * A Boolean indicating whether forwarding from the Execute stage (EX) should
 * be simulated.
 * 
 * You should not modify this value directly; it is set by the command-line
 * argument -enableexefwd.
 */
extern uint32_t ENABLE_EXE_FWD;

/**
 * The branch prediction policy that should be simulated.
 * 
 * Refer to the BpredPolicy enumeration in bpred.h for a description of the
 * possible values.
 * 
 * You should not modify this value directly; it is set by the command-line
 * argument -bpredpolicy.
 */
extern BPredPolicy BPRED_POLICY;

/**
 * One of the latches in the pipeline. Each one of these can contain one
 * operation to be processed by the next pipeline stage.
 */
typedef struct PipelineLatchStruct
{
    /**
     * Is this operation valid?
     * 
     * If false, this latch does not contain an instruction, but rather a
     * bubble in the pipeline, and so it should not be processed further. Any
     * other fields in this struct should be ignored.
     * 
     * Your code may set this to false at any time to create a bubble in the
     * pipeline.
     */
    bool valid;

    /**
     * A unique, monotonically increasing ID for this operation in the trace
     * file.
     * 
     * Unlike the instruction's PC (trace_rec->inst_addr), this is guaranteed
     * to be unique for each operation in the trace file.
     * 
     * Additionally, it is monotonically increasing, which allows it to be used
     * for ordering operations: if A's op_id is less than B's op_id, then A was
     * issued before B.
     */
    uint64_t op_id;

    /**
     * Should this operation be stalled?
     * 
     * Your code may set this to stall or unstall an operation as needed.
     * However, setting this flag does not inherently change anything; it is up
     * to you to write the code to do the right thing when a stall flag is
     * encountered in a PipelineLatch.
     */
    bool stall;

    /**
     * The trace record containing information about this instruction, such as
     * what type of instruction it is, its address, what registers it reads and
     * writes, and so on.
     */
    TraceRec trace_rec;

    /**
     * Is this operation a conditional branch that the branch predictor
     * mispredicted?
     * 
     * If so, you should set this flag to true in order to propagate that
     * information down the pipeline.
     * 
     * This is only relevant for part B of the lab.
     */
    bool is_mispred_cbr;
} PipelineLatch;

/**
 * The types of pipeline latches: one for each stage of the pipeline to write
 * to, except for the final stage.
 */
typedef enum LatchTypeEnum
{
    IF_LATCH, // The pipeline latch for the Instruction Fetch stage (IF).
    ID_LATCH, // The pipeline latch for the Instruction Decode stage (ID).
    EX_LATCH, // The pipeline latch for the Execute stage (EX).
    MA_LATCH, // The pipeline latch for the Memory Access stage (MA).
    NUM_LATCH_TYPES
} LatchType;

/**
 * The data structure for a pipelined processor.
 */
typedef struct Pipeline
{
    /**
     * All pipeline latches for all stages of the pipeline across the entire
     * width of the (possibly superscalar) pipeline.
     * 
     * For instance, in a superscalar pipeline of width 2, the latches to which
     * the IF stage writes are pipe_latch[IF_LATCH][0] and
     * pipe_latch[IF_LATCH][1].
     * 
     * Refer to the global variable PIPE_WIDTH to see how many latches should
     * be used in each stage of the pipeline.
     */
    PipelineLatch pipe_latch[NUM_LATCH_TYPES][MAX_PIPE_WIDTH];

    /**
     * The branch predictor.
     * 
     * You will use this in part B of the lab.
     */
    BPred *b_pred;

    /**
     * Is the Instruction Fetch stage (IF) stalled due to a branch
     * misprediction?
     * 
     * You will need to set and use this in part B of the lab.
     * 
     * A real pipelined processor wouldn't actually stall on a branch
     * misprediction; it would just keep fetching the wrong instructions.
     * When the mispredicted branch resolves, those instructions would be
     * killed, making them bubbles in the pipeline, and the IF stage would
     * start fetching the right instructions.
     * 
     * In our simulator, we simply stall the IF stage, thereby inserting
     * bubbles directly into the pipeline from the start. We do this because:
     * (a) just looking at the trace file, we can't know what "wrong"
     *     instructions would have been fetched, and
     * (b) in implementing our simulator, it's easier to just insert bubbles
     *     than to let the pipeline fill up with "wrong" instructions only to
     *     kill them all when the branch resolves.
     */
    bool fetch_cbr_stall;

    /**
     * The total number of committed instructions.
     * 
     * This counter is already updated for you by the provided skeleton code in
     * pipe_cycle_WB().
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
    /** [Internal] The last op_id assigned. */
    uint64_t last_op_id;
    /** [Internal] The op_id of the last instruction in the trace. */
    uint64_t halt_op_id;
    /** [Internal] Whether the pipeline is done. */
    bool halt;
} Pipeline;

/**
 * Allocate and initialize a new pipeline.
 * 
 * You should not need to modify this function.
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
 * Simulate one cycle of the Instruction Fetch stage (IF) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_IF(Pipeline *p);

/**
 * Simulate one cycle of the Instruction Decode stage (ID) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_ID(Pipeline *p);

/**
 * Simulate one cycle of the Execute stage (EX) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_EX(Pipeline *p);

/**
 * Simulate one cycle of the Memory Access stage (MA) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_MA(Pipeline *p);

/**
 * Simulate one cycle of the Write Back stage (WB) of a pipeline.
 * 
 * Some skeleton code has been provided for you. You must implement anything
 * else you need for the pipeline simulation to work properly.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_WB(Pipeline *p);

/**
 * If the instruction just fetched is a conditional branch, check for a branch
 * misprediction, update the branch predictor, and set appropriate flags in the
 * pipeline.
 * 
 * You must implement this function in part B of the lab.
 * 
 * @param p the pipeline
 * @param fetch_op the pipeline latch containing the operation fetched
 */
void pipe_check_bpred(Pipeline *p, PipelineLatch *fetch_op, unsigned int i);

/**
 * Print out the state of the pipeline latches for debugging purposes.
 * 
 * You may use this function to help debug your pipeline implementation, but
 * please remove calls to this function before submitting the lab.
 * 
 * @param p the pipeline
 */
void pipe_print_state(Pipeline *p);

#endif
